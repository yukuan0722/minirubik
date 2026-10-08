# Mini-Rubik solver for Ripes, RV32I only.
# Input and PDB indexing match the supplied C program.
# Nine direct transitions are generated on the host in C.
# Search executes here on the target: iterative IDA*, no recursion or heap.
# All static data (including the software stack) fits below 128 KiB.
# This source intentionally avoids .if, .space, .include and .rodata:
# the installed Ripes assembler does not support all GNU directives.

.equ RUN_TESTS, 0
# RUN_TESTS = 0: solve input_state. RUN_TESTS = 1: run all three cases.
# The input is exactly seven permutation digits plus seven orientation digits.

.data
input_state: .string "21345671111111"
test_solved: .string "12345671111111"
test_short:  .string "25314672313211"
test_hard:   .string "21345671111111"
.align 2
test_cases:
    .word test_solved, 0
    .word test_short, 1
    .word test_hard, 11

msg_input: .string "Input: "
msg_length: .string "Solution length: "
msg_path: .string "Moves: "
msg_solved: .string "(solved)"
msg_pass: .string "PASS: replay reaches solved state.\n"
msg_all: .string "ALL 3 TESTS PASSED\n"
msg_invalid: .string "FAIL: invalid 14-character state.\n"
msg_search: .string "FAIL: no solution within 11 moves.\n"
msg_expected: .string "FAIL: unexpected solution length.\n"
msg_replay: .string "FAIL: invalid move or replay did not solve the cube.\n"
msg_led: .string "FAIL: LED Matrix must be 35 wide and 25 high.\n"
newline: .string "\n"
space: .string " "
move_0: .string "R"
move_1: .string "R2"
move_2: .string "R'"
move_3: .string "B"
move_4: .string "B2"
move_5: .string "B'"
move_6: .string "D"
move_7: .string "D2"
move_8: .string "D'"
.align 2
move_names:
    .word move_0, move_1, move_2, move_3, move_4, move_5, move_6, move_7, move_8
move_tables:
    .word perm_move_0, ori_move_0
    .word perm_move_1, ori_move_1
    .word perm_move_2, ori_move_2
    .word perm_move_3, ori_move_3
    .word perm_move_4, ori_move_4
    .word perm_move_5, ori_move_5
    .word perm_move_6, ori_move_6
    .word perm_move_7, ori_move_7
    .word perm_move_8, ori_move_8
face_of_move: .byte 0,0,0,1,1,1,2,2,2
next_face_start: .byte 3,3,3,6,6,6,9,9,9

# Direct cubie mappings used for replay, independently of ranked transitions.
# Each row is padded to 8 bytes: move * 8 is just a shift.
move_source:
    @MOVE_SOURCE@
move_twist:
    @MOVE_TWIST@

.align 2
factorials: .word 720,120,24,6,2,1,1
start_state: .zero 14
replay_state: .zero 14
replay_next: .zero 14
solution_path: .zero 11
.align 2
solution_length: .word -1
test_status: .word -1
# 12 frames, 16 bytes each: p_rank*2, o_rank*2, next_move, previous_face.
search_frames: .zero 192
.align 4
software_stack: .zero 256
stack_top: .word 0

.text
main:
    la sp, stack_top
    li t0, RUN_TESTS
    bnez t0, main_tests
    la a0, input_state
    li a1, -1
    jal ra, run_case
    j main_exit
main_tests:
    li s0, 0
    la s1, test_cases
main_test_loop:
    lw a0, 0(s1)
    lw a1, 4(s1)
    jal ra, run_case
    bnez a0, main_exit
    addi s0, s0, 1
    addi s1, s1, 8
    li t0, 3
    blt s0, t0, main_test_loop
    la a0, msg_all
    jal ra, print_string
    li a0, 0
main_exit:
    la t0, test_status
    sw a0, 0(t0)
    li a7, 93
    ecall
    j main_exit

# run_case(a0 = input string, a1 = expected length or -1).
# Returns 0 on success, 1 for solver/test failure, 2 for invalid input.
run_case:
    addi sp, sp, -32
    sw ra, 28(sp)
    sw s0, 24(sp)
    sw s1, 20(sp)
    sw s2, 16(sp)
    sw s3, 12(sp)
    mv s0, a0
    mv s1, a1
    la a0, msg_input
    jal ra, print_string
    mv a0, s0
    jal ra, print_string
    jal ra, print_newline
    mv a0, s0
    la a1, start_state
    jal ra, parse_state
    beqz a0, case_invalid
    la a0, start_state
    jal ra, render_cube
    la a0, start_state
    jal ra, rank_permutation
    sw a0, 0(sp)
    la a0, start_state
    jal ra, rank_orientation
    mv a1, a0
    lw a0, 0(sp)
    la a2, solution_path
    jal ra, ida_star
    mv s2, a0
    la t0, solution_length
    sw s2, 0(t0)
    la a0, msg_length
    jal ra, print_string
    mv a0, s2
    li a7, 1
    ecall
    jal ra, print_newline
    bltz s2, case_search_fail
    li t0, 11
    blt t0, s2, case_search_fail
    bltz s1, case_length_ok
    bne s1, s2, case_expected_fail
case_length_ok:
    la a0, start_state
    la a1, replay_state
    jal ra, copy_state
    la a0, msg_path
    jal ra, print_string
    li s3, 0
    bnez s2, case_replay_loop
    la a0, msg_solved
    jal ra, print_string
    j case_replay_done
case_replay_loop:
    la t0, solution_path
    add t0, t0, s3
    lbu t1, 0(t0)
    li t2, 9
    bgeu t1, t2, case_replay_fail
    sw t1, 4(sp)
    beqz s3, case_no_separator
    la a0, space
    jal ra, print_string
case_no_separator:
    lw t1, 4(sp)
    slli t1, t1, 2
    la t0, move_names
    add t0, t0, t1
    lw a0, 0(t0)
    jal ra, print_string
    la a0, replay_state
    la a1, replay_next
    lw a2, 4(sp)
    jal ra, apply_move
    la a0, replay_next
    la a1, replay_state
    jal ra, copy_state
    la a0, replay_state
    jal ra, render_cube
    addi s3, s3, 1
    blt s3, s2, case_replay_loop
case_replay_done:
    jal ra, print_newline
    la a0, replay_state
    jal ra, is_solved
    beqz a0, case_replay_fail
    la a0, msg_pass
    jal ra, print_string
    li a0, 0
    j case_return
case_invalid:
    la a0, msg_invalid
    jal ra, print_string
    li a0, 2
    j case_return
case_search_fail:
    la a0, msg_search
    j case_fail_print
case_expected_fail:
    la a0, msg_expected
    j case_fail_print
case_replay_fail:
    jal ra, print_newline
    la a0, msg_replay
case_fail_print:
    jal ra, print_string
    li a0, 1
case_return:
    lw s3, 12(sp)
    lw s2, 16(sp)
    lw s1, 20(sp)
    lw s0, 24(sp)
    lw ra, 28(sp)
    addi sp, sp, 32
    ret

print_string:
    # Print one character at a time: some continuous Ripes CLI builds include
    # the terminating NUL in ecall 4 output. Do not emit that byte.
    mv t0, a0
print_string_loop:
    lbu a0, 0(t0)
    beqz a0, print_string_done
    li a7, 11
    ecall
    addi t0, t0, 1
    j print_string_loop
print_string_done:
    ret
print_newline:
    li a0, 10
    li a7, 11
    ecall
    ret

# parse_state(a0 = string, a1 = destination), return Boolean.
# Validate length, digit ranges, uniqueness and orientation sum modulo 3.
# Accumulate each orientation into a value < 3; conditional subtract suffices.
parse_state:
    li t0, 0
    li t1, 0
    li t2, 0
    li a5, 7
parse_loop:
    add t3, a0, t0
    lbu t4, 0(t3)
    addi t4, t4, -49
    bgeu t4, a5, parse_fail
    li t5, 1
    sll t5, t5, t4
    and t6, t1, t5
    bnez t6, parse_fail
    or t1, t1, t5
    add t6, a1, t0
    sb t4, 0(t6)
    lbu t4, 7(t3)
    addi t4, t4, -49
    li t5, 3
    bgeu t4, t5, parse_fail
    sb t4, 7(t6)
    add t2, t2, t4
    addi t2, t2, -3
    bgez t2, parse_sum_ok
    addi t2, t2, 3
parse_sum_ok:
    addi t0, t0, 1
    blt t0, a5, parse_loop
    lbu t0, 14(a0)
    bnez t0, parse_fail
    bnez t2, parse_fail
    li a0, 1
    ret
parse_fail:
    li a0, 0
    ret

# rank_orientation(state): same base-3 indexing as the supplied C/PDB.
rank_orientation:
    lbu t0, 7(a0)
    lbu t1, 8(a0)
    slli t2, t0, 1
    add t0, t0, t2
    add t0, t0, t1
    lbu t1, 9(a0)
    slli t2, t0, 1
    add t0, t0, t2
    add t0, t0, t1
    lbu t1, 10(a0)
    slli t2, t0, 1
    add t0, t0, t2
    add t0, t0, t1
    lbu t1, 11(a0)
    slli t2, t0, 1
    add t0, t0, t2
    add t0, t0, t1
    lbu t1, 12(a0)
    slli t2, t0, 1
    add t0, t0, t2
    add a0, t0, t1
    ret

# rank_permutation(state): Lehmer rank. Executed once per input.
# Multiply each digit by its factorial through <=6 additions; no M extension.
rank_permutation:
    li t0, 0
    li a2, 0
    li a3, 7
    la a4, factorials
rankp_outer:
    add t1, a0, t0
    lbu t2, 0(t1)
    addi t3, t0, 1
    li t4, 0
rankp_inner:
    bge t3, a3, rankp_weight
    add t5, a0, t3
    lbu t5, 0(t5)
    sltu t5, t5, t2
    add t4, t4, t5
    addi t3, t3, 1
    j rankp_inner
rankp_weight:
    lw t5, 0(a4)
rankp_add:
    beqz t4, rankp_next
    add a2, a2, t5
    addi t4, t4, -1
    j rankp_add
rankp_next:
    addi a4, a4, 4
    addi t0, t0, 1
    blt t0, a3, rankp_outer
    mv a0, a2
    ret

copy_state:
    li t0, 14
copy_loop:
    lbu t1, 0(a0)
    sb t1, 0(a1)
    addi a0, a0, 1
    addi a1, a1, 1
    addi t0, t0, -1
    bnez t0, copy_loop
    ret

# apply_move(current, next, move). Buffers must be distinct.
# All turns, including R2/R', use one direct mapping, no repeated copies.
apply_move:
    slli t0, a2, 3
    la t1, move_source
    la t2, move_twist
    add t1, t1, t0
    add t2, t2, t0
    li t0, 7
apply_loop:
    lbu t3, 0(t1)
    add t3, a0, t3
    lbu t4, 0(t3)
    sb t4, 0(a1)
    lbu t4, 7(t3)
    lbu t5, 0(t2)
    add t4, t4, t5
    addi t4, t4, -3
    bgez t4, apply_reduced
    addi t4, t4, 3
apply_reduced:
    sb t4, 7(a1)
    addi t1, t1, 1
    addi t2, t2, 1
    addi a1, a1, 1
    addi t0, t0, -1
    bnez t0, apply_loop
    ret

is_solved:
    li t0, 0
    li t3, 7
solved_loop:
    lbu t1, 0(a0)
    lbu t2, 7(a0)
    bne t1, t0, solved_no
    bnez t2, solved_no
    addi a0, a0, 1
    addi t0, t0, 1
    blt t0, t3, solved_loop
    li a0, 1
    ret
solved_no:
    li a0, 0
    ret

# ida_star(p_rank, o_rank, path) -> length or -1.
# IDA* and search_round are merged to keep the hot loop free of calls.
# Register meanings:
# s0 frame, s1 depth, s2 bound, s3 path, s4 face table,
# s5 permutation PDB, s6 orientation PDB, s7 move-table pointers,
# s8 next bound, s9 root p*2, s10 root o*2, s11 end-of-group table.
ida_star:
    addi sp, sp, -64
    sw ra, 60(sp)
    sw s11, 48(sp)
    sw s0, 40(sp)
    sw s1, 36(sp)
    sw s2, 32(sp)
    sw s3, 28(sp)
    sw s4, 24(sp)
    sw s5, 20(sp)
    sw s6, 16(sp)
    sw s7, 12(sp)
    sw s8, 8(sp)
    sw s9, 4(sp)
    sw s10, 0(sp)
    mv s3, a2
    la s4, face_of_move
    la s5, permutation_pdb
    la s6, orientation_pdb
    la s7, move_tables
    la s11, next_face_start
    slli s9, a0, 1
    slli s10, a1, 1
    or t0, a0, a1
    beqz t0, ida_already_solved
    add t0, s5, a0
    lbu s2, 0(t0)
    add t0, s6, a1
    lbu t1, 0(t0)
    bgeu s2, t1, ida_begin_round
    mv s2, t1
ida_begin_round:
    li t0, 11
    bltu t0, s2, ida_failure
    la s0, search_frames
    sw s9, 0(s0)
    sw s10, 4(s0)
    sw zero, 8(s0)
    li t0, 3
    sw t0, 12(s0)
    li s1, 0
    li s8, 65535
ida_next_child:
    lw t0, 8(s0)
    li t1, 9
    bgeu t0, t1, ida_backtrack
    add t1, s4, t0
    lbu t2, 0(t1)
    lw t3, 12(s0)
    beq t2, t3, ida_skip_face
    addi t1, t0, 1
    sw t1, 8(s0)
    slli t1, t0, 3
    add t1, s7, t1
    lw t3, 0(t1)
    lw t4, 4(t1)
    lw t5, 0(s0)
    lw t6, 4(s0)
    add t3, t3, t5
    add t4, t4, t6
    lhu a0, 0(t3)
    lhu a1, 0(t4)
    add t3, s5, a0
    add t4, s6, a1
    lbu t3, 0(t3)
    lbu t4, 0(t4)
    bgeu t3, t4, ida_have_h
    mv t3, t4
ida_have_h:
    addi t5, s1, 1
    add t3, t3, t5
    bltu s2, t3, ida_prune
    add t4, s3, s1
    sb t0, 0(t4)
    or t6, a0, a1
    beqz t6, ida_found
    li t4, 11
    bgeu t5, t4, ida_next_child
    slli a0, a0, 1
    slli a1, a1, 1
    addi s0, s0, 16
    sw a0, 0(s0)
    sw a1, 4(s0)
    sw zero, 8(s0)
    sw t2, 12(s0)
    mv s1, t5
    j ida_next_child
ida_skip_face:
    add t1, s11, t0
    lbu t1, 0(t1)
    sw t1, 8(s0)
    j ida_next_child
ida_prune:
    bgeu t3, s8, ida_next_child
    mv s8, t3
    j ida_next_child
ida_backtrack:
    beqz s1, ida_round_finished
    addi s0, s0, -16
    addi s1, s1, -1
    j ida_next_child
ida_round_finished:
    li t0, 65535
    beq s8, t0, ida_failure
    mv s2, s8
    j ida_begin_round
ida_found:
    mv a0, t5
    j ida_return
ida_already_solved:
    li a0, 0
    j ida_return
ida_failure:
    li a0, -1
ida_return:
    lw s11, 48(sp)
    lw s10, 0(sp)
    lw s9, 4(sp)
    lw s8, 8(sp)
    lw s7, 12(sp)
    lw s6, 16(sp)
    lw s5, 20(sp)
    lw s4, 24(sp)
    lw s3, 28(sp)
    lw s2, 32(sp)
    lw s1, 36(sp)
    lw s0, 40(sp)
    lw ra, 60(sp)
    addi sp, sp, 64
    ret

@RENDERER@

.data
# PDB values below are copied verbatim from the user's pdb_tables.h.
@PDB@
# The search transition tables below are read-only by convention.
@TRANSITIONS@
static_data_end: .byte 0
