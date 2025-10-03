#ifndef IO_H
#define IO_H


#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
IO_SUCCESS = 0,
IO_QUIT = 1,
IO_EOF_QUIT = 2
} IoStatus;


// Prompts the user and parses a column in [0 - 6].
// Returns IO_SUCCESS and sets *out_col on valid input
// Returns IO_QUIT if the user enters q or Q
// Returns IO_EOF_QUIT on EOF (Ctrl+D) so the caller can exit cleanly
IoStatus io_read_column(int *out_col);


#ifdef __cplusplus
}
#endif


#endif // IO_H
