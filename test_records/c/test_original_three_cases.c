/* Test harness only: solver_original.c is included without changing its solver. */
#define HOST_TEST
#define main original_sample_main
#include "solver_original.c"
#undef main

int main(void) {
    static const char *inputs[] = {
        "12345671111111", "25314672313211", "21345671111111"
    };
    static const int expected[] = {0, 1, 11};
    for (int test = 0; test < 3; ++test) {
        state_t start;
        uint8_t path[11];
        printf("Input: %s\n", inputs[test]);
        if (!parse_state(inputs[test], &start)) {
            puts("FAIL: invalid test input");
            return 1;
        }
        int length = ida_star(&start, path);
        printf("Solution length: %d\n", length);
        if (length != expected[test]) {
            puts("FAIL: unexpected solution length");
            return 1;
        }
        printf("Moves:");
        state_t current = start;
        for (int i = 0; i < length; ++i) {
            state_t next;
            if (path[i] >= MOVES) {
                puts("FAIL: invalid move");
                return 1;
            }
            printf(" %s", move_names[path[i]]);
            apply_move(&current, &next, path[i]);
            current = next;
        }
        if (!length) printf(" (solved)");
        putchar('\n');
        if (!is_solved(&current)) {
            puts("FAIL: replay did not reach solved state");
            return 1;
        }
        puts("PASS: replay reaches solved state.\n");
    }
    puts("ALL 3 ORIGINAL C TESTS PASSED");
    return 0;
}
