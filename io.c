#include "io.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// column reader
IoStatus io_read_column(int *out_col) {
    char buf[128];

    for (;;) {
        fputs("Enter column (1-7) or 'q' to quit: ", stdout);
        fflush(stdout);

        if (!fgets(buf, sizeof buf, stdin)) {
            fputc('\n', stdout);
            return IO_EOF_QUIT;
        }

        // strip trailing spaces
        size_t n = strlen(buf);
        while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r' ||
                         isspace((unsigned char)buf[n-1]))) {
            buf[--n] = '\0';
        }

        // skip leading spaces
        char *p = buf;
        while (*p && isspace((unsigned char)*p)) p++;

        // quit prompt
        if (*p == 'q' || *p == 'Q') return IO_QUIT;

        // move to first digit -> accept first integer
        while (*p && !isdigit((unsigned char)*p) && *p != '+' && *p != '-') p++;
        if (!*p) {
            puts("Invalid input. Type a number 1-7 or 'q'.");
            continue; // reprompt
        }

        long v = strtol(p, NULL, 10);
        if (v < 1 || v > 7) {
            puts("Column must be between 1 and 7.");
            continue; // reprompt
        }

        if (out_col) *out_col = (int)v - 1; // map 1–7 to 0–6
        return IO_SUCCESS;
    }
}
