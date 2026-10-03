#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Memory/Memory.hpp>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <cstring>

#if __cplusplus >= 201703L
#    include <cstdlib>  // for std::aligned_alloc (C++17)
#endif

#if defined( _M_IX86 ) || defined( _M_X64 ) || defined( __i386__ ) || defined( __x86_64__ )
#    define WP_HAS_SSE2_INTRINSICS 1
#else
#    define WP_HAS_SSE2_INTRINSICS 0
#endif

#if WP_HAS_SSE2_INTRINSICS
#    include <emmintrin.h>  // SSE2 intrinsics
#endif

#if WP_USE_ONETBB | WP_USE_TBB
#    include <tbb/scalable_allocator.h>
#endif

#if defined WP_PLATFORM_APPLE
#    include <malloc/_malloc.h>
#endif

namespace workphone
{

    auto memcpy_kaetemi_sse2( [[maybe_unused]] void *dst, [[maybe_unused]] const void *src,
                              [[maybe_unused]] int nBytes ) -> void *
    {
#if WP_ARCH_TYPE == WP_ARCHITECTURE_32
        __asm {
            // Copyright (C) 2009  Jan Boon (Kaetemi)
            // optimized on Intel Core 2 Duo T7500

			mov         ecx, nBytes
			mov         edi, dst
			mov         esi, src
			add         ecx, edi

			prefetchnta[esi]
			prefetchnta[esi + 32]
			prefetchnta[esi + 64]
			prefetchnta[esi + 96]

            // handle nBytes lower than 128
			cmp         nBytes, 512
			jge         fast
			slow :
			mov         bl, [esi]
				mov[edi], bl
				inc         edi
				inc         esi
				cmp         ecx, edi
				jnz         slow
				jmp         end

				fast :
            // align dstEnd to 128 bytes
			and ecx, 0xFFFFFF80

            // get srcEnd aligned to dstEnd aligned to 128 bytes
				mov         ebx, esi
				sub         ebx, edi
				add         ebx, ecx

                // skip unaligned copy if dst is aligned
				mov         eax, edi
				and edi, 0xFFFFFF80
				cmp         eax, edi
				jne         first
				jmp         more

				first :
            // copy the first 128 bytes unaligned
			movdqu      xmm0, [esi]
				movdqu      xmm1, [esi + 16]
				movdqu      xmm2, [esi + 32]
				movdqu      xmm3, [esi + 48]

				movdqu      xmm4, [esi + 64]
				movdqu      xmm5, [esi + 80]
				movdqu      xmm6, [esi + 96]
				movdqu      xmm7, [esi + 112]

				movdqu[eax], xmm0
				movdqu[eax + 16], xmm1
				movdqu[eax + 32], xmm2
				movdqu[eax + 48], xmm3

				movdqu[eax + 64], xmm4
				movdqu[eax + 80], xmm5
				movdqu[eax + 96], xmm6
				movdqu[eax + 112], xmm7

                // add 128 bytes to edi aligned earlier
				add         edi, 128

            // offset esi by the same value
				sub         eax, edi
				sub         esi, eax

                // last bytes if dst at dstEnd
				cmp         ecx, edi
				jnz         more
				jmp         last

				more :
            // handle equally aligned arrays
			mov         eax, esi
				and eax, 0xFFFFFF80
				cmp         eax, esi
				jne         unaligned4k

				aligned4k :
			mov         eax, esi
				add         eax, 4096
				cmp         eax, ebx
				jle         aligned4kin
				cmp         ecx, edi
				jne         alignedlast
				jmp         last

				aligned4kin :
			prefetchnta[esi]
				prefetchnta[esi + 32]
				prefetchnta[esi + 64]
				prefetchnta[esi + 96]

				add         esi, 128

				cmp         eax, esi
				jne         aligned4kin

				sub         esi, 4096

				alinged4kout:
			movdqa      xmm0, [esi]
				movdqa      xmm1, [esi + 16]
				movdqa      xmm2, [esi + 32]
				movdqa      xmm3, [esi + 48]

				movdqa      xmm4, [esi + 64]
				movdqa      xmm5, [esi + 80]
				movdqa      xmm6, [esi + 96]
				movdqa      xmm7, [esi + 112]

				movntdq[edi], xmm0
				movntdq[edi + 16], xmm1
				movntdq[edi + 32], xmm2
				movntdq[edi + 48], xmm3

				movntdq[edi + 64], xmm4
				movntdq[edi + 80], xmm5
				movntdq[edi + 96], xmm6
				movntdq[edi + 112], xmm7

				add         esi, 128
				add         edi, 128

				cmp         eax, esi
				jne         alinged4kout
				jmp         aligned4k

				alignedlast :
			mov         eax, esi

				alignedlastin :
			prefetchnta[esi]
				prefetchnta[esi + 32]
				prefetchnta[esi + 64]
				prefetchnta[esi + 96]

				add         esi, 128

				cmp         ebx, esi
				jne         alignedlastin

				mov         esi, eax

				alignedlastout :
			movdqa      xmm0, [esi]
				movdqa      xmm1, [esi + 16]
				movdqa      xmm2, [esi + 32]
				movdqa      xmm3, [esi + 48]

				movdqa      xmm4, [esi + 64]
				movdqa      xmm5, [esi + 80]
				movdqa      xmm6, [esi + 96]
				movdqa      xmm7, [esi + 112]

				movntdq[edi], xmm0
				movntdq[edi + 16], xmm1
				movntdq[edi + 32], xmm2
				movntdq[edi + 48], xmm3

				movntdq[edi + 64], xmm4
				movntdq[edi + 80], xmm5
				movntdq[edi + 96], xmm6
				movntdq[edi + 112], xmm7

				add         esi, 128
				add         edi, 128

				cmp         ecx, edi
				jne         alignedlastout
				jmp         last

				unaligned4k :
			mov         eax, esi
				add         eax, 4096
				cmp         eax, ebx
				jle         unaligned4kin
				cmp         ecx, edi
				jne         unalignedlast
				jmp         last

				unaligned4kin :
			prefetchnta[esi]
				prefetchnta[esi + 32]
				prefetchnta[esi + 64]
				prefetchnta[esi + 96]

				add         esi, 128

				cmp         eax, esi
				jne         unaligned4kin

				sub         esi, 4096

				unalinged4kout:
			movdqu      xmm0, [esi]
				movdqu      xmm1, [esi + 16]
				movdqu      xmm2, [esi + 32]
				movdqu      xmm3, [esi + 48]

				movdqu      xmm4, [esi + 64]
				movdqu      xmm5, [esi + 80]
				movdqu      xmm6, [esi + 96]
				movdqu      xmm7, [esi + 112]

				movntdq[edi], xmm0
				movntdq[edi + 16], xmm1
				movntdq[edi + 32], xmm2
				movntdq[edi + 48], xmm3

				movntdq[edi + 64], xmm4
				movntdq[edi + 80], xmm5
				movntdq[edi + 96], xmm6
				movntdq[edi + 112], xmm7

				add         esi, 128
				add         edi, 128

				cmp         eax, esi
				jne         unalinged4kout
				jmp         unaligned4k

				unalignedlast :
			mov         eax, esi

				unalignedlastin :
			prefetchnta[esi]
				prefetchnta[esi + 32]
				prefetchnta[esi + 64]
				prefetchnta[esi + 96]

				add         esi, 128

				cmp         ebx, esi
				jne         unalignedlastin

				mov         esi, eax

				unalignedlastout :
			movdqu      xmm0, [esi]
				movdqu      xmm1, [esi + 16]
				movdqu      xmm2, [esi + 32]
				movdqu      xmm3, [esi + 48]

				movdqu      xmm4, [esi + 64]
				movdqu      xmm5, [esi + 80]
				movdqu      xmm6, [esi + 96]
				movdqu      xmm7, [esi + 112]

				movntdq[edi], xmm0
				movntdq[edi + 16], xmm1
				movntdq[edi + 32], xmm2
				movntdq[edi + 48], xmm3

				movntdq[edi + 64], xmm4
				movntdq[edi + 80], xmm5
				movntdq[edi + 96], xmm6
				movntdq[edi + 112], xmm7

				add         esi, 128
				add         edi, 128

				cmp         ecx, edi
				jne         unalignedlastout
				jmp         last

				last :
            // get the last 128 bytes
			mov         ecx, nBytes
				mov         edi, dst
				mov         esi, src
				add         edi, ecx
				add         esi, ecx
				sub         edi, 128
				sub         esi, 128

            // copy the last 128 bytes unaligned
				movdqu      xmm0, [esi]
				movdqu      xmm1, [esi + 16]
				movdqu      xmm2, [esi + 32]
				movdqu      xmm3, [esi + 48]

				movdqu      xmm4, [esi + 64]
				movdqu      xmm5, [esi + 80]
				movdqu      xmm6, [esi + 96]
				movdqu      xmm7, [esi + 112]

				movdqu[edi], xmm0
				movdqu[edi + 16], xmm1
				movdqu[edi + 32], xmm2
				movdqu[edi + 48], xmm3

				movdqu[edi + 64], xmm4
				movdqu[edi + 80], xmm5
				movdqu[edi + 96], xmm6
				movdqu[edi + 112], xmm7

				end :
        }

        return dst;
#elif WP_ARCH_TYPE == WP_ARCHITECTURE_64 && WP_HAS_SSE2_INTRINSICS
        // 64-bit implementation using SSE2 intrinsics
        char *destPtr = static_cast<char *>( dst );
        const char *srcPtr = static_cast<const char *>( src );

        // Handle small copies
        if( nBytes < 512 )
        {
            std::memcpy( dst, src, nBytes );
            return dst;
        }

        // Prefetch initial data
        _mm_prefetch( reinterpret_cast<const char *>( srcPtr ), _MM_HINT_NTA );
        _mm_prefetch( reinterpret_cast<const char *>( srcPtr + 32 ), _MM_HINT_NTA );
        _mm_prefetch( reinterpret_cast<const char *>( srcPtr + 64 ), _MM_HINT_NTA );
        _mm_prefetch( reinterpret_cast<const char *>( srcPtr + 96 ), _MM_HINT_NTA );

        // Calculate aligned destination end (aligned to 128 bytes)
        char *destEnd = destPtr + ( nBytes & 0xFFFFFF80 );

        // Handle unaligned initial block if destination is not aligned
        uintptr_t destAlign = reinterpret_cast<uintptr_t>( destPtr ) & 0x7F;
        if( destAlign != 0 )
        {
            // Copy first 128 bytes unaligned
            __m128i xmm0 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr ) );
            __m128i xmm1 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 16 ) );
            __m128i xmm2 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 32 ) );
            __m128i xmm3 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 48 ) );
            __m128i xmm4 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 64 ) );
            __m128i xmm5 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 80 ) );
            __m128i xmm6 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 96 ) );
            __m128i xmm7 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 112 ) );

            _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr ), xmm0 );
            _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 16 ), xmm1 );
            _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 32 ), xmm2 );
            _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 48 ), xmm3 );
            _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 64 ), xmm4 );
            _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 80 ), xmm5 );
            _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 96 ), xmm6 );
            _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 112 ), xmm7 );

            // Align to next 128-byte boundary
            size_t offset = 128 - destAlign;
            destPtr += offset;
            srcPtr += offset;
        }

        // Main copy loop with prefetching
        uintptr_t srcAlign = reinterpret_cast<uintptr_t>( srcPtr ) & 0x7F;

        while( destPtr < destEnd )
        {
            // Prefetch next block
            _mm_prefetch( reinterpret_cast<const char *>( srcPtr + 128 ), _MM_HINT_NTA );
            _mm_prefetch( reinterpret_cast<const char *>( srcPtr + 160 ), _MM_HINT_NTA );
            _mm_prefetch( reinterpret_cast<const char *>( srcPtr + 192 ), _MM_HINT_NTA );
            _mm_prefetch( reinterpret_cast<const char *>( srcPtr + 224 ), _MM_HINT_NTA );

            __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

            if( srcAlign == 0 )
            {
                // Source is aligned, use aligned loads
                xmm0 = _mm_load_si128( reinterpret_cast<const __m128i *>( srcPtr ) );
                xmm1 = _mm_load_si128( reinterpret_cast<const __m128i *>( srcPtr + 16 ) );
                xmm2 = _mm_load_si128( reinterpret_cast<const __m128i *>( srcPtr + 32 ) );
                xmm3 = _mm_load_si128( reinterpret_cast<const __m128i *>( srcPtr + 48 ) );
                xmm4 = _mm_load_si128( reinterpret_cast<const __m128i *>( srcPtr + 64 ) );
                xmm5 = _mm_load_si128( reinterpret_cast<const __m128i *>( srcPtr + 80 ) );
                xmm6 = _mm_load_si128( reinterpret_cast<const __m128i *>( srcPtr + 96 ) );
                xmm7 = _mm_load_si128( reinterpret_cast<const __m128i *>( srcPtr + 112 ) );
            }
            else
            {
                // Source is unaligned, use unaligned loads
                xmm0 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr ) );
                xmm1 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 16 ) );
                xmm2 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 32 ) );
                xmm3 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 48 ) );
                xmm4 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 64 ) );
                xmm5 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 80 ) );
                xmm6 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 96 ) );
                xmm7 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 112 ) );
            }

            // Non-temporal stores (bypass cache)
            _mm_stream_si128( reinterpret_cast<__m128i *>( destPtr ), xmm0 );
            _mm_stream_si128( reinterpret_cast<__m128i *>( destPtr + 16 ), xmm1 );
            _mm_stream_si128( reinterpret_cast<__m128i *>( destPtr + 32 ), xmm2 );
            _mm_stream_si128( reinterpret_cast<__m128i *>( destPtr + 48 ), xmm3 );
            _mm_stream_si128( reinterpret_cast<__m128i *>( destPtr + 64 ), xmm4 );
            _mm_stream_si128( reinterpret_cast<__m128i *>( destPtr + 80 ), xmm5 );
            _mm_stream_si128( reinterpret_cast<__m128i *>( destPtr + 96 ), xmm6 );
            _mm_stream_si128( reinterpret_cast<__m128i *>( destPtr + 112 ), xmm7 );

            srcPtr += 128;
            destPtr += 128;
        }

        // Copy last 128 bytes (may overlap with already copied data)
        srcPtr = static_cast<const char *>( src ) + nBytes - 128;
        destPtr = static_cast<char *>( dst ) + nBytes - 128;

        __m128i xmm0 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr ) );
        __m128i xmm1 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 16 ) );
        __m128i xmm2 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 32 ) );
        __m128i xmm3 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 48 ) );
        __m128i xmm4 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 64 ) );
        __m128i xmm5 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 80 ) );
        __m128i xmm6 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 96 ) );
        __m128i xmm7 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( srcPtr + 112 ) );

        _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr ), xmm0 );
        _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 16 ), xmm1 );
        _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 32 ), xmm2 );
        _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 48 ), xmm3 );
        _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 64 ), xmm4 );
        _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 80 ), xmm5 );
        _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 96 ), xmm6 );
        _mm_storeu_si128( reinterpret_cast<__m128i *>( destPtr + 112 ), xmm7 );

        return dst;
#else
        std::memcpy( dst, src, nBytes );
        return dst;
#endif
    }

    // Courtesy of William Chan and Google. 30-70% faster than memcpy in Microsoft Visual Studio 2005.
    void X_aligned_memcpy_sse2( [[maybe_unused]] void *dest, [[maybe_unused]] const void *src,
                                [[maybe_unused]] const unsigned long u32 )
    {
#if WP_ARCH_TYPE == WP_ARCHITECTURE_32
        __asm {
			mov esi, src;  // src pointer
			mov edi, dest;  // dest pointer

			mov ebx, u32;  // ebx is our counter 
			shr ebx, 7;  // divide by 128 (8 * 128bit registers)


		loop_copy:
			prefetchnta 128[ESI];  // SSE2 prefetch
			prefetchnta 160[ESI];
			prefetchnta 192[ESI];
			prefetchnta 224[ESI];

			movdqa xmm0, 0[ESI];  // move data from src to registers
			movdqa xmm1, 16[ESI];
			movdqa xmm2, 32[ESI];
			movdqa xmm3, 48[ESI];
			movdqa xmm4, 64[ESI];
			movdqa xmm5, 80[ESI];
			movdqa xmm6, 96[ESI];
			movdqa xmm7, 112[ESI];

			movntdq 0[EDI], xmm0;  // move data from registers to dest
			movntdq 16[EDI], xmm1;
			movntdq 32[EDI], xmm2;
			movntdq 48[EDI], xmm3;
			movntdq 64[EDI], xmm4;
			movntdq 80[EDI], xmm5;
			movntdq 96[EDI], xmm6;
			movntdq 112[EDI], xmm7;

			add esi, 128;
			add edi, 128;
			dec ebx;

			jnz loop_copy;  // loop please
                           //loop_copy_end:
        }
#endif
    }

    void Memory::Memcpy( void *dest0, const void *src0, const int count0 )
    {
#if WP_ARCH_TYPE == WP_ARCHITECTURE_32
        memcpy_kaetemi_sse2( dest0, src0, count0 );
#else
        memcpy( dest0, src0, count0 );
#endif
    }

    void Memory::AlignedMemcpy( void *dest0, const void *src0, const int count0 )
    {
#if WP_ARCH_TYPE == WP_ARCHITECTURE_32
        X_aligned_memcpy_sse2( dest0, src0, count0 );
#else
        memcpy( dest0, src0, count0 );
#endif
    }

    void Memory::Memset( void *dest0, const int byteValue, const int count0 )
    {
        memset( dest0, byteValue, count0 );
    }

    auto Memory::Memcmp( const void *ptr1, const void *ptr2, s32 num ) -> s32
    {
        return memcmp( ptr1, ptr2, num );
    }

    auto Memory::CheckHeap() -> bool
    {
#ifdef WIN32
        /* Check heap status */
        auto heapstatus = _heapchk();
        switch( heapstatus )
        {
        case _HEAPOK:
            return true;
        case _HEAPEMPTY:
            return false;
        case _HEAPBADBEGIN:
            return false;
        case _HEAPBADNODE:
            return false;
        }
#endif

        return true;
    }

    // Helper function for portable aligned allocation fallback
    namespace
    {
        auto portable_aligned_malloc( size_t size, size_t alignment ) -> void *
        {
            // Ensure alignment is a power of 2 and at least sizeof(void*)
            if( alignment == 0 || ( alignment & ( alignment - 1 ) ) != 0 ||
                alignment < sizeof( void * ) )
            {
                return nullptr;
            }

            // Use C++17 std::aligned_alloc if available
#if __cplusplus >= 201703L && defined( __cpp_aligned_new )
            // std::aligned_alloc requires size to be a multiple of alignment
            size_t aligned_size = ( size + alignment - 1 ) & ~( alignment - 1 );
            //return std::aligned_alloc( alignment, aligned_size );
            return nullptr;  // aligned_alloc is not supported in MSVC
#else
            // Fallback: allocate extra memory and manually align
            size_t total_size = size + alignment + sizeof( void * );
            char *raw_ptr = static_cast<char *>( malloc( total_size ) );
            if( !raw_ptr )
            {
                return nullptr;
            }

            // Calculate aligned address
            char *aligned_ptr = raw_ptr + sizeof( void * );
            size_t space = total_size - sizeof( void * );
            void *result =
                std::align( alignment, size, reinterpret_cast<void *&>( aligned_ptr ), space );

            if( !result )
            {
                free( raw_ptr );
                return nullptr;
            }

            // Store the original pointer just before the aligned memory
            static_cast<void **>( result )[-1] = raw_ptr;
            return result;
#endif
        }

        void portable_aligned_free( void *ptr )
        {
            if( !ptr )
            {
                return;
            }

#if __cplusplus >= 201703L && defined( __cpp_aligned_new )
            free( ptr );
#else
            // Retrieve the original pointer and free it
            void *raw_ptr = static_cast<void **>( ptr )[-1];
            free( raw_ptr );
#endif
        }
    }  // namespace

    auto Memory::ScalableAlignedMalloc( size_t size, size_t alignment ) -> void *
    {
#if WP_ENABLE_HEAP_DEBUG
        WP_ASSERT( CheckHeap() );
        auto ptr = scalable_aligned_malloc( size, alignment );
        if( !ptr )
        {
            std::cout << "Out of memory." << std::endl;
            std::terminate();
        }

        WP_ASSERT( CheckHeap() );
        return ptr;
#elif WP_USE_ONETBB | WP_USE_TBB
        auto ptr = scalable_aligned_malloc( size, alignment );
        if( !ptr )
        {
            std::cout << "Out of memory." << std::endl;
            std::terminate();
        }

        return ptr;
#elif defined( WP_PLATFORM_APPLE )
        // macOS posix_memalign implementation
        void *ptr = nullptr;
        if( posix_memalign( &ptr, alignment, size ) != 0 )
        {
            std::cout << "Out of memory." << std::endl;
            std::terminate();
        }
        return ptr;
#elif defined( _WIN32 ) || defined( _WIN64 )
        // Windows _aligned_malloc implementation
        auto ptr = _aligned_malloc( size, alignment );
        if( !ptr )
        {
            std::cout << "Out of memory." << std::endl;
            std::terminate();
        }
        return ptr;
#else
        // Portable fallback using standard library functions
        auto ptr = portable_aligned_malloc( size, alignment );
        if( !ptr )
        {
            std::cout << "Out of memory." << std::endl;
            std::terminate();
        }
        return ptr;
#endif
    }

    void Memory::ScalableAlignedFree( void *ptr )
    {
#if WP_ENABLE_HEAP_DEBUG
        WP_ASSERT( CheckHeap() );
        scalable_aligned_free( ptr );
        WP_ASSERT( CheckHeap() );
#elif WP_USE_ONETBB | WP_USE_TBB
        scalable_aligned_free( ptr );
#elif defined( WP_PLATFORM_APPLE )
        // macOS uses regular free for posix_memalign
        free( ptr );
#elif defined( _WIN32 ) || defined( _WIN64 )
        // Windows _aligned_free implementation
        _aligned_free( ptr );
#else
        // Portable fallback
        portable_aligned_free( ptr );
#endif
    }

    auto Memory::ScalableMalloc( size_t size ) -> void *
    {
#if WP_ENABLE_HEAP_DEBUG
        WP_ASSERT( CheckHeap() );
        return malloc( size );
#elif WP_USE_ONETBB | WP_USE_TBB
        return scalable_malloc( size );
#else
        return malloc( size );
#endif
    }

    void Memory::ScalableFree( void *ptr )
    {
#if WP_ENABLE_HEAP_DEBUG
        WP_ASSERT( CheckHeap() );
        free( ptr );
#elif WP_USE_ONETBB | WP_USE_TBB
        scalable_free( ptr );
#else
        free( ptr );
#endif
    }
}  // namespace workphone
