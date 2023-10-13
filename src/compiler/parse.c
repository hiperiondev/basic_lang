/*
 * @parse.c
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

#include <stdlib.h>
#include <string.h>

#include "compile.h"

// node type names
/*
static char *nodeTypeNames[] = {
        "NodeTypeFunctionDefinition",
        "NodeTypeLetStatement",
        "NodeTypeIfStatement",
        "NodeTypeForStatement",
        "NodeTypeDoWhileStatement",
        "NodeTypeDoUntilStatement",
        "NodeTypeLoopStatement",
        "NodeTypeLoopWhileStatement",
        "NodeTypeLoopUntilStatement",
        "NodeTypeReturnStatement",
        "NodeTypeEndStatement",
        "NodeTypeCallStatement",
        "NodeTypeGlobalRef",
        "NodeTypeArgumentRef",
        "NodeTypeLocalRef",
        "NodeTypeStringLit",
        "NodeTypeIntegerLit",
        "NodeTypeUnaryOp",
        "NodeTypeBinaryOp",
        "NodeTypeArrayRef",
        "NodeTypeFunctionCall",
        "NodeTypeDisjunction",
        "NodeTypeConjunction"
};
*/

// local function prototypes
static void ParseInclude(ParseContext_t *c);
static String_t* AddString(ParseContext_t *c, const char *value);
static ParseTreeNode_t* ParseExpr(ParseContext_t *c);
static ParseTreeNode_t* ParsePrimary(ParseContext_t *c);
static ParseTreeNode_t* GetSymbolRef(ParseContext_t *c, const char *name);
static void ParseFunction(ParseContext_t *c);
static void ParseEndFunction(ParseContext_t *c);
static void EndFunction(ParseContext_t *c);
static void ParseDim(ParseContext_t *c);
static int ParseVariableDecl(ParseContext_t *c, char *name, VMVALUE *pSize);
static VMVALUE ParseScalarInitializer(ParseContext_t *c);
static void ParseArrayInitializers(ParseContext_t *c, VMVALUE size);
static void ClearArrayInitializers(ParseContext_t *c, VMVALUE size);
static void ParseImpliedLetOrFunctionCall(ParseContext_t *c);
static void ParseLet(ParseContext_t *c);
static void ParseIf(ParseContext_t *c);
static void ParseElse(ParseContext_t *c);
static void ParseElseIf(ParseContext_t *c);
static void ParseEndIf(ParseContext_t *c);
static void ParseFor(ParseContext_t *c);
static void ParseNext(ParseContext_t *c);
static void ParseDo(ParseContext_t *c);
static void ParseDoWhile(ParseContext_t *c);
static void ParseDoUntil(ParseContext_t *c);
static void ParseLoop(ParseContext_t *c);
static void ParseLoopWhile(ParseContext_t *c);
static void ParseLoopUntil(ParseContext_t *c);
static void ParseReturn(ParseContext_t *c);
static void ParsePrint(ParseContext_t *c);
static void ParseEnd(ParseContext_t *c);
static ParseTreeNode_t* ParseExpr2(ParseContext_t *c);
static ParseTreeNode_t* ParseExpr3(ParseContext_t *c);
static ParseTreeNode_t* ParseExpr4(ParseContext_t *c);
static ParseTreeNode_t* ParseExpr5(ParseContext_t *c);
static ParseTreeNode_t* ParseExpr6(ParseContext_t *c);
static ParseTreeNode_t* ParseExpr7(ParseContext_t *c);
static ParseTreeNode_t* ParseExpr8(ParseContext_t *c);
static ParseTreeNode_t* ParseExpr9(ParseContext_t *c);
static ParseTreeNode_t* ParseExpr10(ParseContext_t *c);
static ParseTreeNode_t* ParseExpr11(ParseContext_t *c);
static ParseTreeNode_t* ParseSimplePrimary(ParseContext_t *c);
static ParseTreeNode_t* ParseArrayReference(ParseContext_t *c, ParseTreeNode_t *arrayNode);
static ParseTreeNode_t* ParseCall(ParseContext_t *c, ParseTreeNode_t *functionNode);
static ParseTreeNode_t* GetSymbolRef(ParseContext_t *c, const char *name);
static int IsUnknownGlobolRef(ParseContext_t *c, ParseTreeNode_t *node);
static void ResolveVariableRef(ParseContext_t *c, ParseTreeNode_t *node);
static void ResolveFunctionRef(ParseContext_t *c, ParseTreeNode_t *node);
static ParseTreeNode_t* MakeUnaryOpNode(ParseContext_t *c, int op, ParseTreeNode_t *expr);
static ParseTreeNode_t* MakeBinaryOpNode(ParseContext_t *c, int op, ParseTreeNode_t *left, ParseTreeNode_t *right);
//static char* NodeTypeName(NodeType_t type);
static void PushBlock(ParseContext_t *c, BlockType_t type, ParseTreeNode_t *node);
static void PopBlock(ParseContext_t *c);
static int IsIntegerLit(ParseTreeNode_t *node);

// InitParseContext - parse a statement
ParseContext_t* InitParseContext(System_t *sys) {
    ParseContext_t *c = (ParseContext_t*) AllocateHighMemory(sys, sizeof(ParseContext_t));
    if (c) {
        memset(c, 0, sizeof(ParseContext_t));
        c->sys = sys;
        c->unknownType.id = TYPE_UNKNOWN;
        c->integerType.id = TYPE_INTEGER;
        c->stringType.id = TYPE_STRING;
        c->integerFunctionType.id = TYPE_FUNCTION;
        c->integerFunctionType.u.functionInfo.returnType = &c->integerType;
    }
    return c;
}

// ParseStatement - parse a statement
void ParseStatement(ParseContext_t *c, int tkn) {
    // dispatch on the statement keyword
    switch (tkn) {
        case T_INCLUDE:
            ParseInclude(c);
            break;
        case T_REM:
            // just a comment so ignore the rest of the line
            break;
        case T_FUNCTION:
            ParseFunction(c);
            break;
        case T_END_FUNCTION:
            ParseEndFunction(c);
            break;
        case T_DIM:
            ParseDim(c);
            break;
        case T_LET:
            ParseLet(c);
            break;
        case T_IF:
            ParseIf(c);
            break;
        case T_ELSE:
            ParseElse(c);
            break;
        case T_ELSE_IF:
            ParseElseIf(c);
            break;
        case T_END_IF:
            ParseEndIf(c);
            break;
        case T_FOR:
            ParseFor(c);
            break;
        case T_NEXT:
            ParseNext(c);
            break;
        case T_DO:
            ParseDo(c);
            break;
        case T_DO_WHILE:
            ParseDoWhile(c);
            break;
        case T_DO_UNTIL:
            ParseDoUntil(c);
            break;
        case T_LOOP:
            ParseLoop(c);
            break;
        case T_LOOP_WHILE:
            ParseLoopWhile(c);
            break;
        case T_LOOP_UNTIL:
            ParseLoopUntil(c);
            break;
        case T_RETURN:
            ParseReturn(c);
            break;
        case T_PRINT:
            ParsePrint(c);
            break;
        case T_END:
            ParseEnd(c);
            break;
        case T_ASM:
            ParseAsm(c);
            break;
        default:
            SaveToken(c, tkn);
            ParseImpliedLetOrFunctionCall(c);
            break;
    }
}

// ParseInclude - parse the 'INCLUDE' statement
static void ParseInclude(ParseContext_t *c) {
    char name[MAXTOKEN];
    FRequire(c, T_STRING);
    strcpy(name, c->token);
    FRequire(c, T_EOL);
    if (!PushFile(c, name))
        ParseError(c, "include file not found: %s", name);
}

// ParseFunction - parse the 'FUNCTION' statement
static void ParseFunction(ParseContext_t *c) {
    ParseTreeNode_t *function;
    Symbol_t *symbol;
    int tkn;

    if (c->currentFunction != c->mainFunction)
        ParseError(c, "nested function definitions not supported");
    else if (c->bptr->type != BLOCK_FUNCTION)
        ParseError(c, "function definition not allowed in another block");

    // get the function name
    FRequire(c, T_IDENTIFIER);

    // enter the function name in the global symbol table
    if (!(symbol = FindGlobal(c, c->token)))
        symbol = AddGlobal(c, c->token, SC_FUNCTION, &c->integerFunctionType, 0);
    else {
        if (symbol->storageClass != SC_FUNCTION || symbol->type != &c->integerFunctionType || symbol->placed)
            ParseError(c, "invalid definition of a forward referenced function");
    }
    
    // create the function node
    function = StartFunction(c, symbol);

    // get the argument list
    if ((tkn = GetToken(c)) == '(') {
        if ((tkn = GetToken(c)) != ')') {
            SaveToken(c, tkn);
            do {
                FRequire(c, T_IDENTIFIER);
                AddArgument(c, c->token, SC_VARIABLE, &c->integerType, function->u.functionDefinition.argumentOffset);
                ++function->u.functionDefinition.argumentOffset;
            } while ((tkn = GetToken(c)) == ',');
        }
        Require(c, tkn, ')');
    }
    else
        SaveToken(c, tkn);

    FRequire(c, T_EOL);
}

// ParseEndFunction - parse the 'END FUNCTION' statement
static void ParseEndFunction(ParseContext_t *c) {
    if (c->currentFunction == c->mainFunction)
        ParseError(c, "not in a function definition");
    else if (c->bptr->type != BLOCK_FUNCTION)
        ParseError(c, "function definition not complete");
    //PrintNode(c->currentFunction, 0);
    Generate(c->g, c->currentFunction);
    EndFunction(c);
}

// StartFunction - start a function definition
ParseTreeNode_t* StartFunction(ParseContext_t *c, Symbol_t *symbol) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeFunctionDefinition);
    node->u.functionDefinition.symbol = symbol;
    InitSymbolTable(&node->u.functionDefinition.arguments);
    InitSymbolTable(&node->u.functionDefinition.locals);
    PushBlock(c, BLOCK_FUNCTION, node);
    c->bptr->pNextStatement = &node->u.functionDefinition.bodyStatements;
    c->currentFunction = node;
    return node;
}

// EndFunction - end a function definition
static void EndFunction(ParseContext_t *c) {
    PopBlock(c);
    c->currentFunction = c->mainFunction;
}

// ParseDim - parse the 'DIM' statement
static void ParseDim(ParseContext_t *c) {
    char name[MAXTOKEN];
    VMVALUE value = 0, size = 0;
    int isArray;
    int tkn;

    // parse variable declarations
    do {

        // get variable name
        isArray = ParseVariableDecl(c, name, &size);

        // add to the global symbol table if outside a function definition
        if (c->currentFunction == c->mainFunction) {
            Symbol_t *sym;

            // get the address of the data
            value = codeaddr(c->g);
            
            // check for initializers
            if ((tkn = GetToken(c)) == '=') {
                if (isArray)
                    ParseArrayInitializers(c, size);
                else
                    putdword(c->g, ParseScalarInitializer(c));
            }

            // no initializers
            else {
                ClearArrayInitializers(c, isArray ? size : 1);
                SaveToken(c, tkn);
            }

            // add the symbol to the global symbol table
            sym = AddGlobal(c, name, isArray ? SC_CONSTANT : SC_VARIABLE, &c->integerType, value);
            sym->placed = VMTRUE;
        }

        // otherwise, add to the local symbol table
        else {
            if (isArray)
                ParseError(c, "local arrays are not supported");

            // add a local variable
            AddLocal(c, name, SC_VARIABLE, &c->integerType, c->currentFunction->u.functionDefinition.localOffset);
            ++c->currentFunction->u.functionDefinition.localOffset;
            
            // check for an initializer
            if ((tkn = GetToken(c)) == '=') {
                ParseTreeNode_t *expr, *node;

                // get the initializer
                expr = ParseExpr(c);
                
                // add code to set the initial value of the variable
                node = NewParseTreeNode(c, NodeTypeLetStatement);
                node->u.letStatement.lvalue = GetSymbolRef(c, name);
                node->u.letStatement.rvalue = expr;
                AddNodeToList(c, &c->bptr->pNextStatement, node);
            }

            // no initializer
            else {
                SaveToken(c, tkn);
            }

        }

    } while ((tkn = GetToken(c)) == ',');

    Require(c, tkn, T_EOL);
}

// ParseVariableDecl - parse a variable declaration
static int ParseVariableDecl(ParseContext_t *c, char *name, VMVALUE *pSize) {
    int isArray;
    int tkn;

    // parse the variable name
    FRequire(c, T_IDENTIFIER);
    strcpy(name, c->token);

    // handle arrays
    if ((tkn = GetToken(c)) == '[') {

        // check for an array with unspecified size
        if ((tkn = GetToken(c)) == ']')
            *pSize = 0;

        // otherwise, parse the array size
        else {
            ParseTreeNode_t *expr;

            // put back the token
            SaveToken(c, tkn);

            // get the array size
            expr = ParseExpr(c);

            // make sure it's a constant
            if (!IsIntegerLit(expr) || expr->u.integerLit.value <= 0)
                ParseError(c, "expecting a positive constant expression");
            *pSize = expr->u.integerLit.value;

            // only one dimension allowed for now
            FRequire(c, ']');
        }

        // return an array and its size
        isArray = VMTRUE;
        return VMTRUE;
    }

    // not an array
    else {
        SaveToken(c, tkn);
        isArray = VMFALSE;
        *pSize = 1;
    }

    // return array indicator
    return isArray;
}

// ParseScalarInitializer - parse a scalar initializer
static VMVALUE ParseScalarInitializer(ParseContext_t *c) {
    ParseTreeNode_t *expr = ParseExpr(c);
    if (!IsIntegerLit(expr))
        ParseError(c, "expecting a constant expression");
    return expr->u.integerLit.value;
}

// ParseArrayInitializers - parse array initializers
static void ParseArrayInitializers(ParseContext_t *c, VMVALUE size) {
    int done = VMFALSE;
    int tkn;

    FRequire(c, '{');

    // handle each line of initializers
    while (!done) {
        int lineDone = VMFALSE;

        // look for the first non-blank line
        while ((tkn = GetToken(c)) == T_EOL) {
            if (!ParseGetLine(c))
                ParseError(c, "unexpected end of file in initializers");
        }

        // check for the end of the initializers
        if (tkn == '}')
            break;
        SaveToken(c, tkn);

        // handle each initializer in the current line
        while (!lineDone) {
            VMVALUE value;

            // get a constant expression
            value = ParseScalarInitializer(c);

            // check for too many initializers
            if (--size < 0)
                ParseError(c, "too many initializers");

            // store the initial value
            putdword(c->g, value);

            switch (tkn = GetToken(c)) {
                case T_EOL:
                    lineDone = VMTRUE;
                    break;
                case '}':
                    lineDone = VMTRUE;
                    done = VMTRUE;
                    break;
                case ',':
                    break;
                default:
                    ParseError(c, "expecting a comma, right brace or end of line");
                    break;
            }

        }
    }
}

// ClearArrayInitializers - clear the array initializers
static void ClearArrayInitializers(ParseContext_t *c, VMVALUE size) {
    while (--size >= 0)
        putdword(c->g, 0);
}

// ParseImpliedLetOrFunctionCall - parse an implied let statement or a function call
static void ParseImpliedLetOrFunctionCall(ParseContext_t *c) {
    ParseTreeNode_t *node, *expr;
    int tkn;
    expr = ParsePrimary(c);
    ResolveVariableRef(c, expr);
    switch (tkn = GetToken(c)) {
        case '=':
            node = NewParseTreeNode(c, NodeTypeLetStatement);
            node->u.letStatement.lvalue = expr;
            node->u.letStatement.rvalue = ParseExpr(c);
            break;
        default:
            SaveToken(c, tkn);
            node = NewParseTreeNode(c, NodeTypeCallStatement);
            node->u.callStatement.expr = expr;
            break;
    }
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_EOL);
}

// ParseLet - parse the 'LET' statement
static void ParseLet(ParseContext_t *c) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeLetStatement);
    node->u.letStatement.lvalue = ParsePrimary(c);
    ResolveVariableRef(c, node->u.letStatement.lvalue);
    FRequire(c, '=');
    node->u.letStatement.rvalue = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_EOL);
}

// ParseIf - parse the 'IF' statement
static void ParseIf(ParseContext_t *c) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeIfStatement);
    int tkn;
    node->u.ifStatement.test = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_THEN);
    PushBlock(c, BLOCK_IF, node);
    c->bptr->pNextStatement = &node->u.ifStatement.thenStatements;
    if ((tkn = GetToken(c)) != T_EOL) {
        ParseStatement(c, tkn);
        PopBlock(c);
    }
    Require(c, tkn, T_EOL);
}

// ParseElseIf - parse the 'ELSE IF' statement
static void ParseElseIf(ParseContext_t *c) {
    NodeListEntry_t **pNext;
    ParseTreeNode_t *node;
    switch (c->bptr->type) {
        case BLOCK_IF:
            node = NewParseTreeNode(c, NodeTypeIfStatement);
            pNext = &c->bptr->node->u.ifStatement.elseStatements;
            AddNodeToList(c, &pNext, node);
            c->bptr->node = node;
            node->u.ifStatement.test = ParseExpr(c);
            c->bptr->pNextStatement = &node->u.ifStatement.thenStatements;
            FRequire(c, T_THEN);
            FRequire(c, T_EOL);
            break;
        default:
            ParseError(c, "ELSE IF without a matching IF");
            break;
    }
}

// ParseElse - parse the 'ELSE' statement
static void ParseElse(ParseContext_t *c) {
    switch (c->bptr->type) {
        case BLOCK_IF:
            c->bptr->type = BLOCK_ELSE;
            c->bptr->pNextStatement = &c->bptr->node->u.ifStatement.elseStatements;
            FRequire(c, T_EOL);
            break;
        default:
            ParseError(c, "ELSE without a matching IF");
            break;
    }
}

// ParseEndIf - parse the 'END IF' statement
static void ParseEndIf(ParseContext_t *c) {
    switch (c->bptr->type) {
        case BLOCK_IF:
            case BLOCK_ELSE:
            PopBlock(c);
            break;
        default:
            ParseError(c, "END IF without a matching IF/ELSE IF/ELSE");
            break;
    }
    FRequire(c, T_EOL);
}

// ParseFor - parse the 'FOR' statement
static void ParseFor(ParseContext_t *c) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeForStatement);
    int tkn;

    AddNodeToList(c, &c->bptr->pNextStatement, node);

    PushBlock(c, BLOCK_FOR, node);
    c->bptr->pNextStatement = &node->u.forStatement.bodyStatements;

    // get the control variable
    FRequire(c, T_IDENTIFIER);
    node->u.forStatement.var = GetSymbolRef(c, c->token);

    // parse the starting value expression
    FRequire(c, '=');
    node->u.forStatement.startExpr = ParseExpr(c);

    // parse the TO expression and generate the loop termination test
    FRequire(c, T_TO);
    node->u.forStatement.endExpr = ParseExpr(c);

    // get the STEP expression
    if ((tkn = GetToken(c)) == T_STEP) {
        node->u.forStatement.stepExpr = ParseExpr(c);
        tkn = GetToken(c);
    }
    Require(c, tkn, T_EOL);
}

// ParseNext - parse the 'NEXT' statement
static void ParseNext(ParseContext_t *c) {
    switch (c->bptr->type) {
        case BLOCK_FOR:
            FRequire(c, T_IDENTIFIER);
            //if (GetSymbolRef(c, c->token) != c->bptr->node->u.forStatement.var)
            //    ParseError(c, "wrong variable in NEXT");
            PopBlock(c);
            break;
        default:
            ParseError(c, "NEXT without a matching FOR");
            break;
    }
    FRequire(c, T_EOL);
}

// ParseDo - parse the 'DO' statement
static void ParseDo(ParseContext_t *c) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeLoopStatement);
    node->u.loopStatement.test = NULL;
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    PushBlock(c, BLOCK_DO, node);
    c->bptr->pNextStatement = &node->u.loopStatement.bodyStatements;
    FRequire(c, T_EOL);
}

// ParseDoWhile - parse the 'DO WHILE' statement
static void ParseDoWhile(ParseContext_t *c) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeDoWhileStatement);
    node->u.loopStatement.test = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    PushBlock(c, BLOCK_DO, node);
    c->bptr->pNextStatement = &node->u.loopStatement.bodyStatements;
    FRequire(c, T_EOL);
}

// ParseDoUntil - parse the 'DO UNTIL' statement
static void ParseDoUntil(ParseContext_t *c) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeDoUntilStatement);
    node->u.loopStatement.test = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    PushBlock(c, BLOCK_DO, node);
    c->bptr->pNextStatement = &node->u.loopStatement.bodyStatements;
    FRequire(c, T_EOL);
}

// ParseLoop - parse the 'LOOP' statement
static void ParseLoop(ParseContext_t *c) {
    switch (c->bptr->type) {
        case BLOCK_DO:
            PopBlock(c);
            break;
        default:
            ParseError(c, "LOOP without a matching DO");
            break;
    }
    FRequire(c, T_EOL);
}

// ParseLoopWhile - parse the 'LOOP WHILE' statement
static void ParseLoopWhile(ParseContext_t *c) {
    switch (c->bptr->type) {
        case BLOCK_DO:
            if (c->bptr->node->nodeType != NodeTypeLoopStatement)
                ParseError(c, "can't have a test at both the top and bottom of a loop");
            c->bptr->node->nodeType = NodeTypeLoopWhileStatement;
            c->bptr->node->u.loopStatement.test = ParseExpr(c);
            PopBlock(c);
            break;
        default:
            ParseError(c, "LOOP without a matching DO");
            break;
    }
    FRequire(c, T_EOL);
}

// ParseLoopUntil - parse the 'LOOP UNTIL' statement
static void ParseLoopUntil(ParseContext_t *c) {
    switch (c->bptr->type) {
        case BLOCK_DO:
            if (c->bptr->node->nodeType != NodeTypeLoopStatement)
                ParseError(c, "can't have a test at both the top and bottom of a loop");
            c->bptr->node->nodeType = NodeTypeLoopUntilStatement;
            c->bptr->node->u.loopStatement.test = ParseExpr(c);
            PopBlock(c);
            break;
        default:
            ParseError(c, "LOOP without a matching DO");
            break;
    }
    FRequire(c, T_EOL);
}

// ParseReturn - parse the 'RETURN' statement
static void ParseReturn(ParseContext_t *c) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeReturnStatement);
    int tkn;

    // return with no value returns zero
    if ((tkn = GetToken(c)) == T_EOL)
        node->u.returnStatement.expr = NULL;

    // handle return with a value
    else {
        SaveToken(c, tkn);
        node->u.returnStatement.expr = ParseExpr(c);
        FRequire(c, T_EOL);
    }
    
    // add the statement to the current function
    AddNodeToList(c, &c->bptr->pNextStatement, node);
}

// BuildHandlerFunctionCall - compile a call to a runtime print function
static ParseTreeNode_t* BuildHandlerCall(ParseContext_t *c, char *name, ParseTreeNode_t *devExpr, ParseTreeNode_t *expr) {
    ParseTreeNode_t *functionNode, *callNode, *node;
    NodeListEntry_t **pNext;
    Symbol_t *symbol;

    if (!(symbol = FindGlobal(c, name)))
        ParseError(c, "print helper not defined: %s", name);

    functionNode = NewParseTreeNode(c, NodeTypeGlobalRef);
    functionNode->u.symbolRef.symbol = symbol;
    
    // intialize the function call node
    callNode = NewParseTreeNode(c, NodeTypeFunctionCall);
    callNode->u.functionCall.fcn = functionNode;
    callNode->u.functionCall.args = NULL;
    pNext = &callNode->u.functionCall.args;
    
    if (expr) {
        AddNodeToList(c, &pNext, expr);
        ++callNode->u.functionCall.argc;
    }

    AddNodeToList(c, &pNext, devExpr);
    ++callNode->u.functionCall.argc;

    // build the function call statement
    node = NewParseTreeNode(c, NodeTypeCallStatement);
    node->u.callStatement.expr = callNode;
    
    // return the function call statement
    return node;
}

// ParsePrint - handle the 'PRINT' statement
static void ParsePrint(ParseContext_t *c) {
    ParseTreeNode_t *devExpr, *expr;
    int needNewline = VMTRUE;
    int tkn;

    // check for file output
    if ((tkn = GetToken(c)) == '#') {
        devExpr = ParseExpr(c);
        FRequire(c, ',');
    }

    // handle terminal output
    else {
        SaveToken(c, tkn);
        devExpr = NewParseTreeNode(c, NodeTypeIntegerLit);
        devExpr->u.integerLit.value = 0;
    }
    
    while ((tkn = GetToken(c)) != T_EOL) {
        switch (tkn) {
            case ',':
                needNewline = VMFALSE;
                AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printTab", devExpr, NULL));
                break;
            case ';':
                needNewline = VMFALSE;
                break;
            case T_STRING:
                needNewline = VMTRUE;
                expr = NewParseTreeNode(c, NodeTypeStringLit);
                expr->u.stringLit.string = AddString(c, c->token);
                AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printStr", devExpr, expr));
                break;
            default:
                needNewline = VMTRUE;
                SaveToken(c, tkn);
                expr = ParseExpr(c);
                AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printInt", devExpr, expr));
                break;
        }
    }

    if (needNewline)
        AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printNL", devExpr, NULL));
}

// ParseEnd - parse the 'END' statement
static void ParseEnd(ParseContext_t *c) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeEndStatement);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_EOL);
}

// ParseIntegerConstant - parse an integer constant expression
VMVALUE ParseIntegerConstant(ParseContext_t *c) {
    ParseTreeNode_t *expr;
    expr = ParseExpr(c);
    if (!IsIntegerLit(expr))
        ParseError(c, "expecting an integer constant expression");
    return expr->u.integerLit.value;
}

// ParseExpr - handle the OR operator
static ParseTreeNode_t* ParseExpr(ParseContext_t *c) {
    ParseTreeNode_t *node;
    int tkn;
    node = ParseExpr2(c);
    if ((tkn = GetToken(c)) == T_OR) {
        ParseTreeNode_t *node2 = NewParseTreeNode(c, NodeTypeDisjunction);
        NodeListEntry_t **pNext = &node2->u.exprList.exprs;
        AddNodeToList(c, &pNext, node);
        do {
            AddNodeToList(c, &pNext, ParseExpr2(c));
        } while ((tkn = GetToken(c)) == T_OR);
        node = node2;
    }
    SaveToken(c, tkn);
    return node;
}

// ParseExpr2 - handle the AND operator
static ParseTreeNode_t* ParseExpr2(ParseContext_t *c) {
    ParseTreeNode_t *node;
    int tkn;
    node = ParseExpr3(c);
    if ((tkn = GetToken(c)) == T_AND) {
        ParseTreeNode_t *node2 = NewParseTreeNode(c, NodeTypeConjunction);
        NodeListEntry_t **pNext = &node2->u.exprList.exprs;
        AddNodeToList(c, &pNext, node);
        do {
            AddNodeToList(c, &pNext, ParseExpr3(c));
        } while ((tkn = GetToken(c)) == T_AND);
        node = node2;
    }
    SaveToken(c, tkn);
    return node;
}

// ParseExpr3 - handle the BXOR operator
static ParseTreeNode_t* ParseExpr3(ParseContext_t *c) {
    ParseTreeNode_t *expr, *expr2;
    int tkn;
    expr = ParseExpr4(c);
    while ((tkn = GetToken(c)) == '^') {
        expr2 = ParseExpr4(c);
        if (IsIntegerLit(expr) && IsIntegerLit(expr2))
            expr->u.integerLit.value = expr->u.integerLit.value ^ expr2->u.integerLit.value;
        else
            expr = MakeBinaryOpNode(c, OP_BXOR, expr, expr2);
    }
    SaveToken(c, tkn);
    return expr;
}

// ParseExpr4 - handle the BOR operator
static ParseTreeNode_t* ParseExpr4(ParseContext_t *c) {
    ParseTreeNode_t *expr, *expr2;
    int tkn;
    expr = ParseExpr5(c);
    while ((tkn = GetToken(c)) == '|') {
        expr2 = ParseExpr5(c);
        if (IsIntegerLit(expr) && IsIntegerLit(expr2))
            expr->u.integerLit.value = expr->u.integerLit.value | expr2->u.integerLit.value;
        else
            expr = MakeBinaryOpNode(c, OP_BOR, expr, expr2);
    }
    SaveToken(c, tkn);
    return expr;
}

// ParseExpr5 - handle the BAND operator
static ParseTreeNode_t* ParseExpr5(ParseContext_t *c) {
    ParseTreeNode_t *expr, *expr2;
    int tkn;
    expr = ParseExpr6(c);
    while ((tkn = GetToken(c)) == '&') {
        expr2 = ParseExpr6(c);
        if (IsIntegerLit(expr) && IsIntegerLit(expr2))
            expr->u.integerLit.value = expr->u.integerLit.value & expr2->u.integerLit.value;
        else
            expr = MakeBinaryOpNode(c, OP_BAND, expr, expr2);
    }
    SaveToken(c, tkn);
    return expr;
}

// ParseExpr6 - handle the '=' and '<>' operators
static ParseTreeNode_t* ParseExpr6(ParseContext_t *c) {
    ParseTreeNode_t *expr, *expr2;
    int tkn;
    expr = ParseExpr7(c);
    while ((tkn = GetToken(c)) == '=' || tkn == T_NE) {
        int op;
        expr2 = ParseExpr7(c);
        switch (tkn) {
            case '=':
                op = OP_EQ;
                break;
            case T_NE:
                op = OP_NE;
                break;
            default:
                // not reached
                op = 0;
                break;
        }
        expr = MakeBinaryOpNode(c, op, expr, expr2);
    }
    SaveToken(c, tkn);
    return expr;
}

// ParseExpr7 - handle the '<', '<=', '>=' and '>' operators
static ParseTreeNode_t* ParseExpr7(ParseContext_t *c) {
    ParseTreeNode_t *expr, *expr2;
    int tkn;
    expr = ParseExpr8(c);
    while ((tkn = GetToken(c)) == '<' || tkn == T_LE || tkn == T_GE || tkn == '>') {
        int op;
        expr2 = ParseExpr8(c);
        switch (tkn) {
            case '<':
                op = OP_LT;
                break;
            case T_LE:
                op = OP_LE;
                break;
            case T_GE:
                op = OP_GE;
                break;
            case '>':
                op = OP_GT;
                break;
            default:
                // not reached
                op = 0;
                break;
        }
        expr = MakeBinaryOpNode(c, op, expr, expr2);
    }
    SaveToken(c, tkn);
    return expr;
}

// ParseExpr8 - handle the '<<' and '>>' operators
static ParseTreeNode_t* ParseExpr8(ParseContext_t *c) {
    ParseTreeNode_t *expr, *expr2;
    int tkn;
    expr = ParseExpr9(c);
    while ((tkn = GetToken(c)) == T_SHL || tkn == T_SHR) {
        expr2 = ParseExpr9(c);
        if (IsIntegerLit(expr) && IsIntegerLit(expr2)) {
            switch (tkn) {
                case T_SHL:
                    expr->u.integerLit.value = expr->u.integerLit.value << expr2->u.integerLit.value;
                    break;
                case T_SHR:
                    expr->u.integerLit.value = expr->u.integerLit.value >> expr2->u.integerLit.value;
                    break;
                default:
                    // not reached
                    break;
            }
        }
        else {
            int op;
            switch (tkn) {
                case T_SHL:
                    op = OP_SHL;
                    break;
                case T_SHR:
                    op = OP_SHR;
                    break;
                default:
                    // not reached
                    op = 0;
                    break;
            }
            expr = MakeBinaryOpNode(c, op, expr, expr2);
        }
    }
    SaveToken(c, tkn);
    return expr;
}

// ParseExpr9 - handle the '+' and '-' operators
static ParseTreeNode_t* ParseExpr9(ParseContext_t *c) {
    ParseTreeNode_t *expr, *expr2;
    int tkn;
    expr = ParseExpr10(c);
    while ((tkn = GetToken(c)) == '+' || tkn == '-') {
        expr2 = ParseExpr10(c);
        if (IsIntegerLit(expr) && IsIntegerLit(expr2)) {
            switch (tkn) {
                case '+':
                    expr->u.integerLit.value = expr->u.integerLit.value + expr2->u.integerLit.value;
                    break;
                case '-':
                    expr->u.integerLit.value = expr->u.integerLit.value - expr2->u.integerLit.value;
                    break;
                default:
                    // not reached
                    break;
            }
        }
        else {
            int op;
            switch (tkn) {
                case '+':
                    op = OP_ADD;
                    break;
                case '-':
                    op = OP_SUB;
                    break;
                default:
                    // not reached
                    op = 0;
                    break;
            }
            expr = MakeBinaryOpNode(c, op, expr, expr2);
        }
    }
    SaveToken(c, tkn);
    return expr;
}

// ParseExpr10 - handle the '*', '/' and MOD operators
static ParseTreeNode_t* ParseExpr10(ParseContext_t *c) {
    ParseTreeNode_t *node, *node2;
    int tkn;
    node = ParseExpr11(c);
    while ((tkn = GetToken(c)) == '*' || tkn == '/' || tkn == T_MOD) {
        node2 = ParseExpr11(c);
        if (IsIntegerLit(node) && IsIntegerLit(node2)) {
            switch (tkn) {
                case '*':
                    node->u.integerLit.value = node->u.integerLit.value * node2->u.integerLit.value;
                    break;
                case '/':
                    if (node2->u.integerLit.value == 0)
                        ParseError(c, "division by zero in constant expression");
                    node->u.integerLit.value = node->u.integerLit.value / node2->u.integerLit.value;
                    break;
                case T_MOD:
                    if (node2->u.integerLit.value == 0)
                        ParseError(c, "division by zero in constant expression");
                    node->u.integerLit.value = node->u.integerLit.value % node2->u.integerLit.value;
                    break;
                default:
                    // not reached
                    break;
            }
        }
        else {
            int op;
            switch (tkn) {
                case '*':
                    op = OP_MUL;
                    break;
                case '/':
                    op = OP_DIV;
                    break;
                case T_MOD:
                    op = OP_REM;
                    break;
                default:
                    // not reached
                    op = 0;
                    break;
            }
            node = MakeBinaryOpNode(c, op, node, node2);
        }
    }
    SaveToken(c, tkn);
    return node;
}

// ParseExpr11 - handle unary operators
static ParseTreeNode_t* ParseExpr11(ParseContext_t *c) {
    ParseTreeNode_t *node;
    int tkn;
    switch (tkn = GetToken(c)) {
        case '+':
            node = ParsePrimary(c);
            break;
        case '-':
            node = ParsePrimary(c);
            if (IsIntegerLit(node))
                node->u.integerLit.value = -node->u.integerLit.value;
            else
                node = MakeUnaryOpNode(c, OP_NEG, node);
            break;
        case T_NOT:
            node = ParsePrimary(c);
            if (IsIntegerLit(node))
                node->u.integerLit.value = !node->u.integerLit.value;
            else
                node = MakeUnaryOpNode(c, OP_NOT, node);
            break;
        case '~':
            node = ParsePrimary(c);
            if (IsIntegerLit(node))
                node->u.integerLit.value = ~node->u.integerLit.value;
            else
                node = MakeUnaryOpNode(c, OP_BNOT, node);
            break;
        default:
            SaveToken(c, tkn);
            node = ParsePrimary(c);
            break;
    }
    return node;
}

// ParsePrimary - parse function calls and array references
ParseTreeNode_t* ParsePrimary(ParseContext_t *c) {
    ParseTreeNode_t *node;
    int tkn;
    node = ParseSimplePrimary(c);
    while ((tkn = GetToken(c)) == '(' || tkn == '[') {
        switch (tkn) {
            case '[':
                node = ParseArrayReference(c, node);
                break;
            case '(':
                node = ParseCall(c, node);
                break;
        }
    }
    SaveToken(c, tkn);
    return node;
}

// ParseArrayReference - parse an array reference
static ParseTreeNode_t* ParseArrayReference(ParseContext_t *c, ParseTreeNode_t *arrayNode) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeArrayRef);

    // setup the array reference
    node->u.arrayRef.array = arrayNode;

    // get the index expression
    node->u.arrayRef.index = ParseExpr(c);

    // check for the close bracket
    FRequire(c, ']');
    return node;
}

// ParseCall - parse a function or subroutine call
static ParseTreeNode_t* ParseCall(ParseContext_t *c, ParseTreeNode_t *functionNode) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeFunctionCall);
    //NodeListEntry **pNext;
    int tkn;

    // intialize the function call node
    ResolveFunctionRef(c, functionNode);
    node->u.functionCall.fcn = functionNode;
    node->type = &c->integerType;
    //pNext = &node->u.functionCall.args;

    // parse the argument list
    if ((tkn = GetToken(c)) != ')') {
        SaveToken(c, tkn);
        do {
            NodeListEntry_t *actual = (NodeListEntry_t*) AllocateHighMemory(c->sys, sizeof(NodeListEntry_t));
            actual->node = ParseExpr(c);
            actual->next = node->u.functionCall.args;
            node->u.functionCall.args = actual;
            ++node->u.functionCall.argc;
        } while ((tkn = GetToken(c)) == ',');
        Require(c, tkn, ')');
    }

    // return the function call node
    return node;
}

// ParseSimplePrimary - parse a primary expression
static ParseTreeNode_t* ParseSimplePrimary(ParseContext_t *c) {
    ParseTreeNode_t *node;
    switch (GetToken(c)) {
        case '(':
            node = ParseExpr(c);
            FRequire(c, ')');
            break;
        case T_NUMBER:
            node = NewParseTreeNode(c, NodeTypeIntegerLit);
            node->type = &c->integerType;
            node->u.integerLit.value = c->tokenValue;
            break;
        case T_STRING:
            node = NewParseTreeNode(c, NodeTypeStringLit);
            node->type = &c->stringType;
            node->u.stringLit.string = AddString(c, c->token);
            break;
        case T_IDENTIFIER:
            node = GetSymbolRef(c, c->token);
            break;
        default:
            ParseError(c, "Expecting a primary expression");
            node = NULL; // not reached
            break;
    }
    return node;
}

// IsUnknownGlobolRef - check for a reference to a global symbol that has not yet been defined
static int IsUnknownGlobolRef(ParseContext_t *c, ParseTreeNode_t *node) {
    return node->nodeType == NodeTypeGlobalRef && node->u.symbolRef.symbol->storageClass == SC_UNKNOWN;
}

// ResolveVariableRef - resolve an unknown global reference to a variable reference
static void ResolveVariableRef(ParseContext_t *c, ParseTreeNode_t *node) {
    if (IsUnknownGlobolRef(c, node)) {
        Symbol_t *symbol = node->u.symbolRef.symbol;
        symbol->storageClass = SC_VARIABLE;
        symbol->type = &c->integerType;
    }
}

// ResolveFunctionRef - resolve an unknown global symbol reference to a function reference
static void ResolveFunctionRef(ParseContext_t *c, ParseTreeNode_t *node) {
    if (IsUnknownGlobolRef(c, node)) {
        Symbol_t *symbol = node->u.symbolRef.symbol;
        symbol->storageClass = SC_FUNCTION;
        symbol->type = &c->integerFunctionType;
    }
}

// GetSymbolRef - setup a symbol reference
static ParseTreeNode_t* GetSymbolRef(ParseContext_t *c, const char *name) {
    ParseTreeNode_t *node;
    Symbol_t *symbol;

    // handle local variables within a function or subroutine
    if (c->currentFunction != c->mainFunction && (symbol = FindLocal(c, name)) != NULL) {
        if (symbol->storageClass == SC_CONSTANT) {
            node = NewParseTreeNode(c, NodeTypeIntegerLit);
            node->type = &c->integerType;
            node->u.integerLit.value = symbol->value;
        }
        else {
            node = NewParseTreeNode(c, NodeTypeLocalRef);
            node->type = symbol->type;
            node->u.symbolRef.symbol = symbol;
        }
    }

    // handle function or subroutine arguments or the return value symbol
    else if (c->currentFunction != c->mainFunction && (symbol = FindArgument(c, name)) != NULL) {
        node = NewParseTreeNode(c, NodeTypeArgumentRef);
        node->type = symbol->type;
        node->u.symbolRef.symbol = symbol;
    }

    // handle global symbols
    else if ((symbol = FindGlobal(c, c->token)) != NULL) {
        if (symbol->storageClass == SC_CONSTANT) {
            node = NewParseTreeNode(c, NodeTypeIntegerLit);
            node->type = &c->integerType;
            node->u.integerLit.value = symbol->value;
        }
        else {
            node = NewParseTreeNode(c, NodeTypeGlobalRef);
            node->type = symbol->type;
            node->u.symbolRef.symbol = symbol;
        }
    }

    // handle undefined symbols
    else {
        symbol = AddGlobal(c, name, SC_UNKNOWN, &c->unknownType, 0);
        node = NewParseTreeNode(c, NodeTypeGlobalRef);
        node->type = symbol->type;
        node->u.symbolRef.symbol = symbol;
    }

    // return the symbol reference node
    return node;
}

// MakeUnaryOpNode - allocate a unary operation parse tree node
static ParseTreeNode_t* MakeUnaryOpNode(ParseContext_t *c, int op, ParseTreeNode_t *expr) {
    ParseTreeNode_t *node;
    ResolveVariableRef(c, expr);
    if (IsIntegerLit(expr)) {
        node = expr;
        switch (op) {
            case OP_NEG:
                node->u.integerLit.value = -expr->u.integerLit.value;
                break;
            case OP_NOT:
                node->u.integerLit.value = !expr->u.integerLit.value;
                break;
            case OP_BNOT:
                node->u.integerLit.value = ~expr->u.integerLit.value;
                break;
        }
    }
    else if (expr->type == &c->integerType) {
        node = NewParseTreeNode(c, NodeTypeUnaryOp);
        node->type = &c->integerType;
        node->u.unaryOp.op = op;
        node->u.unaryOp.expr = expr;
    }
    else {
        ParseError(c, "Expecting a numeric expression");
        node = NULL; // not reached
    }
    return node;
}

// MakeBinaryOpNode - allocate a binary operation parse tree node
static ParseTreeNode_t* MakeBinaryOpNode(ParseContext_t *c, int op, ParseTreeNode_t *left, ParseTreeNode_t *right) {
    ParseTreeNode_t *node;
    ResolveVariableRef(c, left);
    ResolveVariableRef(c, right);
    if (IsIntegerLit(left) && IsIntegerLit(right)) {
        node = left;
        switch (op) {
            case OP_BXOR:
                node->u.integerLit.value = left->u.integerLit.value ^ right->u.integerLit.value;
                break;
            case OP_BOR:
                node->u.integerLit.value = left->u.integerLit.value | right->u.integerLit.value;
                break;
            case OP_BAND:
                node->u.integerLit.value = left->u.integerLit.value & right->u.integerLit.value;
                break;
            case OP_EQ:
                node->u.integerLit.value = left->u.integerLit.value == right->u.integerLit.value;
                break;
            case OP_NE:
                node->u.integerLit.value = left->u.integerLit.value != right->u.integerLit.value;
                break;
            case OP_LT:
                node->u.integerLit.value = left->u.integerLit.value < right->u.integerLit.value;
                break;
            case OP_LE:
                node->u.integerLit.value = left->u.integerLit.value <= right->u.integerLit.value;
                break;
            case OP_GE:
                node->u.integerLit.value = left->u.integerLit.value >= right->u.integerLit.value;
                break;
            case OP_GT:
                node->u.integerLit.value = left->u.integerLit.value > right->u.integerLit.value;
                break;
            case OP_SHL:
                node->u.integerLit.value = left->u.integerLit.value << right->u.integerLit.value;
                break;
            case OP_SHR:
                node->u.integerLit.value = left->u.integerLit.value >> right->u.integerLit.value;
                break;
            case OP_ADD:
                node->u.integerLit.value = left->u.integerLit.value + right->u.integerLit.value;
                break;
            case OP_SUB:
                node->u.integerLit.value = left->u.integerLit.value - right->u.integerLit.value;
                break;
            case OP_MUL:
                node->u.integerLit.value = left->u.integerLit.value * right->u.integerLit.value;
                break;
            case OP_DIV:
                if (right->u.integerLit.value == 0)
                    ParseError(c, "division by zero in constant expression");
                node->u.integerLit.value = left->u.integerLit.value / right->u.integerLit.value;
                break;
            case OP_REM:
                if (right->u.integerLit.value == 0)
                    ParseError(c, "division by zero in constant expression");
                node->u.integerLit.value = left->u.integerLit.value % right->u.integerLit.value;
                break;
            default:
                goto integerOp;
        }
    }
    else {
        if (left->type == &c->integerType) {
            if (right->type == &c->integerType) {
integerOp:
                node = NewParseTreeNode(c, NodeTypeBinaryOp);
                node->type = &c->integerType;
                node->u.binaryOp.op = op;
                node->u.binaryOp.left = left;
                node->u.binaryOp.right = right;
            }
            else {
                ParseError(c, "Expecting right operand to be anumeric expression: %d", right->nodeType);
                node = NULL; // not reached
            }
        }
        else {
            ParseError(c, "Expecting left operand to be a numeric expression: %d", left->nodeType);
            node = NULL; // not reached
        }
    }
    return node;
}

// PushBlock - push a block on the stack
static void PushBlock(ParseContext_t *c, BlockType_t type, ParseTreeNode_t *node) {
    if (++c->bptr >= c->btop)
        ParseError(c, "statements too deeply nested");
    c->bptr->type = type;
    c->bptr->node = node;
}

// PopBlock - pop a block off the stack
static void PopBlock(ParseContext_t *c) {
    --c->bptr;
}

// NewParseTreeNode - allocate a new parse tree node
ParseTreeNode_t* NewParseTreeNode(ParseContext_t *c, int type) {
    ParseTreeNode_t *node = (ParseTreeNode_t*) AllocateHighMemory(c->sys, sizeof(ParseTreeNode_t));
    memset(node, 0, sizeof(ParseTreeNode_t));
    node->nodeType = type;
    return node;
}

// AddNodeToList - add a node to a parse tree node list
void AddNodeToList(ParseContext_t *c, NodeListEntry_t ***ppNextEntry, ParseTreeNode_t *node) {
    NodeListEntry_t *entry = (NodeListEntry_t*) AllocateHighMemory(c->sys, sizeof(NodeListEntry_t));
    entry->node = node;
    entry->next = NULL;
    **ppNextEntry = entry;
    *ppNextEntry = &entry->next;
}

// NodeTypeName - get the name of a node type
//static char* NodeTypeName(NodeType_t type) {
//    return type >= 0 && type < _MaxNodeType ? nodeTypeNames[type] : "<unknown>";
//}

// IsIntegerLit - check to see if a node is an integer literal
static int IsIntegerLit(ParseTreeNode_t *node) {
    return node->nodeType == NodeTypeIntegerLit;
}

// AddString - add a string to the string table
static String_t* AddString(ParseContext_t *c, const char *value) {
    String_t *str;
    
    // check to see if the string is already in the table
    for (str = c->strings; str != NULL; str = str->next)
        if (strcmp(value, str->data) == 0)
            return str;

    // allocate the string structure
    str = (String_t*) AllocateLowMemory(c->sys, sizeof(String_t) + strlen(value));
    memset(str, 0, sizeof(String_t));
    strcpy(str->data, value);
    str->next = c->strings;
    c->strings = str;

    // return the string table entry
    return str;
}

// DumpStrings - dump the string table
void DumpStrings(ParseContext_t *c) {
    String_t *str = c->strings;
    if (str) {
        VM_printf("Strings:\n");
        for (; str != NULL; str = str->next)
            VM_printf("  '%s' %08x\n", str->data, (VMVALUE) ((uint8_t*) str->data - c->g->codeBuf));
    }
}
