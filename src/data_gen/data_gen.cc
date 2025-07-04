#include "data_gen.h"

#include <fmt/color.h>
#include <fmt/format.h>

#include <csignal>
#include <filesystem>
#include <fstream>

#include "../chess/board.h"
#include "../engine/search/search.h"
#include "format/binpack.h"
#include "format/fens.h"

namespace data_gen {

// clang-format off
constexpr std::array<int, 64> kCenterScore = {
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  2,  2,  2,  2,  2,  2,  1,
  2,  3,  3,  3,  3,  3,  3,  2,
  3,  5,  5,  5,  5,  5,  5,  3,
  4,  6,  7,  9,  9,  7,  6,  4,
  4,  6,  8,  8,  8,  8,  6,  4,
  3,  5,  6,  6,  6,  6,  5,  3,
  1,  1,  4,  4,  4,  4,  1,  1
};
// clang-format on

Move SelectPreferredMove(MoveList &moves, Color stm) {
  if (moves.Empty()) return Move();

  std::vector<int> move_scores;
  move_scores.reserve(moves.Size());

  // Score moves based on destination square
  for (int i = 0; i < moves.Size(); ++i) {
    const auto to_square = moves[i].GetTo();
    move_scores.push_back(kCenterScore[to_square.RelativeTo(stm)]);
  }

  // Create a distribution weighted by scores
  std::discrete_distribution<> move_dist(move_scores.begin(),
                                         move_scores.end());
  static thread_local std::random_device rd;
  static thread_local std::mt19937 gen(rd());

  int selected_index = move_dist(gen);
  return moves[selected_index];
}

void FindStartingPosition(Board &board,
                          const Config &config,
                          const std::vector<std::string> &fens) {
  if (!fens.empty()) {
    const auto fen = fens[RandomU64(0, fens.size() - 1)];
    // Choose a random FEN from the fens list
    board.SetFromFen(fen);
  } else {
    board.SetFromFen(fen::kStartFen);
  }

  I32 current_ply = 0;
  I32 target_plies = RandomU64(config.min_move_plies, config.max_move_plies);

  while (current_ply < target_plies) {
    Move random_move;

    if (!fens.empty()) {
      auto legal_moves = board.GetLegalMoves();

      // If no legal moves are available, reset the board
      if (legal_moves.Empty()) {
        current_ply = 0;
        const auto fen = fens[RandomU64(0, fens.size() - 1)];
        if (fen.empty()) {
          current_ply = 0;
          continue;
        }
        // Choose a random FEN from the fens list
        board.SetFromFen(fen);
        continue;
      }

      random_move = legal_moves[RandomU64(0, legal_moves.Size() - 1)];
    } else {
      auto legal_moves = board.GetLegalMoves();

      // If no legal moves are available, reset the board
      if (legal_moves.Empty()) {
        current_ply = 0;
        board.SetFromFen(fen::kStartFen);
        continue;
      }

      // Bucket for moves categorized by the piece type
      std::array<MoveList, kNumPieceTypes> piece_moves;

      // Gather all moves that don't lose material and bucket them by piece type
      for (int i = 0; i < legal_moves.Size(); i++) {
        const auto move = legal_moves[i];
        const auto moving_piece = board.GetState().GetPieceType(move.GetFrom());
        piece_moves[moving_piece].Push(move);
      }

      constexpr std::array<int, kNumPieceTypes> kPieceProbabilities = {
          35, 25, 25, 5, 5, 5};

      static thread_local std::random_device rd;
      static thread_local std::mt19937 gen(rd());
      static thread_local std::discrete_distribution<> dist(
          kPieceProbabilities.begin(), kPieceProbabilities.end());

      int chosen_piece = dist(gen);
      auto &chosen_moves = piece_moves[chosen_piece];
      if (!chosen_moves.Empty()) {
        random_move = SelectPreferredMove(chosen_moves, board.GetState().turn);
      } else {
        current_ply = 0;
        board.SetFromFen(fen::kStartFen);
        continue;
      }
    }

    board.MakeMove(random_move);

    // Prevent the last ply from being a checkmate/stalemate
    if (++current_ply == target_plies && board.GetLegalMoves().Empty()) {
      current_ply = 0;
      board.SetFromFen(fen::kStartFen);
      continue;
    }
  }
}

std::atomic<U64> positions_written = 0, games_completed = 0, start_time = 0;
std::mutex display_mutex;

void PrintProgress(const Config &config, U64 completed, U64 written) {
  std::lock_guard lock(display_mutex);

  auto current_time = GetCurrentTime();
  auto elapsed_time = current_time - start_time;
  auto games_left = config.num_games - completed;
  auto time_per_game = elapsed_time / std::max<U64>(1, completed);
  auto time_remaining = time_per_game * games_left;

  // Calculate progress bar
  int bar_width = 50;
  double progress = static_cast<double>(completed) / config.num_games;
  int filled_length = static_cast<int>(std::round(bar_width * progress));

  std::string bar;
  for (int i = 0; i < bar_width; ++i) {
    if (i < filled_length) {
      bar += "\u2588";  // Full block
    } else {
      bar += " ";  // Light shade
    }
  }

  // Calculate speeds
  double games_per_second =
      static_cast<double>(completed) / (elapsed_time / 1000.0);
  double positions_per_second =
      static_cast<double>(written) / (elapsed_time / 1000.0);

  // Format time remaining
  std::string time_str;
  if (time_remaining >= 3600000) {
    time_str = fmt::format("{}h {}m {}s",
                           time_remaining / 3600000,
                           (time_remaining % 3600000) / 60000,
                           (time_remaining % 60000) / 1000);
  } else if (time_remaining >= 60000) {
    time_str = fmt::format(
        "{}m {}s", time_remaining / 60000, (time_remaining % 60000) / 1000);
  } else {
    time_str = fmt::format("{}s", time_remaining / 1000);
  }

  // Clear previous lines (5 lines total)
  fmt::print("\033[5F\033[J");

  // Print updated progress with green bar and aligned gray data
  fmt::print("{:15} [", "Progress:");
  fmt::print(fg(fmt::color::green), "{}", bar);
  fmt::print("] ");
  fmt::print(
      fg(fmt::color::gray), "{}% complete\n", static_cast<int>(progress * 100));

  fmt::print("{:15} ", "Games:");
  fmt::print(fg(fmt::color::gray), "{} / {}\n", completed, config.num_games);

  fmt::print("{:15} ", "Positions:");
  fmt::print(fg(fmt::color::gray), "{}\n", written);

  fmt::print("{:15} ", "Time remaining:");
  fmt::print(fg(fmt::color::gray), "{}\n", time_str);

  fmt::print("{:15} ", "Speed:");
  fmt::print(fg(fmt::color::gray),
             "{:.1f} games/s, {:.1f} pos/s\n",
             games_per_second,
             positions_per_second);

  // Ensure output is displayed immediately
  std::cout.flush();
}

void GameLoop(const Config &config,
              int thread_id,
              std::ostream &output_stream,
              const std::vector<std::string> &fens) {
  RandomSeed(thread_id, GetCurrentTime());

  constexpr int kWinThreshold = 2500;
  constexpr int kWinPliesThreshold = 5;
  constexpr int kDrawThreshold = 2;
  constexpr int kDrawPliesThreshold = 8;
  constexpr int kInitialScoreThreshold = 300;

  search::TimeConfig time_config{.nodes = config.hard_node_limit,
                                 .soft_nodes = config.soft_node_limit};
  format::BinPackFormatter formatter(output_stream);

  auto thread = std::make_unique<search::Thread>(0);

  search::Searcher searcher(thread->board);
  searcher.ResizeHash(16);

  const int workload = config.num_games / config.num_threads;
  for (int i = 0; i < workload && !stop; i++) {
    // Find a valid legal position to play the game from
    FindStartingPosition(thread->board, config, fens);

    const auto &state = thread->board.GetState();
    formatter.SetPosition(state);

    searcher.NewGame();
    thread->NewGame();

    const auto [initial_score, _] = searcher.DataGenStart(
        thread, search::TimeConfig{.depth = 10, .nodes = 1'000'000});

    if (std::abs(initial_score) >= kInitialScoreThreshold) {
      --i;
      continue;
    }

    U64 win_plies = 0, loss_plies = 0, draw_plies = 0;

    std::optional<double> wdl_outcome;
    while (!stop) {
      // Score returned as white-relative
      const auto [score, best_move] =
          searcher.DataGenStart(thread, time_config);

      // The game has ended
      if (!best_move) {
        wdl_outcome = state.InCheck() ? state.turn == Color::kBlack : 0.5;
        break;
      } else {
        if (std::abs(score) >= kMateInMaxPlyScore) {
          // Return the correct score depending on who is getting checkmated
          wdl_outcome = score > 0;
        } else {
          if (score >= kWinThreshold) {
            ++win_plies, loss_plies = draw_plies = 0;
          } else if (score <= -kWinThreshold) {
            ++loss_plies, win_plies = draw_plies = 0;
          } else if (std::abs(score) <= kDrawThreshold &&
                     state.half_moves >= 200) {
            ++draw_plies, win_plies = loss_plies = 0;
          }

          if (win_plies >= kWinPliesThreshold) {
            wdl_outcome = 1.0;
          } else if (loss_plies >= kWinPliesThreshold) {
            wdl_outcome = 0.0;
          } else if (draw_plies >= kDrawPliesThreshold) {
            wdl_outcome = 0.5;
          }
        }
      }

      thread->board.MakeMove(best_move);

      // Check for draw here since search doesn't terminate with an adjudicated
      // draw score at root
      if (thread->board.IsRepetition(0) || thread->board.IsInsufficientMaterial()) {
        wdl_outcome = 0.5;
        break;
      }

      formatter.PushMove(best_move, state.turn, score);

      if (wdl_outcome) {
        break;
      }
    }

    if (wdl_outcome) {
      const auto written = positions_written.fetch_add(
          formatter.WriteOutcome(*wdl_outcome), std::memory_order_relaxed);
      const auto completed =
          games_completed.fetch_add(1, std::memory_order_relaxed) + 1;

      if (completed % std::min<U64>(config.num_games / 50, 1000ULL) == 0 ||
          completed == 1) {
        PrintProgress(config, completed, written);
      }
    }
  }

  PrintProgress(config,
                games_completed.load(std::memory_order_relaxed),
                positions_written.load(std::memory_order_relaxed));
}

void signal_handler([[maybe_unused]] int signum) {
  stop = true;
}

void Generate(Config config) {
  fmt::println("Starting data generation process...\n");

  games_completed = positions_written = 0;

  // Handle Ctrl + C
  std::signal(SIGINT, signal_handler);

  // Change the number of games to fit evenly within the number of threads
  config.num_games -= config.num_games % config.num_threads;

  const auto time = std::time(nullptr);
  const auto tm = *std::localtime(&time);

  std::stringstream buffer;
  buffer << std::put_time(&tm, "%d-%m-%Y");

  const auto path = config.output_file + "-" + buffer.str();
  start_time = GetCurrentTime();

  std::vector<std::thread> threads;
  threads.reserve(config.num_threads);

  // Parse FENs file for opening positions if it exists
  std::vector<std::string> fens;
  if (!config.fens_file.empty()) {
    std::ifstream input_stream(config.fens_file);
    std::string fen;
    while (std::getline(input_stream, fen)) {
      fens.emplace_back(fen);
    }

    std::random_device rd;
    std::ranges::shuffle(fens, std::mt19937{rd()});
  }

  std::vector<std::string> temp_files;

  for (int i = 0; i < config.num_threads; i++) {
    auto thread_path = path + fmt::format("_temp{}", i);
    temp_files.push_back(thread_path);

    threads.emplace_back(
        [&config, thread_path = std::move(thread_path), i, &fens]() {
          std::ofstream output_stream(thread_path,
                                      std::ios::binary | std::ios::app);
          if (!output_stream) {
            fmt::println(
                "Error: Failed to open output file {} for thread {} '{}'",
                thread_path,
                i,
                strerror(errno));
            return;
          }

          GameLoop(config, i, output_stream, fens);

          output_stream.close();
          if (output_stream.good()) {
            // Explicitly flush to ensure all data is written
            output_stream.flush();
          } else {
            fmt::println(
                "Error: Thread {} encountered an issue while closing the file",
                i);
          }
        });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  // Add a small delay to ensure file system sync
  std::this_thread::sleep_for(std::chrono::seconds(1));

  fmt::println("");

  // Concatenate all temp files into one big file
  std::ofstream final_output(path, std::ios::binary);
  if (!final_output) {
    fmt::println("Error: Failed to open final output file {}", path);
    return;
  }

  int concatenated_files = 0;
  for (const auto &temp_file : temp_files) {
    std::ifstream input(temp_file, std::ios::binary);
    if (input) {
      final_output << input.rdbuf();
      input.close();
      // Delete the temp file
      if (std::remove(temp_file.c_str()) == 0) {
        concatenated_files++;
        fmt::println("Successfully concatenated and removed temp file {}",
                     temp_file);
      } else {
        fmt::println(
            "Error: Failed to remove temp file {} after concatenation. Error: "
            "{}",
            temp_file,
            strerror(errno));
      }
    } else {
      fmt::println(
          "Error: Failed to open temp file {} for concatenation. Error: {}",
          temp_file,
          strerror(errno));
    }
  }

  final_output.close();

  fmt::println("Concatenated {} out of {} expected temp files",
               concatenated_files,
               config.num_threads);
}

}  // namespace data_gen