#include "board.h"

static inline bool in_bounds(int r, int c) {
    return r >= 0 && r < ROWS && c >= 0 && c < COLS;
}

void board_init(Board *b) {
    if (!b) return;
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c)
            b->cells[r][c] = ' ';
}

void board_print(const Board *b, FILE *out) {
    if (!b) return;
    if (!out) out = stdout;

    fprintf(out, "\n  CONNECT FOUR (A vs B)\n  Columns: 1 2 3 4 5 6 7\n");
    for (int r = 0; r < ROWS; ++r) {
        fputc(' ', out);
        fputc('|', out);
        for (int c = 0; c < COLS; ++c) {
            fputc(b->cells[r][c], out);
            fputc('|', out);
        }
        fputc('\n', out);
    }
    fputs(" +---------------+\n  1 2 3 4 5 6 7\n\n", out);
}

bool board_drop(Board *b, int col, char piece, int *out_row) {
    if (!b || col < 0 || col >= COLS) return false;
    for (int r = ROWS - 1; r >= 0; --r) {
        if (b->cells[r][col] == ' ') {
            b->cells[r][col] = piece;
            if (out_row) *out_row = r;
            return true;
        }
    }
    return false; // column full
}

bool board_full(const Board *b) {
    if (!b) return false;
    for (int c = 0; c < COLS; ++c) {
        if (b->cells[0][c] == ' ') return false;
    }
    return true;
}

bool board_is_winning(const Board *b, int row, int col, char piece) {
    if (!b) return false;
    const int dirs[4][2] = { {0,1}, {1,0}, {1,1}, {-1,1} };
    for (int d = 0; d < 4; ++d) {
        int dr = dirs[d][0];
        int dc = dirs[d][1];
        for (int shift = -3; shift <= 0; ++shift) {
            int sr = row + dr * shift;
            int sc = col + dc * shift;
            bool ok = true;
            for (int k = 0; k < 4; ++k) {
                int rr = sr + dr * k;
                int cc = sc + dc * k;
                if (!in_bounds(rr, cc) || b->cells[rr][cc] != piece) {
                    ok = false;
                    break;
                }
            }
            if (ok) return true;
        }
    }
    return false;
}
