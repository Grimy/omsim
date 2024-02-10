#include "verifier.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define METRIC(x) verifier_evaluate_metric(verifier, x)
#define METRICF(x) verifier_evaluate_approximate_metric(verifier, x)

static char solution[131072];
static char puzzle_name[32];
static char puzzle_path[128];
static char puzzle[512];
static size_t puzzle_length;

int main(int argc, char *argv[])
{
    while (1) {
        fflush(stdout);

        // Read cycles limit
        size_t cycle_limit = 0;
        if (!read(0, &cycle_limit, 4))
            return 0;

        // Read solution
        size_t solution_length = read(0, solution, sizeof(solution));
        struct timeval timeout = { .tv_usec = 10 };
        fd_set rfds = { 0 };
        FD_SET(0, &rfds);
        while (select(1, &rfds, 0, 0, &timeout))
            solution_length += read(0, solution + solution_length, sizeof(solution) - solution_length);

        // Read puzzle
        int puzzle_name_length = solution[4];
        if (puzzle_name_length < sizeof(puzzle_name) && strncmp(solution + 5, puzzle_name, puzzle_name_length)) {
            memcpy(puzzle_name, solution + 5, puzzle_name_length);
            puzzle_name[puzzle_name_length] = '\0';
            snprintf(puzzle_path, sizeof(puzzle_path), "omsim/puzzles/%s.puzzle", puzzle_name);
            FILE *fh = fopen(puzzle_path, "r");
            if (fh) {
                puzzle_length = fread(puzzle, 1, sizeof(puzzle), fh);
                fclose(fh);
            }
        }

        // Create verifier
        void* verifier = verifier_create_from_bytes_without_copying(puzzle, puzzle_length, solution, solution_length);
        if (!verifier) {
            printf("\n");
            continue;
        }

        // Check that the solution is valid
        verifier_set_cycle_limit(verifier, cycle_limit);
        if (METRIC("cycles") < 0) {
            printf("\n");
            verifier_destroy(verifier);
            continue;
        }

        // Output results
        int outputs = METRIC("per repetition outputs");
        int rate = -1;
        int areaINF = -1;
        int heightINF = -1;
        int widthINF = -1;

        if (outputs > 0) {
            rate = (METRIC("per repetition cycles") * 100 + outputs - 1) / outputs;
            areaINF = METRICF("per repetition^2 area") * 1000000 / outputs / outputs;
            if (!areaINF)
                areaINF = (METRICF("per repetition area") * 1000 + (outputs - 1) / 2) / outputs;
            if (!areaINF)
                areaINF = METRICF("steady state area");
            heightINF = METRIC("steady state height");
            widthINF = METRIC("steady state width*2");
        }

        printf("%dt/%dc/%dg/%da/%di/%dh/%dw/%dl/%ur/%ua/%uh/%uw/%do\n",
                METRIC("parts of type track") ? 1 : 0,
                METRIC("cycles"),
                METRIC("cost"),
                METRIC("area"),
                METRIC("instructions"),
                METRIC("height"),
                METRIC("width*2"),
                outputs <= 0,
                rate,
                areaINF,
                heightINF,
                widthINF,
                METRIC("overlap") ? 1 : 0);

        verifier_destroy(verifier);
    }
}
