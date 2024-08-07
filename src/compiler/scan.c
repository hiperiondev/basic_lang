/*
 * @scan.c
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include <ctype.h>

#include "compile.h"

// keyword table
static struct {
    char *keyword;
    int token;
} ktab[] = {
// these must be in the same order as the int enum
        { "INCLUDE" , T_INCLUDE  },
        { "REM"     , T_REM      },
        { "DIM"     , T_DIM      },
        { "FUNCTION", T_FUNCTION },
        { "SUB"     , T_SUB      },
        { "AS"      , T_AS       },
        { "LET"     , T_LET      },
        { "IF"      , T_IF       },
        { "THEN"    , T_THEN     },
        { "ELSE"    , T_ELSE     },
        { "END"     , T_END      },
        { "FOR"     , T_FOR      },
        { "TO"      , T_TO       },
        { "STEP"    , T_STEP     },
        { "NEXT"    , T_NEXT     },
        { "DO"      , T_DO       },
        { "WHILE"   , T_WHILE    },
        { "UNTIL"   , T_UNTIL    },
        { "LOOP"    , T_LOOP     },
        { "GOTO"    , T_GOTO     },
        { "MOD"     , T_MOD      },
        { "AND"     , T_AND      },
        { "OR"      , T_OR       },
        { "NOT"     , T_NOT      },
        { "RETURN"  , T_RETURN   },
        { "PRINT"   , T_PRINT    },
        { "ASM"     , T_ASM      },
        { "INCLUDE" , T_INCLUDE  },
        { NULL      , 0          }
};

// local function prototypes
static int GetToken1(ParseContext_t *c);
static int IdentifierToken(ParseContext_t *c, int ch);
static int IdentifierCharP(int ch);
static int NumberToken(ParseContext_t *c, int ch);
static int HexNumberToken(ParseContext_t *c);
static int BinaryNumberToken(ParseContext_t *c);
static int StringToken(ParseContext_t *c);
static int CharToken(ParseContext_t *c);
static int LiteralChar(ParseContext_t *c);
static int SkipComment(ParseContext_t *c);
static int XGetC(ParseContext_t *c);

// InitScan - initialize the scanner
void InitScan(ParseContext_t *c) {
    c->inComment = VMFALSE;
    c->savedToken = T_NONE;
}

// FRequire - fetch a token and check it
void FRequire(ParseContext_t *c, int requiredToken) {
    Require(c, GetToken(c), requiredToken);
}

// Require - check for a required token
void Require(ParseContext_t *c, int token, int requiredToken) {
    char tknbuf[MAXTOKEN];
    if (token != requiredToken) {
        strcpy(tknbuf, TokenName(requiredToken));
        ParseError(c, "Expecting '%s', found '%s'", tknbuf, TokenName(token));
    }
}

// GetToken - get the next token
int GetToken(ParseContext_t *c) {
    int tkn;

    // check for a saved token
    if ((tkn = c->savedToken) != T_NONE)
        c->savedToken = T_NONE;
    
    // otherwise, get the next token
    else
        tkn = GetToken1(c);

    // return the token
    return tkn;
}

// SaveToken - save a token
void SaveToken(ParseContext_t *c, int token) {
    c->savedToken = token;
}

// TokenName - get the name of a token
char* TokenName(int token) {
    static char nameBuf[4];
    char *name;

    switch (token) {
        case T_NONE:
            name = "<NONE>";
            break;
        case T_REM:
            case T_DIM:
            case T_FUNCTION:
            case T_SUB:
            case T_AS:
            case T_LET:
            case T_IF:
            case T_THEN:
            case T_ELSE:
            case T_END:
            case T_FOR:
            case T_TO:
            case T_STEP:
            case T_NEXT:
            case T_DO:
            case T_WHILE:
            case T_UNTIL:
            case T_LOOP:
            case T_GOTO:
            case T_MOD:
            case T_AND:
            case T_OR:
            case T_NOT:
            case T_RETURN:
            case T_PRINT:
            name = ktab[token - T_REM].keyword;
            break;
        case T_END_FUNCTION:
            name = "END FUNCTION";
            break;
        case T_END_SUB:
            name = "END SUB";
            break;
        case T_END_IF:
            name = "END IF";
            break;
        case T_END_ASM:
            name = "END ASM";
            break;
        case T_DO_WHILE:
            name = "DO WHILE";
            break;
        case T_DO_UNTIL:
            name = "DO UNTIL";
            break;
        case T_LOOP_WHILE:
            name = "LOOP WHILE";
            break;
        case T_LOOP_UNTIL:
            name = "LOOP UNTIL";
            break;
        case T_LE:
            name = "<=";
            break;
        case T_NE:
            name = "<>";
            break;
        case T_GE:
            name = ">=";
            break;
        case T_SHL:
            name = "<<";
            break;
        case T_SHR:
            name = ">>";
            break;
        case T_IDENTIFIER:
            name = "<IDENTIFIER>";
            break;
        case T_INTNUMBER:
            name = "<INTNUMBER>";
            break;
        case T_UINTNUMBER:
            name = "<UINTNUMBER>";
            break;
        case T_FLOATNUMBER:
            name = "<FLOATNUMBER>";
            break;
        case T_STRING:
            name = "<STRING>";
            break;
        case T_EOL:
            name = "<EOL>";
            break;
        case T_EOF:
            name = "<EOF>";
            break;
        default:
            nameBuf[0] = '\'';
            nameBuf[1] = token;
            nameBuf[2] = '\'';
            nameBuf[3] = '\0';
            name = nameBuf;
            break;
    }

    // return the token name
    return name;
}

// GetToken1 - read the next token
static int GetToken1(ParseContext_t *c) {
    int ch, tkn;
    
    // skip leading blanks
    ch = SkipSpaces(c);

    // remember the start of the current token
    c->tokenOffset = (int) (c->sys_line->linePtr - c->sys_line->lineBuf);

    // check the next character
    switch (ch) {
        case EOF:
            tkn = T_EOL;
            break;
        case '"':
            tkn = StringToken(c);
            break;
        case '\'':
            tkn = CharToken(c);
            break;
        case '<':
            if ((ch = GetChar(c)) == '=')
                tkn = T_LE;
            else if (ch == '>')
                tkn = T_NE;
            else if (ch == '<')
                tkn = T_SHL;
            else {
                UngetC(c);
                tkn = '<';
            }
            break;
        case '>':
            if ((ch = GetChar(c)) == '=')
                tkn = T_GE;
            else if (ch == '>')
                tkn = T_SHR;
            else {
                UngetC(c);
                tkn = '>';
            }
            break;
        case '0':
            switch (GetChar(c)) {
                case 'x':
                    case 'X':
                    tkn = HexNumberToken(c);
                    break;
                case 'b':
                    case 'B':
                    tkn = BinaryNumberToken(c);
                    break;
                default:
                    UngetC(c);
                    tkn = NumberToken(c, '0');
                    break;
            }
            break;
        default:
            if (isdigit(ch))
                tkn = NumberToken(c, ch);
            else if (IdentifierCharP(ch)) {
                char *savePtr;
                switch (tkn = IdentifierToken(c, ch)) {
                    case T_ELSE:
                        savePtr = c->sys_line->linePtr;
                        if ((ch = SkipSpaces(c)) != EOF && IdentifierCharP(ch)) {
                            switch (IdentifierToken(c, ch)) {
                                case T_IF:
                                    tkn = T_ELSE_IF;
                                    break;
                                default:
                                    c->sys_line->linePtr = savePtr;
                                    break;
                            }
                        }
                        else
                            c->sys_line->linePtr = savePtr;
                        break;
                    case T_END:
                        savePtr = c->sys_line->linePtr;
                        if ((ch = SkipSpaces(c)) != EOF && IdentifierCharP(ch)) {
                            switch (IdentifierToken(c, ch)) {
                                case T_FUNCTION:
                                    tkn = T_END_FUNCTION;
                                    break;
                                case T_SUB:
                                    tkn = T_END_SUB;
                                    break;
                                case T_IF:
                                    tkn = T_END_IF;
                                    break;
                                case T_ASM:
                                    tkn = T_END_ASM;
                                    break;
                                default:
                                    c->sys_line->linePtr = savePtr;
                                    break;
                            }
                        }
                        else
                            c->sys_line->linePtr = savePtr;
                        break;
                    case T_DO:
                        savePtr = c->sys_line->linePtr;
                        if ((ch = SkipSpaces(c)) != EOF && IdentifierCharP(ch)) {
                            switch (IdentifierToken(c, ch)) {
                                case T_WHILE:
                                    tkn = T_DO_WHILE;
                                    break;
                                case T_UNTIL:
                                    tkn = T_DO_UNTIL;
                                    break;
                                default:
                                    c->sys_line->linePtr = savePtr;
                                    break;
                            }
                        }
                        else
                            c->sys_line->linePtr = savePtr;
                        break;
                    case T_LOOP:
                        savePtr = c->sys_line->linePtr;
                        if ((ch = SkipSpaces(c)) != EOF && IdentifierCharP(ch)) {
                            switch (IdentifierToken(c, ch)) {
                                case T_WHILE:
                                    tkn = T_LOOP_WHILE;
                                    break;
                                case T_UNTIL:
                                    tkn = T_LOOP_UNTIL;
                                    break;
                                default:
                                    c->sys_line->linePtr = savePtr;
                                    break;
                            }
                        }
                        else
                            c->sys_line->linePtr = savePtr;
                        break;
                }
            }
            else
                tkn = ch;
    }

    // return the token
    return tkn;
}

// IdentifierToken - get an identifier
static int IdentifierToken(ParseContext_t *c, int ch) {
    int len, i;
    char *p;

    // get the identifier
    p = c->token;
    *p++ = ch;
    len = 1;
    while ((ch = GetChar(c)) != EOF && IdentifierCharP(ch)) {
        if (++len > MAXTOKEN)
            ParseError(c, "Identifier too long");
        *p++ = ch;
    }
    UngetC(c);
    *p = '\0';

    // check to see if it is a keyword
    for (i = 0; ktab[i].keyword != NULL; ++i)
        if (strcasecmp(ktab[i].keyword, c->token) == 0)
            return ktab[i].token;

    // otherwise, it is an identifier
    return T_IDENTIFIER;
}

// IdentifierCharP - is this an identifier character?
static int IdentifierCharP(int ch) {
    return isupper(ch)
            || islower(ch)
            || isdigit(ch)
            || strchr("$%_", ch) != NULL;
}

// NumberToken - get a number
static int NumberToken(ParseContext_t *c, int ch) {
    char *p = c->token;

    // get the number
    *p++ = ch;
    while ((ch = GetChar(c)) != EOF) {
        if (isdigit(ch))
            *p++ = ch;
        else if (ch != '_')
            break;
    }
    UngetC(c);
    *p = '\0';
    
    // convert the string to an integer
    c->tokenValue = (VMVALUE) atol(c->token);
    
    // return the token
    return T_INTNUMBER;
}

// HexNumberToken - get a hexadecimal number
static int HexNumberToken(ParseContext_t *c) {
    char *p = c->token;
    int ch;

    // get the number
    while ((ch = GetChar(c)) != EOF) {
        if (isxdigit(ch))
            *p++ = ch;
        else if (ch != '_')
            break;
    }
    UngetC(c);
    *p = '\0';
    
    // convert the string to an integer
    c->tokenValue = (VMVALUE) strtoul(c->token, NULL, 16);
    
    // return the token
    return T_UINTNUMBER;
}

// BinaryNumberToken - get a binary number
static int BinaryNumberToken(ParseContext_t *c) {
    char *p = c->token;
    int ch;

    // get the number
    while ((ch = GetChar(c)) != EOF) {
        if (ch == '0' || ch == '1')
            *p++ = ch;
        else if (ch != '_')
            break;
    }
    UngetC(c);
    *p = '\0';
    
    // convert the string to an integer
    c->tokenValue = (VMVALUE) strtoul(c->token, NULL, 2);
    
    // return the token
    return T_UINTNUMBER;
}

// StringToken - get a string
static int StringToken(ParseContext_t *c) {
    int ch, len;
    char *p;

    // collect the string
    p = c->token;
    len = 0;
    while ((ch = XGetC(c)) != EOF && ch != '"') {
        if (++len > MAXTOKEN)
            ParseError(c, "String too long");
        *p++ = (ch == '\\' ? LiteralChar(c) : ch);
    }
    *p = '\0';

    // check for premature end of file
    if (ch != '"')
        ParseError(c, "unterminated string");

    // return the token
    return T_STRING;
}

// CharToken - get a character constant
static int CharToken(ParseContext_t *c) {
    int ch = LiteralChar(c);
    if (XGetC(c) != '\'')
        ParseError(c, "Expecting a closing single quote");
    c->token[0] = ch;
    c->token[1] = '\0';
    c->tokenValue = ch;
    return T_INTNUMBER;
}

// LiteralChar - get a character from a literal string
static int LiteralChar(ParseContext_t *c) {
    int ch;
    switch (ch = XGetC(c)) {
        case 'n':
            ch = '\n';
            break;
        case 'r':
            ch = '\r';
            break;
        case 't':
            ch = '\t';
            break;
        case EOF:
            ch = '\\';
            break;
    }
    return ch;
}

// SkipSpaces - skip leading spaces and the the next non-blank character
int SkipSpaces(ParseContext_t *c) {
    int ch;
    while ((ch = GetChar(c)) != EOF)
        if (!isspace(ch))
            break;
    return ch;
}

// SkipComment - skip characters up to the end of a comment
static int SkipComment(ParseContext_t *c) {
    int lastch, ch;
    lastch = '\0';
    while ((ch = XGetC(c)) != EOF) {
        if (lastch == '*' && ch == '/')
            return VMTRUE;
        lastch = ch;
    }
    return VMFALSE;
}

// GetChar - get the next character
int GetChar(ParseContext_t *c) {
    int ch;

    // check for being in a comment
    if (c->inComment) {
        if (!SkipComment(c))
            return EOF;
        c->inComment = VMFALSE;
    }

    // loop until we find a non-comment character
    for (;;) {
        
        // get the next character
        if ((ch = XGetC(c)) == EOF)
            return EOF;

        // check for a comment
        else if (ch == '/') {
            if ((ch = XGetC(c)) == '/') {
                while ((ch = XGetC(c)) != EOF)
                    ;
            }
            else if (ch == '*') {
                if (!SkipComment(c)) {
                    c->inComment = VMTRUE;
                    return EOF;
                }
            }
            else {
                UngetC(c);
                ch = '/';
                break;
            }
        }

        // handle a normal character
        else
            break;
    }

    // return the character
    return ch;
}

// XGetC - get the next character without checking for comments
static int XGetC(ParseContext_t *c) {
    int ch;
    
    // get the next character on the current line
    if (!(ch = *c->sys_line->linePtr++)) {
        --c->sys_line->linePtr;
        return EOF;
    }
    
    // return the character
    return ch;
}

// UngetC - unget the most recent character
void UngetC(ParseContext_t *c) {
    // backup the input pointer
    --c->sys_line->linePtr;
}

// ParseError - report a parsing error
void ParseError(ParseContext_t *c, const char *fmt, ...) {
    va_list ap;

    // print the error message
    va_start(ap, fmt);
    vm_printf("error: ");
    vm_printf(fmt, ap);
    vm_putchar('\n');
    va_end(ap);

    // show the context
    vm_printf("  line %d\n", c->lineNumber);
    vm_printf("    %s", c->sys_line->lineBuf);
    vm_printf("    %*s^\n", c->tokenOffset, "");

    // exit until we fix the compiler so it can recover from parse errors
    longjmp(c->sys->errorTarget, 1);
}
