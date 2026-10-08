# GUI-only renderer. Instantiate LED Matrix 0, Width=35, Height=25.
# MMIO symbols are supplied by the peripheral, never hardcoded here.
# Short delay after each redraw; edit FRAME_DELAY to suit the processor.
.equ FRAME_DELAY, 5000

.data
.align 2
led_palette:
    .word 0x00ffffff, 0x0000b050, 0x00e02020
    .word 0x002060ff, 0x00ff8000, 0x00ffff00
# Color order for each cubie's three stickers (orientation zero).
# Faces: U=0, F=1, R=2, B=3, L=4, D=5.
# Physical corners 0..7: FUL, FUR, FDR, FDL, BUR, BDR, BDL, BUL.
# Each row is padded to 4 bytes; cubie * 4 requires only a shift.
corner_colors:
    .byte 0,1,4,0
    .byte 0,2,1,0
    .byte 5,1,2,0
    .byte 5,4,1,0
    .byte 0,3,2,0
    .byte 5,2,3,0
    .byte 5,3,4,0
    .byte 0,4,3,0
# 24 rows of [physical corner, sticker slot, x, y].
# Net: U above F; L F R B across; D below F.
# Facelets occupy 4 by 3 pixels. There are only inter-face separators.
led_facelets:
    @FACELETS@

.text
render_cube:
    addi sp, sp, -32
    sw ra, 28(sp)
    sw s0, 24(sp)
    sw s1, 20(sp)
    sw s2, 16(sp)
    sw s3, 12(sp)
    sw s4, 8(sp)
    mv s0, a0
    li t0, LED_MATRIX_0_WIDTH
    li t1, 35
    bne t0, t1, render_bad_size
    li t0, LED_MATRIX_0_HEIGHT
    li t1, 25
    bne t0, t1, render_bad_size
    li s3, LED_MATRIX_0_BASE
    mv t0, s3
    li t1, 875
render_clear:
    sw zero, 0(t0)
    addi t0, t0, 4
    addi t1, t1, -1
    bnez t1, render_clear
    la s2, led_facelets
    li s1, 24
render_facelet:
    lbu t0, 0(s2)
    lbu t1, 1(s2)
    beqz t0, render_fixed_corner
    addi t0, t0, -1
    add t2, s0, t0
    lbu t0, 0(t2)
    addi t0, t0, 1
    lbu t3, 7(t2)
    add t1, t1, t3
    addi t1, t1, -3
    bgez t1, render_have_slot
    addi t1, t1, 3
    j render_have_slot
render_fixed_corner:
    li t0, 0
render_have_slot:
    slli t0, t0, 2
    la t2, corner_colors
    add t2, t2, t0
    add t2, t2, t1
    lbu t2, 0(t2)
    slli t2, t2, 2
    la t3, led_palette
    add t3, t3, t2
    lw s4, 0(t3)
    lbu t0, 2(s2)
    lbu t1, 3(s2)
    # y * 35 = (y << 5) + (y << 1) + y.
    slli t2, t1, 5
    slli t3, t1, 1
    add t2, t2, t3
    add t2, t2, t1
    add t2, t2, t0
    slli t2, t2, 2
    add t2, s3, t2
    li t3, 3
render_three_rows:
    sw s4, 0(t2)
    sw s4, 4(t2)
    sw s4, 8(t2)
    sw s4, 12(t2)
    addi t2, t2, 140
    addi t3, t3, -1
    bnez t3, render_three_rows
    addi s2, s2, 4
    addi s1, s1, -1
    bnez s1, render_facelet
    li t0, FRAME_DELAY
    beqz t0, render_done
render_pause:
    addi t0, t0, -1
    bnez t0, render_pause
render_done:
    # Set a breakpoint here to inspect each state during GUI execution.
    lw s4, 8(sp)
    lw s3, 12(sp)
    lw s2, 16(sp)
    lw s1, 20(sp)
    lw s0, 24(sp)
    lw ra, 28(sp)
    addi sp, sp, 32
    ret
render_bad_size:
    la a0, msg_led
    li a7, 4
    ecall
    li a0, 1
    li a7, 93
    ecall
    j render_bad_size
