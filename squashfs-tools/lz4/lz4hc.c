/*
   LZ4 HC - High Compression Mode of LZ4
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

/*
Note : this source file requires "lz4hc_encoder.h"
*/

//**************************************
// Includes
//**************************************
#include "lz4def.h"
#include "lz4hc.h"



//**************************************
// Constants
//**************************************
#define HASH_LOG (DICTIONARY_LOGSIZE-1)
#undef HASHTABLESIZE
#define HASHTABLESIZE (1 << HASH_LOG)
#define HASH_MASK (HASHTABLESIZE - 1)

#define MAX_NB_ATTEMPTS 256

#define OPTIMAL_ML (int)((ML_MASK-1)+MINMATCH)

#define KB *(1U<<10)
#define MB *(1U<<20)
#define GB *(1U<<30)

#undef HTYPE
#define HTYPE                 U32
#undef INITBASE
#define INITBASE(b,s)         const BYTE* const b = s


//************************************************************
// Local Types
//************************************************************
typedef struct 
{
    const BYTE* inputBuffer;
    const BYTE* base;
    const BYTE* end;
    HTYPE hashTable[HASHTABLESIZE];
    U16 chainTable[MAXD];
    const BYTE* nextToUpdate;
} LZ4HC_Data_Structure;


//**************************************
// Macros
//**************************************
#define HASH_FUNCTION(i)       (((i) * 2654435761U) >> ((MINMATCH*8)-HASH_LOG))
#define HASH_VALUE(p)          HASH_FUNCTION(A32(p))
#define HASH_POINTER(p)        (HashTable[HASH_VALUE(p)] + base)
#define DELTANEXT(p)           chainTable[(size_t)(p) & MAXD_MASK] 
#define GETNEXT(p)             ((p) - (size_t)DELTANEXT(p))


//**************************************
// Private functions
//**************************************
static inline int LZ4_InitHC (LZ4HC_Data_Structure* hc4, const BYTE* base)
{
    MEM_INIT((void*)hc4->hashTable, 0, sizeof(hc4->hashTable));
    MEM_INIT(hc4->chainTable, 0xFF, sizeof(hc4->chainTable));
    hc4->nextToUpdate = base + 1;
    hc4->base = base;
    hc4->inputBuffer = base;
    hc4->end = base;
    return 1;
}


void* LZ4_createHC (const char* slidingInputBuffer)
{
    void* hc4 = ALLOCATOR(sizeof(LZ4HC_Data_Structure));
    LZ4_InitHC ((LZ4HC_Data_Structure*)hc4, (const BYTE*)slidingInputBuffer);
    return hc4;
}


int LZ4_freeHC (void* LZ4HC_Data)
{
    FREEMEM(LZ4HC_Data);
    return (0);
}


// Update chains up to ip (excluded)
static forceinline void LZ4HC_Insert (LZ4HC_Data_Structure* hc4, const BYTE* ip)
{
    U16*   chainTable = hc4->chainTable;
    HTYPE* HashTable  = hc4->hashTable;
    INITBASE(base,hc4->base);

    while(hc4->nextToUpdate < ip)
    {
        const BYTE* const p = hc4->nextToUpdate;
        size_t delta = (p) - HASH_POINTER(p); 
        if (delta>MAX_DISTANCE) delta = MAX_DISTANCE; 
        DELTANEXT(p) = (U16)delta; 
        HashTable[HASH_VALUE(p)] = (HTYPE)((p) - base);
        hc4->nextToUpdate++;
    }
}


char* LZ4_slideInputBufferHC(void* LZ4HC_Data)
{
    LZ4HC_Data_Structure* hc4 = (LZ4HC_Data_Structure*)LZ4HC_Data;
    U32 distance = (U32)(hc4->end - hc4->inputBuffer) - 64 KB;
    distance = (distance >> 16) << 16;   // Must be a multiple of 64 KB
    LZ4HC_Insert(hc4, hc4->end - MINMATCH);
    memcpy((void*)(hc4->end - 64 KB - distance), (const void*)(hc4->end - 64 KB), 64 KB);
    hc4->nextToUpdate -= distance;
    hc4->base -= distance;
    if ((U32)(hc4->inputBuffer - hc4->base) > 1 GB + 64 KB)   // Avoid overflow
    {
        int i;
        hc4->base += 1 GB;
        for (i=0; i<HASHTABLESIZE; i++) hc4->hashTable[i] -= 1 GB;
    }
    hc4->end -= distance;
    return (char*)(hc4->end);
}


static forceinline size_t LZ4HC_CommonLength (const BYTE* p1, const BYTE* p2, const BYTE* const matchlimit)
{
    const BYTE* p1t = p1;

    while (p1t<matchlimit-(STEPSIZE-1))
    {
        UARCH diff = AARCH(p2) ^ AARCH(p1t);
        if (!diff) { p1t+=STEPSIZE; p2+=STEPSIZE; continue; }
        p1t += LZ4_NbCommonBytes(diff);
        return (p1t - p1);
    }
    if (LZ4_ARCH64) if ((p1t<(matchlimit-3)) && (A32(p2) == A32(p1t))) { p1t+=4; p2+=4; }
    if ((p1t<(matchlimit-1)) && (A16(p2) == A16(p1t))) { p1t+=2; p2+=2; }
    if ((p1t<matchlimit) && (*p2 == *p1t)) p1t++;
    return (p1t - p1);
}


static forceinline int LZ4HC_InsertAndFindBestMatch (LZ4HC_Data_Structure* hc4, const BYTE* ip, const BYTE* const matchlimit, const BYTE** matchpos)
{
    U16* const chainTable = hc4->chainTable;
    HTYPE* const HashTable = hc4->hashTable;
    const BYTE* ref;
    INITBASE(base,hc4->base);
    int nbAttempts=MAX_NB_ATTEMPTS;
    size_t repl=0, ml=0;
    U16 delta=0;  // useless assignment, to remove an uninitialization warning

    // HC4 match finder
    LZ4HC_Insert(hc4, ip);
    ref = HASH_POINTER(ip);

#define REPEAT_OPTIMIZATION
#ifdef REPEAT_OPTIMIZATION
    // Detect repetitive sequences of length <= 4
    if ((U32)(ip-ref) <= 4)        // potential repetition
    {
        if (A32(ref) == A32(ip))   // confirmed
        {
            delta = (U16)(ip-ref);
            repl = ml  = LZ4HC_CommonLength(ip+MINMATCH, ref+MINMATCH, matchlimit) + MINMATCH;
            *matchpos = ref;
        }
        ref = GETNEXT(ref);
    }
#endif

    while (((U32)(ip-ref) <= MAX_DISTANCE) && (nbAttempts))
    {
        nbAttempts--;
        if (*(ref+ml) == *(ip+ml))
        if (A32(ref) == A32(ip))
        {
            size_t mlt = LZ4HC_CommonLength(ip+MINMATCH, ref+MINMATCH, matchlimit) + MINMATCH;
            if (mlt > ml) { ml = mlt; *matchpos = ref; }
        }
        ref = GETNEXT(ref);
    }

#ifdef REPEAT_OPTIMIZATION
    // Complete table
    if (repl)
    {
        const BYTE* ptr = ip;
        const BYTE* end;

        end = ip + repl - (MINMATCH-1);
        while(ptr < end-delta)
        {
            DELTANEXT(ptr) = delta;    // Pre-Load
            ptr++;
        }
        do
        {
            DELTANEXT(ptr) = delta;    
            HashTable[HASH_VALUE(ptr)] = (HTYPE)((ptr) - base);     // Head of chain
            ptr++;
        } while(ptr < end);
        hc4->nextToUpdate = end;
    }
#endif 

    return (int)ml;
}


static forceinline int LZ4HC_InsertAndGetWiderMatch (LZ4HC_Data_Structure* hc4, const BYTE* ip, const BYTE* startLimit, const BYTE* matchlimit, int longest, const BYTE** matchpos, const BYTE** startpos)
{
    U16* const  chainTable = hc4->chainTable;
    HTYPE* const HashTable = hc4->hashTable;
    INITBASE(base,hc4->base);
    const BYTE*  ref;
    int nbAttempts = MAX_NB_ATTEMPTS;
    int delta = (int)(ip-startLimit);

    // First Match
    LZ4HC_Insert(hc4, ip);
    ref = HASH_POINTER(ip);

    while (((U32)(ip-ref) <= MAX_DISTANCE) && (nbAttempts))
    {
        nbAttempts--;
        if (*(startLimit + longest) == *(ref - delta + longest))
        if (A32(ref) == A32(ip))
        {
#if 1
            const BYTE* reft = ref+MINMATCH;
            const BYTE* ipt = ip+MINMATCH;
            const BYTE* startt = ip;

            while (ipt<matchlimit-(STEPSIZE-1))
            {
                UARCH diff = AARCH(reft) ^ AARCH(ipt);
                if (!diff) { ipt+=STEPSIZE; reft+=STEPSIZE; continue; }
                ipt += LZ4_NbCommonBytes(diff);
                goto _endCount;
            }
            if (LZ4_ARCH64) if ((ipt<(matchlimit-3)) && (A32(reft) == A32(ipt))) { ipt+=4; reft+=4; }
            if ((ipt<(matchlimit-1)) && (A16(reft) == A16(ipt))) { ipt+=2; reft+=2; }
            if ((ipt<matchlimit) && (*reft == *ipt)) ipt++;
_endCount:
            reft = ref;
#else
            // Easier for code maintenance, but unfortunately slower too
            const BYTE* startt = ip;
            const BYTE* reft = ref;
            const BYTE* ipt = ip + MINMATCH + LZ4HC_CommonLength(ip+MINMATCH, ref+MINMATCH, matchlimit);
#endif

            while ((startt>startLimit) && (reft > hc4->inputBuffer) && (startt[-1] == reft[-1])) {startt--; reft--;}

            if ((ipt-startt) > longest)
            {
                longest = (int)(ipt-startt);
                *matchpos = reft;
                *startpos = startt;
            }
        }
        ref = GETNEXT(ref);
    }

    return longest;
}



//**************************************
// Compression functions
//**************************************

/*
int LZ4_compressHC(
                 const char* source,
                 char* dest,
                 int inputSize)

Compress 'inputSize' bytes from 'source' into an output buffer 'dest'.
Destination buffer must be already allocated, and sized at a minimum of LZ4_compressBound(inputSize).
return : the number of bytes written in buffer 'dest'
*/
#define FUNCTION_NAME LZ4_compressHC
#include "lz4hc_encoder.h"


/*
int LZ4_compressHC_limitedOutput(
                 const char* source,
                 char* dest,
                 int inputSize,
                 int maxOutputSize)

Compress 'inputSize' bytes from 'source' into an output buffer 'dest' of maximum size 'maxOutputSize'.
If it cannot achieve it, compression will stop, and result of the function will be zero.
return : the number of bytes written in buffer 'dest', or 0 if the compression fails
*/
#define FUNCTION_NAME LZ4_compressHC_limitedOutput
#define LIMITED_OUTPUT
#include "lz4hc_encoder.h"

