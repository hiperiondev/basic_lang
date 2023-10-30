/*
 * @optimize.c
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

#ifndef OPTIMIZE_H_
#define OPTIMIZE_H_

#include "compile.h"

void optimize(ParseContext_t *c, bool dump);

#endif /* OPTIMIZE_H_ */
