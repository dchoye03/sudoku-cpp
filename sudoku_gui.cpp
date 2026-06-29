// ============================================================================
// Sudoku — graphics.h (WinBGIm) windowed version
//
// Opens a real game window. Click a cell to select it, then type 1-9 to fill
// (Backspace / 0 clears). On-screen buttons + keyboard shortcuts:
//     New (n)   Hint (h)   Solve (s)   Quit (q / Esc)
//
// Build (Code::Blocks / MinGW with WinBGIm installed):
//   g++ sudoku_gui.cpp -o sudoku_gui ^
//       -lbgi -lgdi32 -lcomdlg32 -luuid -loleaut32 -lole32
// Run:
//   sudoku_gui.exe
// ============================================================================

#include <graphics.h>
#include <array>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include <conio.h>

using Board = std::array<std::array<int, 9>, 9>;

// ----- Layout constants (pixels) --------------------------------------------
const int CELL   = 60;                 // cell size
const int BOARD  = CELL * 9;           // 540
const int WIN_W  = BOARD;              // 540
const int WIN_H  = BOARD + 120;        // 660 (board + status panel)
const int PANEL_Y = BOARD;             // top of status panel

// ----- Core Sudoku logic (ported from sudoku.cpp) ---------------------------

bool isValid(const Board& b, int row, int col, int num) {
    for (int i = 0; i < 9; ++i) {
        if (b[row][i] == num) return false;
        if (b[i][col] == num) return false;
    }
    int br = (row / 3) * 3, bc = (col / 3) * 3;
    for (int r = br; r < br + 3; ++r)
        for (int c = bc; c < bc + 3; ++c)
            if (b[r][c] == num) return false;
    return true;
}

bool findEmpty(const Board& b, int& row, int& col) {
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            if (b[r][c] == 0) { row = r; col = c; return true; }
    return false;
}

bool solve(Board& b) {
    int row, col;
    if (!findEmpty(b, row, col)) return true;
    for (int num = 1; num <= 9; ++num) {
        if (isValid(b, row, col, num)) {
            b[row][col] = num;
            if (solve(b)) return true;
            b[row][col] = 0;
        }
    }
    return false;
}

bool fillBoard(Board& b, std::mt19937& rng) {
    int row, col;
    if (!findEmpty(b, row, col)) return true;
    std::array<int, 9> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::shuffle(nums.begin(), nums.end(), rng);
    for (int num : nums) {
        if (isValid(b, row, col, num)) {
            b[row][col] = num;
            if (fillBoard(b, rng)) return true;
            b[row][col] = 0;
        }
    }
    return false;
}

int blanksForLevel(int level) {          // 0 easy, 1 medium, 2 hard
    if (level == 0) return 35;
    if (level == 2) return 55;
    return 45;
}

void generatePuzzle(Board& puzzle, Board& solution, int level,
                    std::mt19937& rng) {
    Board b{};
    fillBoard(b, rng);
    solution = b;
    std::vector<std::pair<int,int>> cells;
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) cells.emplace_back(r, c);
    std::shuffle(cells.begin(), cells.end(), rng);
    int blanks = blanksForLevel(level);
    for (int i = 0; i < blanks; ++i) b[cells[i].first][cells[i].second] = 0;
    puzzle = b;
}

bool isComplete(Board b) {
    int row, col;
    if (findEmpty(b, row, col)) return false;
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) {
            int num = b[r][c];
            b[r][c] = 0;
            if (!isValid(b, r, c, num)) return false;
            b[r][c] = num;
        }
    return true;
}

std::string formatTime(long s) {
    char buf[16];
    std::sprintf(buf, "%02ld:%02ld", s / 60, s % 60);
    return buf;
}

// ----- Game state -----------------------------------------------------------

Board puzzle{}, solution{};
std::array<std::array<bool, 9>, 9> given{};
int selR = -1, selC = -1;              // selected cell, -1 = none
int moves = 0, hints = 0;
int level = 1;                          // 0 easy, 1 medium, 2 hard
const char* levelName[] = {"Easy", "Medium", "Hard"};
time_t startTime;
std::string message;
bool finished = false;
std::mt19937 rng((unsigned)time(nullptr));

// ----- Drawing helpers ------------------------------------------------------

void drawText(int x, int y, const std::string& s, int color,
              int size = 1, int hjust = LEFT_TEXT) {
    setcolor(color);
    settextstyle(SANS_SERIF_FONT, HORIZ_DIR, size);
    settextjustify(hjust, CENTER_TEXT);
    outtextxy(x, y, const_cast<char*>(s.c_str()));
}

// A clickable button. Returns true if (mx,my) is inside it; draws it.
struct Button { int x, y, w, h; std::string label; };
Button btnNew   {  8, PANEL_Y + 50, 120, 38, "New (n)"};
Button btnHint  {138, PANEL_Y + 50, 120, 38, "Hint (h)"};
Button btnSolve {268, PANEL_Y + 50, 120, 38, "Solve (s)"};
Button btnLevel {398, PANEL_Y + 50, 134, 38, "Easy"};

void drawButton(const Button& b) {
    setfillstyle(SOLID_FILL, COLOR(40, 56, 92));
    bar(b.x, b.y, b.x + b.w, b.y + b.h);
    setcolor(COLOR(120, 150, 200));
    rectangle(b.x, b.y, b.x + b.w, b.y + b.h);
    drawText(b.x + b.w / 2, b.y + b.h / 2, b.label, WHITE, 1, CENTER_TEXT);
}

bool inside(const Button& b, int mx, int my) {
    return mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h;
}

void draw() {
    // Background.
    setfillstyle(SOLID_FILL, COLOR(15, 20, 34));
    bar(0, 0, WIN_W, WIN_H);

    // Cell backgrounds (highlight selected row/col/box + the selected cell).
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) {
            int x = c * CELL, y = r * CELL;
            int bg = COLOR(31, 41, 64);                 // default
            if (given[r][c]) bg = COLOR(19, 26, 40);    // fixed clue
            if (selR >= 0) {
                bool peer = (r == selR) || (c == selC) ||
                            (r / 3 == selR / 3 && c / 3 == selC / 3);
                if (peer) bg = COLOR(26, 35, 54);
                if (selR >= 0 && puzzle[selR][selC] != 0 &&
                    puzzle[r][c] == puzzle[selR][selC]) bg = COLOR(35, 51, 86);
                if (r == selR && c == selC) bg = COLOR(37, 60, 110);
            }
            setfillstyle(SOLID_FILL, bg);
            bar(x + 1, y + 1, x + CELL - 1, y + CELL - 1);
        }

    // Numbers.
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) {
            if (puzzle[r][c] == 0) continue;
            int color = given[r][c] ? COLOR(200, 210, 230)
                                    : COLOR(110, 175, 255);
            drawText(c * CELL + CELL / 2, r * CELL + CELL / 2,
                     std::to_string(puzzle[r][c]), color, 3, CENTER_TEXT);
        }

    // Grid lines (thick every 3).
    for (int i = 0; i <= 9; ++i) {
        int t = (i % 3 == 0) ? 3 : 1;
        setcolor(i % 3 == 0 ? COLOR(120, 140, 175) : COLOR(58, 70, 95));
        setlinestyle(SOLID_LINE, 0, t);
        line(i * CELL, 0, i * CELL, BOARD);
        line(0, i * CELL, BOARD, i * CELL);
    }
    setlinestyle(SOLID_LINE, 0, 1);

    // Status panel.
    long elapsed = finished ? 0 : (long)(time(nullptr) - startTime);
    static long frozen = 0;
    if (!finished) frozen = elapsed; else elapsed = frozen;
    std::string stat = "Time " + formatTime(elapsed) +
                       "    Moves " + std::to_string(moves) +
                       "    Hints " + std::to_string(hints);
    drawText(WIN_W / 2, PANEL_Y + 22, stat, COLOR(200, 210, 230), 1, CENTER_TEXT);

    btnLevel.label = levelName[level];
    drawButton(btnNew);
    drawButton(btnHint);
    drawButton(btnSolve);
    drawButton(btnLevel);

    if (!message.empty())
        drawText(WIN_W / 2, PANEL_Y + 102, message, COLOR(255, 207, 92),
                 1, CENTER_TEXT);
}

// ----- Game actions ---------------------------------------------------------

void newGame() {
    generatePuzzle(puzzle, solution, level, rng);
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) given[r][c] = (puzzle[r][c] != 0);
    selR = selC = -1;
    moves = hints = 0;
    finished = false;
    message = std::string(levelName[level]) + " puzzle — good luck!";
    startTime = time(nullptr);
}

void placeNumber(int value) {
    if (finished || selR < 0) return;
    if (given[selR][selC]) { message = "That's a fixed clue."; return; }
    if (value == 0) {
        if (puzzle[selR][selC] != 0) { puzzle[selR][selC] = 0; ++moves; }
        message.clear();
        return;
    }
    int prev = puzzle[selR][selC];
    puzzle[selR][selC] = 0;
    if (isValid(puzzle, selR, selC, value)) {
        puzzle[selR][selC] = value;
        ++moves;
        message.clear();
        if (isComplete(puzzle)) { finished = true; message = "Solved! Press n for a new game."; }
    } else {
        puzzle[selR][selC] = prev;
        message = "Breaks Sudoku rules (row / column / box).";
    }
}

void doHint() {
    if (finished) return;
    std::vector<std::pair<int,int>> empties;
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            if (puzzle[r][c] == 0) empties.emplace_back(r, c);
    if (empties.empty()) return;
    std::uniform_int_distribution<size_t> dist(0, empties.size() - 1);
    auto [r, c] = empties[dist(rng)];
    puzzle[r][c] = solution[r][c];
    ++moves; ++hints;
    message = "Hint placed at row " + std::to_string(r + 1) +
              ", col " + std::to_string(c + 1) + ".";
    if (isComplete(puzzle)) { finished = true; message = "Solved! Press n for a new game."; }
}

void doSolve() {
    if (finished) return;
    Board work = puzzle;
    if (solve(work)) { puzzle = work; finished = true; message = "Auto-solved. Press n for a new game."; }
    else message = "Can't solve from current state.";
}

// ----- Main loop ------------------------------------------------------------

int main() {
    initwindow(WIN_W, WIN_H, "Sudoku");
    newGame();

    bool running = true;
    long lastDrawnSec = -1;

    while (running) {
        // Mouse clicks.
        if (ismouseclick(WM_LBUTTONDOWN)) {
            int mx, my;
            getmouseclick(WM_LBUTTONDOWN, mx, my);
            if (my < BOARD) {                      // clicked the board
                selC = mx / CELL;
                selR = my / CELL;
            } else {                                // clicked the panel
                if (inside(btnNew, mx, my))   newGame();
                else if (inside(btnHint, mx, my))  doHint();
                else if (inside(btnSolve, mx, my)) doSolve();
                else if (inside(btnLevel, mx, my)) { level = (level + 1) % 3; }
            }
            draw();
        }

        // Keyboard.
        if (kbhit()) {
            int ch = getch();
            if (ch >= '1' && ch <= '9') placeNumber(ch - '0');
            else if (ch == '0' || ch == 8 || ch == 127) placeNumber(0);
            else if (ch == 'n' || ch == 'N') newGame();
            else if (ch == 'h' || ch == 'H') doHint();
            else if (ch == 's' || ch == 'S') doSolve();
            else if (ch == 'q' || ch == 'Q' || ch == 27) running = false;
            else if (ch == 0 || ch == 224) {        // arrow keys (two-byte)
                int k = getch();
                if (selR < 0) { selR = selC = 0; }
                else if (k == 72) selR = (selR + 8) % 9;   // up
                else if (k == 80) selR = (selR + 1) % 9;   // down
                else if (k == 75) selC = (selC + 8) % 9;   // left
                else if (k == 77) selC = (selC + 1) % 9;   // right
            }
            draw();
        }

        // Redraw once per second so the timer ticks.
        long sec = (long)(time(nullptr) - startTime);
        if (!finished && sec != lastDrawnSec) { lastDrawnSec = sec; draw(); }

        delay(20);                                  // ~50 fps, low CPU
    }

    closegraph();
    return 0;
}
