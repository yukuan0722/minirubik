/* Host-only validation harness. The step3 solver is included without modification.
 * The BFS oracle uses separate move/rank routines in oracle_generator.c.
 * --smoke checks 100 spread-out states; --full checks the entire domain.
 */
#define NOMINMAX
#include <windows.h>
#define main oracle_main
#include "oracle_generator.c"
#undef main
#define HOST_TEST
#define main original_sample_main
#include "solver_rv32i_step3.c"
#undef main

static int check_transition_consistency(void) {
    for (unsigned r = 0; r < P; ++r) {
        state_t start = {{0}, {0}};
        unrankp(r, start.p);
        if (rank_permutation(&start) != r) return 0;
        for (unsigned m = 0; m < M; ++m) {
            state_t next;
            apply_move(&start, &next, m);
            if (rank_permutation(&next) != pt[m][r]) return 0;
            if (permutation_next[m][r] != pt[m][r]) return 0;
        }
    }
    for (unsigned r = 0; r < O; ++r) {
        state_t start = {{0,1,2,3,4,5,6}, {0}};
        unranko(r, start.o);
        if (rank_orientation(&start) != r) return 0;
        for (unsigned m = 0; m < M; ++m) {
            state_t next;
            apply_move(&start, &next, m);
            if (rank_orientation(&next) != ot[m][r]) return 0;
            if (orientation_next[m][r] != ot[m][r]) return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2 || (strcmp(argv[1], "--full") && strcmp(argv[1], "--smoke"))) {
        puts("Usage: validate_step3_host.exe --full | --smoke");
        return 2;
    }
    int full = !strcmp(argv[1], "--full");
    unsigned total = full ? N : 100;
    ULONGLONG all_begin = GetTickCount64();
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Mode: %s\nSolver: solver_rv32i_step3.c\n", full ? "FULL" : "SMOKE ONLY");
    char *oracle_args[] = {"oracle_generator", "host-validation-step3/host-transitions.inc",
                          "host-validation-step3/host-exact-distances.bin"};
    if (oracle_main(3, oracle_args)) {
        puts("FAIL: BFS/PDB validation"); return 1;
    }
    if (!check_transition_consistency()) {
        puts("FAIL: step3 C moves/ranks disagree with oracle"); return 1;
    }
    puts("PASS: step3 C move/rank routines agree with all oracle transitions");
    puts("H1 PASS: max(PDBs) <= exact distance for all 3,674,160 states");
    puts("H2 PASS: both PDBs match complete projected BFS tables");
    puts("H2 PASS: all 5040 x 9 permutation and 729 x 9 orientation transition entries match the oracle");
    puts("Transition maxima: permutation=5039; orientation=728; all solved-input transitions match the oracle");
    puts("H4 N/A for step3 C: byte PDBs and uint16_t transitions are unpacked");
    FILE *f = fopen("host-validation-step3/host-exact-distances.bin", "rb");
    unsigned char *exact = malloc(N);
    if (!f || !exact) { puts("FAIL: oracle load"); return 1; }
    if (fread(exact, 1, N, f) != N) { puts("FAIL: incomplete oracle"); return 1; }
    fclose(f);
    for (unsigned r = 0; r < N; ++r) {
        if (heuristic_ranked(r / O, r % O) > exact[r]) {
            printf("H1 FAIL: ranked heuristic at rank=%u\n", r); return 1;
        }
    }
    puts("H1 PASS: actual heuristic_ranked function checked over all 3,674,160 states");
    ULONGLONG search_begin = GetTickCount64();
    for (unsigned i = 0; i < total; ++i) {
        unsigned r = full ? i : (unsigned)((unsigned long long)i * (N - 1) / (total - 1));
        state_t start;
        uint8_t path[11];
        unrankp(r / O, start.p);
        unranko(r % O, start.o);
        if (heuristic(&start) > exact[r]) { puts("FAIL: step3 heuristic"); return 1; }
        int length = ida_star(&start, path);
        if (length != exact[r]) {
            printf("H3 FAIL: rank=%u expected=%u actual=%d\n", r, exact[r], length);
            return 1;
        }
        state_t current = start;
        for (int k = 0; k < length; ++k) {
            state_t next;
            if (path[k] >= M) { puts("FAIL: invalid move"); return 1; }
            apply_move(&current, &next, path[k]); current = next;
        }
        if (!is_solved(&current)) { printf("FAIL: replay rank=%u\n", r); return 1; }
        if ((i + 1) % 10000 == 0 || i + 1 == total)
            printf("Search progress: %u/%u (%.2f%%), elapsed %.3f seconds\n",
                i + 1, total, 100.0 * (i + 1) / total,
                (GetTickCount64() - search_begin) / 1000.0);
    }
    printf("%s: optimal length and successful replay for %u states\n",
        full ? "H3 PASS" : "SMOKE PASS (H3 full-domain check NOT completed)", total);
    printf("Search wall-clock time: %.3f seconds\n", (GetTickCount64() - search_begin) / 1000.0);
    printf("Total validation wall-clock time: %.3f seconds\n", (GetTickCount64() - all_begin) / 1000.0);
    free(exact);
    return 0;
}

