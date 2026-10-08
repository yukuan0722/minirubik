#include <stdint.h>

#ifdef HOST_TEST
#include <stdio.h>
#include <string.h>
#endif
#define CUBIES 7
#define MOVES 9
#define ORIENTATIONS 729
#define PERMUTATIONS 5040

typedef struct {
    uint8_t p[CUBIES];// p in [0, 6]
    uint8_t o[CUBIES];// o in [0, 2]  
} state_t;

#include "pdb_tables.h"

static const char *const move_names[MOVES] = {"R",  "R2", "R'", "B", "B2",
                                              "B'", "D",  "D2", "D'"};

static const uint8_t move_face[MOVES] = {
    0, 0, 0, 1, 1, 1, 2, 2, 2
};

static const uint8_t move_turns[MOVES] = {
    1, 2, 3, 1, 2, 3, 1, 2, 3
};


static const uint8_t source[3][CUBIES] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};
static const uint8_t twist[3][CUBIES] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};

uint16_t rank_orientation(const state_t *s){
    uint16_t index = 0;
    for (int i = 0; i < CUBIES - 1; i++){
        index = index * 3 + s->o[i];
    }
    return index;
}

uint16_t factorial(uint8_t n){
    if (n == 0 || n == 1){
        return 1;
    }
    else if(n == 2){
        return 2;
    }
    else if(n == 3){
        return 6;
    }
    else if(n == 4){
        return 24;
    }
    else if(n == 5){
        return 120;
    }
    else if(n == 6){
        return 720;
    }
    else if(n == 7){
        return 5040;
    }
    return 0;
}

uint16_t rank_permutation(const state_t *s)
{
    uint16_t index = 0;
    for(int i = 0; i < CUBIES; i++){
        int count = 0;
        for(int j = i + 1; j < CUBIES; j++){
            if(s->p[j] < s->p[i]){
                count++;
            }
        }
        uint16_t sum = 0;
        for(int j = 1; j <= count; j++){
            sum += factorial(CUBIES - 1 - i);
        }
        index += sum;
    }
    return index;
}


void quarter_turn_orientation(// 轉某一面的方向
    const state_t *current,
    state_t *next,
    uint8_t face){ // face 0:R， 1:B， 2:D
    /* 在這裡計算 next->o */
    for (int i = 0; i < CUBIES; i++) {

        next->o[i] = current->o[source[face][i]] + twist[face][i];
        if(next->o[i]>=3){// 因為都是0~2 最多4
            next->o[i] -= 3;
        }
    }
}
void quarter_turn_permutation(// 轉某一面的排列
    const state_t *current,
    state_t *next,
    uint8_t face){ // face 0:R， 1:B， 2:D
    // 在這裡計算 next->p 
    for (int i = 0; i < CUBIES; i++) {
        next->p[i] = current->p[source[face][i]];
    }
}   

void apply_orientation_move(
    const state_t *current,
    state_t *next,
    uint8_t move){
    uint8_t face = move_face[move];
    uint8_t turns = move_turns[move];
    state_t temp; //用於在轉動 (quarter_turn_orientation) 時存儲中間狀態，或直接使用 current 會導致角塊遺失
    for (int i = 0; i < turns; i++){
        quarter_turn_orientation(current, &temp, face);
        for (int i = 0; i < CUBIES; i++) {
            next->o[i] = temp.o[i];
        }
        current = next;
    }
}

void apply_permutation_move(
    const state_t *current,
    state_t *next,
    uint8_t move) {
    uint8_t face = move_face[move];
    uint8_t turns = move_turns[move];
    state_t temp; //用於在轉動 (quarter_turn_permutation) 時存儲中間狀態，或直接使用 current 會導致角塊遺失
    for (int i = 0;i<turns; i++){
        quarter_turn_permutation(current, &temp, face);
        for (int i = 0; i < CUBIES; i++) {
            next->p[i] = temp.p[i];
        }
        current = next;
    }
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

uint16_t heuristic(const state_t *s){
    uint16_t orientation_index = rank_orientation(s);
    uint16_t permutation_index = rank_permutation(s);

    return max(orientation_pdb[orientation_index], permutation_pdb[permutation_index]);
}

void apply_move(const state_t *current, state_t *next, uint8_t move){
    apply_permutation_move(current, next, move);
    apply_orientation_move(current, next, move);
}

int is_solved(const state_t *s){
    // 完成時回傳 1，否則回傳 0
    for (int i = 0; i < CUBIES; i++) {
        if (s->p[i] != i || s->o[i] != 0) {
            return 0;
        }
    }
    return 1; 
}

int search_round(const state_t *start, uint16_t bound, uint8_t path[11], uint16_t *next_bound){
    state_t states[12];
    uint8_t next_move[12];
    int depth = 0;

    states[0] = *start;
    next_move[0] = 0;
    *next_bound = UINT16_MAX;
    while(depth >= 0){
        if(states[depth].o[0] == 0 && states[depth].p[0] == 0 && is_solved(&states[depth])){
            return depth;
        }

        if(next_move[depth] >= MOVES){
            depth--;
            continue;
        }

        uint8_t move = next_move[depth];
        next_move[depth]++;
        
        if (depth > 0 && move_face[move] == move_face[path[depth - 1]]) {
            continue;
        }

        state_t child;
        apply_move(&states[depth], &child, move);

        int child_depth = depth + 1;
        uint16_t f = child_depth + heuristic(&child);

        if(f > bound){
            if(f < *next_bound){
                *next_bound = f;
            }
            continue;
        }

        states[child_depth] = child;
        path[depth] = move;
        next_move[child_depth] = 0;
        depth = child_depth;
    }
    return -1;
}

int ida_star(const state_t *start, uint8_t path[11]){
    uint16_t bound = heuristic(start);
    uint16_t next_bound;
    while(bound <= 11){
        int result = search_round(start, bound, path, &next_bound);
        if(result >= 0){
            return result;
        }
        if(next_bound == UINT16_MAX){
            return -1;
        }
        bound = next_bound;
    }
    return -1;
}



#ifdef HOST_TEST


int parse_state(const char *input, state_t *s)
{
    uint8_t orientation_sum = 0;
    if(strlen(input) != 14){
        return 0;
    }
    for(int i=0; i<CUBIES; i++){
        if(input[i] < '1' || input[i] > '7'){
            return 0;
        }
        if(input[i+7] < '1' || input[i+7] > '3'){
            return 0;
        }
        s->p[i] = input[i] - '1';
        s->o[i] = input[i+7] - '1';
    }
    for(int i=0; i<CUBIES; i++){//確認沒有重複角塊
        for(int j=i+1; j<CUBIES; j++){
            if(s->p[i] == s->p[j]){
                return 0;
            }
        }
        orientation_sum += s->o[i];
    }
    if(orientation_sum % 3 != 0){
        return 0;
    }
    return 1;
}



int main(void){
    state_t start;
    uint8_t path[11];

    if (!parse_state("21345671111111", &start)) {
        puts("FAIL: invalid input");
        return 1;
    }

    int length = ida_star(&start, path);
    printf("Solution length: %d\n", length);

    if (length < 0 || length > 11) {
        puts("FAIL: search failed");
        return 1;
    }

    state_t current = start;

    for (int i = 0; i < length; i++) {
        state_t next;

        if (path[i] >= MOVES) {
            puts("FAIL: invalid move");
            return 1;
        }

        printf("%s ", move_names[path[i]]);
        apply_move(&current, &next, path[i]);
        current = next;
    }

    putchar('\n');

    if (!is_solved(&current)) {
        puts("FAIL: path does not solve the cube");
        return 1;
    }

    puts("PASS");
    return 0;
}
#endif