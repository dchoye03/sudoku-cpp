// ============================================================================
// Interactive Sudoku Game (9x9) in C++
//
// Features:
//   - Play an auto-generated puzzle (Easy / Medium / Hard), OR enter your own
//     ARBITRARY puzzle and play / auto-solve it.
//   - Every move is validated in REAL TIME against row, column, and 3x3 box
//     constraints; illegal moves are rejected immediately.
//   - A RECURSIVE BACKTRACKING solver can auto-solve any valid puzzle.
//   - Hint system, move counter, and a timer.
//
// Build:   g++ -std=c++17 -O2 -o sudoku sudoku.cpp
// Run:     ./sudoku        (Windows: sudoku.exe)
// ============================================================================

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <random>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <cctype>

using Board = std::array<std::array<int, 9>, 9>;

// Difficulty -> number of blank cells removed from a full board.
int blanksForLevel(const std::string& level) {
    if (level == "easy")   return 35;
    if (level == "medium") return 45;
    if (level == "hard")   return 55;
    return 45;
}

// ----------------------------------------------------------------------------
// Core Sudoku rules
// ----------------------------------------------------------------------------

// Real-time constraint check: can `num` go at (row, col) without violating
// the row, column, or 3x3 box? Assumes (row, col) is currently empty/ignored.
bool isValid(const Board& board, int row, int col, int num) {
    // Row and column.
    for (int i = 0; i < 9; ++i) {
        if (board[row][i] == num) return false;
        if (board[i][col] == num) return false;
    }
    // 3x3 box.
    int boxRow = (row / 3) * 3;
    int boxCol = (col / 3) * 3;
    for (int r = boxRow; r < boxRow + 3; ++r)
        for (int c = boxCol; c < boxCol + 3; ++c)
            if (board[r][c] == num) return false;

    return true;
}

// Find the next empty cell (value 0). Returns true and sets row/col if found.
bool findEmpty(const Board& board, int& row, int& col) {
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            if (board[r][c] == 0) { row = r; col = c; return true; }
    return false;
}

// ----------------------------------------------------------------------------
// Recursive backtracking solver
// ----------------------------------------------------------------------------

// Solves the board in place. Returns true if a solution exists.
bool solve(Board& board) {
    int row, col;
    if (!findEmpty(board, row, col))
        return true;  // No empty cells -> solved.

    for (int num = 1; num <= 9; ++num) {
        if (isValid(board, row, col, num)) {
            board[row][col] = num;
            if (solve(board)) return true;
            board[row][col] = 0;  // Backtrack.
        }
    }
    return false;
}

// Same backtracking, but tries digits in random order to produce a varied
// full solution when generating puzzles.
bool fillBoard(Board& board, std::mt19937& rng) {
    int row, col;
    if (!findEmpty(board, row, col))
        return true;

    std::array<int, 9> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::shuffle(nums.begin(), nums.end(), rng);

    for (int num : nums) {
        if (isValid(board, row, col, num)) {
            board[row][col] = num;
            if (fillBoard(board, rng)) return true;
            board[row][col] = 0;
        }
    }
    return false;
}

// ----------------------------------------------------------------------------
// Puzzle generation
// ----------------------------------------------------------------------------

void generatePuzzle(Board& puzzle, Board& solution, int numBlanks,
                    std::mt19937& rng) {
    Board board{};                 // zero-initialized
    fillBoard(board, rng);
    solution = board;

    std::vector<std::pair<int,int>> cells;
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            cells.emplace_back(r, c);
    std::shuffle(cells.begin(), cells.end(), rng);

    int removed = 0;
    for (auto& [r, c] : cells) {
        if (removed >= numBlanks) break;
        board[r][c] = 0;
        ++removed;
    }
    puzzle = board;
}

// ----------------------------------------------------------------------------
// Display
// ----------------------------------------------------------------------------

void printBoard(const Board& board) {
    std::cout << "\n    1 2 3   4 5 6   7 8 9\n";
    std::cout << "  +-------+-------+-------+\n";
    for (int r = 0; r < 9; ++r) {
        std::cout << (r + 1) << " | ";
        for (int c = 0; c < 9; ++c) {
            int v = board[r][c];
            std::cout << (v == 0 ? '.' : char('0' + v)) << ' ';
            if ((c + 1) % 3 == 0) std::cout << "| ";
        }
        std::cout << '\n';
        if ((r + 1) % 3 == 0)
            std::cout << "  +-------+-------+-------+\n";
    }
    std::cout << '\n';
}

// Is the board fully filled and consistent with all constraints?
bool isComplete(Board board) {  // by value: we temporarily clear cells
    int row, col;
    if (findEmpty(board, row, col)) return false;
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) {
            int num = board[r][c];
            board[r][c] = 0;
            if (!isValid(board, r, c, num)) return false;
            board[r][c] = num;
        }
    return true;
}

std::string formatTime(long long seconds) {
    long long m = seconds / 60;
    long long s = seconds % 60;
    std::ostringstream os;
    os << (m < 10 ? "0" : "") << m << ":" << (s < 10 ? "0" : "") << s;
    return os.str();
}

// ----------------------------------------------------------------------------
// Input helpers
// ----------------------------------------------------------------------------

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) { return std::tolower(ch); });
    return s;
}

// Let the user type an arbitrary puzzle: 9 lines of 9 digits, 0 (or .) = blank.
// Returns true on success; validates that the entered puzzle is self-consistent.
bool readCustomPuzzle(Board& puzzle) {
    std::cout << "\nEnter your puzzle: 9 rows, 9 chars each (use 0 or . for "
                 "blanks).\nExample row: 53..7....\n\n";
    Board board{};
    for (int r = 0; r < 9; ++r) {
        std::string line;
        std::cout << "Row " << (r + 1) << ": ";
        if (!std::getline(std::cin, line)) return false;

        // Strip spaces.
        line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
        if (static_cast<int>(line.size()) != 9) {
            std::cout << "Each row must have exactly 9 characters. Try again.\n";
            --r;
            continue;
        }
        bool ok = true;
        for (int c = 0; c < 9; ++c) {
            char ch = line[c];
            if (ch == '.' || ch == '0') {
                board[r][c] = 0;
            } else if (ch >= '1' && ch <= '9') {
                board[r][c] = ch - '0';
            } else {
                std::cout << "Invalid character '" << ch
                          << "'. Use 1-9, 0, or '.'. Try again.\n";
                ok = false;
                break;
            }
        }
        if (!ok) { --r; continue; }
    }

    // Validate self-consistency of the supplied clues.
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) {
            int num = board[r][c];
            if (num == 0) continue;
            board[r][c] = 0;
            if (!isValid(board, r, c, num)) {
                board[r][c] = num;
                std::cout << "The clue " << num << " at row " << (r + 1)
                          << ", col " << (c + 1)
                          << " conflicts with another. Puzzle is invalid.\n";
                return false;
            }
            board[r][c] = num;
        }

    puzzle = board;
    return true;
}

// ----------------------------------------------------------------------------
// Game loop
// ----------------------------------------------------------------------------

void play() {
    std::random_device rd;
    std::mt19937 rng(rd());

    std::cout << "=================================\n";
    std::cout << "       WELCOME TO SUDOKU\n";
    std::cout << "=================================\n\n";

    Board puzzle{}, solution{};
    bool haveSolution = false;
    std::vector<std::vector<bool>> given(9, std::vector<bool>(9, false));

    // Choose mode.
    std::cout << "Choose mode:\n";
    std::cout << "  1. Generated puzzle (easy / medium / hard)\n";
    std::cout << "  2. Enter your own (arbitrary) puzzle\n";
    std::string mode;
    while (true) {
        std::cout << "Enter 1 or 2: ";
        if (!std::getline(std::cin, mode)) return;
        mode = toLower(mode);
        if (mode == "1" || mode == "2") break;1
        std::cout << "Invalid choice.\n";
    }

    if (mode == "1") {
        std::string level;
        while (true) {
            std::cout << "\nChoose a difficulty:\n";
            std::cout << "  1. Easy\n  2. Medium\n  3. Hard\n";
            std::cout << "Enter 1, 2, or 3: ";
            std::string c;
            if (!std::getline(std::cin, c)) return;
            if (c == "1") { level = "easy";   break; }
            if (c == "2") { level = "medium"; break; }
            if (c == "3") { level = "hard";   break; }
            std::cout << "Invalid choice.\n";
        }
        int numBlanks = blanksForLevel(level);
        std::cout << "\nGenerating a " << level << " puzzle ("
                  << numBlanks << " blanks)...\n";
        generatePuzzle(puzzle, solution, numBlanks, rng);
        haveSolution = true;
    } else {
        if (!readCustomPuzzle(puzzle)) {
            std::cout << "Could not load a valid puzzle. Exiting.\n";
            return;
        }
        // Compute a solution so hints work; warn if unsolvable.
        solution = puzzle;
        if (solve(solution)) {
            haveSolution = true;
        } else {
            std::cout << "Warning: this puzzle has no solution. "
                         "Hints will be unavailable.\n";
            haveSolution = false;
        }
    }

    // Record the given (fixed) clues.
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            given[r][c] = (puzzle[r][c] != 0);

    std::cout << "\nInstructions:\n";
    std::cout << "  - Enter a move as: row col value  (e.g. 1 3 7)\n";
    std::cout << "  - Clear a cell:    row col 0       (e.g. 1 3 0)\n";
    std::cout << "  - 'hint'  -> reveal one correct cell\n";
    std::cout << "  - 'solve' -> auto-solve via backtracking\n";
    std::cout << "  - 'quit'  -> exit\n";

    int moves = 0, hints = 0;
    auto start = std::chrono::steady_clock::now();

    while (true) {
        printBoard(puzzle);

        auto now = std::chrono::steady_clock::now();
        long long elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        std::cout << "Time: " << formatTime(elapsed)
                  << "   Moves: " << moves
                  << "   Hints: " << hints << "\n";

        if (isComplete(puzzle)) {
            std::cout << "\nCongratulations! You solved the puzzle!\n";
            std::cout << "Finished in " << formatTime(elapsed) << " with "
                      << moves << " moves and " << hints << " hints.\n";
            break;
        }

        std::cout << "Your move (row col value): ";
        std::string line;
        if (!std::getline(std::cin, line)) break;
        std::string cmd = toLower(line);

        if (cmd == "quit") {
            std::cout << "Thanks for playing! Time " << formatTime(elapsed)
                      << ", " << moves << " moves, " << hints << " hints.\n";
            break;
        }

        if (cmd == "solve") {
            Board work = puzzle;
            if (solve(work)) {
                puzzle = work;
                std::cout << "\nSolved by backtracking:\n";
            } else {
                std::cout << "This puzzle cannot be solved from its current "
                             "state.\n";
            }
            continue;
        }

        if (cmd == "hint") {
            if (!haveSolution) {
                std::cout << "No solution available for hints.\n";
                continue;
            }
            std::vector<std::pair<int,int>> empties;
            for (int r = 0; r < 9; ++r)
                for (int c = 0; c < 9; ++c)
                    if (puzzle[r][c] == 0) empties.emplace_back(r, c);
            if (empties.empty()) {
                std::cout << "No empty cells left to hint.\n";
                continue;
            }
            std::uniform_int_distribution<size_t> dist(0, empties.size() - 1);
            auto [r, c] = empties[dist(rng)];
            puzzle[r][c] = solution[r][c];
            ++moves; ++hints;
            std::cout << "Hint: placed " << solution[r][c] << " at row "
                      << (r + 1) << ", col " << (c + 1) << ".\n";
            continue;
        }

        // Parse "row col value".
        std::istringstream iss(line);
        int row, col, value;
        if (!(iss >> row >> col >> value)) {
            std::cout << "Invalid input. Enter: row col value (e.g. 1 3 7)\n";
            continue;
        }
        if (row < 1 || row > 9 || col < 1 || col > 9 || value < 0 || value > 9) {
            std::cout << "Out of range. Row/col 1-9, value 0-9.\n";
            continue;
        }

        int r = row - 1, c = col - 1;
        if (given[r][c]) {
            std::cout << "That cell is a fixed clue and cannot be changed.\n";
            continue;
        }

        if (value == 0) {
            puzzle[r][c] = 0;  // Clear.
            ++moves;
            continue;
        }

        // Real-time rule check against row, column, and box.
        int prev = puzzle[r][c];
        puzzle[r][c] = 0;
        if (isValid(puzzle, r, c, value)) {
            puzzle[r][c] = value;
            ++moves;
        } else {
            puzzle[r][c] = prev;
            std::cout << "That move breaks Sudoku rules (row/column/box). "
                         "Try again.\n";
        }
    }
}

int main() {
    play();
    return 0;
}
