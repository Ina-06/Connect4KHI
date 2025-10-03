#include "io.h"


#include <stdio.h>
#include <string.h>
#include <ctype.h>


static void rstrip(char *s) {
if (!s) return;
size_t n = strlen(s);
while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || isspace((unsigned char)s[n-1]))) {
s[--n] = '\0';
}
}


IoStatus io_read_column(int *out_col) {
char line[64];
fputs("Choose column (1-7) or 'q' to quit: ", stdout);
fflush(stdout);


if (!fgets(line, sizeof line, stdin)) {
// EOF or error = treat as quit
fputc('\n', stdout);
return IO_EOF_QUIT;
}
rstrip(line);


if (line[0] == '\0') {
puts("Empty input. Please enter 1-7 or 'q'.");
return io_read_column(out_col);
}


if (line[0] == 'q' || line[0] == 'Q') {
return IO_QUIT;
}


if (line[1] == '\0' && line[0] >= '1' && line[0] <= '7') {
if (out_col) *out_col = (line[0] - '1'); // map to 0 - 6
return IO_SUCCESS;
}


puts("Invalid input. Please enter a single digit 1-7 or 'q'.");
return io_read_column(out_col);
}
