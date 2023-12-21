#include <stdio.h>

// #define NRW        11     // number of reserved words
#define NRW        12     // add 'print'
#define TXMAX      500    // length of identifier table
#define MAXNUMLEN  14     // maximum number of digits in numbers
#define NSYM       10     // maximum number of symbols in array ssym and csym
#define MAXIDLEN   10     // length of identifiers

#define MAXADDRESS 32767  // maximum address
#define MAXLEVEL   32     // maximum depth of nesting block
#define CXMAX      500    // size of code array

// #define MAXSYM     30     // maximum number of symbols  
#define MAXSYM     31     // add 'SYM_PRINT'

#define STACKSIZE  1000   // maximum storage

// 所有符号类型枚举变量：保留关键词、运算符、标识符、数字、空、标点符号
enum symtype
{
	SYM_NULL,
	SYM_IDENTIFIER,
	SYM_NUMBER,
	SYM_PLUS,
	SYM_MINUS,
	SYM_TIMES,
	SYM_SLASH,
	SYM_ODD,
	SYM_EQU,
	SYM_NEQ,
	SYM_LES,
	SYM_LEQ,
	SYM_GTR,
	SYM_GEQ,
	SYM_LPAREN,
	SYM_RPAREN,
	SYM_COMMA,
	SYM_SEMICOLON,
	SYM_PERIOD,//句号
	SYM_BECOMES,//赋值
    SYM_BEGIN,
	SYM_END,
	SYM_IF,
	SYM_THEN,
	SYM_WHILE,
	SYM_DO,
	SYM_CALL,
	SYM_CONST,
	SYM_VAR,
	SYM_PROCEDURE,
	// SYM_ARRAY,	// array
	// SYM_LSQUARE,	// [
	// SYM_RSQUARE,	// ]
	// SYM_LBRACE,	// {
	// SYM_RBRACE, // }
	SYM_PRINT	// print
};

// 标识符类型枚举变量（常量，变量，过程）
enum idtype
{
	ID_CONSTANT, ID_VARIABLE, ID_PROCEDURE
	// ID_CONSTANT, ID_VARIABLE, ID_PROCEDURE, ID_ARRAY
};

// 指令类型枚举变量
enum opcode
{
	LIT, OPR, LOD, STO, CAL, INT, JMP, JPC,
	PRINT
	// LIT, OPR, LOD, STO, CAL, INT, JMP, JPC, LEA, LODA, STOA
};

// 算术和逻辑运算符枚举变量，均是后缀表达式，即生成表达式计算指令后，再生成运算指令
enum oprcode
{
	OPR_RET, OPR_NEG, OPR_ADD, OPR_MIN,
	OPR_MUL, OPR_DIV, OPR_ODD, OPR_EQU,
	OPR_NEQ, OPR_LES, OPR_LEQ, OPR_GTR,
	OPR_GEQ,
	OPR_NLN	// '\n'
};


typedef struct
{
	int f; // function code
	int l; // level
	int a; // displacement address
} instruction;

//////////////////////////////////////////////////////////////////////
char* err_msg[] =
{
/*  0 */    "",
/*  1 */    "Found ':=' when expecting '='.",
/*  2 */    "There must be a number to follow '='.",
/*  3 */    "There must be an '=' to follow the identifier.",
/*  4 */    "There must be an identifier to follow 'const', 'var', or 'procedure'.",
/*  5 */    "Missing ',' or ';'.",
/*  6 */    "Incorrect procedure name.",
/*  7 */    "Statement expected.",
/*  8 */    "Follow the statement is an incorrect symbol.",
/*  9 */    "'.' expected.",
/* 10 */    "';' expected.",
/* 11 */    "Undeclared identifier.",
/* 12 */    "Illegal assignment.",
/* 13 */    "':=' expected.",
/* 14 */    "There must be an identifier to follow the 'call'.",
/* 15 */    "A constant or variable can not be called.",
/* 16 */    "'then' expected.",
/* 17 */    "';' or 'end' expected.",
/* 18 */    "'do' expected.",
/* 19 */    "Incorrect symbol.",
/* 20 */    "Relative operators expected.",
/* 21 */    "Procedure identifier can not be in an expression.",
/* 22 */    "Missing ')'.",
/* 23 */    "The symbol can not be followed by a factor.",
/* 24 */    "The symbol can not be as the beginning of an expression.",
/* 25 */    "The number is too great.",
/* 26 */    "",
/* 27 */    "",
/* 28 */    "",
/* 29 */    "",
/* 30 */    "",
/* 31 */    "",
/* 32 */    "There are too many levels."
};

//////////////////////////////////////////////////////////////////////
char ch;         // last character read
int  sym;        // last symbol read
char id[MAXIDLEN + 1]; // last identifier read
int  num;        // last number read
int  cc;         // character count
int  ll;         // line length
int  kk;
int  err;		 // 错误个数
int  cx;         // index of current instruction to be generated. CX 表示下一条即将生成的目标指令的地址
int  level = 0;
int  tx = 0;	 // 指向符号表顶部的指针

char line[80];	// 读取一行源程序

instruction code[CXMAX];

// 所有保留关键词，用于比较输入的标识符是否为保留关键词
char* word[NRW + 1] =
{
	"", /* place holder */
	"begin", "call", "const", "do", "end","if",
	"odd", "procedure", "then", "var", "while",
	"print"
};

// 所有保留关键词对应 symtype 枚举变量，用于设置 sys 变量
int wsym[NRW + 1] =
{
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE,
	SYM_PRINT
};

// 除了保留关键词，自定义标识符，数字，赋值和比较运算符，其他符号的符号表，算数运算符，标点，括号等
int ssym[NSYM + 1] =
{
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON
	// SYM_LSQUARE, SYM_RSQUARE, SYM_LBRACE, SYM_RBRACE
};

// 除了保留关键词，自定义标识符，数字，赋值和比较运算符，其他符号的符号表对应的字符，算数运算符，标点，括号等
char csym[NSYM + 1] =
{
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';'
	// '[' , ']', '{', '}'
};

// #define MAXINS   8
#define MAXINS   9	// add 'PRINT'
char* mnemonic[MAXINS] =
{
	"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC",
	"PRINT"
	// "LEA", "LODA", "STOA"
};	// 指令伪操作符

// 常量结构体：标识符名字，类型，值。存在符号表中，与变量和过程在内存中不区分，操作时 kind==SYM_CONST 区分
typedef struct
{
	char name[MAXIDLEN + 1];
	int  kind;
	int  value;
} comtab;

// 符号表：保存每个符号的属性，常量就是comtab结构体，变量和过程就是mask结构体
comtab table[TXMAX];

// 变量和过程结构体：名字，类型，层次，地址（变量是修正量/偏移量，过程则是入口地址）。存在符号表中，与常量在内存中不区分，操作时 kind==SYM_VAR 或 SYM_PROCEDURE 区分
typedef struct
{
	char  name[MAXIDLEN + 1];
	int   kind;
	short level;
	short address;
} mask;

FILE* infile;

// EOF PL0.h
