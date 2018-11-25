.global ASScnt
ASScnt:
	ldrh r1, [r0] /*r0 is a pointer to the int we are using*/
	add r1,r1,#1 /*plus equals 1*/
	strh r1,[r0]/*store it back into the location of r0*/
	cmp r1,#0
	beq .tru
	cmp r1,#20
	beq .tru
	cmp r1,#40
	beq .tru
	mov r0,#0
	b .done
.tru:
	mov r0,#1
.done:
	mov pc,lr
