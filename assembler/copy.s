	#拷贝字符串	
	BYTE	str[24] = "Simulator and Assembler"
	BYTE	s[24]
	LOADI	A	24		
	LOADI	B	0
	LOADI	C	0
	LOADI	D	0
	LOADI	E	0
	LOADI	G	0		# 将数组下限设为0
loop:   LOADB	B	str
	STOREB	B	s
	LOADB	C	s
	ADD	D	G	E
	ADDI	D	48	
	OUT	C	15
	ADDI	G	1
	LT	G	A
	CJMP	loop
	HLT
