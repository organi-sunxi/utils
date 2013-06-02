/*
   LZ4 - Fast LZ compression algorithm
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
Note : this source file requires "lz4_encoder.h" and "lz4_decoder.h"
*/

#include "lz4def.h"
#include "lz4.h"



//******************************
// Compression functions
//******************************

/*
int LZ4_compress_stack(
                 const char* source,
                 char* dest,
                 int inputSize)

Compress 'inputSize' bytes from 'source' into an output buffer 'dest'.
Destination buffer must be already allocated, and sized at a minimum of LZ4_compressBound(inputSize).
return : the number of bytes written in buffer 'dest'
*/
#define FUNCTION_NAME LZ4_compress_stack
#include "lz4_encoder.h"


/*
int LZ4_compress_stack_limitedOutput(
                 const char* source,
                 char* dest,
                 int inputSize,
                 int maxOutputSize)

Compress 'inputSize' bytes from 'source' into an output buffer 'dest' of maximum size 'maxOutputSize'.
If it cannot achieve it, compression will stop, and result of the function will be zero.
return : the number of bytes written in buffer 'dest', or 0 if the compression fails
*/
#define FUNCTION_NAME LZ4_compress_stack_limitedOutput
#define LIMITED_OUTPUT
#include "lz4_encoder.h"


/*
int LZ4_compress64k_stack(
                 const char* source,
                 char* dest,
                 int inputSize)

Compress 'inputSize' bytes from 'source' into an output buffer 'dest'.
This function compresses better than LZ4_compress_stack(), on the condition that
'inputSize' must be < to LZ4_64KLIMIT, or the function will fail.
Destination buffer must be already allocated, and sized at a minimum of LZ4_compressBound(inputSize).
return : the number of bytes written in buffer 'dest', or 0 if compression fails
*/
#define FUNCTION_NAME LZ4_compress64k_stack
#define COMPRESS_64K
#include "lz4_encoder.h"


/*
int LZ4_compress64k_stack_limitedOutput(
                 const char* source,
                 char* dest,
                 int inputSize,
                 int maxOutputSize)

Compress 'inputSize' bytes from 'source' into an output buffer 'dest' of maximum size 'maxOutputSize'.
This function compresses better than LZ4_compress_stack_limitedOutput(), on the condition that
'inputSize' must be < to LZ4_64KLIMIT, or the function will fail.
If it cannot achieve it, compression will stop, and result of the function will be zero.
return : the number of bytes written in buffer 'dest', or 0 if the compression fails
*/
#define FUNCTION_NAME LZ4_compress64k_stack_limitedOutput
#define COMPRESS_64K
#define LIMITED_OUTPUT
#include "lz4_encoder.h"


/*
void* LZ4_createHeapMemory();
int LZ4_freeHeapMemory(void* ctx);

Used to allocate and free hashTable memory 
to be used by the LZ4_compress_heap* family of functions.
LZ4_createHeapMemory() returns NULL is memory allocation fails.
*/
void* LZ4_create() { return malloc(HASHTABLESIZE); }
int   LZ4_free(void* ctx) { free(ctx); return 0; }


/*
int LZ4_compress_heap(
                 void* ctx,
                 const char* source,
                 char* dest,
                 int inputSize)

Compress 'inputSize' bytes from 'source' into an output buffer 'dest'.
The memory used for compression must be created by LZ4_createHeapMemory() and provided by pointer 'ctx'.
Destination buffer must be already allocated, and sized at a minimum of LZ4_compressBound(inputSize).
return : the number of bytes written in buffer 'dest'
*/
#define FUNCTION_NAME LZ4_compress_heap
#define USE_HEAPMEMORY
#include "lz4_encoder.h"


/*
int LZ4_compress_heap_limitedOutput(
                 void* ctx,
                 const char* source,
                 char* dest,
                 int inputSize,
                 int maxOutputSize)

Compress 'inputSize' bytes from 'source' into an output buffer 'dest' of maximum size 'maxOutputSize'.
If it cannot achieve it, compression will stop, and result of the function will be zero.
The memory used for compression must be created by LZ4_createHeapMemory() and provided by pointer 'ctx'.
return : the number of bytes written in buffer 'dest', or 0 if the compression fails
*/
#define FUNCTION_NAME LZ4_compress_heap_limitedOutput
#define LIMITED_OUTPUT
#define USE_HEAPMEMORY
#include "lz4_encoder.h"


/*
int LZ4_compress64k_heap(
                 void* ctx,
                 const char* source,
                 char* dest,
                 int inputSize)

Compress 'inputSize' bytes from 'source' into an output buffer 'dest'.
The memory used for compression must be created by LZ4_createHeapMemory() and provided by pointer 'ctx'.
'inputSize' must be < to LZ4_64KLIMIT, or the function will fail.
Destination buffer must be already allocated, and sized at a minimum of LZ4_compressBound(inputSize).
return : the number of bytes written in buffer 'dest'
*/
#define FUNCTION_NAME LZ4_compress64k_heap
#define COMPRESS_64K
#define USE_HEAPMEMORY
#include "lz4_encoder.h"


/*
int LZ4_compress64k_heap_limitedOutput(
                 void* ctx,
                 const char* source,
                 char* dest,
                 int inputSize,
                 int maxOutputSize)

Compress 'inputSize' bytes from 'source' into an output buffer 'dest' of maximum size 'maxOutputSize'.
If it cannot achieve it, compression will stop, and result of the function will be zero.
The memory used for compression must be created by LZ4_createHeapMemory() and provided by pointer 'ctx'.
'inputSize' must be < to LZ4_64KLIMIT, or the function will fail.
return : the number of bytes written in buffer 'dest', or 0 if the compression fails
*/
#define FUNCTION_NAME LZ4_compress64k_heap_limitedOutput
#define COMPRESS_64K
#define LIMITED_OUTPUT
#define USE_HEAPMEMORY
#include "lz4_encoder.h"


int LZ4_compress(const char* source, char* dest, int inputSize)
{
#if HEAPMODE
    void* ctx = LZ4_create();
    int result;
    if (ctx == NULL) return 0;    // Failed allocation => compression not done
    if (inputSize < LZ4_64KLIMIT)
        result = LZ4_compress64k_heap(ctx, source, dest, inputSize);
    else result = LZ4_compress_heap(ctx, source, dest, inputSize);
    LZ4_free(ctx);
    return result;
#else
    if (inputSize < (int)LZ4_64KLIMIT) return LZ4_compress64k_stack(source, dest, inputSize);
    return LZ4_compress_stack(source, dest, inputSize);
#endif
}


int LZ4_compress_limitedOutput(const char* source, char* dest, int inputSize, int maxOutputSize)
{
#if HEAPMODE
    void* ctx = LZ4_create();
    int result;
    if (ctx == NULL) return 0;    // Failed allocation => compression not done
    if (inputSize < LZ4_64KLIMIT)
        result = LZ4_compress64k_heap_limitedOutput(ctx, source, dest, inputSize, maxOutputSize);
    else result = LZ4_compress_heap_limitedOutput(ctx, source, dest, inputSize, maxOutputSize);
    LZ4_free(ctx);
    return result;
#else
    if (inputSize < (int)LZ4_64KLIMIT) return LZ4_compress64k_stack_limitedOutput(source, dest, inputSize, maxOutputSize);
    return LZ4_compress_stack_limitedOutput(source, dest, inputSize, maxOutputSize);
#endif
}


//****************************
// Decompression functions
//****************************

/*
int LZ4_decompress_safe(const char* source,
                        char* dest,
                        int inputSize,
                        int maxOutputSize);

LZ4_decompress_safe() guarantees it will never write nor read outside of the provided output buffers.
This function is safe against "buffer overflow" attacks.
A corrupted input will produce an error result, a negative int.
*/
#define FUNCTION_NAME LZ4_decompress_safe
#define EXITCONDITION_INPUTSIZE
#include "lz4_decoder.h"


/*
int LZ4_decompress_safe_withPrefix64k(
                        const char* source,
                        char* dest,
                        int inputSize,
                        int maxOutputSize);

Same as LZ4_decompress_safe(), but will also use 64K of memory before the beginning of input buffer.
Typically used to decode streams of inter-dependant blocks.
Note : the 64K of memory before pointer 'source' must be allocated and read-allowed.
*/
#define FUNCTION_NAME LZ4_decompress_safe_withPrefix64k
#define EXITCONDITION_INPUTSIZE
#define PREFIX_64K
#include "lz4_decoder.h"


/*
int LZ4_decompress_safe_partial(
                        const char* source,
                        char* dest,
                        int inputSize,
                        int targetOutputSize,
                        int maxOutputSize);

LZ4_decompress_safe_partial() objective is to decompress only a part of the compressed input block provided.
The decoding process stops as soon as 'targetOutputSize' bytes have been decoded, reducing decoding time.
The result of the function is the number of bytes decoded.
LZ4_decompress_safe_partial() may decode less than 'targetOutputSize' if input doesn't contain enough bytes to decode.
Always verify how many bytes were decoded to ensure there are as many as wanted into the output buffer 'dest'.
A corrupted input will produce an error result, a negative int.
*/
#define FUNCTION_NAME LZ4_decompress_safe_partial
#define EXITCONDITION_INPUTSIZE
#define PARTIAL_DECODING
#include "lz4_decoder.h"


/*
int LZ4_decompress_fast(const char* source,
                        char* dest,
                        int outputSize);

This function is faster than LZ4_decompress_safe().
LZ4_decompress_fast() guarantees it will never write nor read outside of output buffer.
Since LZ4_decompress_fast() doesn't know the size of input buffer.
it can only guarantee that it will never write into the input buffer, and will never read before its beginning.
To be used preferably in a controlled environment (when the compressed data to be decoded is from a trusted source).
A detected corrupted input will produce an error result, a negative int.
*/
#define FUNCTION_NAME LZ4_decompress_fast
#include "lz4_decoder.h"


/*
int LZ4_decompress_fast_withPrefix64k(
                        const char* source,
                        char* dest,
                        int inputSize
                        int maxOutputSize);

Same as LZ4_decompress_fast(), but will use the 64K of memory before the beginning of input buffer.
Typically used to decode streams of dependant inter-blocks.
Note : the 64K of memory before pointer 'source' must be allocated and read-allowed.
*/
#define FUNCTION_NAME LZ4_decompress_fast_withPrefix64k
#define PREFIX_64K
#include "lz4_decoder.h"


