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

// Common
#define VMTRUE      1
#define VMFALSE     0

// system heap size (includes compiler heap and image buffer)
#ifndef HEAPSIZE
#define HEAPSIZE        5000
#endif

// size of image buffer (allocated from system heap)
#ifndef IMAGESIZE
#define IMAGESIZE       2500
#endif

// edit buffer size (separate from the system heap)
#ifndef EDITBUFSIZE
#define EDITBUFSIZE     1500
#endif

#define VMVALUE         int32_t
#define VMUVALUE        uint32_t

#define VMCODEBYTE(p)   *(uint8_t *)(p)
#define VMINTRINSIC(i)  intrinsics[i]

#define ALIGN_MASK      (sizeof(VMVALUE) - 1)

#endif
