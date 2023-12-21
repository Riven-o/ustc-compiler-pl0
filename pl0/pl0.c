// pl0 compiler source code

#pragma warning(disable:4996)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "PL0.h"
#include "set.c"

//////////////////////////////////////////////////////////////////////
// print error message.
void error(int n)
{
	int i;

	printf("      ");
	for (i = 1; i <= cc - 1; i++)
		printf(" ");
	printf("^\n");
	printf("Error %3d: %s\n", n, err_msg[n]);
	err++;
} // error

//////////////////////////////////////////////////////////////////////
void getch(void)
{
	if (cc == ll)
	{
		if (feof(infile))
		{
			printf("\nPROGRAM INCOMPLETE\n");
			exit(1);
		}
		ll = cc = 0;
		printf("%5d  ", cx);
		while ( (!feof(infile)) // added & modified by alex 01-02-09
			    && ((ch = getc(infile)) != '\n'))
		{
			printf("%c", ch);
			line[++ll] = ch;
		} // while
		printf("\n");
		line[++ll] = ' ';
	}
	ch = line[++cc];
} // getch

//////////////////////////////////////////////////////////////////////
// gets a symbol from input stream. 调用完后，sym 里面存放的是当前读取到的符号类型，然后据此从对应全局变量找到对应的符号（数字，标识符等），ch指向读完后的下一个字符
void getsym(void)
{
	int i, k;
	char a[MAXIDLEN + 1];

	while (ch == ' '||ch == '\t')
		getch();

	if (isalpha(ch))
	{ // symbol is a reserved word or an identifier.
		k = 0;
		do
		{
			if (k < MAXIDLEN)
				a[k++] = ch;
			getch();
		}
		while (isalpha(ch) || isdigit(ch));
		a[k] = 0;
		strcpy(id, a);
		word[0] = id;
		i = NRW;
		while (strcmp(id, word[i--]));
		if (++i)
			sym = wsym[i]; // symbol is a reserved word
		else
			sym = SYM_IDENTIFIER;   // symbol is an identifier
	}
	else if (isdigit(ch))
	{ // symbol is a number.
		k = num = 0;
		sym = SYM_NUMBER;
		do
		{
			num = num * 10 + ch - '0';
			k++;
			getch();
		}
		while (isdigit(ch));
		if (k > MAXNUMLEN)
			error(25);     // The number is too great.
	}
	else if (ch == ':')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_BECOMES; // :=
			getch();
		}
		else
		{
			sym = SYM_NULL;       // illegal?
		}
	}
	else if (ch == '>')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_GEQ;     // >=
			getch();
		}
		else
		{
			sym = SYM_GTR;     // >
		}
	}
	else if (ch == '<')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_LEQ;     // <=
			getch();
		}
		else if (ch == '>')
		{
			sym = SYM_NEQ;     // <>
			getch();
		}
		else
		{
			sym = SYM_LES;     // <
		}
	}
	else
	{ // other tokens
		i = NSYM;
		csym[0] = ch;
		while (csym[i--] != ch);
		if (++i)
		{
			sym = ssym[i];
			getch();
		}
		else
		{
			printf("Fatal Error: Unknown character.\n");
			exit(1);
		}
	}
} // getsym

//////////////////////////////////////////////////////////////////////
// generates (assembles) an instruction.
void gen(int x, int y, int z)
{
	if (cx > CXMAX)
	{
		printf("Fatal Error: Program too long.\n");
		exit(1);
	}
	code[cx].f = x;
	code[cx].l = y;
	code[cx++].a = z;
} // gen

//////////////////////////////////////////////////////////////////////
// tests if error occurs and skips all symbols that do not belongs to s1 or s2. 遇到错误输入时，跳过后面部分输入，直到下一句正常代码开始
void test(symset s1, symset s2, int n)
{
	symset s;

	if (! inset(sym, s1))
	{
		error(n);
		s = uniteset(s1, s2);
		while(! inset(sym, s))
			getsym();
		destroyset(s);
	}
} // test

//////////////////////////////////////////////////////////////////////
int dx;  // data allocation index

// enter object(constant, variable or procedre) into table.
void enter(int kind)
{
	mask* mk;

	tx++;
	strcpy(table[tx].name, id);
	table[tx].kind = kind;
	switch (kind)
	{
	case ID_CONSTANT:
		if (num > MAXADDRESS)
		{
			error(25); // The number is too great.
			num = 0;
		}
		table[tx].value = num;
		break;
	case ID_VARIABLE:
		mk = (mask*) &table[tx];
		mk->level = level;
		mk->address = dx++;
		break;
	case ID_PROCEDURE:
		mk = (mask*) &table[tx];
		mk->level = level;
		break;
	} // switch
} // enter

//////////////////////////////////////////////////////////////////////
// locates identifier in symbol table. 返回符号在符号表中的位置，然后通过table[i]找到对应的符号
int position(char* id)
{
	int i;
	strcpy(table[0].name, id);
	i = tx + 1;
	while (strcmp(table[--i].name, id) != 0);
	return i;
} // position

//////////////////////////////////////////////////////////////////////
// 已经读完const和标识符，接下来读取等号和数字，然后将常量加入符号表，调用完成后，sym指向下一个标识符
void constdeclaration()
{
	if (sym == SYM_IDENTIFIER)
	{
		getsym();
		if (sym == SYM_EQU || sym == SYM_BECOMES)
		{
			if (sym == SYM_BECOMES)
				error(1); // Found ':=' when expecting '='.
			getsym();
			if (sym == SYM_NUMBER)
			{
				enter(ID_CONSTANT);
				getsym();
			}
			else
			{
				error(2); // There must be a number to follow '='.
			}
		}
		else
		{
			error(3); // There must be an '=' to follow the identifier.
		}
	} else	error(4);
	 // There must be an identifier to follow 'const', 'var', or 'procedure'.
} // constdeclaration

//////////////////////////////////////////////////////////////////////
// 已经读完var和标识符，接下来将变量加入符号表，调用完成后，sym指向下一个标识符
void vardeclaration(void)
{
	if (sym == SYM_IDENTIFIER)
	{
		enter(ID_VARIABLE);
		getsym();
	}
	else
	{
		error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
	}
} // vardeclaration

//////////////////////////////////////////////////////////////////////
// 打印指定范围内的汇编代码
void listcode(int from, int to)
{
	int i;
	
	printf("\n");
	for (i = from; i < to; i++)
	{
		printf("%5d %s\t%d\t%d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
	}
	printf("\n");
} // listcode

//////////////////////////////////////////////////////////////////////

/*
	PL/0 的编译程序为每一条 PL/0 源程序的可执行语句生成后缀式目标代码。
*/


void factor(symset fsys)
{
	void expression(symset fsys);
	int i;
	symset set;
	
	test(facbegsys, fsys, 24); // The symbol can not be as the beginning of an expression.

	if (inset(sym, facbegsys))
	{
		if (sym == SYM_IDENTIFIER)	// 从符号表中找出标识符，然后生成LOD指令，即将变量的值放入栈顶，如果是常量符号，生成LIT指令，将常量值放入栈顶
		{
			if ((i = position(id)) == 0)
			{
				error(11); // Undeclared identifier.
			}
			else
			{
				switch (table[i].kind)
				{
					mask* mk;
				case ID_CONSTANT:
					gen(LIT, 0, table[i].value);
					break;
				case ID_VARIABLE:
					mk = (mask*) &table[i];
					gen(LOD, level - mk->level, mk->address);
					break;
				case ID_PROCEDURE:
					error(21); // Procedure identifier can not be in an expression.
					break;
				} // switch
			}
			getsym();
		}
		else if (sym == SYM_NUMBER)	// 生成LIT指令，将代码中直接出现的常量值放入栈顶
		{
			if (num > MAXADDRESS)
			{
				error(25); // The number is too great.
				num = 0;
			}
			gen(LIT, 0, num);
			getsym();
		}
		else if (sym == SYM_LPAREN)	// 分析括号内的表达式，然后生成相应指令
		{
			getsym();
			set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys);
			expression(set);
			destroyset(set);
			if (sym == SYM_RPAREN)
			{
				getsym();
			}
			else
			{
				error(22); // Missing ')'.
			}
		}
		else if(sym == SYM_MINUS) // UMINUS,  Expr -> '-' Expr。由于是后缀表达式，所以实际生成代码：Fact -> Fact '-'
		{  
			 getsym();
			 factor(fsys);
			 gen(OPR, 0, OPR_NEG);
		}
		test(fsys, createset(SYM_LPAREN, SYM_NULL), 23);
	} // if
} // factor

//////////////////////////////////////////////////////////////////////
void term(symset fsys)
{
	int mulop;
	symset set;
	
	// set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH, SYM_NULL));
	set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH, SYM_COMMA, SYM_RPAREN, SYM_NULL)); // print语句中表达式的follow={','，')'}
	factor(set);
	while (sym == SYM_TIMES || sym == SYM_SLASH)
	{
		mulop = sym;
		getsym();
		factor(set);
		if (mulop == SYM_TIMES)
		{
			gen(OPR, 0, OPR_MUL);	// 后缀乘表达式,  Term -> Fact Fact '*'
		}
		else
		{
			gen(OPR, 0, OPR_DIV);
		}
	} // while
	destroyset(set);
} // term

//////////////////////////////////////////////////////////////////////
void expression(symset fsys)
{
	int addop;
	symset set;

	set = uniteset(fsys, createset(SYM_PLUS, SYM_MINUS, SYM_NULL));
	
	term(set);
	while (sym == SYM_PLUS || sym == SYM_MINUS)
	{
		addop = sym;
		getsym();
		term(set);
		if (addop == SYM_PLUS)
		{
			gen(OPR, 0, OPR_ADD);	// 后缀加表达式,  Expr -> Term Term '+'
		}
		else
		{
			gen(OPR, 0, OPR_MIN);
		}
	} // while

	destroyset(set);
} // expression

//////////////////////////////////////////////////////////////////////
void condition(symset fsys)
{
	int relop;
	symset set;

	if (sym == SYM_ODD)
	{
		getsym();
		expression(fsys);
		gen(OPR, 0, OPR_ODD);	// 奇偶表达式,  Cond -> 'odd' Expr，生成后缀代码：Cond -> Expr 'odd'
	}
	else
	{
		set = uniteset(relset, fsys);
		expression(set);	// 分析第一个表达式
		destroyset(set);
		if (! inset(sym, relset))
		{
			error(20);
		}
		else
		{
			relop = sym;
			getsym();
			expression(fsys);	// 分析第二个表达式
			// 后缀关系表达式,  Cond -> Expr Expr Relop
			switch (relop)
			{
			case SYM_EQU:
				gen(OPR, 0, OPR_EQU);
				break;
			case SYM_NEQ:
				gen(OPR, 0, OPR_NEQ);
				break;
			case SYM_LES:
				gen(OPR, 0, OPR_LES);
				break;
			case SYM_GEQ:
				gen(OPR, 0, OPR_GEQ);
				break;
			case SYM_GTR:
				gen(OPR, 0, OPR_GTR);
				break;
			case SYM_LEQ:
				gen(OPR, 0, OPR_LEQ);
				break;
			} // switch
		} // else
	} // else
} // condition

//////////////////////////////////////////////////////////////////////
void statement(symset fsys)
{
	int i, cx1, cx2;
	symset set1, set;

	if (sym == SYM_IDENTIFIER)
	{ // variable assignment。先找出符号表中符号，然后读取赋值号，再分析表达式，最后生成STO指令，即将表达式的值存入变量中
		mask* mk;
		if (! (i = position(id)))
		{
			error(11); // Undeclared identifier.
		}
		else if (table[i].kind != ID_VARIABLE)
		{
			error(12); // Illegal assignment.
			i = 0;
		}
		getsym();
		if (sym == SYM_BECOMES)
		{
			getsym();
		}
		else
		{
			error(13); // ':=' expected.
		}
		expression(fsys);
		mk = (mask*) &table[i];
		if (i)
		{
			gen(STO, level - mk->level, mk->address);
		}
	}
	else if (sym == SYM_CALL)
	{ // procedure call。从符号表中找出过程入口，生成CAL指令，即调用过程
		getsym();
		if (sym != SYM_IDENTIFIER)
		{
			error(14); // There must be an identifier to follow the 'call'.
		}
		else
		{
			if (! (i = position(id)))
			{
				error(11); // Undeclared identifier.
			}
			else if (table[i].kind == ID_PROCEDURE)
			{
				mask* mk;
				mk = (mask*) &table[i];
				gen(CAL, level - mk->level, mk->address);
			}
			else
			{
				error(15); // A constant or variable can not be called. 
			}
			getsym();
		}
	} 
	else if (sym == SYM_IF)
	{ // if statement
		getsym();
		set1 = createset(SYM_THEN, SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);	// 分析条件
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_THEN)
		{
			getsym();
		}
		else
		{
			error(16); // 'then' expected.
		}
		// 回填！！！文档 11 页
		cx1 = cx;
		gen(JPC, 0, 0);
		statement(fsys);
		code[cx1].a = cx;	
	}
	else if (sym == SYM_BEGIN)	// 分析复合语句，即begin...end中间的代码
	{ // block
		getsym();
		set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
		set = uniteset(set1, fsys);
		statement(set);
		while (sym == SYM_SEMICOLON || inset(sym, statbegsys))
		{
			if (sym == SYM_SEMICOLON)
			{
				getsym();
			}
			else
			{
				error(10);
			}
			statement(set);
		} // while
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_END)
		{
			getsym();
		}
		else
		{
			error(17); // ';' or 'end' expected.
		}
	}
	else if (sym == SYM_WHILE)
	{ // while statement
		cx1 = cx;
		getsym();
		set1 = createset(SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);	// 分析条件
		destroyset(set1);
		destroyset(set);
		cx2 = cx;
		gen(JPC, 0, 0);
		if (sym == SYM_DO)
		{
			getsym();
		}
		else
		{
			error(18); // 'do' expected.
		}
		statement(fsys);	// 分析循环体
		gen(JMP, 0, cx1);	// 回填 JMP 地址，无条件跳转到条件判断前，即完成一次循环
		code[cx2].a = cx;	// 回填 JPC 地址，条件为假时，跳转到循环体后面
	}
	else if (sym == SYM_PRINT)
	{ // print
		int is_print_none = 0;
		getsym();
		if (sym != SYM_LPAREN)
		{
			// error()
			printf("Error: '(' expected.\n");
		}
		do{
			getsym();
			if (sym == SYM_RPAREN)
			{
				is_print_none = 1;
				gen(OPR, 0, OPR_NLN);	// print(None) 生成打印换行指令
				break;
			}
			expression(fsys);	// 分析表达式，结果放在栈顶
			gen(PRINT, 0, 0);	// 生成打印指令，打印栈顶的值
		}while(sym == SYM_COMMA);
		if (sym != SYM_RPAREN)
		{
			// error()
			printf("Error: ')' expected.\n");
		}
		if (!is_print_none)
			gen(OPR, 0, OPR_NLN);	// print(...) 生成打印换行指令
		getsym();
	}
	test(fsys, phi, 19);
} // statement
			
//////////////////////////////////////////////////////////////////////
// 分析程序体
void block(symset fsys)
{
	int cx0; // initial code index
	mask* mk;
	int block_dx;	// 记录"本"过程体的数据区大小，即变量个数（常量直接定义，不需要分配）加上三个内部变量 RA，DL 和 SL
	int savedTx;
	symset set1, set;

	// 开始编译一个过程时，要对数据单元的下标 dx 赋初值，表示新开辟一个数据区。
	dx = 3;	// 初值为 3，因为每个数据区包含三个内部变量 RA，DL 和 SL，即 return address，dynamic link 和 static link
	block_dx = dx;

	// 每个程序体入口处都生成一条JMP指令，用于在执行时跳过定义内容（因为总是先定义所有需要的变量，常量，过程，然后再定义执行语句），进入执行语句
	mk = (mask*) &table[tx];
	mk->address = cx;
	gen(JMP, 0, 0);
	if (level > MAXLEVEL)
	{
		error(32); // There are too many levels.
	}
	do
	{
		if (sym == SYM_CONST)
		{ // constant declarations
			getsym();	// 读标识符
			do
			{
				constdeclaration();
				while (sym == SYM_COMMA)
				{
					getsym();
					constdeclaration();
				}
				if (sym == SYM_SEMICOLON)
				{
					getsym();
				}
				else
				{
					error(5); // Missing ',' or ';'.
				}
			}
			while (sym == SYM_IDENTIFIER);	// 分号后面如果还是标识符，当作上一句 CONST 的一部分
		} // if

		if (sym == SYM_VAR)
		{ // variable declarations
			getsym();
			do
			{
				vardeclaration();
				while (sym == SYM_COMMA)
				{
					getsym();
					vardeclaration();
				}
				if (sym == SYM_SEMICOLON)
				{
					getsym();
				}
				else
				{
					error(5); // Missing ',' or ';'.
				}
			}
			while (sym == SYM_IDENTIFIER);
		} // if
		block_dx = dx; //save dx before handling procedure call!	子过程需要其自己来维护自己的数据区
		while (sym == SYM_PROCEDURE)
		{ // procedure declarations
			// 已经读了过程标识符，接下来将过程名加入符号表，然后读取结尾分号
			getsym();
			if (sym == SYM_IDENTIFIER)
			{
				enter(ID_PROCEDURE);
				getsym();
			}
			else
			{
				error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
			}


			if (sym == SYM_SEMICOLON)
			{
				getsym();
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}
			// 定义该过程体，过程体就是一个分程序，所以调用block函数
			level++;	// 进入一个新的分程序，层次加一
			savedTx = tx;	// 保存父符号表指针，子过程定义需要自己的符号表
			set1 = createset(SYM_SEMICOLON, SYM_NULL);
			set = uniteset(set1, fsys);
			block(set);	// 分析子过程体
			destroyset(set1);
			destroyset(set);
			tx = savedTx;	// 恢复父符号表指针
			level--;	// 退回父程序，继续分析父程序

			if (sym == SYM_SEMICOLON)	// 分析完子过程体后，读取结尾END关键词后面的分号
			{
				getsym();
				set1 = createset(SYM_IDENTIFIER, SYM_PROCEDURE, SYM_NULL);
				set = uniteset(statbegsys, set1);
				test(set, fsys, 6);
				destroyset(set1);
				destroyset(set);
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}
		} // while
		dx = block_dx; //restore dx after handling procedure call!
		set1 = createset(SYM_IDENTIFIER, SYM_NULL);
		set = uniteset(statbegsys, set1);
		test(set, declbegsys, 7);
		destroyset(set1);
		destroyset(set);
	}
	while (inset(sym, declbegsys));
	// 一个过程总是先定义所有需要的常量、变量和过程，然后才是执行语句，上面都while循环定义完了，接下来分析执行语句

	code[mk->address].a = cx;	// 回填程序体第一条指令 JMP 的地址，即进入一个程序跳转到执行语句的开始处执行
	mk->address = cx;
	cx0 = cx;
	gen(INT, 0, block_dx);	// 生成INT指令，分配数据区（在数据栈中分配存贮空间）。注意每个过程需要分配的地址为局部变量个数加三个内部变量 RA，DL 和 SL
	set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
	set = uniteset(set1, fsys);
	statement(set);	// 分析过程体执行语句，即本过程begin...end中间的代码，注意上面是声明部分
	destroyset(set1);
	destroyset(set);
	gen(OPR, 0, OPR_RET); // return
	test(fsys, phi, 8); // test for error: Follow the statement is an incorrect symbol.
	listcode(cx0, cx);	// 每次分析完一个过程体，打印生成的需要执行的汇编代码
} // block

//////////////////////////////////////////////////////////////////////
int base(int stack[], int currentLevel, int levelDiff)
{
	int b = currentLevel;
	
	while (levelDiff--)
		b = stack[b];
	return b;
} // base

//////////////////////////////////////////////////////////////////////
// interprets and executes codes.
void interpret()
{
	int pc;        // program counter
	int stack[STACKSIZE];
	int top;       // top of stack
	int b;         // program, base, and top-stack register
	instruction i; // instruction register

	printf("Begin executing PL/0 program.\n");

	pc = 0;
	b = 1;
	top = 3;
	stack[1] = stack[2] = stack[3] = 0;
	do
	{
		i = code[pc++];
		switch (i.f)
		{
		case LIT:
			stack[++top] = i.a;
			break;
		case OPR:	// 由于生成的是后缀表达式，所以执行时非常容易，读到一个运算符，说明前两个或一个操作数已经翻译完成，放在了栈顶，直接执行对应操作即可
			switch (i.a) // operator
			{
			case OPR_RET:
				top = b - 1;	// 对应父过程下一条指令的地址，退出该过程后直接执行父过程调用完成的下一条指令
				pc = stack[top + 3];	// 通过返回地址找到返回点，即退出子过程，返回父过程
				b = stack[top + 2];	// 通过动态链找到父过程的基地址，恢复调用前的状态
				break;
			case OPR_NEG:
				stack[top] = -stack[top];
				break;
			case OPR_ADD:
				top--;
				stack[top] += stack[top + 1];
				break;
			case OPR_MIN:
				top--;
				stack[top] -= stack[top + 1];
				break;
			case OPR_MUL:
				top--;
				stack[top] *= stack[top + 1];
				break;
			case OPR_DIV:
				top--;
				if (stack[top + 1] == 0)
				{
					fprintf(stderr, "Runtime Error: Divided by zero.\n");
					fprintf(stderr, "Program terminated.\n");
					continue;
				}
				stack[top] /= stack[top + 1];
				break;
			case OPR_ODD:
				stack[top] %= 2;
				break;
			case OPR_EQU:
				top--;
				stack[top] = stack[top] == stack[top + 1];
				break;
			case OPR_NEQ:
				top--;
				stack[top] = stack[top] != stack[top + 1];
				break;
			case OPR_LES:
				top--;
				stack[top] = stack[top] < stack[top + 1];
				break;
			case OPR_GEQ:
				top--;
				stack[top] = stack[top] >= stack[top + 1];
				break;
			case OPR_GTR:
				top--;
				stack[top] = stack[top] > stack[top + 1];
				break;
			case OPR_LEQ:
				top--;
				stack[top] = stack[top] <= stack[top + 1];
				break;
			case OPR_NLN:
				printf("\n");
				break;
			} // switch
			break;
		case LOD:
			stack[++top] = stack[base(stack, b, i.l) + i.a];
			break;
		case STO:
			stack[base(stack, b, i.l) + i.a] = stack[top];
			printf("%d\n", stack[top]);
			top--;
			break;
		case CAL:
			stack[top + 1] = base(stack, b, i.l);	// 建立静态链
			// generate new block mark
			stack[top + 2] = b;	// 建立动态链
			stack[top + 3] = pc;	// 保存返回地址
			b = top + 1;	// 改变基地址指针
			pc = i.a;	// 跳转到过程入口
			break;
		case INT:
			// CAL之后会跳到程序入口，程序入口有一条JMP指令进入执行语句，编译分析执行语句前生成了该指令gen(INT, 0, block_dx)，这里就是执行时分配数据区
			// 分配量由dx决定，dx仅在entry函数读到变量时加1，这时表明来预留变量的存储空间
			// 而变量仅在factor函数中读到该变量时才会拿来计算，即需要取出该变量的值，这时才会生成LOD指令，所以全局就这一个生成LOD指令的地方
			top += i.a;	
			break;
		case JMP:
			pc = i.a;
			break;
		case JPC:
			if (stack[top] == 0)
				pc = i.a;
			top--;
			break;
		case PRINT:
			printf("%d ", stack[top]);
			top--;
			break;
		} // switch
	}
	while (pc);

	printf("End executing PL/0 program.\n");
} // interpret

//////////////////////////////////////////////////////////////////////
void main ()
{
	FILE* hbin;
	char s[80];
	int i;
	symset set, set1, set2;

	printf("Please input source file name: "); // get file name to be compiled
	scanf("%s", s);
	if ((infile = fopen(s, "r")) == NULL)
	{
		printf("File %s can't be opened.\n", s);
		exit(1);
	}

	phi = createset(SYM_NULL);
	relset = createset(SYM_EQU, SYM_NEQ, SYM_LES, SYM_LEQ, SYM_GTR, SYM_GEQ, SYM_NULL);
	
	// create begin symbol sets
	declbegsys = createset(SYM_CONST, SYM_VAR, SYM_PROCEDURE, SYM_NULL);
	statbegsys = createset(SYM_BEGIN, SYM_CALL, SYM_IF, SYM_WHILE, SYM_NULL);
	facbegsys = createset(SYM_IDENTIFIER, SYM_NUMBER, SYM_LPAREN, SYM_MINUS, SYM_NULL);

	err = cc = cx = ll = 0; // initialize global variables
	ch = ' ';
	kk = MAXIDLEN;

	getsym();

	set1 = createset(SYM_PERIOD, SYM_NULL);
	set2 = uniteset(declbegsys, statbegsys);
	set = uniteset(set1, set2);
	block(set);	// 从主程序进入，分析主程序体
	destroyset(set1);
	destroyset(set2);
	destroyset(set);
	destroyset(phi);
	destroyset(relset);
	destroyset(declbegsys);
	destroyset(statbegsys);
	destroyset(facbegsys);

	if (sym != SYM_PERIOD)	// 每个文件均以一个句号结束
		error(9); // '.' expected.
	if (err == 0)
	{
		hbin = fopen("hbin.txt", "w");
		for (i = 0; i < cx; i++)
			fwrite(&code[i], sizeof(instruction), 1, hbin);	// 将生成的汇编代码写入文件
		fclose(hbin);
	}
	if (err == 0)
		interpret();	// 生成汇编代码后，开始执行
	else
		printf("There are %d error(s) in PL/0 program.\n", err);
	listcode(0, cx);	// 打印生成的所有汇编代码
} // main

//////////////////////////////////////////////////////////////////////
// eof pl0.c
