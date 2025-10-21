#include "game.h"
#include "board.h"
#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* Pick a random valid column (0..6), or -1 if none */
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

/* Read "1" or "2" as mode using fgets (consistent with io.c style) */
static int read_mode_choice(void) {
    char line[32];
    while (1) {
        puts("Select mode:");
        puts("  1 - Play against Easy Bot (You are Player A)");
        puts("  2 - Two Player mode (A vs B)");
        fputs("Enter 1 or 2: ", stdout);
        fflush(stdout);

        if (!fgets(line, sizeof line, stdin)) {
            fputc('\n', stdout);
            return 0; // treat EOF as cancel
        }

        // trim trailing newline
        size_t n = strlen(line);
        if (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = '\0';

        if ((n == 1) && (line[0] == '1' || line[0] == '2')) {
            return (line[0] - '0');
        }

        puts("Invalid choice. Please enter 1 or 2.\n");
    }
}

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
            /* Easy Bot: choose any random valid column */
            col = random_valid_column(&b);
            if (col < 0) {
                // board full safeguard (should be caught by board_full below)
                puts("It's a draw. No more moves!");
                break;
            }
            printf("EasyBot (Player B) chooses column %d\n", col + 1);
        } else {
            /* Human turn (Player A in mode 1, both A & B in mode 2) */
            printf("Player %c \xE2\x86\x92 ", p); // nice arrow like your original
            IoStatus st = io_read_column(&col);   // uses your existing prompt & parsing
            if (st == IO_QUIT || st == IO_EOF_QUIT) {
                puts("\nGoodbye!");
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
            printf("Player %c WINS!\n", p);
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