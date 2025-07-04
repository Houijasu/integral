#include "evaluation.h"

#include "nnue/nnue.h"
#include "eval_cache.h"

namespace eval {

TUNABLE_STEP(kMaterialScaleBase, 26909, 10000, 32768, false, 500);

// Thread-local evaluation cache for each thread
thread_local EvalCache eval_cache;

void ClearEvalCache() {
  eval_cache.Clear();
}

Score Evaluate(Board &board) {
  const auto &state = board.GetState();
  const U64 key = state.zobrist_key;
  
  // Check cache first
  Score cached_score;
  if (eval_cache.Probe(key, cached_score)) {
    return cached_score;
  }
  
  const auto network_eval = nnue::Evaluate(board);

#if DATAGEN
  eval_cache.Store(key, network_eval);
  return network_eval;
#endif

  const auto material_phase =
      *kSeePieceScores[kKnight] * state.Knights().PopCount() +
      *kSeePieceScores[kBishop] * state.Bishops().PopCount() +
      *kSeePieceScores[kRook] * state.Rooks().PopCount() +
      *kSeePieceScores[kQueen] * state.Queens().PopCount();

  const Score final_eval = network_eval * (kMaterialScaleBase + material_phase) / 32768;
  
  // Store in cache
  eval_cache.Store(key, final_eval);
  
  return final_eval;
}

bool StaticExchange(Move move, int threshold, const BoardState &state) {
  const auto from = move.GetFrom();
  const auto to = move.GetTo();

  const PieceType &from_piece = state.GetPieceType(from);
  // Ignore en passant captures and castling
  if (move.IsEnPassant(state) ||
      (from_piece == kKing && std::abs(static_cast<int>(from) - to) == 2)) {
    return threshold <= 0;
  }

  // Score represents the maximum number of points the opponent can gain with
  // the next capture
  Score score = *kSeePieceScores[state.GetPieceType(to)] - threshold;
  // If the captured piece is worth less than what we can give up, we lose
  if (score < 0) {
    return false;
  }

  score = *kSeePieceScores[from_piece] - score;
  // If we captured a piece with equal/greater value than our capturing piece,
  // we win
  if (score <= 0) {
    return true;
  }

  const BitBoard &pawns = state.Pawns();
  const BitBoard &knights = state.Knights();
  const BitBoard &bishops = state.Bishops();
  const BitBoard &rooks = state.Rooks();
  const BitBoard &queens = state.Queens();
  const BitBoard &kings = state.Kings();

  BitBoard occupied = state.Occupied();
  occupied.ClearBit(from);
  occupied.ClearBit(to);

  // Get all pieces that attack the capture square
  // Pawns attack differently based on color, so handle separately
  const BitBoard white_pawn_attacks = move_gen::PawnAttacks(to, Color::kWhite);
  const BitBoard black_pawn_attacks = move_gen::PawnAttacks(to, Color::kBlack);
  const BitBoard pawn_attackers = (white_pawn_attacks & state.Pawns(Color::kBlack)) |
                                  (black_pawn_attacks & state.Pawns(Color::kWhite));
  
  // Non-sliding pieces
  const BitBoard knight_attackers = move_gen::KnightMoves(to) & knights;
  const BitBoard king_attackers = move_gen::KingAttacks(to) & kings;

  // Sliding pieces - calculate attacks once and reuse
  const BitBoard bishop_attacks = move_gen::BishopMoves(to, occupied);
  const BitBoard rook_attacks = move_gen::RookMoves(to, occupied);
  
  const BitBoard diagonal_attackers = bishop_attacks & (bishops | queens);
  const BitBoard straight_attackers = rook_attacks & (rooks | queens);

  // Compute all attacking pieces for this square
  BitBoard all_attackers = (pawn_attackers | knight_attackers | king_attackers |
                           diagonal_attackers | straight_attackers) & occupied;

  Color turn = state.turn;
  Color winner = state.turn;
  
  // Pre-calculate pinned pieces and rays for efficiency
  const BitBoard white_pinned = state.pinned[Color::kWhite] & state.Occupied(Color::kWhite);
  const BitBoard black_pinned = state.pinned[Color::kBlack] & state.Occupied(Color::kBlack);
  
  // Only calculate king rays if there are pinned pieces
  BitBoard white_pinned_aligned = 0;
  BitBoard black_pinned_aligned = 0;
  
  if (white_pinned) {
    const BitBoard white_king_ray = move_gen::RayIntersecting(to, state.King(Color::kWhite).GetLsb());
    white_pinned_aligned = white_king_ray & white_pinned;
  }
  
  if (black_pinned) {
    const BitBoard black_king_ray = move_gen::RayIntersecting(to, state.King(Color::kBlack).GetLsb());
    black_pinned_aligned = black_king_ray & black_pinned;
  }

  // Loop through all pieces that attack the capture square
  while (true) {
    turn = FlipColor(turn);
    all_attackers &= occupied;

    BitBoard our_attackers = all_attackers & state.Occupied(turn);
    
    // Handle pinned pieces - only pieces pinned and aligned to target square can move
    if (turn == Color::kWhite && white_pinned) {
      our_attackers &= ~white_pinned | white_pinned_aligned;
    } else if (turn == Color::kBlack && black_pinned) {
      our_attackers &= ~black_pinned | black_pinned_aligned;
    }

    // If the current side to move has no attackers left, they lose
    if (!our_attackers) {
      break;
    }

    // Without considering piece values, the winner of an exchange is whoever
    // has more attackers, therefore we set the winner's side to the current
    // side to move only after we check if they can attack
    winner = FlipColor(winner);

    // Find the least valuable attacker
    BitBoard next_attacker;
    int attacker_value;

    if ((next_attacker = our_attackers & pawns)) {
      attacker_value = *kSeePieceScores[kPawn];
      occupied.ClearBit(next_attacker.GetLsb());

      // Add pieces that were diagonal xray attacking the captured piece
      const BitBoard new_bishop_attacks = move_gen::BishopMoves(to, occupied);
      all_attackers |= new_bishop_attacks & (bishops | queens);
    } else if ((next_attacker = our_attackers & knights)) {
      attacker_value = *kSeePieceScores[kKnight];
      occupied.ClearBit(next_attacker.GetLsb());
    } else if ((next_attacker = our_attackers & bishops)) {
      attacker_value = *kSeePieceScores[kBishop];
      occupied.ClearBit(next_attacker.GetLsb());

      // Add pieces that were xray attacking the captured piece
      const BitBoard new_bishop_attacks = move_gen::BishopMoves(to, occupied);
      all_attackers |= new_bishop_attacks & (bishops | queens);
    } else if ((next_attacker = our_attackers & rooks)) {
      attacker_value = *kSeePieceScores[kRook];
      occupied.ClearBit(next_attacker.GetLsb());

      // Add pieces that were xray attacking the captured piece
      const BitBoard new_rook_attacks = move_gen::RookMoves(to, occupied);
      all_attackers |= new_rook_attacks & (rooks | queens);
    } else if ((next_attacker = our_attackers & queens)) {
      attacker_value = *kSeePieceScores[kQueen];
      occupied.ClearBit(next_attacker.GetLsb());

      // Add pieces that were xray attacking the captured piece
      const BitBoard new_rook_attacks = move_gen::RookMoves(to, occupied);
      const BitBoard new_bishop_attacks = move_gen::BishopMoves(to, occupied);
      all_attackers |= (new_rook_attacks & (queens | rooks)) |
                       (new_bishop_attacks & (queens | bishops));
    } else {
      // King: check if we capture a piece that our opponent is still
      // attacking
      return (all_attackers & state.Occupied(FlipColor(turn)))
               ? state.turn != winner
               : state.turn == winner;
    }

    // Score represents how many points the other side can gain after this
    // capture. If initially a knight captured a queen, the other side can
    // gain 3 - 9 = -6 points. If we flip it and initially a queen captured a
    // knight, the other side can gain 9 - 3 = 6 points
    score = -score + 1 + attacker_value;
    // Quit early if the exchange is lost or neutral
    if (score <= 0) {
      break;
    }
  }

  return state.turn == winner;
}

}  // namespace eval