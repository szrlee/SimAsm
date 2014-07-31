#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define REG0 ((IR>>24)&0x07)
#define REG1 ((IR>>20)&0x0f)
#define REG2 ((IR>>16)&0x0f)
#define IMMEDIATE ((short)(IR&0xffff))
#define ADDRESS (IR&0xffffff)
#define PORT (IR&0xff)
#define OPCODE ((IR>>27)&0x1f)

typedef struct PROG_STATE_WORD
{
    unsigned short overflow_flg: 1;
    unsigned short compare_flg: 1;
    unsigned short reserve: 14;
} PROG_STATE_WORD;

typedef struct Extra
{
    PROG_STATE_WORD ES_PSW;
    short ES_GR[8];
    unsigned char *ES_PC;
} EXTRA;

unsigned char  *MEM;         //用动态存储区模拟内存，大小由命令行参数确定
unsigned char  *PC;          //指令计数器，用来存放下条指令的内存地址
short GR[8];                //通用寄存器的模拟
PROG_STATE_WORD PSW;
unsigned int IR;
unsigned char *CS;          //代码段寄存器
unsigned char *DS;          //数据段寄存器
unsigned short  *SS;                 //堆栈段寄存器
EXTRA  *ES;                 //附加段寄存器

//用32个函数实现32条指令的相应功能
int HLT(void);
int JMP(void);
int CJMP(void);
int OJMP(void);
int CALL(void);
int RET(void);
int PUSH(void);
int POP(void);
int LOADB(void);
int LOADW(void);
int STOREB(void);
int STOREW(void);
int LOADI(void);
int NOP(void);
int IN(void);
int OUT(void);
int ADD(void);
int ADDI(void);
int SUB(void);
int SUBI(void);
int MUL(void);
int DIV(void);
int AND(void);
int OR(void);
int NOR(void);
int NOTB(void);
int SAL(void);
int SAR(void);
int EQU(void);
int LT(void);
int LTE(void);
int NOTC(void);

int main(int argc, char **argv)
{
    unsigned int mem_size;
    int (*ops[])(void) = {HLT,JMP,CJMP,OJMP,CALL,RET,PUSH,POP,LOADB,LOADW,
                          STOREB,STOREW,LOADI,NOP,IN,OUT,ADD,ADDI,SUB,SUBI,
                          MUL,DIV,AND,OR,NOR,NOTB,SAL,SAR,EQU,LT,LTE,NOTC
                         };     //函数指针数组，用于指令对应函数的调用
    FILE *pfIn;
    int ret = 1;
    if (argc<2)
    {
        perror("no enough command line arguments!\n");
        exit(-1);
    }
    mem_size = 1u << 24;
    if ((MEM=(unsigned char *)malloc(mem_size))==NULL)
    {
        perror("allocate memory");
        exit(-1);
    }
    PC = MEM;      //使指令计数器指向模拟内存的顶端
    if ((pfIn=fopen(argv[1], "rb"))==NULL)
    {
        perror("open file");
        exit(-1);
    }
    unsigned char data;
    while(1)
    {
        fread (&data,1,1,pfIn);
        if (feof(pfIn)) break;
        memcpy (PC, &data, sizeof(unsigned char));
        PC++;
    }
    PC-=4;
    DS = PC - *(unsigned int *)PC;
    SS = (unsigned short *)PC;
    ES = (struct Extra *)(MEM + mem_size - sizeof(struct Extra));
    fclose(pfIn);
    PC = MEM;      //使PC指向模拟内存顶端的第一条指令
    CS = PC;
    while (ret)             //模拟处理器执行指令
    {
        IR = *(unsigned int *)PC;           //取指：将PC指示的指令加载到指令寄存器IR
        PC+=4;              //PC指向下一条执行指令
        ret = (*ops[OPCODE])(); //解码并执行指令
    }
    free (MEM);
    return 0;
}

int HLT(void)
{
    return 0;
}

int JMP(void)
{
    PC = (MEM + ADDRESS);
    return 1;
}

int CJMP(void)
{
    if (PSW.compare_flg)
        PC = (MEM + ADDRESS);
    return 1;
}

int OJMP(void)
{
    if (PSW.overflow_flg)
        PC = (MEM + ADDRESS);
    return 1;
}

int CALL(void)
{
    int i;
    for(i = 0; i < 8; i++)
        ES->ES_GR[i] = GR[i];
    ES->ES_PSW = PSW;
    ES->ES_PC = PC;
    PC = CS + ADDRESS;
    ES--;
    return 1;
}

int RET(void)
{
    int i;
    ES++;
    PC = ES->ES_PC;
    PSW = ES->ES_PSW;
    for (i = 0; i < 8; i++)
        GR[i] = ES->ES_GR[i];
    // ES--;
    return 1;
}

int PUSH(void)
{
    *SS = GR[REG0];
    SS++;
    return 1;
}

int POP(void)
{
    if(REG0 == 0)
    {
        printf("ERROR!\n");
        exit(-1);
    }
    SS--;
    GR[REG0] = *SS;
    return 1;
}


int LOADB(void)
{
    GR[REG0] = (char)(*(DS + ADDRESS + (GR[7])));
    return 1;
}

int LOADW(void)
{
    GR[REG0] = (short)(*(DS + ADDRESS + (GR[7])));
    return 1;
}

int STOREB(void)
{
    *(DS + ADDRESS + (GR[7])) = (char)GR[REG0];
    return 1;
}

int STOREW(void)
{
    *((unsigned short *)(DS + ADDRESS + (GR[7]))) = (short)GR[REG0];
    return 1;
}

int LOADI(void)
{
    GR[REG0] = (short)(IMMEDIATE);
    return 1;
}

int ADD(void)
{
    GR[REG0] = GR[REG1] + GR[REG2];
    if (GR[REG2]>0)
    {
        if (GR[REG0]<GR[REG1])
            PSW.overflow_flg = 1;
        else
            PSW.overflow_flg = 0;
    }
    else if (GR[REG2]<0)
    {
        if (GR[REG0]>GR[REG1])
            PSW.overflow_flg = 1;
        else
            PSW.overflow_flg = 0;
    }
    else
        PSW.overflow_flg = 0;
    return 1;
}

int ADDI(void)
{
    short n = GR[REG0];
    GR[REG0] = (short)(GR[REG0] + IMMEDIATE);
    if (IMMEDIATE>0)
    {
        if (GR[REG0]<n)
            PSW.overflow_flg = 1;
        else
            PSW.overflow_flg = 0;
    }
    else if (IMMEDIATE<0)
    {
        if (GR[REG0]>n)
            PSW.overflow_flg = 1;
        else
            PSW.overflow_flg = 0;
    }
    else
        PSW.overflow_flg = 0;
    return 1;
}

int SUB(void)
{
    GR[REG0] = GR[REG1] - GR[REG2];
    if (GR[REG2]>0)
    {
        if (GR[REG0]>GR[REG1])
            PSW.overflow_flg = 1;
        else
            PSW.overflow_flg = 0;
    }
    else if (GR[REG2]<0)
    {
        if (GR[REG0]<GR[REG1])
            PSW.overflow_flg = 1;
        else
            PSW.overflow_flg = 0;
    }
    else
        PSW.overflow_flg = 0;
    return 1;
}

int SUBI(void)
{
    short n = GR[REG0];
    GR[REG0] = GR[REG0] - IMMEDIATE;
    if (IMMEDIATE>0)
    {
        if (GR[REG0]>n)
            PSW.overflow_flg = 1;
        else
            PSW.overflow_flg = 0;
    }
    else if (IMMEDIATE<0)
    {
        if (GR[REG0]<n)
            PSW.overflow_flg = 1;
        else
            PSW.overflow_flg = 0;
    }
    else
        PSW.overflow_flg = 0;
    return 1;
}

int MUL(void)
{
    GR[REG0] = GR[REG1] * GR[REG2];
    if (GR[REG2]!=0)
    {
        if (GR[REG1] * GR[REG2] / GR[REG2] != GR[REG1])
            PSW.overflow_flg = 1;
        else
            PSW.overflow_flg = 0;
    }
    else
        PSW.overflow_flg = 0;
    return 1;
}

int DIV(void)
{
    if (GR[REG2] == 0)
    {
        printf("ERROR:the divisor is 0!\n");
        exit(-1);
    }
    else
        GR[REG0] = GR[REG1] / GR[REG2];
    return 1;
}

int NOP(void)
{
    return 1;
}

int IN(void)
{
    scanf("%1s", (char *)(GR + REG0));
    return 1;
}

int OUT(void)
{
    printf("%c", *(char *)(GR + REG0));
    return 1;
}

int AND(void)
{
    GR[REG0] = GR[REG1] & GR[REG2];
    return 1;
}

int OR(void)
{
    GR[REG0] = GR[REG1] | GR[REG2];
    return 1;
}

int NOR(void)
{
    GR[REG0] = GR[REG1] ^ GR[REG2];
    return 1;
}

int NOTB(void)
{
    GR[REG0] = ~GR[REG1];
    return 1;
}

int SAL(void)
{
    GR[REG0] = GR[REG1] << GR[REG2];
    return 1;
}

int SAR(void)
{
    GR[REG0] = GR[REG1] >> GR[REG2];
    return 1;
}

int EQU(void)
{
    PSW.compare_flg = (GR[REG0] == GR[REG1]);
    return 1;
}

int LT(void)
{
    PSW.compare_flg = (GR[REG0] < GR[REG1]);
    return 1;
}

int LTE(void)
{
    PSW.compare_flg = (GR[REG0] <= GR[REG1]);
    return 1;
}

int NOTC(void)
{
    PSW.compare_flg = !PSW.compare_flg;
    return 1;
}
