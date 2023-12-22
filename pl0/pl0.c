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
void error(int n) {
    int i;

    printf("      ");
    for (i = 1; i <= cc - 1; i++)
        printf(" ");
    printf("^\n");
    printf("Error %3d: %s\n", n, err_msg[n]);
    err++;
} // error

//////////////////////////////////////////////////////////////////////
void getch(void) {
    if (cc == ll) {
        if (feof(infile)) {
            printf("\nPROGRAM INCOMPLETE\n");
            exit(1);
        }
        ll = cc = 0;
        printf("%5d  ", cx);
        while ((!feof(infile)) // added & modified by alex 01-02-09
               && ((ch = getc(infile)) != '\n')) {
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
void getsym(void) {
    int i, k;
    char a[MAXIDLEN + 1];
    // added & modified as labwork_1
    // an un-recursive method to eliminate repeated comments
    ////////////////////////////////////////////////
    // two kinds of comments supported
    // 1. line comment:     //...
    // 2. block comment:    /*...*/
    ////////////////////////////////////////////////
    do {
        while (ch == ' ' || ch == '\t')
            getch();

        if (ch == '/') {
            getch();
            if (ch == '/') {
                cc = ll = 0;
                ch = ' ';
            } else if (ch == '*') {
                do {
                    ch = ' ';
                    while (ch != '*')
                        getch();
                    getch();
                } while (ch != '/');
                ch = ' ';
            } else {
                sym = SYM_SLASH;
                return;
            }
        }
    } while (ch == ' ');

    cc_p = cc - 1; // e2-20
    sym_p = sym;
    if (isalpha(ch) || ch == '_') // e2-20
    {                              // symbol is a reserved word or an identifier.
        k = 0;
        do {
            if (k < MAXIDLEN)
                a[k++] = ch;
            getch();
        } while (isalpha(ch) || isdigit(ch) || ch == '_');
        a[k] = 0;
        strcpy(id, a);
        word[0] = id;
        i = NRW;
        while (strcmp(id, word[i--]));
        if (++i)
            sym = wsym[i]; // symbol is a reserved word
        else
            sym = SYM_IDENTIFIER; // symbol is an identifier
    } else if (isdigit(ch)) { // symbol is a number.
        k = num = 0;
        sym = SYM_NUMBER;
        do {
            num = num * 10 + ch - '0';
            k++;
            getch();
        } while (isdigit(ch));
        if (k > MAXNUMLEN)
            error(25); // The number is too great.
    } else if (ch == ':') {
        getch();
        if (ch == '=') {
            sym = SYM_BECOMES; // :=
            getch();
        } else {
            sym = SYM_COLON; //:                  												// e1
        }
    } else if (ch == '>') {
        getch();
        if (ch == '=') {
            sym = SYM_GEQ; // >=
            getch();
        } else {
            sym = SYM_GTR; // >
        }
    } else if (ch == '<') {
        getch();
        if (ch == '=') {
            sym = SYM_LEQ; // <=
            getch();
        } else if (ch == '>') {
            sym = SYM_NEQ; // <>
            getch();
        } else {
            sym = SYM_LES; // <
        }
    } else { // other tokens
        i = NSYM;
        csym[0] = ch;
        while (csym[i--] != ch);
        if (++i) {
            sym = ssym[i];
            getch();
        } else {
            printf("Fatal Error: Unknown character.\n");
            exit(1);
        }
    }
} // getsym

//////////////////////////////////////////////////////////////////////
// generates (assembles) an instruction.
void gen(int x, int y, int z) {
    if (cx > CXMAX) {
        printf("Fatal Error: Program too long.\n");
        exit(1);
    }
    code[cx].f = x;
    code[cx].l = y;
    code[cx++].a = z;
} // gen

//////////////////////////////////////////////////////////////////////
// tests if error occurs and skips all symbols that do not belongs to s1 or s2. 遇到错误输入时，跳过后面部分输入，直到下一句正常代码开始
void test(symset s1, symset s2, int n) {
    symset s;

    if (!inset(sym, s1)) {
        error(n);
        s = uniteset(s1, s2);
        while (!inset(sym, s))
            getsym();
        destroyset(s);
    }
} // test

//////////////////////////////////////////////////////////////////////
int dx;  // data allocation index

void free_ptr(comtyp *ptr) // e1
{
    if (ptr) {
        while ((ptr->k)--) {
            free_ptr(ptr[ptr->k + 1].ptr);
        }
        if (ptr->ptr) {
            free(ptr->ptr);
        }
        free(ptr);
    }
}

int expression(symset ssys, int CONST_CHECK);

// enter object(constant, variable or procedre) into table.
void enter(int kind) {
    int position(char *id, int tx_beg);

    if (kind == ID_PROCEDURE && position(id, TABLE_BEGIN) || // e1-20
        kind != ID_PROCEDURE && position(id, tx_b)) {
        error(31); // Redeclaration of an identifier.
        return;
    }

    mask *mk;

    tx++;
    strcpy(table[tx].name, id);
    table[tx].kind = kind;
    free_ptr(table[tx].ptr);
    switch (kind) {
        case ID_CONSTANT:
            if (num > MAXADDRESS) {
                error(25); // The number is too great.
                num = 0;
            }
            table[tx].value = num;
            break;
        case ID_VARIABLE:
        case ID_POINTER: // e1
            mk = (mask *) &table[tx];
            mk->level = level;
            mk->address = dx++;
            mk->ptr = ptr; // if VARIABLE / POINTER TO VARIABLE, ptr == NULL
            ptr = 0;
            break;
        case ID_PROCEDURE:
            mk = (mask *) &table[tx];
            mk->level = level;
            break;
        case ID_ARRAY: // e1
            mk = (mask *) &table[tx];
            mk->level = level;
            mk->address = dx;
            int i = ptr[0].k, s = ptr[1].k;
            while (--i) {
                s *= ptr[ptr[0].k - i + 1].k;
            }
            dx += s;
            mk->ptr = ptr;
            ptr = 0;
            break;
        case ID_LABEL:              // e1
            table[tx].value = cx; // label pointing to an ins
            break;
    }// switch
} // enter

//////////////////////////////////////////////////////////////////////
int createarray(symset ssys) // e2-16
{
    int con_expr;
    symset set;

    char id_t[MAXIDLEN + 1];
    strcpy(id_t, id);
    int dim[MAXARYDIM + 1] = {};
    do {
        getsym();
        set = expandset(ssys, SYM_RSQUARE, SYM_NULL);
        con_expr = expression(set, CONST_EXPR);
        destroyset(set);
        if (con_expr <= 0 || con_expr > MAXARYVOL) {
            error(36); // Volume of a dimension is out of range.
            break;
        }
        if ((++dim[0]) > MAXARYDIM) {
            error(35); // There are too many dimensions.
            --dim[0];
            break;
        }
        dim[dim[0]] = con_expr;
        if (sym != SYM_RSQUARE) {
            error(34); // ']' expected.				// if ']' lost, go finding the next '['
        } else {
            getsym();
        }
    } while (sym == SYM_LSQUARE);
    if (dim[0]) // e1
    {
        ptr = (comtyp *) malloc((dim[0] + 1) * sizeof(comtyp));
        ptr[0].k = dim[0];
        ptr[0].ptr = NULL;
        int i = dim[0] + 1;
        while (--i) // e1
        {
            ptr[i].k = dim[i];
            ptr[i].ptr = NULL;
        }
        strcpy(id, id_t);
    }
    return dim[0] ? TRUE : FALSE;
}

//////////////////////////////////////////////////////////////////////
int countindex(int i, symset ssys) {
    symset set;

    ptr = table[i].ptr;
    int d = ptr->k; // d <- dimensions of the array
    set = expandset(ssys, SYM_RSQUARE, SYM_NULL);
    expression(set, UNCONST_EXPR);
    if (sym != SYM_RSQUARE) {
        error(34); // ']' expected.
    } else {
        getsym();
    }
    --d;
    while (sym == SYM_LSQUARE && d) {
        getsym(); // take '['
        gen(LIT, 0, ptr[ptr[0].k - d + 1].k);
        gen(OPR, 0, OPR_MUL);
        expression(set, UNCONST_EXPR);
        gen(OPR, 0, OPR_ADD);
        if (sym != SYM_RSQUARE) {
            error(34); // ']' expected.
        } else {
            getsym();
        }
        --d;
    }
    destroyset(set);
    return d ? FALSE : TRUE;
}

//////////////////////////////////////////////////////////////////////
int getarrayaddr(int i, symset ssys) {
    mask *mk = (mask *) &table[i];
    if (sym != SYM_LSQUARE) {
        error(38); // '[' expected
        mk = 0;
    } else {
        getsym();
        if (countindex(i, ssys)) // e1
        {                         // number of subscripts read == array dimensions
            gen(LIT, 0, sizeof(int));
            gen(OPR, 0, OPR_MUL);
        } else {               // number of subscripts read < array dimensions
            error(29); // Too few subscripts.
            mk = 0;
        }
    }
    if (!mk) {                          // discard the leftover parts of the subscripts
        test(ssys, ssys, 19); // Incorrect symbol.
    } else if (sym == SYM_LSQUARE) // e1-15
    {
        test(ssys, ssys, 30); // Too many subscripts.
    }
    return mk ? TRUE : FALSE;
}

//////////////////////////////////////////////////////////////////////
int assignment(int i, symset ssys) {
    symset set, set1;

    int CORRECT_ASSIGN = TRUE;
    set = createset(SYM_BECOMES, SYM_NULL); // e1
    set1 = uniteset(ssys, set);
    if (table[i].kind == ID_CONSTANT) {
        error(12); // Illegal assignment.
        CORRECT_ASSIGN = FALSE;
    } else if (table[i].kind == ID_VARIABLE) {      // variable assignment
        ; // do nothing
    } else if (table[i].kind == ID_POINTER) // e1
    {                                      // indirect assignment (temporally only in procedure)
        mask *mk = (mask *) &table[i];
        gen(LOD, level - mk->level, mk->address);
        if (table[i].ptr) { // array_pointer
            CORRECT_ASSIGN = getarrayaddr(i, set1);
            gen(OPR, 0, OPR_ADD);
        }
    } else if (table[i].kind == ID_ARRAY) { // array assignment
        mask *mk = (mask *) &table[i];
        gen(LIT, 0, mk->address);
        CORRECT_ASSIGN = getarrayaddr(i, set1);
        gen(OPR, 0, OPR_ADD);
    } // if

    if (sym == SYM_LSQUARE) {               // Non-Array / Too many subscripts in array
        error(27); // Applying the index operator on non-array.
        getsym();
        test(set, set1, 19); // Incorrect symbol.
    } else if (!CORRECT_ASSIGN) {                         // discard the leftover of the errored subscripts
        test(set, set1, 19); // Incorrect symbol.
    }
    destroyset(set);
    destroyset(set1);
    if (sym == SYM_BECOMES) {
        getsym();
    } else {
        error(13); // ':=' expected.
    }
    expression(ssys, UNCONST_EXPR); // e1
    mask *mk = (mask *) &table[i];
    if (CORRECT_ASSIGN) {
        if (table[i].kind == ID_VARIABLE) {
            gen(STO, level - mk->level, mk->address);
        } else if (table[i].kind == ID_POINTER) // e1
        {
            gen(STOS, 0, 0);
        } else // ID_ARRAY
        {
            gen(STOI, level - mk->level, 0);
        }
    }
    return CORRECT_ASSIGN;
}

//////////////////////////////////////////////////////////////////////
// locates identifier in symbol table. 返回符号在符号表中的位置，然后通过table[i]找到对应的符号
int position(char *id, int tx_beg) // e1
{
    int i;
    i = tx + 1;
    if (tx_beg) {
        strcpy(table[0].name, table[tx_beg].name);
    }
    strcpy(table[tx_beg].name, id);
    while (strcmp(table[--i].name, id) != 0);
    if (tx_beg) {
        strcpy(table[tx_beg].name, table[0].name);
    }
    return tx_beg == i ? i - tx_beg : i;
} // position

//////////////////////////////////////////////////////////////////////
// 已经读完const和标识符，接下来读取等号和数字，然后将常量加入符号表，调用完成后，sym指向下一个标识符
void constdeclaration(symset fsys) {
    if (sym == SYM_IDENTIFIER) {
        getsym();
        if (sym == SYM_EQU || sym == SYM_BECOMES) {
            if (sym == SYM_BECOMES)
                error(1); // Found ':=' when expecting '='.
            getsym();
            num = expression(fsys, CONST_EXPR);
            ptr = NULL; // e1
            enter(ID_CONSTANT);
        } else {
            error(3); // There must be an '=' to follow the identifier.
        }
    } else {
        error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
    }
    // There must be an identifier to follow 'const', 'var', or 'procedure'.
} // constdeclaration

//////////////////////////////////////////////////////////////////////
// 已经读完var和标识符，接下来将变量加入符号表，调用完成后，sym指向下一个标识符
void vardeclaration(symset ssys) {
    if (sym == SYM_IDENTIFIER) {
        getsym();
        if (sym == SYM_LSQUARE) {                           // array declaration
            if (createarray(ssys)) // e1
            {
                enter(ID_ARRAY);
            }
        } else {                // variable declaration
            ptr = NULL; // e2-15
            enter(ID_VARIABLE);
        }
    } else {
        error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
    }
} // vardeclaration

//////////////////////////////////////////////////////////////////////
void subprocdeclaration(symset ssys) // e2-16
{
    symset set, set1;

    if (sym == SYM_LPAREN) {
        getsym();
        int n = 0;
        comtyp pmt[MAXFUNPMT + 1] = {};
        set = createset(SYM_COMMA, SYM_RPAREN, SYM_NULL); // e1
        set1 = uniteset_mul(ssys, set, pmt_first_sys, 0);
        while (inset(sym, pmt_first_sys)) // e1
        {
            if ((++n) > MAXFUNPMT) {
                error(39); // Too many parameters in a procedure.
                --n;
                break;
            }
            if (sym == SYM_AMPERSAND) { // argument passing by address_variable
                getsym();
                if (sym != SYM_IDENTIFIER) {
                    error(15);                                   // There must be an identifier to follow '&'.
                    --n;                                       // e2-15
                    deleteset(set1, SYM_IDENTIFIER,
                              SYM_NULL); // if there's syms between '&' and id, this is not a correct
                    test(set, set1,
                         40);                       // Missing ',' or ')'.		// declaration, and this id should be jumped off
                    setinsert(set1, SYM_IDENTIFIER);
                } else {
                    pmt[n].k = ID_POINTER;
                    pmt[n].ptr = NULL;
                    getsym();
                }
            } else if (sym == SYM_IDENTIFIER) // e1
            {                                // argument passing by value / (address_array)
                getsym();
                if (sym == SYM_LSQUARE) { // array declaration
                    if (createarray(set1)) {
                        pmt[n].k = ID_POINTER;
                        pmt[n].ptr = ptr;
                        ptr = NULL;
                    }
                } else if (inset(sym, set)) { // variable declaration
                    pmt[n].k = ID_VARIABLE;
                    pmt[n].ptr = NULL;
                } else if (sym == SYM_LPAREN) // e1
                {
                    error(61); // 'void' or 'int' needed before a procedure declared.
                }
                if (pmt[n].k == ID_VOID) // did not set the value of pmt[n]
                {
                    --n;
                }
            } else // sym == SYM_VOID / SYM_INT													// e2-18
            {     // sub-procedure declaration
                int rt = sym == SYM_VOID ? ID_VOID : ID_VARIABLE;
                getsym();
                if (sym != SYM_IDENTIFIER) {
                    error(62); // There must be an identifier to follow 'void' or 'int'.
                } else {
                    getsym();
                }
                pmt[n].k = ID_PROCEDURE;
                subprocdeclaration(set1);
                ptr->ptr[1].k = rt;
                pmt[n].ptr = ptr;
                ptr = NULL;
            }

            // clear the leftovers
            test(set, set1, 19); // Incorrect symbol.

            if (sym == SYM_COMMA) {
                getsym();
                // ensure that when this iteration ends, next sym is correct
                test(pmt_first_sys, set1, 19); // Incorrect symbol.
            } else if (sym == SYM_RPAREN) {
                break;
            }
        }                 // while
        destroyset(set); // e1
        destroyset(set1);
        if (sym == SYM_RPAREN) {
            getsym();
        }
        ptr = (comtyp *) malloc((n + 1) * sizeof(comtyp));
        ptr->ptr = (comtyp *) malloc(3 * sizeof(comtyp));
        ptr->ptr[0].k = PMT_PROC;
        ptr->k = n;
        int sum_alloc = 0; // e2-18
        while (n--) {
            ptr[n + 1].k = pmt[n + 1].k;
            ptr[n + 1].ptr = pmt[n + 1].ptr;
            sum_alloc += pmt[n + 1].k == ID_PROCEDURE ? 2 : 1;
        }
        ptr->ptr[2].k = sum_alloc;
    } else // procedure without parameters														// e2-18
    {
        ptr = (comtyp *) malloc(sizeof(comtyp)); // e1
        ptr->ptr = (comtyp *) malloc(3 * sizeof(comtyp));
        ptr->ptr[0].k = PMT_PROC;
        ptr->ptr[2].k = 0;
        ptr[0].k = 0;
    }
}

//////////////////////////////////////////////////////////////////////
// 打印指定范围内的汇编代码
void listcode(int from, int to) {
    int i;

    printf("\n");
    for (i = from; i < to; i++) {
        printf("%5d %s\t%d\t%d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
    }
    printf("\n");
} // listcode

//////////////////////////////////////////////////////////////////////
void backpatch(int *list) {
    int *p = list + 1;
    while (list[0]--) {
        code[*p++].a = cx;
    }
    free(list);
}


//////////////////////////////////////////////////////////////////////
int isSameCompType(comtyp *ptr1, comtyp *ptr2) {
    if (ptr1 && ptr2) {
        if (ptr1[0].k != ptr2[0].k) {
            return FALSE;
        } else {
            comtyp *p1 = ptr1->ptr; // e1
            comtyp *p2 = ptr2->ptr;
            if (p1 && p2) {
                if (p1[1].k != p2[1].k) {
                    return FALSE; // not have the same retval type
                }
            } else if (p1 || p2) {
                return FALSE;
            }
            int n = ptr1[0].k;
            while (n--) {
                if (ptr1[n + 1].k != ptr2[n + 1].k ||
                    !isSameCompType(ptr1[n + 1].ptr, ptr2[n + 1].ptr)) {
                    return FALSE;
                }
            }
            return TRUE;
        }
    } else if (!ptr1 && !ptr2) {
        return TRUE;
    } else {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////
int condition(symset ssys, int CONST_CHECK) {
    int rv = 0; // e1-26
    int relop;
    symset set;

    if (sym == SYM_ODD) {
        getsym();
        expression(ssys, UNCONST_EXPR); // e1
        gen(OPR, 0, OPR_ODD);
    } else { // expression
        set = uniteset(relset, ssys);
        rv = expression(set, CONST_CHECK); // e1
        destroyset(set);
        if (inset(sym, relset)) // e1
        {
            if (!CONST_CHECK) // e-26
            {                  // CONST_EXPR
                error(20);      // Relop not admitted in const expression.
                return 0;
            }
            relop = sym;
            getsym();
            expression(ssys, UNCONST_EXPR); // e1
            switch (relop) {
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
        }      // e1
    }          // else
    return rv;
} // condition

//////////////////////////////////////////////////////////////////////
// this function must be called after the id is confirmed as a procedure
int callprocedure(int i, symset ssys) {
    symset set, set1;

    mask *mk = (mask *) &table[i];
    comtyp *p = table[i].ptr;
    int n = p->k;
    if (p->ptr[1].k == ID_VARIABLE) // retval placed at base - p->ptr[2].k - 1
    {                                // retval type is var
        gen(INT, 0, 1);
    }
    if (sym == SYM_LPAREN) {
        getsym();
        if (n) {
            if (sym == SYM_RPAREN) {
                error(42); // Too few parameters in a procedure.
            } else {
                set1 = uniteset(ssys, exp_first_sys);
                setinsert_mul(set1, SYM_COMMA, SYM_RPAREN, SYM_NULL);
                test(exp_first_sys, set1, 24); // The symbol can not be as the beginning of an expression.
                destroyset(set1);
            }
        }
        while (inset(sym, exp_first_sys) && n--) {
            int kind = p[p->k - n].k;
            comtyp *pmt_ptr = p[p->k - n].ptr;
            set = createset(SYM_COMMA, SYM_RPAREN, SYM_NULL);
            set1 = uniteset_mul(ssys, set, exp_first_sys, 0);
            if (kind == ID_VARIABLE) {
                expression(set1, UNCONST_EXPR);
            } else if (kind == ID_POINTER) {
                if (sym == SYM_IDENTIFIER) {
                    if ((i = position(id, TABLE_BEGIN)) == 0) {
                        error(11); // Undeclared identifier.
                        getsym();
                    } else {
                        getsym();
                        mask *mk = (mask *) &table[i];
                        if (pmt_ptr) { // array_pointer
                            if (mk->kind == ID_ARRAY && isSameCompType(mk->ptr, pmt_ptr)) {
                                gen(LEA, level - mk->level, mk->address);
                            } else if (mk->kind == ID_POINTER && isSameCompType(mk->ptr, pmt_ptr)) {
                                gen(LOD, level - mk->level, mk->address);
                            } else {
                                error(41); // Non-array type/incorrect indices.
                            }
                        } else { // variable pointer
                            if (mk->kind == ID_VARIABLE || mk->kind == ID_ARRAY) {
                                gen(LEA, level - mk->level, mk->address);
                            } else if (mk->kind == ID_POINTER) {
                                gen(LOD, level - mk->level, mk->address);
                            } else {
                                error(54); // Incorrect type as an lvalue expression.
                            }
                            if (mk->kind == ID_ARRAY ||
                                (mk->kind == ID_POINTER && mk->ptr)) { // pointer <- array[i][j]...
                                getarrayaddr(i, set1);
                                gen(OPR, 0, OPR_ADD);
                            }
                        }
                    } // if
                } else {
                    error(53);             // The symbol can not be as the beginning of an lvalue expression.
                    test(set, set1, 19); // Incorrect symbol;
                }
                if (pmt_ptr && sym == SYM_LSQUARE) {
                    error(69);             // Assigning non-array type to an array.
                    test(set, set1, 19); // Incorrect symbol;
                }
            } else if (kind == ID_PROCEDURE) {
                if (sym == SYM_IDENTIFIER) {
                    if ((i = position(id, TABLE_BEGIN)) == 0) {
                        error(11); // Undeclared identifier.
                        getsym();
                    } else {
                        getsym();
                        mask *mk = (mask *) &table[i];
                        if (mk->kind == ID_PROCEDURE && isSameCompType(mk->ptr, pmt_ptr)) {
                            if (mk->ptr->ptr[0].k == NON_PMT_PROC) {
                                gen(LEA, level - mk->level, 0); // LIT the StaticLink
                                gen(LIT, 0, mk->address);        // LIT cx
                            } else // PMT_PROC
                            {
                                gen(LOD, level - mk->level, mk->address - 1); // LIT the StaticLink
                                gen(LOD, level - mk->level, mk->address);      // LIT cx
                            }
                        } else {
                            error(56); // Non-procedure type/incorrect parameter types.
                        }
                    } // if
                } else {
                    error(55);             // The symbol can not be as the beginning of a function call.
                    test(set, set1, 19); // Incorrect symbol;
                }
                if (sym == SYM_LPAREN) {
                    error(70);             // Assigning non-function type to a function.
                    test(set, set1, 19); // Incorrect symbol;
                }
            } // if
            destroyset(set);
            destroyset(set1);
            if (sym == SYM_COMMA) {
                getsym();
                set1 = uniteset(ssys, exp_first_sys);
                setinsert(set1, SYM_RPAREN);
                test(exp_first_sys, set1, 24); // The symbol can not be as the beginning of an expression.
                destroyset(set1);
            } else if (sym == SYM_RPAREN) {
                if (n) {
                    error(42); // Too few parameters in a procedure.
                }
                break;
            } else {
                error(40); // Missing ',' or ')'.
            }
        } // while
        if (inset(sym, exp_first_sys)) {
            error(39); // Too many parameters in a procedure.
        }
        if (sym == SYM_RPAREN) {
            getsym();
        }
    } // if
    if (!n) {
        if (p->ptr[0].k == PMT_PROC) {
            gen(CALS, level - mk->level, mk->address);
        } else { // NON_PMT_PROC
            gen(CAL, level - mk->level, mk->address);
        }
    }
    return n ? FALSE : TRUE;
}

int and_condition(symset ssys, int CONST_CHECK) {
    int rv;                  // e1
    int *t_list, *f_list; // e1
    symset set;

    set = expandset(ssys, SYM_AND, SYM_NULL);
    rv = condition(set, CONST_CHECK);
    f_list = (int *) malloc((CXMAX + 1) * sizeof(int));
    f_list[0] = 0;
    if (sym == SYM_AND && !CONST_CHECK) // e1-26
    {                                    // CONST_EXPR
        error(20);                        // Relop not admitted in const expression.
        test(ssys, ssys, 19);            // Incorrect symbol.
        rv = 0;
    }
    while (sym == SYM_AND && CONST_CHECK) {
        f_list[++f_list[0]] = cx;
        gen(JND, 0, 0); // if stack[top] == 0, jump the other and_condition
        getsym();
        condition(set, UNCONST_EXPR);
        gen(OPR, 0, OPR_AND);
    }
    destroyset(set);
    list[0] = f_list;
    return rv;
}

//////////////////////////////////////////////////////////////////////
int or_condition(symset ssys, int CONST_CHECK) {
    int rv;         // e1
    int *t_list; // e1
    symset set;

    set = expandset(ssys, SYM_OR, SYM_NULL);
    rv = and_condition(set, CONST_CHECK);
    t_list = (int *) malloc((CXMAX + 1) * sizeof(int));
    t_list[0] = 0;
    if (sym == SYM_OR && !CONST_CHECK) // e-26
    {                                   // CONST_EXPR
        error(20);                       // Relop not admitted in const expression.
        test(ssys, ssys, 19);           // Incorrect symbol.
        rv = 0;
    }
    while (sym == SYM_OR && CONST_CHECK) {
        t_list[++t_list[0]] = cx;
        gen(JNDN, 0, 0); // if stack[top] != 0, jump the other and_condition
        backpatch(list[0]);
        getsym();
        and_condition(set, UNCONST_EXPR);
        gen(OPR, 0, OPR_OR);
    }
    destroyset(set);
    list[1] = t_list;
    return rv;
}

//////////////////////////////////////////////////////////////////////

/*
	PL/0 的编译程序为每一条 PL/0 源程序的可执行语句生成后缀式目标代码。
*/
int factor(symset ssys, int CONST_CHECK) {
    int i, rv = 0;
    symset set;

    test(fac_first_sys, ssys, 24); // The symbol can not be as the beginning of an expression.
    // "factor" is unfamiliar for the user, thus "expression" used.
    if (inset(sym, fac_first_sys)) // e1
    {
        if (sym == SYM_IDENTIFIER) {
            if ((i = position(id, TABLE_BEGIN)) == 0) {
                error(11); // Undeclared identifier.
                getsym();
            } else {
                mask *mk = (mask *) &table[i]; // e1
                getsym();                      // e1
                switch (table[i].kind)          // e1
                {
                    case ID_CONSTANT:
                        if (CONST_CHECK) { // UNCONST_EXPR
                            gen(LIT, 0, table[i].value);
                        } else { // CONST_EXPR
                            rv = table[i].value;
                        }
                        break;
                    case ID_VARIABLE:
                        if (CONST_CHECK) { // UNCONST_EXPR
                            gen(LOD, level - mk->level, mk->address);
                        } else {               // CONST_EXPR
                            error(28); // Variables can not be in a const expression.
                        }
                        break;
                    case ID_POINTER: // e1
                        if (CONST_CHECK) { // UNCONST_EXPR
                            if (mk->ptr) { // array_pointer
                                gen(LOD, level - mk->level, mk->address);
                                getarrayaddr(i, ssys);
                                gen(OPR, 0, OPR_ADD);
                            } else { // variable_pointer
                                gen(LOD, level - mk->level, mk->address);
                            }
                            gen(LODS, 0, 0);
                        } else {                          // CONST_EXPR
                            error(28);              // Variables can not be in a const expression.
                            test(ssys, ssys, 19); // Incorrect symbol;
                        }
                        break;
                    case ID_PROCEDURE:
                        if (CONST_CHECK) {
                            if (mk->ptr->ptr[1].k != ID_VOID) {
                                callprocedure(i, ssys);
                            } else {
                                error(63); // Void value not ignored as it ought to be.
                            }
                        } else {
                            error(60);              // Procedure can not be in const expression.
                            test(ssys, ssys, 19); // Incorrect symbol;
                        }
                        break;
                    case ID_ARRAY:         // e1-15
                        if (CONST_CHECK) // e1
                        {                 // UNCONST_EXPR
                            gen(LIT, 0, mk->address);
                            getarrayaddr(i, ssys);
                            gen(OPR, 0, OPR_ADD);
                            gen(LODI, level - mk->level, 0);
                        } else {                          // CONST_EXPR
                            error(28);              // Variables can not be in a const expression.
                            test(ssys, ssys, 19); // Incorrect symbol;
                        }
                        break;
                } // switch
                if (sym == SYM_LSQUARE) {
                    error(27); // Applying the index operator on non-array.
                    getsym();
                } else if (sym == SYM_LPAREN) {
                    error(64); // Applying function calling operator on non-array.
                    getsym();
                }
                test(ssys, ssys, 23); // The symbol can not be followed by an expression.
            }                          // if
        } else if (sym == SYM_NUMBER) {
            if (num > MAXADDRESS) {
                error(25); // The number is too great.
                num = 0;
            }
            if (CONST_CHECK) { // UNCONST_EXPR
                gen(LIT, 0, num);
            } else { // CONST_EXPR
                rv = num;
            }
            getsym();
        } else if (sym == SYM_LPAREN) {
            getsym();
            set = expandset(ssys, SYM_RPAREN, SYM_NULL);
            rv = or_condition(set, CONST_CHECK); // e1
            destroyset(set);
            backpatch(list[0]); // e1-26
            backpatch(list[1]);
            list[0] = list[1] = NULL; // e1
            if (sym == SYM_RPAREN) {
                getsym();
            } else {
                error(22); // Missing ')'.
            }
        } else if (sym == SYM_MINUS) // UMINUS,  Expr -> '-' Expr
        {
            getsym();
            rv = factor(ssys, CONST_CHECK); // e1
            if (CONST_CHECK) { // UNCONST_EXPR
                gen(OPR, 0, OPR_NEG);
            } else { // CONST_EXPR
                rv = -rv;
            }
        } else if (sym == SYM_NOT) // e1-26
        {
            getsym();
            factor(ssys, UNCONST_EXPR);
            if (CONST_CHECK) { // UNCONST_EXPR
                gen(OPR, 0, OPR_NOT);
            } else {               // CONST_EXPR
                error(20); // Relop not admitted in const expression.
            }
        } else if (sym == SYM_TRUE) {
            getsym();
            if (CONST_CHECK) { // UNCONST_EXPR
                gen(LIT, 0, 1);
            } else { // CONST_EXPR
                rv = 1;
            }
        } else if (sym == SYM_FALSE) {
            getsym();
            if (CONST_CHECK) { // UNCONST_EXPR
                gen(LIT, 0, 0);
            } else { // CONST_EXPR
                rv = 0;
            }
        }
        test(ssys, ssys, 23); // The symbol can not be followed by an expression.
    }                          // if
    return rv;                  // e1-14
} // factor

//////////////////////////////////////////////////////////////////////
int term(symset ssys, int CONST_CHECK) {
    int mulop, r, rv = 0;
    symset set;

    set = expandset(ssys, SYM_TIMES, SYM_SLASH, SYM_NULL);

    rv = factor(set, CONST_CHECK); // e1
    while (sym == SYM_TIMES || sym == SYM_SLASH) {
        mulop = sym;
        getsym();
        r = factor(set, CONST_CHECK); // e1
        if (mulop == SYM_TIMES) {
            if (CONST_CHECK) { // UNCONST_EXPR
                gen(OPR, 0, OPR_MUL);
            } else { // CONST_EXPR
                rv *= r;
            }
        } else {
            if (CONST_CHECK) { // UNCONST_EXPR
                gen(OPR, 0, OPR_DIV);
            } else { // CONST_EXPR
                rv /= r;
            }
        }
    } // while
    destroyset(set);
    return rv;
} // term

//////////////////////////////////////////////////////////////////////
int expression(symset ssys, int CONST_CHECK) {
    int addop, r, rv = 0;
    symset set;

    set = expandset(ssys, SYM_PLUS, SYM_MINUS, SYM_NULL);

    rv = term(set, CONST_CHECK); // e1
    while (sym == SYM_PLUS || sym == SYM_MINUS) {
        addop = sym;
        getsym();
        r = term(set, CONST_CHECK); // e1
        if (addop == SYM_PLUS) {
            if (CONST_CHECK) { // UNCONST_EXPR
                gen(OPR, 0, OPR_ADD);
            } else { // CONST_EXPR
                rv += r;
            }
        } else {
            if (CONST_CHECK) { // UNCONST_EXPR
                gen(OPR, 0, OPR_MIN);
            } else { // CONST_EXPR
                rv -= r;
            }
        }
    } // while
    destroyset(set);
    return rv;
} // expression

//////////////////////////////////////////////////////////////////////
void retreat() // e2-20
{               // for LL(2) feature: lookahead another token and put it back afterwards
    cc = cc_p;
    getch();
    sym = sym_p;
}

//////////////////////////////////////////////////////////////////////
void statement(symset ssys) {
    int i, cx1, cx2, cx3, cx4;
    symset set1, set;

    if (sym == SYM_IDENTIFIER) // e
    {                           // LABEL compels the grammar to be LL(2)
        getsym();
        if (sym == SYM_COLON) { // label
            i = position(id, TABLE_BEGIN);
            if (i != 0) {
                error(71); // id can't be used as a label
            } else {
                enter(ID_LABEL);
            }
            getsym();
        } else { // not a label
            retreat();
        }
    } // if
    if (sym == SYM_IDENTIFIER) {
        getsym();
        if (!(i = position(id, TABLE_BEGIN))) {
            error(11); // Undeclared identifier.
        } else if (table[i].kind == ID_PROCEDURE) // e1
        {                                        // procedure call
            callprocedure(i, ssys);
        } else // e1
        {     // assignment
            assignment(i, ssys);
        } // if
    } else if (sym == SYM_IF) // e1
    {                        // if statement
        int cx_list[MAXELIF + 1];
        i = 0;
        getsym();
        set = uniteset(ssys, stat_first_sys);
        setinsert_mul(set, SYM_THEN, SYM_SEMICOLON, SYM_ELIF, SYM_ELSE, SYM_NULL);
        or_condition(set, UNCONST_EXPR);
        if (sym == SYM_THEN) {
            getsym();
        } else {
            error(16); // 'then' expected.
        }
        cx1 = cx;
        gen(JPC, 0, 0);
        statement(set); // e1
        destroyset(set);
        cx_list[i] = cx;
        gen(JMP, 0, 0);
        code[cx1].a = cx;
        if (sym == SYM_SEMICOLON) {
            getsym();
        } else if (sym == SYM_ELIF || sym == SYM_ELSE) {
            error(26); // Missing ';'.
        }
        if (sym != SYM_ELIF && sym != SYM_ELSE) {
            if (sym_p == SYM_SEMICOLON) {
                retreat();
            }
            code[cx_list[0]].a = cx;
            return;
        }
        int bRet = FALSE; // mark if retreating process is necessary
        while (sym == SYM_ELIF) {
            getsym();
            if ((++i) > MAXELIF) {
                error(67); // Too many elifs.
                --i;
                break;
            }
            set = uniteset(ssys, stat_first_sys);
            setinsert_mul(set, SYM_THEN, SYM_PERIOD, SYM_ELIF, SYM_ELSE, SYM_NULL);
            or_condition(set, UNCONST_EXPR);
            if (sym == SYM_THEN) {
                getsym();
            } else {
                error(16); // 'then' expected.
            }
            cx1 = cx;
            gen(JPC, 0, 0);
            statement(set);
            destroyset(set);
            cx_list[i] = cx;
            gen(JMP, 0, 0);
            code[cx1].a = cx;
            if (sym == SYM_SEMICOLON) {
                getsym();
                bRet = TRUE; // if sym != SYM_ELIF / SYM_ELSE, retreat
            } else if (sym == SYM_ELIF || sym == SYM_ELSE) {
                error(26); // Missing ';'.
            } else {
                break;
            }
        } // while
        if (sym == SYM_ELSE) {
            getsym();
            bRet = FALSE; // sym == SYM_ELSE, not retreat
            statement(ssys);
        }
        ++i;
        while (i--) {
            code[cx_list[i]].a = cx;
        }
        if (bRet) {
            retreat();
        }
    } else if (sym == SYM_BEGIN) { // block
        getsym();
        set = uniteset(ssys, stat_first_sys); // e1
        setinsert_mul(set, SYM_END, SYM_SEMICOLON, SYM_NULL);
        do {
            statement(set);
            if (sym == SYM_SEMICOLON) // e1
            {
                getsym();
            } else {
                error(10); // ';' expected.
            }
        } while (inset(sym, stat_first_sys));
        destroyset(set);
        if (sym == SYM_END) {
            getsym();
        } else {
            error(17); // 'end' expected.														// e1
        }
    } else if (sym == SYM_WHILE) {                             // while statement
        cltab[cltop].c = head;     // store the head when go in a new circulation
        cltab[cltop++].ty = env; // store environment
        env = ENV_WHILE;
        cx1 = cx;
        head = cx;
        getsym();
        set = uniteset(ssys, stat_first_sys);
        setinsert(set, SYM_DO);
        or_condition(set, UNCONST_EXPR); // e1
        destroyset(set);
        cx2 = cx;
        gen(JPC, 0, 0);
        if (sym == SYM_DO) {
            getsym();
        } else {
            error(18); // 'do' expected.
        }
        statement(ssys);
        gen(JMP, 0, cx1);
        code[cx2].a = cx;
        tail = cx;
        int i;
        for (i = cltop - count; i < cltop; i++) {
            code[cltab[i].c].a = tail; // backpatch break
        }
        cltop -= count;
        cltop--;
        count = 0;
        head = cltab[cltop].c; // regain head
    } else if (sym == SYM_FOR) {                           // for statement
        cltab[cltop].c = head; // e1
        cltab[cltop++].ty = env;
        env = ENV_FOR;
        getsym();
        if (sym != SYM_LPAREN) {
            error(43); // missing '('
        } else {
            getsym();
        }
        set = uniteset(ssys, stat_first_sys); // e1
        setinsert_mul(ssys, SYM_SEMICOLON, SYM_RPAREN, SYM_IDENTIFIER, SYM_NULL);
        if (sym == SYM_IDENTIFIER) {
            getsym();
            if (!(i = position(id, TABLE_BEGIN))) {
                error(11); // Undeclared identifier.
            } else if (table[i].kind == ID_PROCEDURE) {
                error(54); // Incorrect type as an lvalue expression.
            } else {
                assignment(i, set);
            }
        } else {
            error(44); // There must be a variable in 'for' statement.
        }
        set1 = createset(SYM_SEMICOLON, SYM_NULL);
        test(set1, set, 10); // ';' expected
        if (sym == SYM_SEMICOLON) {
            getsym();
        }
        cx1 = cx;
        // e1
        or_condition(set, UNCONST_EXPR); // condition
        destroyset(set);
        test(set1, set, 10); // ';' expected
        destroyset(set1);
        if (sym == SYM_SEMICOLON) {
            getsym();
        }
        cx2 = cx;
        gen(JPC, 0, 0);
        cx3 = cx;
        gen(JMP, 0, 0);
        cx4 = cx;
        head = cx;
        set = uniteset(ssys, stat_first_sys); // e1
        setinsert_mul(ssys, SYM_SEMICOLON, SYM_RPAREN, SYM_NULL);
        if (sym == SYM_IDENTIFIER) {
            getsym();
            if (!(i = position(id, TABLE_BEGIN))) {
                error(11); // Undeclared identifier.
            } else if (table[i].kind == ID_PROCEDURE) {
                error(54); // Incorrect type as an lvalue expression.
            } else {
                assignment(i, set);
            }
        } else {
            error(44); // There must be a variable in 'for' statement.
        }
        destroyset(set);
        set1 = createset(SYM_RPAREN, SYM_NULL);
        test(set1, set, 22); // Missing ')'.
        destroyset(set1);
        if (sym == SYM_RPAREN) {
            getsym();
        }
        gen(JMP, 0, cx1);
        code[cx3].a = cx;
        statement(ssys); // body of 'for'
        gen(JMP, 0, cx4);
        code[cx2].a = cx;
        tail = cx; // e1
        for (i = cltop - count; i < cltop; i++) {
            code[cltab[i].c].a = tail;
        }
        cltop = cltop - count - 1;
        count = 0;
        head = cltab[cltop].c;
        env = cltab[cltop].ty;
    } else if (sym == SYM_PRINT) // e2-21
    {
        getsym();
        int n = 0;
        if (sym == SYM_LPAREN) {
            getsym();
            set1 = uniteset(ssys, exp_first_sys);
            set = expandset(set1, SYM_RPAREN, SYM_COMMA, SYM_NULL);
            while (inset(sym, exp_first_sys)) {
                if ((++n) > MAXFUNPMT) {
                    error(39); // Too many parameters in a procedure.
                    --n;
                    break;
                }
                expression(set, UNCONST_EXPR);
                if (sym == SYM_COMMA) {
                    getsym();
                    test(exp_first_sys, set, 19); // Incorrect symbol;
                } else if (sym == SYM_RPAREN) {
                    break;
                } else {
                    error(40);             // Missing ',' or ')'.
                    test(set1, set, 19); // Incorrect symbol;
                }
            } // while
            destroyset(set);
            destroyset(set1);
            if (sym == SYM_RPAREN) {
                getsym();
            }
        } // if
        gen(PRINT, 0, n);
    } // if

    test(ssys, ssys, 19); // e1
} // statement

//////////////////////////////////////////////////////////////////////
// 分析程序体
void block(symset fsys) {
    int cx0; // initial code index
    mask *mk;
    int block_dx;    // 记录"本"过程体的数据区大小，即变量个数（常量直接定义，不需要分配）加上三个内部变量 RA，DL 和 SL
    int block_tx = tx_b;    // 记录"本"过程体的符号表起始位置
    int savedTx, savedDx;
    symset set1, set;

    // 开始编译一个过程时，要对数据单元的下标 dx 赋初值，表示新开辟一个数据区。
    dx = 3;    // 初值为 3，因为每个数据区包含三个内部变量 RA，DL 和 SL，即 return address，dynamic link 和 static link
    block_dx = dx;

    // 每个程序体入口处都生成一条JMP指令，用于在执行时跳过定义内容（因为总是先定义所有需要的变量，常量，过程，然后再定义执行语句），进入执行语句
    mk = (mask *) &table[tx_b];
    mk->address = cx;
    gen(JMP, 0, 0);
    if (level > MAXLEVEL) {
        error(32); // There are too many levels.
    }
    do {
        if (sym == SYM_CONST) // WARNING: other types of declaration precede the procedure!!! or tx_b will be reset!
        {                      // constant declarations																// e1
            getsym();
            do {
                set = uniteset(fsys, blk_first_sys);
                setinsert_mul(set, SYM_COMMA, SYM_SEMICOLON, SYM_IDENTIFIER, SYM_NULL);
                constdeclaration(set);
                while (sym == SYM_COMMA) {
                    getsym();
                    constdeclaration(set);
                }
                destroyset(set);
                if (sym == SYM_SEMICOLON) {
                    getsym();
                    break;
                } else {
                    error(5); // Missing ',' or ';'.
                }
            } while (sym == SYM_IDENTIFIER);
        } // if
        if (sym == SYM_VAR) { // variable declarations
            getsym();
            do // e1
            {
                set = uniteset(fsys, blk_first_sys);
                setinsert_mul(set, SYM_COMMA, SYM_SEMICOLON, SYM_IDENTIFIER, SYM_NULL);
                vardeclaration(set);
                while (sym == SYM_COMMA) {
                    getsym();
                    vardeclaration(set);
                }
                if (sym == SYM_SEMICOLON) {
                    getsym();
                    break;
                } else {
                    error(5); // Missing ',' or ';'.
                }
                destroyset(set);
            } while (sym == SYM_IDENTIFIER);
        }               // if
        block_dx = dx; // save dx before handling procedure call!
        while (sym == SYM_PROCEDURE) { // procedure declarations
            int PROC_CREATED = TRUE;
            getsym();
            int rt;
            if (sym == SYM_VOID || sym == SYM_INT) // e2-18
            {
                rt = sym == SYM_VOID ? ID_VOID : ID_VARIABLE;
                getsym();
            } else {
                error(61); // 'void' or 'int' needed before a procedure declared.
                rt = ID_VOID;
            }
            if (sym == SYM_IDENTIFIER) {
                enter(ID_PROCEDURE);
                getsym();
            } else {
                error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
                PROC_CREATED = FALSE;
            }
            level++;                     // e1
            savedTx = tx_b = tx;         // entry of the procedure created(if succeeded)
            comtyp *pmt_list, *cur_node; // e2-15
            cur_node = pmt_list = (comtyp *) malloc(sizeof(comtyp));
            pmt_list->ptr = NULL;
            if (sym == SYM_LPAREN) {
                getsym();
                savedDx = dx; // n parameters bound to dx -1-Sum(np),...,-1. The actual dx doesn't increase.
                int n = 0;
                set = createset(SYM_COMMA, SYM_RPAREN, SYM_NULL); // e1
                set1 = uniteset_mul(fsys, set, pmt_first_sys, blk_first_sys, 0);
                setinsert(set1, SYM_SEMICOLON);
                while (inset(sym, pmt_first_sys)) // e1
                {
                    if ((++n) > MAXFUNPMT) {
                        error(39); // Too many parameters in a procedure.
                        --n;
                        break;
                    }
                    if (sym == SYM_AMPERSAND) { // argument passing by address_variable
                        getsym();
                        if (sym != SYM_IDENTIFIER) {
                            error(15);                                   // There must be an identifier to follow '&'.
                            --n;                                       // e2-15
                            deleteset(set1, SYM_IDENTIFIER,
                                      SYM_NULL); // if there's syms between '&' and id, this is not a correct
                            test(set, set1,
                                 40);                       // Missing ',' or ')'.	// declaration, and this id should be jumped off
                            setinsert(set1, SYM_IDENTIFIER);
                        } else {
                            ptr = NULL; // to differentiate from array_pointer					// e1
                            enter(ID_POINTER);
                            getsym();
                        }
                    } else if (sym == SYM_IDENTIFIER) // e1
                    {                                // argument passing by value / (address_array)
                        int tx_p = tx;
                        getsym();
                        if (sym == SYM_LSQUARE) { // array declaration
                            if (createarray(set1)) {
                                enter(ID_POINTER);
                            }
                        } else if (inset(sym, set)) { // variable declaration
                            ptr = NULL;
                            enter(ID_VARIABLE);
                        } else if (sym == SYM_LPAREN) {
                            error(61); // 'void' or 'int' needed before a procedure declared.
                        }
                        if (tx_p == tx) {
                            --n;
                        }
                    } else // sym == SYM_VOID / SYM_INT											// e2-18
                    {     // sub-procedure declaration
                        int rt = sym == SYM_VOID ? ID_VOID : ID_VARIABLE;
                        getsym();
                        if (sym != SYM_IDENTIFIER) {
                            error(62); // There must be an identifier to follow 'void' or 'int'.
                        } else {
                            getsym();
                        }
                        enter(ID_PROCEDURE);
                        subprocdeclaration(set1);
                        ptr->ptr[1].k = rt;
                        table[tx].ptr = ptr;
                        ptr = NULL;
                    }

                    // clear the leftovers
                    test(set, set1, 19); // Incorrect symbol.

                    if (sym == SYM_COMMA) {
                        getsym();
                        // ensure that when this iteration ends, next sym is correct
                        test(pmt_first_sys, set1, 19); // Incorrect symbol.
                    } else if (sym == SYM_RPAREN) {
                        break;
                    }
                }                 // while
                destroyset(set); // e1
                destroyset(set1);
                if (sym == SYM_RPAREN) {
                    getsym();
                }
                dx = savedDx;
                if (PROC_CREATED) // e1
                {
                    int sum_alloc = 0;
                    ptr = table[tx_b].ptr = (comtyp *) malloc((n + 1) * sizeof(comtyp)); // e1
                    ptr->ptr = (comtyp *) malloc(3 * sizeof(comtyp));
                    ptr->ptr[0].k = NON_PMT_PROC;
                    ptr->ptr[1].k = rt;
                    ptr->k = n;
                    for (int i = 0;
                         i < n; ++i) { // allocate the parameters in reverse order and set ptr in forward order
                        mask *mk = (mask *) &table[tx - i];
                        mk->address =
                                -1 - sum_alloc; // every parameter's address is the last word of the storage occupied
                        ptr[n - i].k = mk->kind;      // e1
                        if (mk->ptr)                  // mk is a composite type
                        {
                            cur_node->ptr = (comtyp *) malloc(sizeof(comtyp));
                            cur_node = cur_node->ptr;
                            cur_node->k = tx -
                                          i; // before the entry, table[tx - i], released in Line 2064, set its ptr to NULL
                            cur_node->ptr = NULL;
                        }
                        ptr[n - i].ptr = mk->ptr; // e1
                        sum_alloc += (mk->kind == ID_PROCEDURE) ? 2 : 1;
                    }
                    ptr->ptr[2].k = sum_alloc;
                }
            } else if (PROC_CREATED) // procedure without parameters								// e1
            {
                ptr = table[tx_b].ptr = (comtyp *) malloc(sizeof(comtyp)); // e1
                ptr->ptr = (comtyp *) malloc(3 * sizeof(comtyp));
                ptr->ptr[0].k = NON_PMT_PROC;
                ptr->ptr[1].k = rt;
                ptr->ptr[2].k = 0;
                ptr[0].k = 0;
            }

            set1 = createset(SYM_SEMICOLON, SYM_NULL);
            set = uniteset_mul(fsys, set1, blk_first_sys, 0);
            test(set1, set, 26); // Missing ';'.
            if (sym == SYM_SEMICOLON) {
                getsym();
            }
            block(set); // e1
            destroyset(set1);
            destroyset(set);
            for (cur_node = pmt_list->ptr; cur_node; cur_node = cur_node->ptr) // e2-15
            {
                table[cur_node->k].ptr = NULL;
            }
            tx = savedTx; // all the codes of the procedure generated, release the entries in TABLE
            level--;

            if (sym == SYM_SEMICOLON) {
                getsym();
            } else {
                error(26); // Missing ';'.														// e1
            }
        }                 // while
        dx = block_dx;     // restore dx after handling procedure call!
        tx_b = block_tx; // restore tx_b to the beginning index of current block;				// e1
        set = uniteset(blk_first_sys, fsys);
        test(blk_first_sys, set, 7); // Declaration/Statement expected.							// e1
        destroyset(set);
    } while (inset(sym, decl_first_sys));
    // 一个过程总是先定义所有需要的常量、变量和过程，然后才是执行语句，上面都while循环定义完了，接下来分析执行语句

    code[mk->address].a = cx;    // 回填程序体第一条指令 JMP 的地址，即进入一个程序跳转到执行语句的开始处执行
    mk->address = cx;
    cx0 = cx;
    gen(INT, 0, block_dx);    // 生成INT指令，分配数据区（在数据栈中分配存贮空间）。注意每个过程需要分配的地址为局部变量个数加三个内部变量 RA，DL 和 SL
    statement(fsys);           // e1
    if (code[cx - 1].f != RET) // e1
    {
        if (tx_b && table[tx_b].ptr->ptr[1].k != ID_VOID) {
            error(66); // Return void in function returning a value.
        } else if (!tx_b) { // main block
            gen(RET, 0, -1);
        } else { // procedure block
            gen(RET, 0, -1 - table[tx_b].ptr->ptr[2].k);
        }
    }
    test(fsys, fsys, 8); // test for error: Follow the statement is an incorrect symbol.
    listcode(cx0, cx);    // 每次分析完一个过程体，打印生成的需要执行的汇编代码
} // block

//////////////////////////////////////////////////////////////////////
int base(int stack[], int currentLevel, int levelDiff) {
    int b = currentLevel;

    while (levelDiff--)
        b = stack[b];
    return b;
} // base

//////////////////////////////////////////////////////////////////////
// interprets and executes codes.
void interpret() {
    int pc; // program counter
    int stack[STACKSIZE];
    int top;       // top of stack
    int b;           // program, base, and top-stack register
    instruction i; // instruction register

    printf("Begin executing PL/0 program.\n");

    pc = 0;
    b = 1;
    top = 0; // e1
    stack[1] = stack[2] = stack[3] = 0;
    do {
        i = code[pc++];
        switch (i.f) {
            case LIT:
                stack[++top] = i.a;
                break;
            case OPR:
                switch (i.a) // operator
                {
                    case OPR_RET:                                                                        // e1
                        top = b - 1;
                        pc = stack[top + 3];
                        b = stack[top + 2];
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
                        if (stack[top + 1] == 0) {
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
                        break; // e1-26
                    case OPR_LES:
                        top--;
                        stack[top] = stack[top] < stack[top + 1];
                        break;
                    case OPR_GEQ:
                        top--;
                        stack[top] = stack[top] >= stack[top + 1];
                        break; // e1-26
                    case OPR_GTR:
                        top--;
                        stack[top] = stack[top] > stack[top + 1];
                        break;
                    case OPR_LEQ:
                        top--;
                        stack[top] = stack[top] <= stack[top + 1];
                        break;      // e1-26
                    case OPR_NOT: // e1-26
                        stack[top] = !stack[top];
                        break;
                    case OPR_AND: // e1-26
                        top--;
                        stack[top] = stack[top] && stack[top + 1];
                        break;
                    case OPR_OR: // e1-26
                        top--;
                        stack[top] = stack[top] || stack[top + 1];
                        break;
                    case OPR_ITOB: // e2-19
                        stack[top] = stack[top] ? TRUE : FALSE;
                } // switch
                break;
            case LOD:
                stack[++top] = stack[base(stack, b, i.l) + i.a];
                break;
            case LODI:
                stack[top] = stack[base(stack, b, i.l) + stack[top]];
                break;
            case LODS: // e1
                stack[top] = stack[stack[top]];
                break;
            case LEA: // e1-26
                stack[++top] = base(stack, b, i.l) + i.a;
                break;
            case STO:
                stack[base(stack, b, i.l) + i.a] = stack[top];
                // printf("%d\n", stack[top]);
                top--;
                break;
            case STOI:
                stack[base(stack, b, i.l) + stack[top - 1]] = stack[top];
                // printf("%d\n", stack[top]);
                top -= 2;
                break;
            case STOS: // e1
                stack[stack[top - 1]] = stack[top];
                // printf("%d\n", stack[top]);
                top -= 2;
                break;
            case CAL:
                stack[top + 1] = base(stack, b, i.l);
                // generate new block mark
                stack[top + 2] = b;
                stack[top + 3] = pc;
                b = top + 1;
                pc = i.a;
                break;
            case CALS:
                stack[top + 1] = stack[base(stack, b, i.l) + i.a - 1];
                // generate new block mark
                stack[top + 2] = b;
                stack[top + 3] = pc;
                pc = stack[base(stack, b, i.l) + i.a];
                b = top + 1;
                break;
            case INT:
                top += i.a;
                break;
            case JMP:
                pc = i.a;
                break;
            case JPC: // e1
                if (!stack[top])
                    pc = i.a;
                top--;
                break;
            case JND: // e1
                if (!stack[top])
                    pc = i.a;
                break;
            case JNDN: // e1
                if (stack[top])
                    pc = i.a;
                break;
            case RET:
                top = b + i.a; // e1
                pc = stack[b + 2];
                b = stack[b + 1];
                break;
            case EXT:
                pc = cx; // eion jump to the last one
                break;
            case JET: // e,if the top equals to i.l,jump to i.a
                if (stack[top] == i.l) {
                    pc = i.a;
                }
                break;
            case PRINT: // e2-21
                if (i.a) {
                    top -= i.a;
                    for (int j = 1; j <= i.a; ++j) {
                        printf("%d\n", stack[top + j]);
                    }
                } else {
                    printf("\n");
                }
                break;
        } // switch
    } while (pc);

    printf("End executing PL/0 program.\n");
} // interpret

//////////////////////////////////////////////////////////////////////
void main() {
    FILE *hbin;
    char s[80];
    int i;

    printf("Please input source file name: "); // get file name to be compiled
    scanf("%s", s);
    if ((infile = fopen(s, "r")) == NULL) {
        printf("File %s can't be opened.\n", s);
        exit(1);
    }

    // create First/Synchronizing symbol sets													// e1
    phi = createset(SYM_NULL);
    relset = createset(SYM_EQU, SYM_NEQ, SYM_LES, SYM_LEQ, SYM_GTR, SYM_GEQ, SYM_NULL);

    decl_first_sys = createset(SYM_CONST, SYM_VAR, SYM_PROCEDURE, SYM_NULL);
    stat_first_sys = createset(SYM_IDENTIFIER, SYM_BEGIN, SYM_IF, SYM_WHILE, SYM_FOR,// e1
                               SYM_DO, SYM_PRINT, SYM_NULL);
    blk_first_sys = uniteset(decl_first_sys, stat_first_sys);
    pmt_first_sys = createset(SYM_IDENTIFIER, SYM_VOID, SYM_INT, SYM_NULL); // e2-18
    fac_first_sys = createset(SYM_IDENTIFIER, SYM_NUMBER, SYM_LPAREN,
                              SYM_TRUE, SYM_FALSE, SYM_MINUS, SYM_NOT, SYM_NULL);
    exp_first_sys = fac_first_sys;

    main_blk_follow_sys = createset(SYM_PERIOD, SYM_NULL);

    err = cc = cx = ll = 0; // initialize global variables
    ch = ' ';
    kk = MAXIDLEN;

    getsym();

    block(main_blk_follow_sys);
    destroyset(phi);
    destroyset(relset);
    destroyset(decl_first_sys);
    destroyset(stat_first_sys);
    destroyset(blk_first_sys);
    destroyset(fac_first_sys);
    destroyset(main_blk_follow_sys);

    if (sym != SYM_PERIOD)    // 每个文件均以一个句号结束
        error(9); // '.' expected.
    if (err == 0) {
        hbin = fopen("hbin.txt", "w");
        for (i = 0; i < cx; i++)
            fwrite(&code[i], sizeof(instruction), 1, hbin);    // 将生成的汇编代码写入文件
        fclose(hbin);
    }
    if (err == 0)
        interpret();    // 生成汇编代码后，开始执行
    else
        printf("There are %d error(s) in PL/0 program.\n", err);
    listcode(0, cx);    // 打印生成的所有汇编代码

    for (int i = 0; i < TXMAX; ++i) // e1
    {
        free_ptr(table[i].ptr);
    }
} // main

//////////////////////////////////////////////////////////////////////
// eof pl0.c
