#ifndef INTEGRAL_HISTORY_H
#define INTEGRAL_HISTORY_H

// Enable unified history for better cache locality
// #define USE_UNIFIED_HISTORY

#include "../../../chess/board.h"
// #include "unified_history.h"
#include "capture_history.h"
#include "continuation_history.h"
#include "correction_history.h"
#include "pawn_history.h"
#include "quiet_history.h"

namespace search::history {

TUNABLE(kQuietHistoryWeight, 984, 0, 2048, false);
TUNABLE(kFirstContinuationHistoryWeight,  1192, 0, 2048, false);
TUNABLE(kSecondContinuationHistoryWeight, 949, 0, 2048, false);
TUNABLE(kFourthContinuationHistoryWeight, 969, 0, 2048, false);
TUNABLE(kPawnHistoryWeight, 1047, 0, 2048, false);

constexpr int kHistoryWeightScale = 1024;

class History {
 public:
  History() {
    Initialize();
  }

  void Initialize() {
#ifdef USE_UNIFIED_HISTORY
    unified_history = std::make_unique<UnifiedHistory>();
#else
    quiet_history = std::make_unique<QuietHistory>();
    continuation_history = std::make_unique<ContinuationHistory>();
    correction_history = std::make_unique<CorrectionHistory>();
    capture_history = std::make_unique<CaptureHistory>();
    pawn_history = std::make_unique<PawnHistory>();
#endif
  }

  // Reinitialize the history objects for quicker clearing
  void Clear() {
#ifdef USE_UNIFIED_HISTORY
    unified_history->Clear();
#else
    Initialize();
#endif
  }

  [[nodiscard]] I32 GetMoveScore(const BoardState &state,
                                 Move move,
                                 StackEntry *stack) {
#ifdef USE_UNIFIED_HISTORY
    return unified_history->GetMoveScore(state, move, stack);
#else
    return move.IsCapture(state) ? GetCaptureMoveScore(state, move)
                                 : GetQuietMoveScore(state, move, stack);
#endif
  }

  [[nodiscard]] I32 GetQuietMoveScore(const BoardState &state,
                                      Move move,
                                      StackEntry *stack) const {
#ifdef USE_UNIFIED_HISTORY
    return unified_history->GetMoveScore(state, move, stack);
#else
    I32 move_score = 0;
    move_score += quiet_history->GetScore(state, move, stack->threats) *
                  kQuietHistoryWeight;
    move_score += continuation_history->GetScore(state, move, stack - 1) *
                  kFirstContinuationHistoryWeight;
    move_score += continuation_history->GetScore(state, move, stack - 2) *
                  kSecondContinuationHistoryWeight;
    move_score += continuation_history->GetScore(state, move, stack - 4) *
                  kFourthContinuationHistoryWeight;
    move_score += pawn_history->GetScore(state, move) * kPawnHistoryWeight;

    return move_score / kHistoryWeightScale;
#endif
  }

  [[nodiscard]] I32 GetCaptureMoveScore(const BoardState &state,
                                        Move move) const {
#ifdef USE_UNIFIED_HISTORY
    return unified_history->GetCaptureScore(state, move);
#else
    return capture_history->GetScore(state, move);
#endif
  }
  

 public:
#ifdef USE_UNIFIED_HISTORY
  std::unique_ptr<UnifiedHistory> unified_history;
#else
  std::unique_ptr<QuietHistory> quiet_history;
  std::unique_ptr<CaptureHistory> capture_history;
  std::unique_ptr<PawnHistory> pawn_history;
  std::unique_ptr<ContinuationHistory> continuation_history;
  std::unique_ptr<CorrectionHistory> correction_history;
#endif
};

}  // namespace search::history

#endif  // INTEGRAL_HISTORY_H
