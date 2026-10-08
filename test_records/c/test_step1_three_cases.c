/* Test harness only: solver_rv32i_step1.c is included without changing its solver. */
#define HOST_TEST
#define main original_sample_main
#include "solver_rv32i_step1.c"
#undef main

int main(void) {
    for (unsigned rank = 0; rank < ORIENTATIONS; ++rank) {
        state_t current = {{0,1,2,3,4,5,6}, {0}};
        unsigned value = rank, sum = 0;
        for (int i = 5; i >= 0; --i) {
            current.o[i] = value % 3;
            value /= 3;
            sum += current.o[i];
        }
        current.o[6] = (3 - sum % 3) % 3;
        for (unsigned face = 0; face < 3; ++face) {
            state_t next;
            quarter_turn_orientation(&current, &next, face);
            for (int i = 0; i < CUBIES; ++i) {
                unsigned expected = (current.o[source[face][i]] + twist[face][i]) % 3;
                if (next.o[i] != expected) {
                    printf("FAIL: orientation rank=%u face=%u corner=%d\n", rank, face, i);
                    return 1;
                }
            }
        }
    }
    puts("PASS: all 729 orientation states x 3 faces match the original modulo formula.\n");
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
    puts("ALL 3 STEP1 C TESTS PASSED");
    return 0;
}

