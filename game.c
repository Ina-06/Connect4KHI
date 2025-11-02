
#include "game.h"
#include "board.h"
#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

// Helpers for AI + existing easy bot

// Pick a random valid column (0..6), or -1 if none
static int random_valid_column(const Board *b) {
    int valid_cols[COLS];
    int count = 0;
    for (int c = 0; c < COLS; ++c) {
        if (b->cells[0][c] == ' ')
            valid_cols[count++] = c;
    }
    if (count == 0) return -1;
    return valid_cols[rand() % count];
}

// Read "1".."3" as mode using fgets 
static int read_mode_choice(void) {
    char line[32];
    while (1) {
        puts("Select mode:");
        puts("  1 - Play against Easy Bot (You are Player A)");
        puts("  2 - Play against Medium Bot (You are Player A)");
        puts("  3 - Two Player mode (A vs B)");
        fputs("Enter 1, 2, or 3: ", stdout);
        fflush(stdout);

        if (!fgets(line, sizeof line, stdin)) {
            fputc('\\n', stdout);
            return 0; // treat EOF as cancel
        }

        // trim trailing newline
        size_t n = strlen(line);
        if (n && (line[n-1] == '\\n' || line[n-1] == '\\r')) line[--n] = '\\0';

        if ((n == 1) && (line[0] == '1' || line[0] == '2' || line[0] == '3')) {
            return (line[0] - '0');
        }

        puts("Invalid choice. Please enter 1, 2, or 3.\\n");
    }
}

/* 
   Medium Bot (heuristic 1-ply + safety)
   - Win now if possible
   - Else block opponent's immediate win
   - Else avoid moves that allow opponent immediate win next
   - Else prefer center and higher-scoring positions
   */

static inline char opp_of(char p) { return (p == 'A') ? 'B' : 'A'; }

// Find next open row in column 
static int find_next_open_row(const Board *b, int col) {
    if (col < 0 || col >= COLS) return -1;
    for (int r = ROWS - 1; r >= 0; --r) {
        if (b->cells[r][col] == ' ') return r;
    }
    return -1;
}

// 4-cell window scoring
static int score_window_chars(const char w[4], char me) {
    char you = opp_of(me);
    int meCnt = 0, youCnt = 0, emptyCnt = 0;
    for (int i = 0; i < 4; ++i) {
        if (w[i] == me)      ++meCnt;
        else if (w[i] == you)++youCnt;
        else                 ++emptyCnt;
    }
    if (meCnt == 4) return 100000;
    if (meCnt == 3 && emptyCnt == 1) return 500;
    if (meCnt == 2 && emptyCnt == 2) return 50;

    if (youCnt == 3 && emptyCnt == 1) return -450;
    if (youCnt == 2 && emptyCnt == 2) return -40;
    return 0;
}

static int score_position(const Board *b, char me) {
    int score = 0;

    // Center bias
    int center = COLS / 2;
    int centerCount = 0;
    for (int r = 0; r < ROWS; ++r) if (b->cells[r][center] == me) ++centerCount;
    score += centerCount * 7;

    // Windows of 4 in all directions
    char w[4];
    // Horizontal
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS - 3; ++c) {
            for (int k = 0; k < 4; ++k) w[k] = b->cells[r][c + k];
            score += score_window_chars(w, me);
        }
    }
    // Vertical
    for (int c = 0; c < COLS; ++c) {
        for (int r = 0; r < ROWS - 3; ++r) {
            for (int k = 0; k < 4; ++k) w[k] = b->cells[r + k][c];
            score += score_window_chars(w, me);
        }
    }
    // Diagonal down-right
    for (int r = 0; r < ROWS - 3; ++r) {
        for (int c = 0; c < COLS - 3; ++c) {
            for (int k = 0; k < 4; ++k) w[k] = b->cells[r + k][c + k];
            score += score_window_chars(w, me);
        }
    }
    // Diagonal up-right
    for (int r = 3; r < ROWS; ++r) {
        for (int c = 0; c < COLS - 3; ++c) {
            for (int k = 0; k < 4; ++k) w[k] = b->cells[r - k][c + k];
            score += score_window_chars(w, me);
        }
    }
    return score;
}

/* After AI hypothetically plays 'playedCol', can opponent win immediately? */
static int opp_has_immediate_win_after(const Board *b0, char aiPiece, int playedCol) {
    if (playedCol < 0 || playedCol >= COLS) return 0;
    Board b = *b0;
    int r = find_next_open_row(&b, playedCol);
    if (r < 0) return 0;
    int placed_row = -1;
    if (!board_drop(&b, playedCol, aiPiece, &placed_row)) return 0; // safety

    char opp = opp_of(aiPiece);
    for (int c = 0; c < COLS; ++c) {
        if (b.cells[0][c] != ' ') continue;
        int rr = -1;
        Board bb = b;
        if (!board_drop(&bb, c, opp, &rr)) continue;
        if (board_is_winning(&bb, rr, c, opp)) return 1;
    }
    return 0;
}

static int medium_rule_based_move(const Board *b0, char aiPiece) {
    int valid[COLS], nValid = 0;
    for (int c = 0; c < COLS; ++c)
        if (b0->cells[0][c] == ' ') valid[nValid++] = c;
    if (nValid == 0) return -1;

    /* 1) Win now if possible */
    for (int i = 0; i < nValid; ++i) {
        int c = valid[i];
        Board b = *b0;
        int row = -1;
        if (board_drop(&b, c, aiPiece, &row) && board_is_winning(&b, row, c, aiPiece))
            return c;
    }

    /* 2) Block opponent's immediate win */
    char opp = opp_of(aiPiece);
    for (int i = 0; i < nValid; ++i) {
        int c = valid[i];
        Board b = *b0;
        int row = -1;
        if (board_drop(&b, c, opp, &row) && board_is_winning(&b, row, c, opp))
            return c;
    }

    /* 3) Filter out suicidal moves */
    int safe[COLS], nSafe = 0;
    for (int i = 0; i < nValid; ++i) {
        int c = valid[i];
        if (!opp_has_immediate_win_after(b0, aiPiece, c)) safe[nSafe++] = c;
    }
    int *pool = (nSafe > 0) ? safe : valid;
    int nPool = (nSafe > 0) ? nSafe : nValid;

    /* 4) Prefer center-ish and best 1-ply score */
    int bestCol = pool[0];
    int bestScore = INT_MIN;
    int center = COLS / 2;

    for (int i = 0; i < nPool; ++i) {
        int c = pool[i];
        Board b = *b0;
        int row = -1;
        if (!board_drop(&b, c, aiPiece, &row)) continue;
        int s = score_position(&b, aiPiece);
        // Center bias, smaller prefered
        s -= (c > center ? c - center : center - c);
        if (s > bestScore) { bestScore = s; bestCol = c; }
    }

    return bestCol;
}

// Main game loop

int game_run(void) {
    Board b;
    board_init(&b);

    const char players[2] = { 'A', 'B' };
    int turn = 0;

    puts("\\n=== Welcome to Connect Four (Console) ===");
    puts("Players: A and B. Enter column 1-7 to drop a checker; 'q' to quit.\\n");

    srand((unsigned)time(NULL));
    int mode = read_mode_choice();
    if (mode == 0) {
        puts("\\nGoodbye!");
        return 0;
    }
    puts("");

    while (1) {
        board_print(&b, stdout);
        char p = players[turn];

        int col = -1;

        if (mode == 1 && p == 'B') {
            // Easy Bot: choose any random valid column
            col = random_valid_column(&b);
            if (col < 0) {
                puts("It's a draw. No more moves!");
                break;
            }
            printf("EasyBot (Player B) chooses column %d\\n", col + 1);
        } else if (mode == 2 && p == 'B') {
            // Medium Bot: heuristic 1-ply + safety
            col = medium_rule_based_move(&b, 'B');
            if (col < 0) {
                puts("It's a draw. No more moves!");
                break;
            }
            printf("MediumBot (Player B) chooses column %d\\n", col + 1);
        } else {
            // Human turn (Player A vs bot in modes 1/2, both A & B in mode 3)
            printf("Player %c \xE2\x86\x92 ", p);
            IoStatus st = io_read_column(&col);
            if (st == IO_QUIT || st == IO_EOF_QUIT) {
                puts("\\nGoodbye!");
                return 0;
            }
        }

        int row = -1;
        if (!board_drop(&b, col, p, &row)) {
            puts("That column is full or invalid. Try another.");
            continue;
        }

        if (board_is_winning(&b, row, col, p)) {
            board_print(&b, stdout);
            printf("Player %c WINS!\\n", p);
            break;
        }

        if (board_full(&b)) {
            board_print(&b, stdout);
            puts("It's a draw. No more moves!");
            break;
        }

        turn ^= 1; // swap player
    }

    puts("Press Enter to exit...");
    (void)getchar();
    return 0;
}
