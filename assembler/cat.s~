	#拷贝字符串	
	BYTE	stra[24] = "Simulator and Assembler"
	BYTE	strb[26] = "I design a program. It is "
	BYTE	s[50]
	LOADI	A	26		
	LOADI	B	24
	LOADI	C	0
	LOADI	D	0
	LOADI	E	0
	LOADI	G	0		# 将数组下限设为0
loop1:	LOADB	C	strb
	STOREB	C	s
	ADDI	G	1
	LT	G	A
	CJMP	loop1
	LOADI	G	0
loop2:	LOADB	C	stra
	ADD	G	G	A
	STOREB	C	s
	SUB	G	G	A
	ADDI	G	1
	LT	G	B
	CJMP	loop2
	LOADI	G	0
	ADD	E	A	B
loop3:	LOADB	D	s
	OUT	D	15
	ADDI	G	1
	LT	G	E
	CJMP	loop3
	HLT
