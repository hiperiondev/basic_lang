/*
 * @compile.h
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

#ifndef __COMPILE_H__
#define __COMPILE_H__

#include <stdio.h>
#include <setjmp.h>

#include "vmtypes.h"
#include "vmimage.h"
#include "system.h"

// program limits
#define MAXTOKEN  32

// forward type declarations
typedef struct SymbolTable_s SymbolTable_t;
typedef struct Symbol_s Symbol_t;
typedef struct ParseTreeNod_se ParseTreeNode_t;
typedef struct NodeListEntry_s NodeListEntry_t;
typedef struct ParseFile_s ParseFile_t;
typedef struct IncludedFile_s IncludedFile_t;

// parse file
struct ParseFile_s {
       ParseFile_t *next;
    IncludedFile_t *file;
              void *fp;
               int lineNumber;
};

// included file
struct IncludedFile_s {
    IncludedFile_t *next;
              char name[1];
};

// lexical tokens
enum {
    T_NONE,
    T_REM = 0x100, // keywords start here
    T_DIM,
    T_FUNCTION,
    T_SUB,
    T_AS,
    T_LET,
    T_IF,
    T_THEN,
    T_ELSE,
    T_END,
    T_FOR,
    T_TO,
    T_STEP,
    T_NEXT,
    T_DO,
    T_WHILE,
    T_UNTIL,
    T_LOOP,
    T_GOTO,
    T_MOD,
    T_AND,
    T_OR,
    T_NOT,
    T_STOP,
    T_RETURN,
    T_PRINT,
    T_ASM,
    T_INCLUDE,
    T_END_FUNCTION, // compound keywords
    T_END_SUB,
    T_ELSE_IF,
    T_END_IF,
    T_DO_WHILE,
    T_DO_UNTIL,
    T_LOOP_WHILE,
    T_LOOP_UNTIL,
    T_END_ASM,
    T_LE, // non-keyword tokens
    T_NE,
    T_GE,
    T_SHL,
    T_SHR,
    T_IDENTIFIER,
    T_NUMBER,
    T_STRING,
    T_EOL,
    T_EOF
};

// block type
typedef enum {
    BLOCK_NONE,
    BLOCK_FUNCTION,
    BLOCK_IF,
    BLOCK_ELSE,
    BLOCK_FOR,
    BLOCK_DO
} BlockType_t;

// block structure
typedef struct Block_s Block_t;
struct Block_s {
        BlockType_t type;
    ParseTreeNode_t *node;
    NodeListEntry_t **pNextStatement;
};

// string structure
typedef struct String_s String_t;
struct String_s {
    String_t *next;
        char data[1];
};

// storage class ids
// this must match the array of names in debug.c
typedef enum {
    SC_UNKNOWN,
    SC_CONSTANT,
    SC_VARIABLE,
    SC_FUNCTION,
    _SC_MAX
} StorageClass_t;

// symbol types
// this must match the array of names in debug.c
typedef enum {
    TYPE_UNKNOWN,
    TYPE_INTEGER,
    TYPE_BYTE,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_STRUCT,
    TYPE_POINTER,
    TYPE_FUNCTION,
    _TYPE_MAX
} TypeID_t;

typedef struct Type_s Type_t;
typedef struct Field_s Field_t;

struct Type_s {
    TypeID_t id;
    union {
        struct {
            Type_t *returnType;
        } functionInfo;
        struct {
            Type_t *elementType;
            VMVALUE size;
        } arrayInfo;
        struct {
            Field_t *fields;
        } structInfo;
        struct {
            Type_t *targetType;
        } pointerInfo;
    } u;
};

struct Field_s {
    Field_t *next;
     Type_t *type;
    VMVALUE offset;
       char name[1];
};

// symbol table
struct SymbolTable_s {
    Symbol_t *head;
    Symbol_t **pTail;
         int count;
};

// symbol structure
struct Symbol_s {
          Symbol_t *next;
    StorageClass_t storageClass;
            Type_t *type;
               int placed;
           VMVALUE value;
              char name[1];
};

// parse context
typedef struct {
         vm_context_t *sys;                // system context
        System_line_t *sys_line;           // input line
    GenerateContext_t *g;                  // generate - generate context
          ParseFile_t *currentFile;        // current input file
       IncludedFile_t *includedFiles;      // list of files that have already been included
                  int lineNumber;          // scan - current line number
                  int savedToken;          // scan - lookahead token
                  int tokenOffset;         // scan - offset to the start of the current token
                 char token[MAXTOKEN];     // scan - current token string
              VMVALUE tokenValue;          // scan - current token integer value
                  int inComment;           // scan - inside of a slash/star comment
        SymbolTable_t globals;             // parse - global variables and constants
             String_t *strings;            // parse - string constants
      ParseTreeNode_t *mainFunction;       // parse - the main function
      ParseTreeNode_t *currentFunction;    // parse - the function currently being parsed
              Block_t blockBuf[10];        // parse - stack of nested blocks
              Block_t *bptr;               // parse - current block
              Block_t *btop;               // parse - top of block stack
               Type_t unknownType;         // parse - unknown type
               Type_t integerType;         // parse - integer type
               Type_t stringType;          // parse - string type
               Type_t integerFunctionType; // parse - integer function type
} ParseContext_t;

// parse tree node types
typedef enum {
    NodeTypeFunctionDefinition,
    NodeTypeLetStatement,
    NodeTypeIfStatement,
    NodeTypeForStatement,
    NodeTypeDoWhileStatement,
    NodeTypeDoUntilStatement,
    NodeTypeLoopStatement,
    NodeTypeLoopWhileStatement,
    NodeTypeLoopUntilStatement,
    NodeTypeReturnStatement,
    NodeTypeEndStatement,
    NodeTypeCallStatement,
    NodeTypeAsmStatement,
    NodeTypeGlobalRef,
    NodeTypeArgumentRef,
    NodeTypeLocalRef,
    NodeTypeStringLit,
    NodeTypeIntegerLit,
    NodeTypeUnaryOp,
    NodeTypeBinaryOp,
    NodeTypeArrayRef,
    NodeTypeFunctionCall,
    NodeTypeDisjunction,
    NodeTypeConjunction,
    _MaxNodeType
} NodeType_t;

// parse tree node structure
struct ParseTreeNod_se {
    NodeType_t nodeType;
    Type_t *type;
    union {
        struct {
            Symbol_t *symbol;
            SymbolTable_t arguments;
            SymbolTable_t locals;
            int argumentOffset;
            int localOffset;
            NodeListEntry_t *bodyStatements;
        } functionDefinition;
        struct {
            ParseTreeNode_t *lvalue;
            ParseTreeNode_t *rvalue;
        } letStatement;
        struct {
            ParseTreeNode_t *test;
            NodeListEntry_t *thenStatements;
            NodeListEntry_t *elseStatements;
        } ifStatement;
        struct {
            ParseTreeNode_t *var;
            ParseTreeNode_t *startExpr;
            ParseTreeNode_t *endExpr;
            ParseTreeNode_t *stepExpr;
            NodeListEntry_t *bodyStatements;
        } forStatement;
        struct {
            ParseTreeNode_t *test;
            NodeListEntry_t *bodyStatements;
        } loopStatement;
        struct {
            ParseTreeNode_t *expr;
        } returnStatement;
        struct {
            ParseTreeNode_t *expr;
        } callStatement;
        struct {
            uint8_t *code;
            int length;
        } asmStatement;
        struct {
            Symbol_t *symbol;
        } symbolRef;
        struct {
            String_t *string;
        } stringLit;
        struct {
            VMVALUE value;
        } integerLit;
        struct {
            int op;
            ParseTreeNode_t *expr;
        } unaryOp;
        struct {
            int op;
            ParseTreeNode_t *left;
            ParseTreeNode_t *right;
        } binaryOp;
        struct {
            ParseTreeNode_t *array;
            ParseTreeNode_t *index;
        } arrayRef;
        struct {
            ParseTreeNode_t *fcn;
            NodeListEntry_t *args;
            int argc;
        } functionCall;
        struct {
            NodeListEntry_t *exprs;
        } exprList;
    } u;
};

// node list entry structure
struct NodeListEntry_s {
    ParseTreeNode_t *node;
    NodeListEntry_t *next;
};

// compile.c
 ParseContext_t* InitCompileContext(vm_context_t *sys);
            void Compile(ParseContext_t *c);

// parse.c
 ParseContext_t* InitParseContext(vm_context_t *sys);
             int PushFile(ParseContext_t *c, const char *name);
             int ParseGetLine(ParseContext_t *c);
ParseTreeNode_t* StartFunction(ParseContext_t *c, Symbol_t *symbol);
            void ParseStatement(ParseContext_t *c, int tkn);
         VMVALUE ParseIntegerConstant(ParseContext_t *c);
ParseTreeNode_t* NewParseTreeNode(ParseContext_t *c, int type);
            void AddNodeToList(ParseContext_t *c, NodeListEntry_t ***ppNextEntry, ParseTreeNode_t *node);
            void PrintNode(ParseTreeNode_t *node, int indent);
            void DumpStrings(ParseContext_t *c);

// assemble.c
            void ParseAsm(ParseContext_t *c);

// scan.c
            void InitScan(ParseContext_t *c);
            void FRequire(ParseContext_t *c, int requiredToken);
            void Require(ParseContext_t *c, int token, int requiredToken);
             int GetToken(ParseContext_t *c);
            void SaveToken(ParseContext_t *c, int token);
           char* TokenName(int token);
             int SkipSpaces(ParseContext_t *c);
             int GetChar(ParseContext_t *c);
            void UngetC(ParseContext_t *c);
            void ParseError(ParseContext_t *c, const char *fmt, ...);

// symbols.c
            void InitSymbolTable(SymbolTable_t *table);
       Symbol_t* AddGlobal(ParseContext_t *c, const char *name, StorageClass_t storageClass, Type_t *type, VMVALUE value);
       Symbol_t* AddArgument(ParseContext_t *c, const char *name, StorageClass_t storageClass, Type_t *type, VMVALUE value);
       Symbol_t* AddLocal(ParseContext_t *c, const char *name, StorageClass_t storageClass, Type_t *type, VMVALUE value);
       Symbol_t* FindGlobal(ParseContext_t *c, const char *name);
       Symbol_t* FindArgument(ParseContext_t *c, const char *name);
       Symbol_t* FindLocal(ParseContext_t *c, const char *name);
             int IsConstant(Symbol_t *symbol);
            void DumpSymbols(SymbolTable_t *table, const char *tag);

// generate.c
GenerateContext_t* InitGenerateContext(vm_context_t *sys);
           VMVALUE Generate(GenerateContext_t *c, ParseTreeNode_t *node);
              void PlaceSymbol(GenerateContext_t *c, Symbol_t *sym, VMUVALUE offset);
           VMVALUE StoreVector(GenerateContext_t *c, const VMVALUE *buf, int size);
           VMVALUE StoreByteVector(GenerateContext_t *c, const uint8_t *buf, int size);
              void DumpFunctions(GenerateContext_t *c);
           VMVALUE codeaddr(GenerateContext_t *c);
           VMVALUE putcbyte(GenerateContext_t *c, int b);
           VMVALUE putcword(GenerateContext_t *c, VMVALUE w);
           VMVALUE putdword(GenerateContext_t *c, VMVALUE w);

#endif
