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



// Accepts first integer 1–7 on the line, tolerant of commas/words, trims CRLF, supports q/Q to quit
IoStatus io_read_column(int *out_col) {
    char buf[128];

    fputs("Enter column (1-7) or 'q' to quit: ", stdout);
    fflush(stdout);

    if (!fgets(buf, sizeof buf, stdin)) {
        fputc('\n', stdout);
        return IO_EOF_QUIT; // Ctrl+D / EOF
    }

    // Strip ALL trailing CR/LF
    size_t n = strlen(buf);
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) {
        buf[--n] = '\0';
    }

    // Skip leading spaces
    char *p = buf;
    while (*p && isspace((unsigned char)*p)) p++;

    // Allow q/Q anywhere as first non-space char
    if (*p == 'q' || *p == 'Q') return IO_QUIT;

    // Parse first integer token (handles "3,", "  5 please", etc.)
    char *end = NULL;
    long v = strtol(p, &end, 10);
    if (end == p) {
        // If the line started with a comma/word, advance to first digit and try again
        while (*p && !isdigit((unsigned char)*p) && *p != '+' && *p != '-') p++;
        if (*p) v = strtol(p, &end, 10);
    }

    if (end == p) {
        puts("Invalid input. Type a number 1-7 or 'q'.");
        return IO_RETRY;
    }
    if (v < 1 || v > 7) {
        puts("Column must be between 1 and 7.");
        return IO_RETRY;
    }

    if (out_col) *out_col = (int)v - 1; // map 1–7 to 0–6
    return IO_SUCCESS;
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
