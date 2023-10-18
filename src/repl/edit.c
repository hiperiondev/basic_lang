/*
 * @edit.c
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
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#include "system.h"
#include "edit.h"
#include "compile.h"
#include "vmsystem.h"
#include "vm.h"

#define MAXTOKEN  32

typedef struct {
     int lineNumber;
     int length;
    char text[1];
} Line_t;

typedef struct {
     vm_context_t *sys;
    System_line_t *sys_line;
             char programName[FILENAME_MAX];
          uint8_t *buffer;
          uint8_t *bufferTop;
           Line_t *currentLine;
} EditBuf_t;

// command handlers
static void DoNew(EditBuf_t *buf);
static void DoList(EditBuf_t *buf);
static void DoRun(EditBuf_t *buf);
static void DoRenum(EditBuf_t *buf);
static void DoLoad(EditBuf_t *buf);
static void DoSave(EditBuf_t *buf);
static void DoCat(EditBuf_t *buf);
static void DoHelp(EditBuf_t *buf);

// command table
static struct {
    char *name;
    void (*handler)(EditBuf_t *buf);
    char *help;
} cmds[] = {
        { "NEW"  , DoNew  , "create new file" },
        { "LIST" , DoList , "list loaded program" },
        { "RUN"  , DoRun  , "run loaded file" },
        { "RENUM", DoRenum, "renumber program lines" },
        { "LOAD" , DoLoad , "load file" },
        { "SAVE" , DoSave , "save program" },
        { "CAT"  , DoCat  , "show file content" },
        { "HELP" , DoHelp , "this help" },
        { NULL   , NULL   , NULL }
};

// prototypes
static char* NextToken(System_line_t *sys);
static int ParseNumber(char *token, int *pValue);
static int IsBlank(char *p);
static int SetProgramName(EditBuf_t *buf);

// edit buffer prototypes
static EditBuf_t* BufInit(vm_context_t *sys);
static void BufNew(EditBuf_t *buf);
static int BufAddLineN(EditBuf_t *buf, int lineNumber, const char *text);
static int BufDeleteLineN(EditBuf_t *buf, int lineNumber);
static int BufSeekN(EditBuf_t *buf, int lineNumber);
static char* BufGetLine(EditBuf_t *buf, int *pLineNumber, char *text);
static int FindLineN(EditBuf_t *buf, int lineNumber, Line_t **pLine);

void edit_workspace(vm_context_t *sys, System_line_t *sys_line) {
    EditBuf_t *editBuf;
    int lineNumber;
    char *token;
    
    if (!(editBuf = BufInit(sys)))
        vm_system_abort(sys, "insufficient memory for edit buffer");

    while (system_get_line(sys_line, &lineNumber)) {

        if ((token = NextToken(sys_line)) != NULL) {
            if (ParseNumber(token, &lineNumber)) {
                if (IsBlank(sys_line->linePtr)) {
                    if (!BufDeleteLineN(editBuf, lineNumber))
                        vm_printf("no line %d\n", lineNumber);
                }
                else {
                    if (isspace(*sys_line->linePtr))
                        ++sys_line->linePtr;
                    if (!BufAddLineN(editBuf, lineNumber, sys_line->linePtr))
                        vm_printf("out of edit buffer space\n");
                }
            }

            else {
                int i;
                for (i = 0; cmds[i].name != NULL; ++i)
                    if (strcasecmp(token, cmds[i].name) == 0)
                        break;
                if (cmds[i].handler) {
                    editBuf->sys_line = sys_line;
                    (*cmds[i].handler)(editBuf);
                    vm_printf("OK\n");
                }
                else {
                    vm_printf("Unknown command: %s\n", token);
                }
            }
        }
    }
}

static void DoHelp(EditBuf_t *buf) {
    for (uint32_t i = 0; cmds[i].name != NULL; ++i)
        printf("%s: %s\n", cmds[i].name, cmds[i].help);
}

static void DoNew(EditBuf_t *buf) {
    SetProgramName(buf);
    BufNew(buf);
}

static void DoList(EditBuf_t *buf) {
    int lineNumber;
    BufSeekN(buf, 0);
    while (BufGetLine(buf, &lineNumber, buf->sys_line->lineBuf))
        vm_printf("%d %s", lineNumber, buf->sys_line->lineBuf);
}

static char* EditGetLine(char *buf, int len, int *pLineNumber, void *cookie) {
    EditBuf_t *editBuf = (EditBuf_t*) cookie;
    return BufGetLine(editBuf, pLineNumber, buf);
}

static void DoRun(EditBuf_t *buf) {
    vm_context_t *sys = buf->sys;
    System_line_t *sys_line = buf->sys_line;
    ParseContext_t *c;
    GetLineHandler *getLine;
    void *getLineCookie;
    vm_t *i;
    uint32_t mainCode;
    
    sys->nextHigh = buf->buffer;
    sys->nextLow = sys->freeSpace;

    if (!(c = InitCompileContext(sys)))
        vm_printf("insufficient memory");
    
    system_get_main_source(sys_line, &getLine, &getLineCookie);
    
    system_set_main_source(sys_line, EditGetLine, buf);
    BufSeekN(buf, 0);

    c->sys_line = buf->sys_line;
    mainCode = Compile(c);

    if (!(i = vm_init(c->g->codeBuf, c->g->code_len, 1024, false)))
        vm_printf("insufficient memory");
    else {
        vm_execute(i, mainCode);
        vm_deinit(i);
    }

    system_set_main_source(sys_line, getLine, getLineCookie);
}

static void DoRenum(EditBuf_t *buf) {
    uint8_t *p = buf->buffer;
    int lineNumber = 100;
    int increment = 10;
    while (p < buf->bufferTop) {
        Line_t *line = (Line_t*) p;
        line->lineNumber = lineNumber;
        lineNumber += increment;
        p = p + line->length;
    }
}

static int SetProgramName(EditBuf_t *buf) {
    char *name;
    if ((name = NextToken(buf->sys_line)) != NULL) {
        strncpy(buf->programName, name, FILENAME_MAX - 1);
        buf->programName[FILENAME_MAX - 1] = '\0';
        if (!strchr(buf->programName, '.')) {
            if (strlen(buf->programName) < FILENAME_MAX - 5)
                strcat(buf->programName, ".bas");
        }
    }
    return buf->programName[0] != '\0';
}

static void DoLoad(EditBuf_t *buf) {
    int lineNumber = 100;
    int lineNumberIncrement = 10;
    VMFILE *fp;
    
    // check for a program name on the command line
    if (!SetProgramName(buf)) {
        vm_printf("expecting a file name\n");
        return;
    }
    
    // load the program
    if (!(fp = VM_fopen(buf->programName, "r")))
        vm_printf("error loading '%s'\n", buf->programName);
    else {
        System_line_t *sys_line = buf->sys_line;;
        vm_printf("Loading '%s'\n", buf->programName);
        BufNew(buf);
        while (VM_fgets(sys_line->lineBuf, sizeof(sys_line->lineBuf), fp) != NULL) {
            BufAddLineN(buf, lineNumber, sys_line->lineBuf);
            lineNumber += lineNumberIncrement;
        }
        VM_fclose(fp);
    }
}

static void DoSave(EditBuf_t *buf) {
    VMFILE *fp;
    
    // check for a program name on the command line
    if (!SetProgramName(buf)) {
        vm_printf("expecting a file name\n");
        return;
    }
    
    // save the program
    if (!(fp = VM_fopen(buf->programName, "w")))
        vm_printf("error saving '%s'\n", buf->programName);
    else {
        System_line_t *sys_line = buf->sys_line;
        int lineNumber;
        vm_printf("Saving '%s'\n", buf->programName);
        BufSeekN(buf, 0);
        while (BufGetLine(buf, &lineNumber, sys_line->lineBuf))
            VM_fputs(sys_line->lineBuf, fp);
        VM_fclose(fp);
    }
}

static void DoCat(EditBuf_t *buf) {
    VMDIRENT_t entry;
    VMDIR_t dir;
    if (system_fs_opendir(".", &dir) == 0) {
        while (system_fs_readdir(&dir, &entry) == 0) {
            int len = strlen(entry.name);
            if (len >= 4 && strcasecmp(&entry.name[len - 4], ".bas") == 0)
                vm_printf("  %s\n", entry.name);
        }
        system_fs_closedir(&dir);
    }
}

static char* NextToken(System_line_t *sys) {
    static char token[MAXTOKEN];
    int ch, i;
    
    // skip leading spaces
    while ((ch = *sys->linePtr) != '\0' && isspace(ch))
        ++sys->linePtr;

    // collect a token until the next non-space
    for (i = 0; (ch = *sys->linePtr) != '\0' && !isspace(ch); ++sys->linePtr)
        if (i < sizeof(token) - 1)
            token[i++] = ch;
    token[i] = '\0';
    
    return token[0] == '\0' ? NULL : token;
}

static int ParseNumber(char *token, int *pValue) {
    int ch;
    *pValue = 0;
    while ((ch = *token++) != '\0' && isdigit(ch))
        *pValue = *pValue * 10 + ch - '0';
    return ch == '\0';
}

static int IsBlank(char *p) {
    int ch;
    while ((ch = *p++) != '\0')
        if (!isspace(ch))
            return VMFALSE;
    return VMTRUE;
}

static EditBuf_t* BufInit(vm_context_t *sys) {
    EditBuf_t *buf;
    if (!(buf = (EditBuf_t*) system_allocate_high_memory(sys, sizeof(EditBuf_t))))
        return NULL;
    memset(buf, 0, sizeof(EditBuf_t));
    buf->sys = sys;
    buf->bufferTop = sys->nextHigh;
    buf->buffer = buf->bufferTop;
    buf->currentLine = (Line_t*) buf->buffer;
    return buf;
}

static void BufNew(EditBuf_t *buf) {
    buf->buffer = buf->bufferTop;
    buf->currentLine = (Line_t*) buf->buffer;
}

static int BufAddLineN(EditBuf_t *buf, int lineNumber, const char *text) {
    int newLength = sizeof(Line_t) + strlen(text);
    int spaceNeeded;
    //uint8_t *next;
    Line_t *line;

    // make sure the length is a multiple of the word size
    newLength = (newLength + ALIGN_MASK) & ~ALIGN_MASK;

    // replace an existing line
    if (FindLineN(buf, lineNumber, &line)) {
        //next = (uint8_t*) line + line->length;
        spaceNeeded = newLength - line->length;
    }

    // insert a new line
    else {
        //next = (uint8_t*) line;
        spaceNeeded = newLength;
    }

    // make sure there is enough space
    if (buf->buffer - spaceNeeded < buf->sys->nextLow)
        return VMFALSE;

    // make space for the new line
    memmove(buf->buffer - spaceNeeded, buf->buffer, (uint8_t*) line - buf->buffer);
    buf->buffer -= spaceNeeded;
    
    // adjust the line start
    line = (Line_t*) ((uint8_t*) line - spaceNeeded);

    // insert the new line
    if (newLength > 0) {
        line->lineNumber = lineNumber;
        line->length = newLength;
        strcpy(line->text, text);
    }

    // return successfully
    return VMTRUE;
}

static int BufDeleteLineN(EditBuf_t *buf, int lineNumber) {
    int spaceFreed;
    Line_t *line;

    // find the line to delete
    if (!FindLineN(buf, lineNumber, &line))
        return VMFALSE;

    // get a pointer to the next line
    spaceFreed = line->length;

    // remove the line to be deleted
    memmove(buf->buffer + spaceFreed, buf->buffer, (uint8_t*) line - buf->buffer);
    buf->buffer += spaceFreed;

    // return successfully
    return VMTRUE;
}

static int BufSeekN(EditBuf_t *buf, int lineNumber) {
    // if the line number is zero start at the first line
    if (lineNumber == 0)
        buf->currentLine = (Line_t*) buf->buffer;

    // otherwise, start at the specified line
    else if (!FindLineN(buf, lineNumber, &buf->currentLine))
        return VMFALSE;

    // return successfully
    return VMTRUE;
}

static char* BufGetLine(EditBuf_t *buf, int *pLineNumber, char *text) {
    // check for the end of the buffer
    if ((uint8_t*) buf->currentLine >= buf->bufferTop)
        return NULL;

    // get the current line
    *pLineNumber = buf->currentLine->lineNumber;
    strcpy(text, buf->currentLine->text);

    // move ahead to the next line
    buf->currentLine = (Line_t*) ((char*) buf->currentLine + buf->currentLine->length);

    // return successfully
    return text;
}

static int FindLineN(EditBuf_t *buf, int lineNumber, Line_t **pLine) {
    uint8_t *p = buf->buffer;
    while (p < buf->bufferTop) {
        Line_t *line = (Line_t*) p;
        if (lineNumber <= line->lineNumber) {
            *pLine = line;
            return lineNumber == line->lineNumber;
        }
        p = p + line->length;
    }
    *pLine = (Line_t*) p;
    return VMFALSE;
}
