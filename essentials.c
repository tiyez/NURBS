

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>
#include <malloc.h>
#include <math.h>

#if defined _WIN64 || defined _WIN32
#   define WIN32_LEAN_AND_MEAN
#   define UNICODE
#   include <windows.h>
#   define Memory_Page_Bit 12
#   define SAL_Annotations
#   define __Alloc_Method __Win32_Virtual_Alloc
#   define __Compiler __Compiler_MSVC
#   define __Platform __Platform_x64_Windows_10_SDK
#else
#   error Unknown platform. Please, define essentials for the target platform
#endif

#ifdef Main_TU
#define TU_Decl
#else
#define TU_Decl extern
#endif

#undef __File
#define __File "essentials.c"
#define __FileW Cat (L, __File)

#define __Win32_Virtual_Alloc 1
#define __POSIX_mmap 2

#define __Compiler_MSVC 1
// #define __Compiler_GCC 2
// #define __Compiler_CLang 3

#define __Platform_x64_Windows_10_SDK 1
// #define __Platform_Linux 2
// #define __Platform_MacOS 3
// #define __Platform_MinGW 4

#define Memory_Page (1 << Memory_Page_Bit)

#if SIZE_MAX == 0xFFFFFFFFFFFFFFFFllu
#   define Pointer_Size 64
#else
#   define Pointer_Size 32
#endif

#define _ROnly /* used instead of C's const to remove type checking. It means that modification of data under pointer is not intended. Used only in parameters of function */

#if !defined SAL_Annotations
#   define _In_
#   define _In_z_
#   define _In_opt_
#   define _In_opt_z_
#   define _In_reads_(...)
#   define _In_reads_z_(...)
#   define _In_range_(...)
#   define _In_reads_bytes_(...)
#   define _In_reads_bytes_opt_(...)
#   define _Out_
#   define _Out_z_
#   define _Out_opt_
#   define _Out_opt_z_
#   define _Out_range_(...)
#   define _Out_writes_(...)
#   define _Out_writes_z_(...)
#   define _Outptr_result_nullonfailure_
#   define _Inout_
#   define _Inout_z_
#   define _Inout_opt_
#   define _Inout_opt_z_
#   define _Inout_updates_(...)
#   define _Ret_notnull_
#   define _Ret_maybenull_
#   define _Ret_null_
#   define _Ret_z_
#   define _Ret_maybenull_z_
#   define _Ret_range_(...)
#   define _Check_return_
#   define _Must_inspect_result_
#   define _Success_(...)
#   define _When_(...)
#   define _On_failure_(...)
#   define _Printf_format_string_params_(...)
#   define _Printf_format_string_
#endif

#if __Compiler == __Compiler_MSVC
#   define Thread_Local __declspec (thread)
#   define Try __try
#   define Catch _Pragma ("warning (suppress : 6320)") __except (EXCEPTION_EXECUTE_HANDLER)
#else
#   error unknown compiler. please define essentials for compiler.
#endif

#define Stringize1(a) #a
#define Stringize(a) Stringize1 (a)
#define LineInfo " (" __File ":" Stringize (__LINE__) ")"
#define ErrorLine __File "(" Stringize (__LINE__) "): error: "
#define ErrorFormat "%s(%d): error: "

#if defined __cplusplus
#   define Static_Assert(expr) static_assert (expr, "Static Assert Failed: " #expr)
#   define Align_As(expr) alignas (expr)
#else
#   define Static_Assert(expr) _Static_assert (expr, "Static Assert Failed: " #expr)
#   define Align_As(expr) _Alignas (expr)
#endif

#if defined No_Error_Messages || defined No_Safe_Checks
#   define Error(...)
#else
#   define Error(...) \
do {fprintf (stdout, ErrorFormat, __File, __LINE__);\
    fprintf (stdout, __VA_ARGS__);\
    fprintf (stdout, " (%s)\n", __func__);\
} while (0)
#endif

#   define Print(...) \
do {fprintf (stdout, __VA_ARGS__);\
    fprintf (stdout, " (%s:%d)\n", __File, __LINE__);\
} while (0)

#if defined No_Debug_Messages || defined No_Safe_Checks
#   define Debug(...)
#else
#   define Debug(...) \
do {fprintf (stdout, "%s(%d): ", __File, __LINE__);\
    fprintf (stdout, __VA_ARGS__);\
    fprintf (stdout, " (%s)\n", __func__);\
} while (0)
#endif

#if defined No_Asserts || defined No_Safe_Checks
#   if __Compiler == __Compiler_MSVC
#       define Assert(expr) __assume (expr)
#       define Unreachable() __assume (0)
#   else
#       define Assert(expr)
#       define Unreachable()
#   endif
#   define Todo() fprintf (stdout, "%s(%d): not implemented yet\n", __File, __LINE__)
#else
#   define Assert(expr) ((void) ((expr) ? (1) : (fprintf (stdout, "%s(%d): assertion failed: %s\n", __File, __LINE__, #expr), fflush (stdout), Throw1 ("assertion failed: " #expr), 0)))
#   define Unreachable() Assert (!"unreachable");
#   define Todo() Assert (!"not implemented yet");
#endif

TU_Decl Thread_Local struct {
    const char  *message;
    void        *userptr;
    char        buffer [1024];
} _exception__;
#define Set_Exception_Message(string) (_exception__.message = string LineInfo)
#define Set_Exception_Pointer(ptr) (_exception__.userptr = (void *) ptr)
#define Get_Exception_Message() (_exception__.message)
#define Get_Exception_Pointer() (_exception__.userptr)
#define Clear_Exception() (_exception__.message = 0, Set_Exception_Pointer (0))
#define Throw1(format, ...) (snprintf (_exception__.buffer, sizeof _exception__.buffer, format LineInfo, ##__VA_ARGS__), _exception__.message = _exception__.buffer, *((volatile char *) 0) = 0, abort ())
#define Throw(ptr, format, ...) (Set_Exception_Pointer (ptr), Throw1 (format, ##__VA_ARGS__))

#define NullptrOf(type) ((type *) 0)
#define MemberOf(type, memb) (NullptrOf (type)->memb)
#define Member_Offset(type, memb) ((size_t) &MemberOf (type, memb))
#define Array_Count(arr) (sizeof (arr) / sizeof ((arr) [0]))
#define Is_Power_Of_Two(X) ((X) && !((X) & ((X) - 1)))

#define Zero(ptr) memset ((void *) (ptr), 0, sizeof *(ptr))
#define Zero_Array(ptr) memset ((void *) (ptr), 0, sizeof (ptr))
#define Zero_N(ptr, count) memset ((void *) (ptr), 0, (count) * sizeof *(ptr))

#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))

#define Cat1(a, b) a ## b
#define Cat(a, b) Cat1 (a, b)

static inline size_t to_power_of_two (size_t val) {
    val -= 1;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
#if Pointer_Size == 64
    val |= val >> 32;
#endif
    val += 1;
    return (val);
}

static inline int   discrete_sqrt (size_t val) {
#if __Compiler == __Compiler_MSVC
    unsigned long   index;

    _BitScanReverse64 (&index, val);
    return (index);
#else
#   error undefined compiler.
    // return ((int) sqrt (val));
#endif
}

static inline int   forward_bit_scan (size_t val) {
#if __Compiler == __Compiler_MSVC
    unsigned long   index;

    _BitScanForward64 (&index, val);
    return (index);
#else
#   error undefined compiler.
#endif
}

static inline size_t to_page_size (size_t val) {
    return ((val + Memory_Page - 1) & ~(Memory_Page - 1));
}

static inline size_t get_aligned_value (size_t value, size_t alignment) {
    return ((value + alignment - 1) & ~(alignment - 1));
}

static inline size_t get_alignment_diff (size_t value, size_t alignment) {
    return (((value + alignment - 1) & ~(alignment - 1)) - value);
}

static inline int  is_memory_zero (void *data, int size) {
    return (0 == *(char *) data && 0 == memcmp (data, (char *) data + 1, size - 1));
}

#if __Alloc_Method == __Win32_Virtual_Alloc
_Ret_maybenull_ _Check_return_ static inline void *reserve_virtual_address_space (int maximum_amount_of_pages) {
    return (VirtualAlloc (0, maximum_amount_of_pages * Memory_Page, MEM_RESERVE, PAGE_READWRITE));
}
_Ret_maybenull_ _Check_return_ static inline void *commit_memory_pages (_In_ void *ptr, int count) {
    return (VirtualAlloc (ptr, count * Memory_Page, MEM_COMMIT, PAGE_READWRITE));
}
static inline void release_virtual_address_space (_In_opt_ void *memory) {
    if (memory) {
        VirtualFree (memory, 0, MEM_RELEASE);
    }
}
#else
#   error undefined allocation method.
#endif









