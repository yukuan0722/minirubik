.section .text.start
.globl _start
.type _start, @function
_start:
    la sp, __stack_top
    call main
    li a7, 93
    ecall
1:
    j 1b
.size _start, .-_start

.section .bss.stack,"aw",@nobits
.balign 16
__stack_bottom:
    .zero 4096
.globl __stack_top
__stack_top:
