#ifndef INTEGRAL_UNIFIED_HISTORY_H
#define INTEGRAL_UNIFIED_HISTORY_H

#include <cstring>
#include <algorithm>
#include "../../../chess/board.h"
#include "../../../../shared/multi_array.h"
#include "../stack.h"
#include "bonus.h"

namespace search::history {

using Piece = U8;

// Unified history table with better cache locality
class UnifiedHistory {
 public:
  // Structure to hold all history scores in contiguous memory
  struct HistoryEntry {
    I16 quiet_score[2][2];       // [from_threat][to_threat]
    I16 capture_score;
    I16 continuation_scores[3];   // 1st, 2nd, 4th ply
    I16 pawn_score;
    I16 correction_score;
  };
  
  static constexpr size_t kTableSize = kNumColors * kSquareCount * kSquareCount;
  static constexpr size_t kContinuationSize = kNumPieceTypes * kSquareCount;
  static constexpr size_t kPawnCount = 16384; // 2^14 for pawn hash indexing
  static constexpr size_t kPawnSize = kNumColors * kPawnCount * kSquareCount;
  
  UnifiedHistory() {
    Clear();
  }
  
  void Clear() {
    std::memset(main_table_, 0, sizeof(main_table_));
    std::memset(continuation_table_, 0, sizeof(continuation_table_));
    std::memset(pawn_table_, 0, sizeof(pawn_table_));
    std::memset(capture_table_, 0, sizeof(capture_table_));
    std::memset(correction_table_, 0, sizeof(correction_table_));
  }
  
  // Quiet move history access
  void UpdateQuietScore(Color turn, Move move, BitBoard threats, I16 bonus) {
    const int idx = GetMainIndex(turn, move);
    auto& entry = main_table_[idx];
    const int from_threat = threats.IsSet(move.GetFrom());
    const int to_threat = threats.IsSet(move.GetTo());
    
    I16& score = entry.quiet_score[from_threat][to_threat];
    score += ScaleBonus(score, bonus);
  }
  
  [[nodiscard]] I16 GetQuietScore(const BoardState& state, Move move, BitBoard threats) const {
    const int idx = GetMainIndex(state.turn, move);
    const auto& entry = main_table_[idx];
    const int from_threat = threats.IsSet(move.GetFrom());
    const int to_threat = threats.IsSet(move.GetTo());
    
    return entry.quiet_score[from_threat][to_threat];
  }
  
  // Capture history access
  void UpdateCaptureScore(const BoardState& state, Move move, I16 bonus) {
    const auto from = move.GetFrom();
    const auto to = move.GetTo();
    const auto attacker = state.GetPieceType(from);
    const auto victim = move.IsEnPassant(state) ? kPawn : state.GetPieceType(to);
    
    I16& score = capture_table_[state.turn][attacker][to][victim];
    score += ScaleBonus(score, bonus);
  }
  
  [[nodiscard]] I16 GetCaptureScore(const BoardState& state, Move move) const {
    const auto from = move.GetFrom();
    const auto to = move.GetTo();
    const auto attacker = state.GetPieceType(from);
    const auto victim = move.IsEnPassant(state) ? kPawn : state.GetPieceType(to);
    
    return capture_table_[state.turn][attacker][to][victim];
  }
  
  // Continuation history access
  void UpdateContinuationScore(const BoardState& state, StackEntry* stack, int ply_offset, I16 bonus) {
    if (ply_offset < 0 || ply_offset > 4) return;
    if (!stack || !(stack - ply_offset)->move) return;
    
    const auto prev_move = (stack - ply_offset)->move;
    const auto prev_to = prev_move.GetTo();
    const auto prev_piece_type = state.GetPieceType(prev_to);
    const auto prev_color = state.GetPieceColor(prev_to);
    const Piece prev_piece = prev_piece_type * 2 + prev_color;
    
    if (prev_piece_type == PieceType::kNone) return;
    
    const int cont_idx = GetContinuationIndex(prev_piece, prev_to, stack->move);
    const int score_idx = ply_offset == 1 ? 0 : (ply_offset == 2 ? 1 : 2);
    
    I16& score = continuation_table_[cont_idx].continuation_scores[score_idx];
    score += ScaleBonus(score, bonus);
  }
  
  [[nodiscard]] I16 GetContinuationScore(const BoardState& state, Move move, StackEntry* stack, int ply_offset) const {
    if (ply_offset < 0 || ply_offset > 4) return 0;
    if (!stack || !(stack - ply_offset)->move) return 0;
    
    const auto prev_move = (stack - ply_offset)->move;
    const auto prev_to = prev_move.GetTo();
    const auto prev_piece_type = state.GetPieceType(prev_to);
    const auto prev_color = state.GetPieceColor(prev_to);
    const Piece prev_piece = prev_piece_type * 2 + prev_color;
    
    if (prev_piece_type == PieceType::kNone) return 0;
    
    const int cont_idx = GetContinuationIndex(prev_piece, prev_to, move);
    const int score_idx = ply_offset == 1 ? 0 : (ply_offset == 2 ? 1 : 2);
    
    return continuation_table_[cont_idx].continuation_scores[score_idx];
  }
  
  // Pawn history access  
  void UpdatePawnScore(const BoardState& state, Move move, I16 bonus) {
    const auto pawn_key = state.pawn_key;
    const int idx = GetPawnIndex(state.turn, pawn_key, move.GetTo());
    
    I16& score = pawn_table_[idx];
    score += ScaleBonus(score, bonus);
  }
  
  [[nodiscard]] I16 GetPawnScore(const BoardState& state, Move move) const {
    const auto pawn_key = state.pawn_key;
    const int idx = GetPawnIndex(state.turn, pawn_key, move.GetTo());
    
    return pawn_table_[idx];
  }
  
  // Correction history access
  void UpdateCorrectionScore(const BoardState& state, U64 pawn_key, I16 bonus) {
    const int idx = GetCorrectionIndex(state.turn, pawn_key);
    
    I16& score = correction_table_[idx];
    score = std::clamp<I16>(score + bonus, -32000, 32000);
  }
  
  [[nodiscard]] I16 GetCorrectionScore(const BoardState& state) const {
    const int idx = GetCorrectionIndex(state.turn, state.pawn_key);
    
    return correction_table_[idx];
  }
  
  // Combined score calculation for move ordering
  [[nodiscard]] I32 GetMoveScore(const BoardState& state, Move move, StackEntry* stack) const {
    if (move.IsCapture(state)) {
      return GetCaptureScore(state, move);
    }
    
    I32 score = 0;
    const BitBoard threats = stack->threats;
    
    // Quiet history
    score += GetQuietScore(state, move, threats) * kQuietHistoryWeight;
    
    // Continuation history
    score += GetContinuationScore(state, move, stack, 1) * kFirstContinuationHistoryWeight;
    score += GetContinuationScore(state, move, stack, 2) * kSecondContinuationHistoryWeight;
    score += GetContinuationScore(state, move, stack, 4) * kFourthContinuationHistoryWeight;
    
    // Pawn history
    score += GetPawnScore(state, move) * kPawnHistoryWeight;
    
    return score / kHistoryWeightScale;
  }
  
 private:
  // Index calculation functions
  [[nodiscard]] inline int GetMainIndex(Color turn, Move move) const {
    return turn * kSquareCount * kSquareCount + 
           move.GetFrom() * kSquareCount + move.GetTo();
  }
  
  [[nodiscard]] inline int GetContinuationIndex(Piece piece, Square to, Move move) const {
    const auto piece_type = static_cast<PieceType>(piece & 7);
    const auto color = static_cast<Color>(piece >> 3);
    return (color * kNumPieceTypes + piece_type) * kSquareCount * kSquareCount +
           to * kSquareCount + move.GetTo();
  }
  
  [[nodiscard]] inline int GetPawnIndex(Color turn, U64 pawn_key, Square to) const {
    return (turn * kPawnCount + (pawn_key & (kPawnCount - 1))) * kSquareCount + to;
  }
  
  [[nodiscard]] inline int GetCorrectionIndex(Color turn, U64 pawn_key) const {
    return turn * 16384 + (pawn_key & 16383);
  }
  
 private:
  // Main table for quiet moves (most frequently accessed)
  alignas(64) HistoryEntry main_table_[kNumColors * kSquareCount * kSquareCount];
  
  // Continuation history table
  alignas(64) HistoryEntry continuation_table_[kNumColors * kNumPieceTypes * kSquareCount * kSquareCount];
  
  // Pawn history table
  alignas(64) I16 pawn_table_[kNumColors * kPawnCount * kSquareCount];
  
  // Capture history table
  alignas(64) I16 capture_table_[kNumColors][kNumPieceTypes][kSquareCount][kNumPieceTypes];
  
  // Correction history table
  alignas(64) I16 correction_table_[kNumColors * 16384];
};

}  // namespace search::history

#endif  // INTEGRAL_UNIFIED_HISTORY_H