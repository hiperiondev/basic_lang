/*
 * @assemble.c
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
#include <ctype.h>

#include "compile.h"
#include "vmdebug.h"

static void Assemble(ParseContext_t *c, char *name);

// ParseAsm - parse the 'ASM ... END ASM' statement
void ParseAsm(ParseContext_t *c) {
    ParseTreeNode_t *node = NewParseTreeNode(c, NodeTypeAsmStatement);
    vm_context_t *sys = c->sys;
    uint8_t *start = sys->nextLow;
    int length;
    int tkn;
    
    // check for the end of the 'ASM' statement
    FRequire(c, T_EOL);
    
    // parse each assembly instruction
    for (;;) {

        // get the next line
        if (!ParseGetLine(c))
            ParseError(c, "unexpected end of file in ASM statement");
        
        // check for the end of the assembly instructions
        if ((tkn = GetToken(c)) == T_END_ASM)
            break;

        // check for an empty statement
        else if (tkn == T_EOL)
            continue;

        // check for an opcode name
        else if (tkn != T_IDENTIFIER)
            ParseError(c, "expected an assembly instruction opcode");

        // assemble a single instruction
        Assemble(c, c->token);
    }
    
    // store the code
    length = sys->nextLow - start;
    node->u.asmStatement.code = start;
    node->u.asmStatement.length = length;
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    
    // check for the end of the 'END ASM' statement
    FRequire(c, T_EOL);
}

// Assemble - assemble a single line
static void Assemble(ParseContext_t *c, char *name) {
    GenerateContext_t *g = c->g;
    otdef_t *def;
    
    // lookup the opcode
    for (def = opcode_table; def->name != NULL; ++def)
        if (strcasecmp(name, def->name) == 0) {
            putcbyte(g, def->code);
            switch (def->fmt) {
                case FMT_NONE:
                    break;
                case FMT_BYTE:
                    case FMT_SBYTE:
                    putcbyte(g, ParseIntegerConstant(c));
                    break;
                case FMT_WORD:
                    putcword(g, ParseIntegerConstant(c));
                    break;
                case FMT_NATIVE:
                    putcword(g, ParseIntegerConstant(c));
                    break;
                default:
                    ParseError(c, "instruction not currently supported");
                    break;
            }
            FRequire(c, T_EOL);
            return;
        }

    ParseError(c, "undefined opcode");
}
