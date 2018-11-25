.global ASSdel
ASSdel:
	mov r1,#10
	mul r0,r1,r0
	mov r12,#0
.top:
	add r1,r1,#1
	cmp r0,r1
	beq .done
	b .top
.done:
	mov pc,lr
