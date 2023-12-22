#include <stdio.h>

#define TRUE	   		1																			// e1
#define FALSE	   		0

#define NRW        12     // add 'print'
#define TXMAX      500    // length of identifier table
#define MAXNUMLEN  14     // maximum number of digits in numbers
#define NSYM       14     // maximum number of symbols in array ssym and csym
#define MAXIDLEN   10     // length of identifiers
#define MAXFUNPMT  		10	   // maximum number of parameters of a procedure

#define TABLE_BEGIN 	0	   // the beginning index of the TABLE, used in position()				// e1
#define MAXADDRESS 32767  // maximum address
#define MAXLEVEL   32     // maximum depth of nesting block
#define CXMAX  500    // size of code array
#define MAXARYDIM  	10	   // maximum number of dimensions of an array							// e1
#define MAXARYVOL  	200	   // maximum volume of a dimension of an array
#define MAXELIF			20	   // maximum number of elifs

#define CONST_EXPR 		0	   // the expression is constant
#define UNCONST_EXPR 	1	   // the expression is not constant
#define MAX_CONTROL 	50     //maxinum control statement

#define PMT_PROC   		1	   // indicate that this name is a procedure and a parameter of a procedure	// e1
#define NON_PMT_PROC 	0	   // otherwise

// #define MAXSYM     30     // maximum number of symbols
#define MAXSYM     36

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
    SYM_TRUE,					// e1
    SYM_FALSE,
    SYM_ELSE,
    SYM_ELIF,
    SYM_NOT,
    SYM_FOR,
    SYM_OR,
    SYM_AND,
	SYM_CONST,
	SYM_VAR,
    SYM_COLON,
	SYM_PROCEDURE,
    SYM_AMPERSAND,
    SYM_VOID,
    SYM_INT,
    SYM_LSQUARE,	// [
	SYM_RSQUARE,	// ]
	SYM_LBRACE,	// {
	SYM_RBRACE, // }
	SYM_PRINT	// print
};

// 标识符类型枚举变量（常量，变量，过程）
enum idtype
{
	ID_CONSTANT, ID_VARIABLE, ID_PROCEDURE, ID_ARRAY, ID_POINTER, ID_VOID, ID_LABEL
};

enum environment                                           										//e1
{
    ENV_FOR, ENV_WHILE
};


// 指令类型枚举变量
enum opcode
{
	LIT, OPR, LOD, STO, CAL, INT, JMP, JPC,
	PRINT, STOI, STOS, LODS, LODI, JNDN, JND, LEA, CALS, RET, EXT, JET
};

// 算术和逻辑运算符枚举变量，均是后缀表达式，即生成表达式计算指令后，再生成运算指令
enum oprcode
{
	OPR_NEG, OPR_ADD, OPR_MIN,
	OPR_MUL, OPR_DIV, OPR_ODD, OPR_EQU,
	OPR_NEQ, OPR_LES, OPR_LEQ, OPR_GTR,
	OPR_GEQ, OPR_OR, OPR_AND, OPR_ITOB,
    OPR_NOT, OPR_RET
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
/*  2 */    "There must be a number to follow '='.",						// unused
/*  3 */    "There must be an '=' to follow the identifier.",
/*  4 */    "There must be an identifier to follow 'const', 'var', or 'procedure'.",
/*  5 */    "Missing ',' or ';'.",
/*  6 */    "Incorrect procedure name.",									// unused
/*  7 */    "Declaration/Statement expected.",
/*  8 */    "Follow the statement is an incorrect symbol.",
/*  9 */    "'.' expected.",
/* 10 */    "';' expected.",
/* 11 */    "Undeclared identifier.",
/* 12 */    "Illegal assignment.",
/* 13 */    "':=' expected.",
/* 14 */    "There must be an identifier to follow the 'call'.",			// unused
/* 15 */    "There must be an identifier to follow '&'.",		// e1
/* 16 */    "'then' expected.",
/* 17 */    "'end' expected.",					// e1
/* 18 */    "'do' expected.",
/* 19 */    "Incorrect symbol.",
/* 20 */    "Relop not admitted in const expression.", // e1
/* 21 */    "Procedure identifier can not be in an expression.",
/* 22 */    "Missing ')'.",
/* 23 */    "The symbol can not be followed by an expression.",	// e1
/* 24 */    "The symbol can not be as the beginning of an expression.",
/* 25 */    "The number is too great.",
/* 26 */    "Missing ';'.",						// 26 e1
/* 27 */    "Applying the index operator on non-array.",		// 27-29 e1
/* 28 */    "Variables can not be in a const expression.",
/* 29 */    "Too few subscripts.",
/* 30 */    "Too many subscripts.",				// 30 e1
/* 31 */    "Redeclaration of an identifier.",	// 31 e1
/* 32 */    "There are too many levels.",
/* 33 */	"Numeric constant expected.",		// 33-36 e1
/* 34 */	"']' expected.",
/* 35 */	"There are too many dimensions.",
/* 36 */	"Volume of a dimension is out of range.",
/* 37 */	"'=' expected.",					// 37 e1
/* 38 */	"'[' expected.",					// 38 e1
/* 39 */	"Too many parameters in a procedure.",	// 39-42 e1
/* 40 */	"Missing ',' or ')'.",
/* 41 */	"Non-array type/incorrect indices.",	// e1
/* 42 */	"Too few parameters in a procedure.",
/* 43 */    "'(' expected.",
/* 44 */    "There must be a variable in 'for' statement.",
/* 45 */    "you must return a constant.",
/* 46 */    "no more exit can be added.",
/* 47 */    "'else' expected.",								// unused
/* 48 */    "another '|' is expected.",
/* 49 */     "'while' expected .",
/* 50 */    "there must be 'begin' in switch statement.",                 //e1
/* 51 */    "'case','end',or 'default' is expected .",
/* 52 */    "':' expected .",
/* 53 */	"The symbol can not be as the beginning of an lvalue expression.", // 53-56 e1
/* 54 */	"Incorrect type as an lvalue expression.",
/* 55 */	"The symbol can not be as the beginning of a function call.",
/* 56 */	"Non-procedure type/incorrect parameter types.",
/* 57 */    "procedure can not be in a const factor.",		// unused
/* 58 */    "label must be followed by a statement.",
/* 59 */    "Undeclared label.",
/* 60 */	"Procedure can not be in const expression.",					// e1
/* 61 */	"'void' or 'int' needed before a procedure declared.",			// 61-64 e1
/* 62 */	"There must be an identifier to follow 'void' or 'int'.",
/* 63 */	"Void value not ignored as it ought to be.",
/* 64 */	"Applying function calling operator on non-function.",
/* 65 */	"Return a value in function returning void.",					// 65-68 e1
/* 66 */	"Return void in function returning a value.",
/* 67 */	"Too many elifs.",
/* 68 */	"'elif' or 'else' expected.",
/* 69 */	"Assigning non-array type to an array.",						// 69-70 e1
/* 70 */	"Assigning non-function type to a function.",
/* 71 */    "id can't be used as a label."									// e1
        };

typedef struct type comtyp;
//////////////////////////////////////////////////////////////////////
char ch;         // last character read
int  sym;        // last symbol read
char id[MAXIDLEN + 1]; // last identifier read
int  num;        // last number read
comtyp  *ptr;	 // a dynamic array containing elements composing a composite type
int  cc;         // character count
int  ll;         // line length
int  kk;
int  err;		 // 错误个数
int  cx;         // index of current instruction to be generated. CX 表示下一条即将生成的目标指令的地址
int  level = 0;
int  tx = 0;	 // 指向符号表顶部的指针
int tx_b = 0;
int  *list[2] = {}; // list[0]: f_list, list[1]: t_list
int  cc_p;		 // cc of the first ch of current sym											// e1
int  sym_p;		 // sym before current sym was read
int  env;        // mark the type of environment where break,continue is
int  head;
int tail;

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
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON,
	SYM_LSQUARE, SYM_RSQUARE, SYM_LBRACE, SYM_RBRACE
};

// 除了保留关键词，自定义标识符，数字，赋值和比较运算符，其他符号的符号表对应的字符，算数运算符，标点，括号等
char csym[NSYM + 1] =
{
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';',
	'[' , ']', '{', '}'
};

#define MAXINS   20
char* mnemonic[MAXINS] ={
                "LIT", "OPR", "LOD", "LODI", "LODS", "LEA", "STO", "STOI", "STOS",
                "CAL", "CALS", "INT", "JMP", "JPC", "JND", "JNDN", "RET",
                "EXT", "JET", "PRINT"};
	// 指令伪操作符

// 常量结构体：标识符名字，类型，值。存在符号表中，与变量和过程在内存中不区分，操作时 kind==SYM_CONST 区分
typedef struct
{
	char name[MAXIDLEN + 1];
	int  kind;
	int  value;
    comtyp *ptr;
} comtab;

// 符号表：保存每个符号的属性，常量就是comtab结构体，变量和过程就是mask结构体
comtab table[TXMAX];

struct type																						// e1
{
    int k; // kind of parameter (for procedure) / dimension (for array)
    struct type *ptr;
};

// 变量和过程结构体：名字，类型，层次，地址（变量是修正量/偏移量，过程则是入口地址）。存在符号表中，与常量在内存中不区分，操作时 kind==SYM_VAR 或 SYM_PROCEDURE 区分
typedef struct
{
	char  name[MAXIDLEN + 1];
	int   kind;
	short level;
	short address;
    comtyp *ptr;
} mask;

FILE* infile;

int tx_c = 0;

typedef struct
{
    int ty;                     //type
    int c;                     //cx of the stat
} col;
col cltab[MAX_CONTROL];             //max depth of circulation
int cltop = 0;                      //top of cltab
int count = 0;                   //count num of break and continue

// EOF PL0.h
