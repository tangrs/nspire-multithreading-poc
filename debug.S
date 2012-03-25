
break_inst:
    .word 0
    mov pc, lr

dobreak: .global dobreak
    push {lr}
    ldr r1, =0xe1200070
    ldr r3, =0xfff0
    and r2, r0, #1
    orr r1, r1, r2
    and r2, r0, r3
    orr r1, r1, r2, lsl #4
    str r1, break_inst
    bl break_inst
    pop {pc}