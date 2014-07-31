#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAX_LEN 160
#define INSTRS_COUNT (sizeof(g_instrs_name)/ sizeof(g_instrs_name[0]))
#define INSTR_SYM   {"HLT", "JMP", "CJMP", "OJMP", "CALL", "RET",\
                     "PUSH", "POP",\
                     "LOADB", "LOADW", "STOREB", "STOREW", "LOADI", "NOP",\
                     "IN", "OUT",\
                     "ADD", "ADDI", "SUB", "SUBI", "MUL", "DIV",\
                     "AND", "OR", "NOR", "NOTB", "SAL", "SAR",\
                     "EQU", "LT", "LTE", "NOTC"\
                       }
#define BYTE 1
#define WORD 2
const char *g_instrs_name[]=INSTR_SYM;
const char instr_format[33]="122221\
33\
444451\
66\
757577\
777877\
8881";
unsigned int GetInstrCode(const char *op_sym);
unsigned int TransToCode(char *instr_line, int instr_num);
int GetRegNum(char *instr_line, char *reg_name);
int searchaddr(char *addr);
int searchbyte_word(char *vname);
typedef struct addrnode
{
    int addr;
    char str[10];
    struct addrnode *next;
}*panode, anode;

typedef struct datanode
{
    int type;//BYTE=1; WORD=2
    char name[8];
    int num;
    int dat[65];
    struct datanode *next;
}*pdnode, dnode;
panode addrhead=NULL;
pdnode datahead=NULL;

int main(int argc, char **argv)
{
    char    a_line[MAX_LEN];
    char    op_sym[8];
    int     op_num;

    char    *pcPos;
    FILE    *pfIn, *pfOut;

    int     i=0, cmpb=1, cmpw=1;

    addrhead=(panode)malloc(sizeof(anode));
    datahead=(pdnode)malloc(sizeof(dnode));

    panode pa = addrhead;
    pa->next=NULL;

    pdnode pd = datahead;
    pd->next=NULL;

    if (argc<3)
    {
        perror("no enough command line arguments!");
        return -1;
    }
    if ( (pfIn=fopen(argv[1], "r"))==NULL )
    {
        perror("cannot open file for reading!");
        return -1;
    }

    char    addrstr[10],firstr[10];
    int     addr=0;
    while (fgets(a_line, MAX_LEN, pfIn))
    {

        if ((pcPos=strchr(a_line, '#'))!=NULL)
        {
            *pcPos='\0';
        }
        if( sscanf(a_line, "%s", firstr) < 1 )
            continue;
        if( (strchr(firstr, ':') != NULL ))//handle label
        {
            sscanf(firstr, "%[^:]", addrstr);
            strcpy(pa->str, addrstr);
            pa->addr=addr;
            pa->next=(panode)malloc(sizeof(struct addrnode));
            pa=pa->next;
            pa->next=NULL;
        }
        else if ( ((cmpb=strcmp("BYTE", firstr)) == 0)||((cmpw=strcmp("WORD", firstr) )== 0) )  //handle BYTE and WORD
        {
            if(!cmpb) pd->type=BYTE;
            else if(!cmpw) pd->type=WORD;
            sscanf(a_line, "%*s %[a-zA-Z]", pd->name);
            if ((pcPos=strchr(a_line, '[') )!= NULL)
            {
                sscanf(pcPos+1, "%d", &pd->num);
                if (strchr(a_line, '=') != NULL)
                {
                    if( (pcPos=strchr(a_line, '{')) != NULL )
                    {
                        sscanf(pcPos+1, "%d", &pd->dat[i]);
                        while ((pcPos = strchr(pcPos+1, ',')))
                            sscanf(pcPos+1, "%d", &pd->dat[++i]);
                        i=0;
                    }
                    else if( (pcPos=strchr(a_line, '"')) != NULL )
                    {
                        while(*(++pcPos)!='"')
                            pd->dat[i++]=*pcPos;
                        i=0;
                    }
                }
            }
            else if ( (pcPos=strchr(a_line, '=')) != NULL )
            {
                sscanf(pcPos+1, "%d", &pd->dat[0]);
                pd->num = 1;
            }
            else
                pd->num = 1;
            pd->next=(pdnode)malloc(sizeof(dnode));
            pd=pd->next;
            pd->next=NULL;
            continue;
        }
        addr++;
    }
    fseek(pfIn, 0L, SEEK_SET);
    unsigned int * pTran = (unsigned int *)malloc(sizeof(unsigned int)) ;
    if ( (pfOut=fopen(argv[2], "wb"))==NULL)
    {
        perror("cannot open file for writing!");
        return -1;
    }
    while (fgets(a_line, MAX_LEN, pfIn))
    {

        if ((pcPos=strchr(a_line, '#'))!=NULL)
        {
            *pcPos='\0';
        }
        if( (sscanf(a_line, "%s", firstr) < 1) || (strcmp("BYTE", firstr) == 0) || (strcmp("WORD", firstr) == 0))
            continue;
        if( (strchr(firstr, ':')) != NULL )
        {
            sscanf(a_line, "%*s%s", op_sym);
            strcpy(a_line, strchr(a_line, ':')+1);
        }
        else
            sscanf(a_line, "%s", op_sym);

        op_num=GetInstrCode(op_sym);
        if (op_num>31)
        {
            printf("ERROR: %s is a invalid instruction!\n", a_line);
            exit(-1);
        }
        printf("%08x\n", TransToCode(a_line, op_num));//print hex code
        *pTran = TransToCode(a_line, op_num);
        fwrite(pTran, 1, 4, pfOut);
    }
    int    s=0;
    unsigned int j=0;
    char    bstr;
    short   Bstr;
    for (pd = datahead; (pd->next != NULL) && (pd->num > 0); pd = pd->next)
    {
        if(pd->type == 1)
        {
            for(s = 0; s < pd->num; s++, j++)
            {
                bstr = pd->dat[s];
                printf("%02x\n", bstr);
                fwrite(&bstr, 1, 1, pfOut);
            }
        }
        if(pd->type == 2)
        {
            for(s = 0; s < pd->num; s++, j+=2)
            {
                Bstr = pd->dat[s];
                fwrite(&Bstr, 1, 2, pfOut);
            }
        }
    }
    fwrite(&j, 1, 4, pfOut);
    fclose(pfIn);
    fclose(pfOut);
    return 1;
}

int searchaddr(char *addr)
{
    panode pa=addrhead;
    while(pa->next!=NULL)
    {
        if (strcmp(pa->str, addr)==0)
            return 4*(pa->addr);
        pa=pa->next;
    }
    return -1;
}

int searchbyte_word(char *vname)
{
    int offset = 0;
    pdnode pd=datahead;
    while(pd->next!=NULL)
    {
        if(strcmp(pd->name, vname) == 0)
            return offset;
        else
        {
            offset += pd->num*pd->type;
            pd = pd->next;
        }
    }
    return -1;
}

unsigned int GetInstrCode(const char *op_sym)
{
    int i;
    for (i=0; i<INSTRS_COUNT; i++)
    {
        if (strcmp(op_sym, g_instrs_name[i])==0)
        {
            break;
        }
    }
    return i;
}
unsigned int TransToCode(char *instr_line, int instr_num)
{
    unsigned op_code;
    unsigned arg1, arg2, arg3;
    unsigned instr_code=0u;
    char op_sym[8], reg0[8], reg1[8], reg2[8];
    char addr[10];
    unsigned short int immed, port;
    int n, addrnum;

    switch(instr_format[instr_num]-'0')
    {
    case 1:
        op_code=instr_num;
        instr_code=op_code<<27;
        break;
    case 2:
        n=sscanf(instr_line, "%s %s", op_sym, addr);
        if (n<2)
        {
            printf("ERROR: bad instruction format in\n%s\n", instr_line);
            exit(-1);
        }
        op_code=instr_num;
        if ( (addrnum=searchaddr(addr)) == -1)//addrnum是数据类型为两字节的偏移量
        {
            printf("ERROR: bad instruction format in\n%s\n", instr_line);
            exit(-1);
        }
        instr_code=(op_code<<27)|(addrnum&0x00ffffff);
        break;
    case 3:
        n=sscanf(instr_line, "%s %s", op_sym, reg0);
        if(n<2)
        {
            printf("ERROR: bad instruction format in\n%s\n", instr_line);
            exit(-1);
        }
        op_code=instr_num<<27;
        arg1=GetRegNum(instr_line, reg0);
        instr_code=(op_code)|((arg1<<24)&0x07000000);
        break;
    case 4:
        n=sscanf(instr_line, "%s %s %s", op_sym, reg0, addr);
        if(n<3)
        {
            printf("ERROR: bad instruction format in\n%s\n", instr_line);
            exit(-1);
        }
        if ( (addrnum=searchbyte_word(addr)) == -1)//addrnum是数据类型为一字节的偏移量
        {
            printf("ERROR: bad instruction format in\n%s\n", instr_line);
            exit(-1);
        }
        op_code=instr_num<<27;
        arg1=GetRegNum(instr_line, reg0);
        instr_code=(op_code)|((arg1<<24)&0x07000000)|(addrnum&0x00ffffff);
        break;
    case 5:
        n=sscanf(instr_line, "%s %s %hi", op_sym, reg0, &immed);
        if(n<3)
        {
            printf("ERROR: bad instruction format in\n%s\n", instr_line);
            exit(-1);
        }
        op_code=instr_num<<27;
        arg1=GetRegNum(instr_line, reg0);
        instr_code=(op_code)|((arg1<<24)&0x07000000)|(immed&0xffff);
        break;
    case 6:
        n=sscanf(instr_line, "%s %s %hi", op_sym, reg0, &port);
        if(n<3)
        {
            printf("ERROR: bad instruction format in\n%s\n", instr_line);
            exit(-1);
        }
        op_code=instr_num<<27;
        arg1=GetRegNum(instr_line, reg0);
        instr_code=(op_code)|((arg1<<24)&0x07000000)|(port&0x00ff);
        break;
    case 7:
        n=sscanf(instr_line, "%s %s %s %s", op_sym, reg0, reg1, reg2);
        if(n<4)
        {
            printf("ERROR: bad instruction format in\n%s\n", instr_line);
            exit(-1);
        }
        op_code=instr_num<<27;
        arg1=GetRegNum(instr_line, reg0);
        arg2=GetRegNum(instr_line, reg1);
        arg3=GetRegNum(instr_line, reg2);
        instr_code=(op_code)|((arg1<<24)&0x07000000)|(arg2<<20)|(arg3<<16);
        break;
    case 8:
        n=sscanf(instr_line, "%s %s %s", op_sym, reg0, reg1);
        if(n<3)
        {
            printf("ERROR: bad instruction format!\n");
            exit(-1);
        }
        op_code=instr_num<<27;
        arg1=GetRegNum(instr_line, reg0);
        arg2=GetRegNum(instr_line, reg1);
        instr_code=(op_code)|((arg1<<24)&0x07000000)|(arg2<<20);
        break;
    }
    return instr_code;
}

int GetRegNum(char *instr_line, char *reg_name)
{
    int reg_num;
    if (tolower(*reg_name)=='z')
        reg_num=0;
    else if((tolower(*reg_name)>='a')&&(tolower(*reg_name)<='g'))
        reg_num=tolower(*reg_name)-'a'+1;
    else
    {
        printf("ERROR: bad register name in %s!\n", instr_line);
        exit(-1);
    }
    return reg_num;
}
