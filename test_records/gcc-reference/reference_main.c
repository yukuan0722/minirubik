/* Ripes execution harness; search functions are included from the verified C.
 * No standard C library, compiler arithmetic helper, heap, or renderer is used.
 */
#include "../c/solver_rv32i_step3.c"

static volatile char input_state[] = "21345671111111";

#ifdef REFERENCE_HOST_TEST
#include <stdio.h>
#include <string.h>
static void print_char(int c) { putchar(c); }
static void print_number(int n) { printf("%d", n); }
#else
static void print_char(int c) {
    register int value __asm__("a0") = c;
    register int service __asm__("a7") = 11;
    __asm__ volatile("ecall" : "+r"(value) : "r"(service) : "memory");
}
static void print_number(int n) {
    register int value __asm__("a0") = n;
    register int service __asm__("a7") = 1;
    __asm__ volatile("ecall" : "+r"(value) : "r"(service) : "memory");
}
#endif

static void print_text(const char *s) {
    while (*s) print_char((unsigned char)*s++);
}

/* Input checks mirror the original C. Reduce the bounded orientation sum
 * using subtraction so the target harness does not introduce modulo helpers.
 */
static int parse_target_state(const volatile char *input, state_t *s) {
    unsigned length = 0, sum = 0;
    while (input[length]) ++length;
    if (length != 14) return 0;
    for (int i = 0; i < CUBIES; ++i) {
        char p = input[i], o = input[i+7];
        if (p < '1' || p > '7' || o < '1' || o > '3') return 0;
        s->p[i] = p-'1';
        s->o[i] = o-'1';
    }
    for (int i = 0; i < CUBIES; ++i) {
        for (int j = i+1; j < CUBIES; ++j)
            if (s->p[i] == s->p[j]) return 0;
        sum += s->o[i];
        if (sum >= 3) sum -= 3;
    }
    return sum == 0;
}

#ifdef REFERENCE_HOST_TEST
int main(int argc, char **argv) {
    if (argc == 2) {
        if (strlen(argv[1]) != 14) return 2;
        for (int i = 0; i <= 14; ++i) input_state[i] = argv[1][i];
    }
#else
int main(void) {
#endif
    state_t start;
    uint8_t path[11];
    print_text("Input: ");
    for (unsigned i = 0; input_state[i]; ++i) print_char(input_state[i]);
    print_char('\n');
    if (!parse_target_state(input_state, &start)) {
        print_text("FAIL: invalid input\n"); return 2;
    }
    int length = ida_star(&start, path);
    print_text("Solution length: "); print_number(length); print_char('\n');
    if (length < 0 || length > 11) {
        print_text("FAIL: search failed\n"); return 1;
    }
    print_text("Moves:");
    if (length == 0) print_text(" (solved)");
    state_t current = start;
    for (int i = 0; i < length; ++i) {
        state_t next;
        if (path[i] >= MOVES) { print_text("FAIL: invalid move\n"); return 1; }
        print_char(' '); print_text(move_names[path[i]]);
        apply_move(&current, &next, path[i]);
        current = next;
    }
    print_char('\n');
    if (!is_solved(&current)) { print_text("FAIL: replay failed\n"); return 1; }
    print_text("PASS: replay reaches solved state.\n");
    return 0;
}
