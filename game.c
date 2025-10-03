#include "game.h"
#include "board.h"
#include "io.h"


#include <stdio.h>


int game_run(void) {
Board b;
board_init(&b);


const char players[2] = { 'A', 'B' };
int turn = 0;


puts("\n=== Welcome to Connect Four (Console) ===");
puts("Players: A and B. Enter column 1-7 to drop a checker; 'q' to quit.\n");


while (1) {
board_print(&b, stdout);
char p = players[turn];
printf("Player %c → ", p);


int col = -1;
IoStatus st = io_read_column(&col);
if (st == IO_QUIT || st == IO_EOF_QUIT) {
puts("\nGoodbye!");
return 0;
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
