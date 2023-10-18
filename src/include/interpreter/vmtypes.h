/*
 * @vmtypes.h
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

#ifndef __VMTYPES_H__
#define __VMTYPES_H__

#include <stdint.h>

#define VMTRUE      1
#define VMFALSE     0

#define VMVALUE         int32_t
#define VMUVALUE        uint32_t

#define VMCODEBYTE(p)   *(uint8_t *)(p)
#define VMINTRINSIC(i)  vm_intrinsics[i]

#define ALIGN_MASK      (sizeof(VMVALUE) - 1)

#endif
