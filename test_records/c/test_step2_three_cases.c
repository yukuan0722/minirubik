/* Test harness only: solver_rv32i_step2.c is included without changing its solver. */
#define HOST_TEST
#define main original_sample_main
#include "solver_rv32i_step2.c"
#undef main

int main(void) {
    for (unsigned move = 0; move < MOVES; ++move) {
        if (move_face[move] != move / 3 || move_turns[move] != move % 3 + 1) {
            puts("FAIL: move metadata table");
            return 1;
        }
    }
    for (unsigned rank = 0; rank < PERMUTATIONS; ++rank) {
        state_t start = {{0}, {0}};
        uint8_t available[7] = {0,1,2,3,4,5,6};
        unsigned value = rank;
        for (int i = 0; i < 7; ++i) {
            unsigned weight = factorial(6-i);
            unsigned choice = value / weight;
            value %= weight;
            start.p[i] = available[choice];
            for (unsigned j = choice; j < (unsigned)(6-i); ++j)
                available[j] = available[j+1];
        }
        for (unsigned move = 0; move < MOVES; ++move) {
            state_t actual, expected = start;
            apply_permutation_move(&start, &actual, move);
            for (unsigned turn = 0; turn < move % 3 + 1; ++turn) {
                state_t next = expected;
                for (int i = 0; i < 7; ++i)
                    next.p[i] = expected.p[source[move / 3][i]];
                expected = next;
            }
            if (memcmp(actual.p, expected.p, 7)) {
                printf("FAIL: permutation rank=%u move=%u\n", rank, move);
                return 1;
            }
        }
    }
    puts("PASS: all 5040 permutation states x 9 moves match the reference.");
    for (unsigned rank = 0; rank < ORIENTATIONS; ++rank) {
        state_t current = {{0,1,2,3,4,5,6}, {0}};
        unsigned value = rank, sum = 0;
        for (int i = 5; i >= 0; --i) {
            current.o[i] = value % 3;
            value /= 3;
            sum += current.o[i];
        }
        current.o[6] = (3 - sum % 3) % 3;
        for (unsigned move = 0; move < MOVES; ++move) {
            state_t actual, expected = current;
            apply_orientation_move(&current, &actual, move);
            for (unsigned turn = 0; turn < move % 3 + 1; ++turn) {
                state_t next = expected;
                for (int i = 0; i < 7; ++i)
                    next.o[i] = (expected.o[source[move / 3][i]] + twist[move / 3][i]) % 3;
                expected = next;
            }
            if (memcmp(actual.o, expected.o, 7)) {
                printf("FAIL: orientation rank=%u move=%u\n", rank, move);
                return 1;
            }
        }
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
    puts("PASS: all 729 orientation states x 9 moves match the reference.\n");
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
    puts("ALL 3 STEP2 C TESTS PASSED");
    return 0;
}


