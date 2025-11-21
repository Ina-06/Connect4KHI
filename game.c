#include "game.h"
#include "board.h"
#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <pthread.h>

/* ===============================================================
   Utilities
   =============================================================== */

static inline char opp_of(char p) { return (p == 'A') ? 'B' : 'A'; }

/* Easy bot: choose a random valid column */
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

/* ===============================================================
   Robust menu reader (handles CRLF, commas, spaces)
   =============================================================== */

static int read_mode_choice(void) {
    char line[128];

    for (;;) {
        printf("Select mode:\n");
        printf("  1 - Play against Easy Bot   (You are Player A)\n");
        printf("  2 - Play against Medium Bot (You are Player A)\n");
        printf("  3 - Play against Hard Bot   (You are Player A)\n");
        printf("  4 - Two Player mode (A vs B)\n");
        printf("Enter 1, 2, 3, or 4: ");
        fflush(stdout);

        if (!fgets(line, sizeof line, stdin)) {
            putchar('\n');
            return 0; // EOF => quit
        }

        /* strip trailing CR/LF and spaces */
        size_t n = strlen(line);
        while (n > 0 && (line[n-1] == '\n' || line[n-1] == '\r' ||
                         isspace((unsigned char)line[n-1]))) {
            line[--n] = '\0';
        }

        /* skip leading spaces */
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;

        /* allow 'q'/'Q' to quit */
        if (*p == 'q' || *p == 'Q') return 0;

        /* find first digit 1..4 anywhere in the line */
        while (*p && !isdigit((unsigned char)*p)) p++;
        if (*p >= '1' && *p <= '4') {
            return *p - '0';
        }

        printf("Invalid choice. Please enter 1, 2, 3, or 4.\n\n");
    }
}

/* ===============================================================
   Heuristic scoring (for evaluation)
   =============================================================== */

static int score_window(const char w[4], char me) {
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

    if (youCnt == 3 && emptyCnt == 1) return -450; /* slightly < +500 */
    if (youCnt == 2 && emptyCnt == 2) return -40;
    return 0;
}

static int score_position(const Board *b, char me) {
    int score = 0;

    /* Center bias */
    int center = COLS / 2;
    int centerCount = 0;
    for (int r = 0; r < ROWS; ++r) {
        if (b->cells[r][center] == me)
            ++centerCount;
    }
    score += centerCount * 7;

    /* All windows of 4 */
    char w[4];

    /* Horizontal */
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS - 3; ++c) {
            for (int k = 0; k < 4; ++k)
                w[k] = b->cells[r][c + k];
            score += score_window(w, me);
        }
    }
    /* Vertical */
    for (int c = 0; c < COLS; ++c) {
        for (int r = 0; r < ROWS - 3; ++r) {
            for (int k = 0; k < 4; ++k)
                w[k] = b->cells[r + k][c];
            score += score_window(w, me);
        }
    }
    /* Diagonal down-right */
    for (int r = 0; r < ROWS - 3; ++r) {
        for (int c = 0; c < COLS - 3; ++c) {
            for (int k = 0; k < 4; ++k)
                w[k] = b->cells[r + k][c + k];
            score += score_window(w, me);
        }
    }
    /* Diagonal up-right */
    for (int r = 3; r < ROWS; ++r) {
        for (int c = 0; c < COLS - 3; ++c) {
            for (int k = 0; k < 4; ++k)
                w[k] = b->cells[r - k][c + k];
            score += score_window(w, me);
        }
    }
    return score;
}

/* After AI hypothetically plays 'playedCol', can opponent win immediately? */
static int opp_has_immediate_win_after(const Board *b0, char aiPiece, int playedCol) {
    if (playedCol < 0 || playedCol >= COLS) return 0;
    Board b = *b0;

    int placed_row = -1;
    if (!board_drop(&b, playedCol, aiPiece, &placed_row)) return 0;

    char opp = opp_of(aiPiece);
    for (int c = 0; c < COLS; ++c) {
        if (b.cells[0][c] == ' ') {
            int rr = -1;
            Board bb = b;
            if (!board_drop(&bb, c, opp, &rr)) continue;
            if (board_is_winning(&bb, rr, c, opp)) return 1;
        }
    }
    return 0;
}

/* ===============================================================
   Medium bot (heuristic 1-ply + safety)
   =============================================================== */

static int medium_rule_based_move(const Board *b0, char aiPiece) {
    int valid[COLS], nValid = 0;
    for (int c = 0; c < COLS; ++c) {
        if (b0->cells[0][c] == ' ')
            valid[nValid++] = c;
    }
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
        if (!opp_has_immediate_win_after(b0, aiPiece, c))
            safe[nSafe++] = c;
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
        s -= (c > center ? c - center : center - c); /* light center bias */
        if (s > bestScore) {
            bestScore = s;
            bestCol = c;
        }
    }

    return bestCol;
}

/* ===============================================================
   Hard bot: minimax + alpha-beta pruning
   =============================================================== */

static int minimax(Board *b, int depth, int maximizing,
                   char me, int alpha, int beta) {
    if (depth == 0 || board_full(b)) {
        return score_position(b, me);
    }

    char current = maximizing ? me : opp_of(me);

    if (maximizing) {
        int best = INT_MIN;

        for (int col = 0; col < COLS; ++col) {
            if (b->cells[0][col] != ' ') continue; /* full column */

            Board child = *b;
            int row = -1;
            if (!board_drop(&child, col, current, &row)) continue;

            /* If this move wins immediately for the AI, value it very high */
            if (board_is_winning(&child, row, col, current)) {
                return 1000000 + depth; /* prefer quicker wins */
            }

            int val = minimax(&child, depth - 1, 0, me, alpha, beta);
            if (val > best) best = val;
            if (val > alpha) alpha = val;
            if (beta <= alpha) break; /* prune */
        }

        if (best == INT_MIN) {
            return score_position(b, me);
        }
        return best;
    } else {
        int best = INT_MAX;

        for (int col = 0; col < COLS; ++col) {
            if (b->cells[0][col] != ' ') continue;

            Board child = *b;
            int row = -1;
            if (!board_drop(&child, col, current, &row)) continue;

            /* If opponent can win immediately, very bad for us */
            if (board_is_winning(&child, row, col, current)) {
                return -1000000 - depth; /* prefer slower losses */
            }

            int val = minimax(&child, depth - 1, 1, me, alpha, beta);
            if (val < best) best = val;
            if (val < beta) beta = val;
            if (beta <= alpha) break; /* prune */
        }

        if (best == INT_MAX) {
            return score_position(b, me);
        }
        return best;
    }
}

/* -------- Multithreading at the root of minimax -------- */

typedef struct {
    const Board *root;
    char aiPiece;
    int depth;
    int col;
    int score;
    int valid;
} MoveTask;

static void *evaluate_move_thread(void *arg) {
    MoveTask *task = (MoveTask *)arg;

    if (task->root->cells[0][task->col] != ' ') {
        task->valid = 0;
        return NULL;
    }

    Board child = *(task->root);
    int row = -1;
    if (!board_drop(&child, task->col, task->aiPiece, &row)) {
        task->valid = 0;
        return NULL;
    }

    /* Immediate win for AI? Give huge score. */
    if (board_is_winning(&child, row, task->col, task->aiPiece)) {
        task->score = 1000000 + task->depth;
        task->valid = 1;
        return NULL;
    }

    int score = minimax(&child, task->depth - 1, 0, task->aiPiece,
                        INT_MIN / 2, INT_MAX / 2);
    task->score = score;
    task->valid = 1;
    return NULL;
}

/* Choose best move for hard bot using one thread per possible column. */
static int hard_best_move_parallel(const Board *b0, char aiPiece, int depth) {
    pthread_t threads[COLS];
    MoveTask tasks[COLS];
    int created[COLS] = {0};

    /* Spawn a thread for each non-full column */
    for (int col = 0; col < COLS; ++col) {
        if (b0->cells[0][col] != ' ') {
            tasks[col].valid = 0;
            continue;
        }
        tasks[col].root = b0;
        tasks[col].aiPiece = aiPiece;
        tasks[col].depth = depth;
        tasks[col].col = col;
        tasks[col].score = INT_MIN;
        tasks[col].valid = 0;

        if (pthread_create(&threads[col], NULL, evaluate_move_thread, &tasks[col]) == 0) {
            created[col] = 1;
        } else {
            tasks[col].valid = 0;
        }
    }

    /* Join all created threads */
    for (int col = 0; col < COLS; ++col) {
        if (created[col]) {
            (void)pthread_join(threads[col], NULL);
        }
    }

    /* Pick best scored column, center-first tie-breaking */
    int bestCol = -1;
    int bestScore = INT_MIN;
    int center = COLS / 2;

    for (int offset = 0; offset < COLS; ++offset) {
        int col = center + ((offset % 2 == 0) ? (offset / 2) : -(offset / 2 + 1));
        if (col < 0 || col >= COLS) continue;
        if (!tasks[col].valid) continue;

        int s = tasks[col].score;
        if (s > bestScore || bestCol == -1) {
            bestScore = s;
            bestCol = col;
        }
    }

    if (bestCol == -1) {
        return random_valid_column(b0);
    }
    return bestCol;
}

/* ===============================================================
   Main game loop
   =============================================================== */

int game_run(void) {
    Board b;
    board_init(&b);

    const char players[2] = { 'A', 'B' };
    int turn = 0;

    puts("\n=== Welcome to Connect Four (Console) ===");
    puts("Players: A and B. Enter column 1-7 to drop a checker; 'q' to quit.\n");

    srand((unsigned)time(NULL));
    int mode = read_mode_choice();
    if (mode == 0) {
        puts("\nGoodbye!");
        return 0;
    }
    puts("");

    while (1) {
        board_print(&b, stdout);
        char p = players[turn];

        int col = -1;

        if (mode == 1 && p == 'B') {
            /* Easy Bot: random valid column */
            col = random_valid_column(&b);
            if (col < 0) {
                puts("It's a draw. No more moves!");
                break;
            }
            printf("EasyBot (Player B) chooses column %d\n", col + 1);
        } else if (mode == 2 && p == 'B') {
            /* Medium Bot: heuristic 1-ply + safety */
            col = medium_rule_based_move(&b, 'B');
            if (col < 0) {
                puts("It's a draw. No more moves!");
                break;
            }
            printf("MediumBot (Player B) chooses column %d\n", col + 1);
        } else if (mode == 3 && p == 'B') {
            /* Hard Bot: minimax + alpha-beta with multithreading */
            int hardDepth = 6;  /* adjust depth for strength vs. speed */
            col = hard_best_move_parallel(&b, 'B', hardDepth);
            if (col < 0) {
                puts("It's a draw. No more moves!");
                break;
            }
            printf("HardBot (Player B) chooses column %d\n", col + 1);
        } else {
            /* Human turn (Player A in modes 1-3, both players in mode 4) */
            printf("Player %c \xE2\x86\x92 ", p);
            IoStatus st = io_read_column(&col);
            if (st == IO_QUIT || st == IO_EOF_QUIT) {
                puts("\nGoodbye!");
                return 0;
            }
            if (st != IO_SUCCESS) continue; /* safety */
        }

        int row = -1;
        if (!board_drop(&b, col, p, &row)) {
            puts("That column is full or invalid. Try another.");
            continue;
        }

        if (board_is_winning(&b, row, col, p)) {
            board_print(&b, stdout);
            printf("Player %c WINS!\n", p);
            break;
        }

        if (board_full(&b)) {
            board_print(&b, stdout);
            puts("It's a draw. No more moves!");
            break;
        }

        turn ^= 1; /* swap player */
    }

    puts("Press Enter to exit...");
    (void)getchar();
    return 0;
}