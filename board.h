#ifndef BOARD_H
#define BOARD_H


#include <stdio.h>
#include <stdbool.h>


#define ROWS 6
#define COLS 7


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
char cells[ROWS][COLS];
} Board;


void board_init(Board *b);
void board_print(const Board *b, FILE *out);


// Drops piece ('A' or 'B') into column [0 - COLS-1]
// On success, stores the placed row index to *out_row and returns true
// Returns false if the column is full or col is out of range
bool board_drop(Board *b, int col, char piece, int *out_row);


bool board_full(const Board *b);


// Check if the last move at (row, col) by piece forms a forms a winning combination
bool board_is_winning(const Board *b, int row, int col, char piece);


#ifdef __cplusplus
}
#endif


#endif // BOARD_H
