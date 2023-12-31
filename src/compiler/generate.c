/*
 * @generate.c
 *
 * @brief
 * @details
 * This is based on other projects:
 *   junkbasic (David Michael Betz): https://github.com/dbetz/junkbasic/
 *   Others (see individual files)
 *
 *   please contact their authors for more information.
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/basic_lang
 */

#include <string.h>
#include <stdbool.h>

#include "compile.h"
#include "vmdebug.h"

bool lastReturn = false;

// partial value
typedef struct PVAL_s PVAL_t;

// partial value function codes
typedef enum {
    PV_LOAD,
    PV_STORE
} PValOp_t;

typedef void GenFcn(GenerateContext_t *c, PValOp_t op, PVAL_t *pv);

#define GEN_NULL    ((GenFcn *)0)

// partial value structure
struct PVAL_s {
    GenFcn *fcn;
    union {
        Symbol_t *sym;
        String_t *str;
        VMVALUE val;
    } u;
};

functions_t generate_functions[100];
int generate_functionCount = 0;

// local function prototypes
static void code_lvalue(GenerateContext_t *c, ParseTreeNode_t *expr, PVAL_t *pv);
static void code_rvalue(GenerateContext_t *c, ParseTreeNode_t *expr);
static void code_function_definition(GenerateContext_t *c, ParseTreeNode_t *node);
static void code_if_statement(GenerateContext_t *c, ParseTreeNode_t *node);
static void code_for_statement(GenerateContext_t *c, ParseTreeNode_t *node);
static void code_do_while_statement(GenerateContext_t *c, ParseTreeNode_t *node);
static void code_do_until_statement(GenerateContext_t *c, ParseTreeNode_t *node);
static void code_loop_statement(GenerateContext_t *c, ParseTreeNode_t *node);
static void code_loop_while_statement(GenerateContext_t *c, ParseTreeNode_t *node);
static void code_loop_until_statement(GenerateContext_t *c, ParseTreeNode_t *node);
static void code_return_statement(GenerateContext_t *c, ParseTreeNode_t *node);
static void code_asm_statement(GenerateContext_t *c, ParseTreeNode_t *node);
static void code_statement_list(GenerateContext_t *c, NodeListEntry_t *entry);
static void code_shortcircuit(GenerateContext_t *c, int op, ParseTreeNode_t *expr);
static void code_call(GenerateContext_t *c, ParseTreeNode_t *expr);
static void code_symbolRef(GenerateContext_t *c, Symbol_t *sym);
static void code_arrayref(GenerateContext_t *c, ParseTreeNode_t *expr, PVAL_t *pv);
static void code_index(GenerateContext_t *c, PValOp_t fcn, PVAL_t *pv);
static void code_expr(GenerateContext_t *c, ParseTreeNode_t *expr, PVAL_t *pv);
static void code_global(GenerateContext_t *c, PValOp_t fcn, PVAL_t *pv);
static void code_local(GenerateContext_t *c, PValOp_t fcn, PVAL_t *pv);
static VMVALUE rd_cword(GenerateContext_t *c, VMUVALUE off);
static void wr_cword(GenerateContext_t *c, VMUVALUE off, VMVALUE w);
static void fixup(GenerateContext_t *c, VMUVALUE chn, VMUVALUE val);
static void fixupbranch(GenerateContext_t *c, VMUVALUE chn, VMUVALUE val);
static VMVALUE AddSymbolRef(GenerateContext_t *c, Symbol_t *sym, VMUVALUE offset);
static VMVALUE AddStringRef(GenerateContext_t *c, String_t *str);
static void GenerateError(GenerateContext_t *c, const char *fmt, ...);
static void GenerateFatal(GenerateContext_t *c, const char *fmt, ...);

// InitGenerateContext - initialize a generate context
GenerateContext_t* InitGenerateContext(vm_context_t *sys) {
    GenerateContext_t *g;
    if (!(g = (GenerateContext_t*) system_allocate_high_memory(sys, sizeof(GenerateContext_t))))
        return NULL;
    g->sys = sys;
    g->codeBuf = sys->nextLow;
    generate_functionCount = 0;
    return g;
}

// Generate - generate code for a function
VMVALUE Generate(GenerateContext_t *c, ParseTreeNode_t *node) {
    VMVALUE code = codeaddr(c);
    PVAL_t pv;
    code_expr(c, node, &pv);
    return code;
}

// code_lvalue - generate code for an l-value expression
static void code_lvalue(GenerateContext_t *c, ParseTreeNode_t *expr, PVAL_t *pv) {
    code_expr(c, expr, pv);
    if (pv->fcn == GEN_NULL)
        GenerateError(c, "Expecting an lvalue");
}

// code_rvalue - generate code for an r-value expression
static void code_rvalue(GenerateContext_t *c, ParseTreeNode_t *expr) {
    PVAL_t pv;
    code_expr(c, expr, &pv);
    if (pv.fcn)
        (*pv.fcn)(c, PV_LOAD, &pv);
}

// code_expr - generate code for an expression parse tree
static void code_expr(GenerateContext_t *c, ParseTreeNode_t *expr, PVAL_t *pv) {
    VMVALUE ival;
    switch (expr->nodeType) {
        case NodeTypeFunctionDefinition:
            code_function_definition(c, expr);
            break;
        case NodeTypeLetStatement:
            code_rvalue(c, expr->u.letStatement.rvalue);
            code_lvalue(c, expr->u.letStatement.lvalue, pv);
            (pv->fcn)(c, PV_STORE, pv);
            break;
        case NodeTypeIfStatement:
            code_if_statement(c, expr);
            break;
        case NodeTypeForStatement:
            code_for_statement(c, expr);
            break;
        case NodeTypeDoWhileStatement:
            code_do_while_statement(c, expr);
            break;
        case NodeTypeDoUntilStatement:
            code_do_until_statement(c, expr);
            break;
        case NodeTypeLoopStatement:
            code_loop_statement(c, expr);
            break;
        case NodeTypeLoopWhileStatement:
            code_loop_while_statement(c, expr);
            break;
        case NodeTypeLoopUntilStatement:
            code_loop_until_statement(c, expr);
            break;
        case NodeTypeReturnStatement:
            code_return_statement(c, expr);
            break;
        case NodeTypeAsmStatement:
            code_asm_statement(c, expr);
            break;
        case NodeTypeCallStatement:
            code_rvalue(c, expr->u.callStatement.expr);
            putcbyte(c, OP_DROP);
            break;
        case NodeTypeGlobalRef:
            pv->fcn = code_global;
            pv->u.sym = expr->u.symbolRef.symbol;
            break;
        case NodeTypeArgumentRef:
            pv->fcn = code_local;
            pv->u.val = expr->u.symbolRef.symbol->value;
            break;
        case NodeTypeLocalRef:
            pv->fcn = code_local;
            pv->u.val = -1 - expr->u.symbolRef.symbol->value;
            break;
        case NodeTypeStringLit:
            putcbyte(c, OP_LIT);
            putcword(c, AddStringRef(c, expr->u.stringLit.string));
            pv->fcn = GEN_NULL;
            break;
        case NodeTypeIntegerLit:
            ival = expr->u.integerLit.value;
            if (ival >= -128 && ival <= 127) {
                putcbyte(c, OP_SLIT);
                putcbyte(c, ival);
            }
            else {
                putcbyte(c, OP_LIT);
                putcword(c, ival);
            }
            pv->fcn = GEN_NULL;
            break;
        case NodeTypeUnaryOp:
            code_rvalue(c, expr->u.unaryOp.expr);
            putcbyte(c, expr->u.unaryOp.op);
            pv->fcn = GEN_NULL;
            break;
        case NodeTypeBinaryOp:
            code_rvalue(c, expr->u.binaryOp.left);
            code_rvalue(c, expr->u.binaryOp.right);
            putcbyte(c, expr->u.binaryOp.op);
            pv->fcn = GEN_NULL;
            break;
        case NodeTypeFunctionCall:
            code_call(c, expr);
            pv->fcn = GEN_NULL;
            break;
        case NodeTypeArrayRef:
            code_arrayref(c, expr, pv);
            break;
        case NodeTypeDisjunction:
            code_shortcircuit(c, OP_BRTSC, expr);
            pv->fcn = GEN_NULL;
            break;
        case NodeTypeConjunction:
            code_shortcircuit(c, OP_BRFSC, expr);
            pv->fcn = GEN_NULL;
            break;
        default:
            // error
            break;
    }
}

// code_function_definition - generate code for a function definition
static void code_function_definition(GenerateContext_t *c, ParseTreeNode_t *node) {
    vm_context_t *sys = c->sys;
    uint8_t *base = sys->nextLow;
    size_t codeSize;
    VMVALUE code = codeaddr(c);
    putcbyte(c, OP_FRAME);
    putcbyte(c, F_SIZE + node->u.functionDefinition.localOffset);
    code_statement_list(c, node->u.functionDefinition.bodyStatements);

    if (node->u.functionDefinition.symbol) {
        if (!lastReturn)
            putcbyte(c, OP_RETURNZ);
    } else
        putcbyte(c, OP_HALT);

    codeSize = sys->nextLow - base;
    if (node->u.functionDefinition.symbol)
        PlaceSymbol(c, node->u.functionDefinition.symbol, code);
    generate_functions[generate_functionCount].symbol = node->u.functionDefinition.symbol;
    generate_functions[generate_functionCount].code = code;
    generate_functions[generate_functionCount].codeLen = codeSize;
    ++generate_functionCount;
}

// code_if_statement - generate code for an IF statement
static void code_if_statement(GenerateContext_t *c, ParseTreeNode_t *node) {
    VMUVALUE nxt, end;
    code_rvalue(c, node->u.ifStatement.test);
    putcbyte(c, OP_BRF);
    nxt = putcword(c, 0);
    code_statement_list(c, node->u.ifStatement.thenStatements);
    putcbyte(c, OP_BR);
    end = putcword(c, 0);
    fixupbranch(c, nxt, codeaddr(c));
    code_statement_list(c, node->u.ifStatement.elseStatements);
    fixupbranch(c, end, codeaddr(c));
}

// code_for_statement - generate code for a FOR statement
static void code_for_statement(GenerateContext_t *c, ParseTreeNode_t *node) {
    VMUVALUE nxt, upd, inst;
    PVAL_t pv;
    code_rvalue(c, node->u.forStatement.startExpr);
    code_lvalue(c, node->u.forStatement.var, &pv);
    putcbyte(c, OP_BR);
    upd = putcword(c, 0);
    nxt = codeaddr(c);
    code_statement_list(c, node->u.forStatement.bodyStatements);
    (*pv.fcn)(c, PV_LOAD, &pv);
    if (node->u.forStatement.stepExpr)
        code_rvalue(c, node->u.forStatement.stepExpr);
    else {
        putcbyte(c, OP_SLIT);
        putcbyte(c, 1);
    }
    putcbyte(c, OP_ADD);
    fixupbranch(c, upd, codeaddr(c));
    putcbyte(c, OP_DUP);
    (*pv.fcn)(c, PV_STORE, &pv);
    code_rvalue(c, node->u.forStatement.endExpr);
    putcbyte(c, OP_LE);
    inst = putcbyte(c, OP_BRT);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

// code_do_while_statement - generate code for a DO WHILE statement
static void code_do_while_statement(GenerateContext_t *c, ParseTreeNode_t *node) {
    VMUVALUE nxt, test, inst;
    putcbyte(c, OP_BR);
    test = putcword(c, 0);
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    fixupbranch(c, test, codeaddr(c));
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRT);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

// code_do_until_statement - generate code for a DO UNTIL statement
static void code_do_until_statement(GenerateContext_t *c, ParseTreeNode_t *node) {
    VMUVALUE nxt, test, inst;
    putcbyte(c, OP_BR);
    test = putcword(c, 0);
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    fixupbranch(c, test, codeaddr(c));
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRF);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

// code_loop_statement - generate code for a LOOP statement
static void code_loop_statement(GenerateContext_t *c, ParseTreeNode_t *node) {
    VMUVALUE nxt, inst;
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    inst = putcbyte(c, OP_BR);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

// code_loop_while_statement - generate code for a LOOP WHILE statement
static void code_loop_while_statement(GenerateContext_t *c, ParseTreeNode_t *node) {
    VMUVALUE nxt, inst;
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRT);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

// code_loop_until_statement - generate code for a LOOP UNTIL statement
static void code_loop_until_statement(GenerateContext_t *c, ParseTreeNode_t *node) {
    VMUVALUE nxt, inst;
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRF);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

// code_return_statement - generate code for a RETURN statement
static void code_return_statement(GenerateContext_t *c, ParseTreeNode_t *node) {
    if (node->u.returnStatement.expr) {
        code_rvalue(c, node->u.returnStatement.expr);
        putcbyte(c, OP_RETURN);
    }
    else
        putcbyte(c, OP_RETURNZ);
}

// code_asm_statement - generate code for an ASM statement
static void code_asm_statement(GenerateContext_t *c, ParseTreeNode_t *node) {
    vm_context_t *sys = c->sys;
    int length = node->u.asmStatement.length;
    if (sys->nextLow + length > sys->nextHigh)
        GenerateFatal(c, "insufficient memory");
    memcpy(sys->nextLow, node->u.asmStatement.code, length);
    sys->nextLow += length;
}

// code_statement_list - code a list of statements
static void code_statement_list(GenerateContext_t *c, NodeListEntry_t *entry) {
    while (entry) {
        PVAL_t pv;
        code_expr(c, entry->node, &pv);
        entry = entry->next;
    }
}

// code_shortcircuit - generate code for a conjunction or disjunction of boolean expressions
static void code_shortcircuit(GenerateContext_t *c, int op, ParseTreeNode_t *expr) {
    NodeListEntry_t *entry = expr->u.exprList.exprs;
    int end = 0;

    code_rvalue(c, entry->node);
    entry = entry->next;

    do {
        putcbyte(c, op);
        end = putcword(c, end);
        code_rvalue(c, entry->node);
    } while ((entry = entry->next) != NULL);

    fixupbranch(c, end, codeaddr(c));
}

// code_call - code a function call
static void code_call(GenerateContext_t *c, ParseTreeNode_t *expr) {
    NodeListEntry_t *arg;
    
    // code each argument expression
    for (arg = expr->u.functionCall.args; arg != NULL; arg = arg->next)
        code_rvalue(c, arg->node);

    // get the value of the function
    code_rvalue(c, expr->u.functionCall.fcn);

    // call the function
    putcbyte(c, OP_CALL);
    if (expr->u.functionCall.argc > 0) {
        if (expr->u.functionCall.argc > 1) {
            putcbyte(c, OP_CLEAN);
            putcbyte(c, expr->u.functionCall.argc);
        } else
            putcbyte(c, OP_DROP);
    }
}

// code_symbolRef - code a global reference
static void code_symbolRef(GenerateContext_t *c, Symbol_t *sym) {
    VMUVALUE offset;
    putcbyte(c, OP_LIT);
    offset = codeaddr(c);
    putcword(c, AddSymbolRef(c, sym, offset));
}

// code_arrayref - code an array reference
static void code_arrayref(GenerateContext_t *c, ParseTreeNode_t *expr, PVAL_t *pv) {
    code_lvalue(c, expr->u.arrayRef.array, pv);
    code_rvalue(c, expr->u.arrayRef.index);
    putcbyte(c, OP_INDEX);
    pv->fcn = code_index;
}

// code_global - compile a global variable reference
static void code_global(GenerateContext_t *c, PValOp_t fcn, PVAL_t *pv) {
    Symbol_t *sym = pv->u.sym;
    code_symbolRef(c, sym);
    switch (fcn) {
        case PV_LOAD:
            if (sym->storageClass == SC_VARIABLE)
                putcbyte(c, OP_LOAD);
            break;
        case PV_STORE:
            if (sym->storageClass == SC_VARIABLE)
                putcbyte(c, OP_STORE);
            else
                GenerateFatal(c, "'%s' is not a variable", sym->name);
            break;
    }
}

// code_local - compile an local reference
static void code_local(GenerateContext_t *c, PValOp_t fcn, PVAL_t *pv) {
    switch (fcn) {
        case PV_LOAD:
            putcbyte(c, OP_LREF);
            putcbyte(c, pv->u.val);
            break;
        case PV_STORE:
            putcbyte(c, OP_LSET);
            putcbyte(c, pv->u.val);
            break;
    }
}

// code_index - compile a vector reference
static void code_index(GenerateContext_t *c, PValOp_t fcn, PVAL_t *pv) {
    switch (fcn) {
        case PV_LOAD:
            putcbyte(c, OP_LOAD);
            break;
        case PV_STORE:
            putcbyte(c, OP_STORE);
            break;
    }
}

// codeaddr - get the current code address (actually, offset)
VMVALUE codeaddr(GenerateContext_t *c) {
    return (VMVALUE) (c->sys->nextLow - c->codeBuf);
}

// putcbyte - put a code byte into the code buffer
VMVALUE putcbyte(GenerateContext_t *c, int b) {
    vm_context_t *sys = c->sys;
    VMVALUE addr = codeaddr(c);
    if (sys->nextLow >= sys->nextHigh)
        GenerateFatal(c, "bytecode buffer overflow");
    *sys->nextLow++ = b;

    if (b == OP_RETURN)
        lastReturn = true;
    else
        lastReturn = false;

    return addr;
}

// putcword - put a code word into the code buffer
VMVALUE putcword(GenerateContext_t *c, VMVALUE w) {
    vm_context_t *sys = c->sys;
    VMVALUE addr = codeaddr(c);
    uint8_t *p;
    int cnt = sizeof(VMVALUE);
    if (sys->nextLow + sizeof(VMVALUE) > sys->nextHigh)
        GenerateFatal(c, "bytecode buffer overflow");
    sys->nextLow += sizeof(VMVALUE);
    p = sys->nextLow;
    while (--cnt >= 0) {
        *--p = w;
        w >>= 8;
    }
    return addr;
}

// putdword - put a code word into the code buffer
VMVALUE putdword(GenerateContext_t *c, VMVALUE w) {
    vm_context_t *sys = c->sys;
    VMVALUE addr = codeaddr(c);
    if (sys->nextLow + sizeof(VMVALUE) > sys->nextHigh)
        GenerateFatal(c, "bytecode buffer overflow");
    *((VMVALUE*) sys->nextLow) = w;
    sys->nextLow += sizeof(VMVALUE);
    return addr;
}

// rd_cword - get a code word from the code buffer
static VMVALUE rd_cword(GenerateContext_t *c, VMUVALUE off) {
    int cnt = sizeof(VMVALUE);
    VMVALUE w = 0;
    while (--cnt >= 0)
        w = (w << 8) | c->codeBuf[off++];
    return w;
}

// wr_cword - put a code word into the code buffer
static void wr_cword(GenerateContext_t *c, VMUVALUE off, VMVALUE w) {
    uint8_t *p = &c->codeBuf[off] + sizeof(VMVALUE);
    int cnt = sizeof(VMVALUE);
    while (--cnt >= 0) {
        *--p = w;
        w >>= 8;
    }
}

// fixup - fixup a reference chain
static void fixup(GenerateContext_t *c, VMUVALUE chn, VMUVALUE val) {
    while (chn != 0) {
        int nxt = rd_cword(c, chn);
        wr_cword(c, chn, val);
        chn = nxt;
    }
}

// fixupbranch - fixup a reference chain
static void fixupbranch(GenerateContext_t *c, VMUVALUE chn, VMUVALUE val) {
    while (chn != 0) {
        VMUVALUE nxt = rd_cword(c, chn);
        VMUVALUE off = val - (chn + sizeof(VMUVALUE));
        wr_cword(c, chn, off);
        chn = nxt;
    }
}

// AddSymbolRef - add a reference to a symbol
static VMVALUE AddSymbolRef(GenerateContext_t *c, Symbol_t *sym, VMUVALUE offset) {
    VMVALUE link;

    // handle strings that have already been placed
    if (sym->placed)
        return sym->value;

    // add a new entry to the fixup list
    link = sym->value;
    sym->value = offset;
    return link;
}

// PlaceSymbol - place any global symbols defined in the current function
void PlaceSymbol(GenerateContext_t *c, Symbol_t *sym, VMUVALUE offset) {
    if (sym->placed)
        GenerateFatal(c, "Duplicate definition of '%s'", sym->name);
    else {
        fixup(c, sym->value, offset);
        sym->placed = VMTRUE;
        sym->value = offset;
    }
}

// AddStringRef - add a reference to a string in the string table
static VMVALUE AddStringRef(GenerateContext_t *c, String_t *str) {
    return (VMVALUE) ((uint8_t*) str->data - c->codeBuf);
}

// StoreVector - store a VMVALUE vector
VMVALUE StoreVector(GenerateContext_t *c, const VMVALUE *buf, int size) {
    return StoreByteVector(c, (uint8_t*) buf, size * sizeof(VMVALUE));
}

// StoreByteVector - store a byte vector
VMVALUE StoreByteVector(GenerateContext_t *c, const uint8_t *buf, int size) {
    vm_context_t *sys = c->sys;
    uint8_t *p;
    if (!(p = vm_allocate_low_memory(sys, size)))
        return 0;
    memcpy(p, buf, size);
    return p - sys->freeSpace;
}

// DumpFunctions - dump function definitions
void DumpFunctions(GenerateContext_t *c) {
    int i;
    for (i = 0; i < generate_functionCount; ++i) {
        vm_printf("function '%s':\n", generate_functions[i].symbol ? generate_functions[i].symbol->name : "<main>");
        vmdebug_decode_function(generate_functions[i].code, c->codeBuf + generate_functions[i].code, generate_functions[i].codeLen, NULL, NULL, false);
        vm_printf("\n");
    }
}

// GenerateError - report a code generation error
static void GenerateError(GenerateContext_t *c, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vm_printf("error: ");
    vm_printf(fmt, ap);
    va_end(ap);
}

// GenerateFatal - report a fatal code generation error
static void GenerateFatal(GenerateContext_t *c, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vm_printf("fatal: ");
    vm_printf(fmt, ap);
    vm_putchar('\n');
    va_end(ap);
}
