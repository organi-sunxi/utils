/*
   LZ4 - common define
   Copyright (C) 2011-2013, Yann Collet.
   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - LZ4 homepage : http://fastcompression.blogspot.com/p/lz4.html
   - LZ4 source repository : http://code.google.com/p/lz4/
*/


//**************************************
// Tuning parameters
//**************************************
// MEMORY_USAGE :
// Memory usage formula : N->2^N Bytes (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
// Increasing memory usage improves compression ratio
// Reduced memory usage can improve speed, due to cache effect
// Default value is 14, for 16KB, which nicely fits into Intel x86 L1 cache
#define MEMORY_USAGE 14

// HEAPMODE :
// Select how default compression function will allocate memory for its hash table,
// in memory stack (0:default, fastest), or in memory heap (1:requires memory allocation (malloc)).
// Default allocation strategy is to use stack (HEAPMODE 0)
// Note : explicit functions *_stack* and *_heap* are unaffected by this setting
#define HEAPMODE 0

// BIG_ENDIAN_NATIVE_BUT_INCOMPATIBLE :
// This will provide a small boost to performance for big endian cpu, but the resulting compressed stream will be incompatible with little-endian CPU.
// You can set this option to 1 in situations where data will remain within closed environment
// This option is useless on Little_Endian CPU (such as x86)
//#define BIG_ENDIAN_NATIVE_BUT_INCOMPATIBLE 1



//**************************************
// CPU Feature Detection
//**************************************
// 32 or 64 bits ?
#if (defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) \
  || defined(__ppc64__) || defined(_WIN64) || defined(__LP64__) || defined(_LP64) \
  || defined(__ia64__) )   // Detects 64 bits mode
#  define LZ4_ARCH64 1
#else
#  define LZ4_ARCH64 0
#endif

// Little Endian or Big Endian ?
// Overwrite the #define below if you know your architecture endianess
#if defined (__GLIBC__)
#  include <endian.h>
#  if (__BYTE_ORDER == __BIG_ENDIAN)
#     define LZ4_BIG_ENDIAN 1
#  endif
#elif (defined(__BIG_ENDIAN__) || defined(__BIG_ENDIAN) || defined(_BIG_ENDIAN)) && !(defined(__LITTLE_ENDIAN__) || defined(__LITTLE_ENDIAN) || defined(_LITTLE_ENDIAN))
#  define LZ4_BIG_ENDIAN 1
#elif defined(__sparc) || defined(__sparc__) \
   || defined(__ppc__) || defined(_POWER) || defined(__powerpc__) || defined(_ARCH_PPC) || defined(__PPC__) || defined(__PPC) || defined(PPC) || defined(__powerpc__) || defined(__powerpc) || defined(powerpc) \
   || defined(__hpux)  || defined(__hppa) \
   || defined(_MIPSEB) || defined(__s390__)
#  define LZ4_BIG_ENDIAN 1
#else
// Little Endian assumed. PDP Endian and other very rare endian format are unsupported.
#endif

// Unaligned memory access is automatically enabled for "common" CPU, such as x86.
// For others CPU, the compiler will be more cautious, and insert extra code to ensure aligned access is respected
// If you know your target CPU supports unaligned memory access, you want to force this option manually to improve performance
#if defined(__ARM_FEATURE_UNALIGNED)
#  define LZ4_FORCE_UNALIGNED_ACCESS 1
#endif

// Define this parameter if your target system or compiler does not support hardware bit count
#if defined(_MSC_VER) && defined(_WIN32_WCE)            // Visual Studio for Windows CE does not support Hardware bit count
#  define LZ4_FORCE_SW_BITCOUNT
#endif


//**************************************
// Compiler Options
//**************************************
#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   // C99
/* "restrict" is a known keyword */
#else
#  define restrict // Disable restrict
#endif

#define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#ifdef _MSC_VER
#  define inline __inline             // Visual is not C99, but supports some kind of inline
#  define forceinline __forceinline   
#  include <intrin.h>                 // For Visual 2005
#  if LZ4_ARCH64	// 64-bit
#    pragma intrinsic(_BitScanForward64) // For Visual 2005
#    pragma intrinsic(_BitScanReverse64) // For Visual 2005
#  else
#    pragma intrinsic(_BitScanForward)   // For Visual 2005
#    pragma intrinsic(_BitScanReverse)   // For Visual 2005
#  endif
#  pragma warning(disable : 4127)        // disable: C4127: conditional expression is constant
#  pragma warning(disable : 4701)        // disable: C4701: potentially uninitialized local variable used
#else 
#  ifdef __GNUC__
#    define forceinline inline __attribute__((always_inline))
#  else
#    define forceinline inline
#  endif
#endif

#ifdef _MSC_VER  // Visual Studio
#define lz4_bswap16(x) _byteswap_ushort(x)
#else
#define lz4_bswap16(x)  ((unsigned short int) ((((x) >> 8) & 0xffu) | (((x) & 0xffu) << 8)))
#endif

#if (GCC_VERSION >= 302) || (__INTEL_COMPILER >= 800) || defined(__clang__)
#  define expect(expr,value)    (__builtin_expect ((expr),(value)) )
#else
#  define expect(expr,value)    (expr)
#endif

#define likely(expr)     expect((expr) != 0, 1)
#define unlikely(expr)   expect((expr) != 0, 0)


//**************************************
// Includes
//**************************************
#include <stdlib.h>   // for malloc
#include <string.h>   // for memset

//**************************************
// Memory routines
//**************************************
#define ALLOCATOR(s)  calloc(1,s)
#define FREEMEM       free
#define MEM_INIT      memset

#include "lz4.h"


//**************************************
// Basic Types
//**************************************
#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   // C99
# include <stdint.h>
  typedef uint8_t  BYTE;
  typedef uint16_t U16;
  typedef uint32_t U32;
  typedef  int32_t S32;
  typedef uint64_t U64;
#else
  typedef unsigned char       BYTE;
  typedef unsigned short      U16;
  typedef unsigned int        U32;
  typedef   signed int        S32;
  typedef unsigned long long  U64;
#endif

#if defined(__GNUC__)  && !defined(LZ4_FORCE_UNALIGNED_ACCESS)
#  define _PACKED __attribute__ ((packed))
#else
#  define _PACKED
#endif

#if !defined(LZ4_FORCE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  pragma pack(push, 1)
#endif

typedef struct _U16_S { U16 v; } _PACKED U16_S;
typedef struct _U32_S { U32 v; } _PACKED U32_S;
typedef struct _U64_S { U64 v; } _PACKED U64_S;

#if !defined(LZ4_FORCE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  pragma pack(pop)
#endif

#define A64(x) (((U64_S *)(x))->v)
#define A32(x) (((U32_S *)(x))->v)
#define A16(x) (((U16_S *)(x))->v)


//**************************************
// Constants
//**************************************
#define HASHTABLESIZE (1 << MEMORY_USAGE)

#define MINMATCH 4

#define COPYLENGTH 8
#define LASTLITERALS 5
#define MFLIMIT (COPYLENGTH+MINMATCH)
#define MINLENGTH (MFLIMIT+1)

#define LZ4_64KLIMIT ((1<<16) + (MFLIMIT-1))
#define SKIPSTRENGTH 6     // Increasing this value will make the compression run slower on incompressible data

#define DICTIONARY_LOGSIZE 16
#define MAXD (1<<DICTIONARY_LOGSIZE)
#define MAXD_MASK ((U32)(MAXD - 1))
#define MAX_DISTANCE (MAXD - 1)

#define ML_BITS  4
#define ML_MASK  ((1U<<ML_BITS)-1)
#define RUN_BITS (8-ML_BITS)
#define RUN_MASK ((1U<<RUN_BITS)-1)


//**************************************
// Architecture-specific macros
//**************************************
#if LZ4_ARCH64	// 64-bit
#  define STEPSIZE 8
#  define UARCH U64
#  define AARCH A64
#  define LZ4_COPYSTEP(s,d)       A64(d) = A64(s); d+=8; s+=8;
#  define LZ4_COPYPACKET(s,d)     LZ4_COPYSTEP(s,d)
#  define LZ4_SECURECOPY(s,d,e)   if (d<e) LZ4_WILDCOPY(s,d,e)
#  define HTYPE                   U32
#  define INITBASE(base)          const BYTE* const base = ip
#else		// 32-bit
#  define STEPSIZE 4
#  define UARCH U32
#  define AARCH A32
#  define LZ4_COPYSTEP(s,d)       A32(d) = A32(s); d+=4; s+=4;
#  define LZ4_COPYPACKET(s,d)     LZ4_COPYSTEP(s,d); LZ4_COPYSTEP(s,d);
#  define LZ4_SECURECOPY          LZ4_WILDCOPY
#  define HTYPE                   const BYTE*
#  define INITBASE(base)          const int base = 0
#endif

#if (defined(LZ4_BIG_ENDIAN) && !defined(BIG_ENDIAN_NATIVE_BUT_INCOMPATIBLE))
#  define LZ4_READ_LITTLEENDIAN_16(d,s,p) { U16 v = A16(p); v = lz4_bswap16(v); d = (s) - v; }
#  define LZ4_WRITE_LITTLEENDIAN_16(p,i)  { U16 v = (U16)(i); v = lz4_bswap16(v); A16(p) = v; p+=2; }
#else		// Little Endian
#  define LZ4_READ_LITTLEENDIAN_16(d,s,p) { d = (s) - A16(p); }
#  define LZ4_WRITE_LITTLEENDIAN_16(p,v)  { A16(p) = v; p+=2; }
#endif


//**************************************
// Macros
//**************************************
#define LZ4_WILDCOPY(s,d,e)     do { LZ4_COPYPACKET(s,d) } while (d<e);
#define LZ4_BLINDCOPY(s,d,l)    { BYTE* e=(d)+(l); LZ4_WILDCOPY(s,d,e); d=e; }


//****************************
// Private functions
//****************************
#if LZ4_ARCH64

static inline int LZ4_NbCommonBytes (register U64 val)
{
#if defined(LZ4_BIG_ENDIAN)
    #if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
    unsigned long r = 0;
    _BitScanReverse64( &r, val );
    return (int)(r>>3);
    #elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
    return (__builtin_clzll(val) >> 3);
    #else
    int r;
    if (!(val>>32)) { r=4; } else { r=0; val>>=32; }
    if (!(val>>16)) { r+=2; val>>=8; } else { val>>=24; }
    r += (!val);
    return r;
    #endif
#else
    #if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
    unsigned long r = 0;
    _BitScanForward64( &r, val );
    return (int)(r>>3);
    #elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
    return (__builtin_ctzll(val) >> 3);
    #else
    static const int DeBruijnBytePos[64] = { 0, 0, 0, 0, 0, 1, 1, 2, 0, 3, 1, 3, 1, 4, 2, 7, 0, 2, 3, 6, 1, 5, 3, 5, 1, 3, 4, 4, 2, 5, 6, 7, 7, 0, 1, 2, 3, 3, 4, 6, 2, 6, 5, 5, 3, 4, 5, 6, 7, 1, 2, 4, 6, 4, 4, 5, 7, 2, 6, 5, 7, 6, 7, 7 };
    return DeBruijnBytePos[((U64)((val & -val) * 0x0218A392CDABBD3F)) >> 58];
    #endif
#endif
}

#else

static inline int LZ4_NbCommonBytes (register U32 val)
{
#if defined(LZ4_BIG_ENDIAN)
#  if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
    unsigned long r = 0;
    _BitScanReverse( &r, val );
    return (int)(r>>3);
#  elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
    return (__builtin_clz(val) >> 3);
#  else
    int r;
    if (!(val>>16)) { r=2; val>>=8; } else { r=0; val>>=24; }
    r += (!val);
    return r;
#  endif
#else
#  if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
    unsigned long r;
    _BitScanForward( &r, val );
    return (int)(r>>3);
#  elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
    return (__builtin_ctz(val) >> 3);
#  else
    static const int DeBruijnBytePos[32] = { 0, 0, 3, 0, 3, 1, 3, 0, 3, 2, 2, 1, 3, 2, 0, 1, 3, 3, 1, 2, 2, 2, 2, 0, 3, 1, 2, 0, 1, 0, 1, 1 };
    return DeBruijnBytePos[((U32)((val & -(S32)val) * 0x077CB531U)) >> 27];
#  endif
#endif
}

#endif



