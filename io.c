
/* deps: essentials.c */

#define set_errno(val) _set_errno (val)
#define get_errno(out) _get_errno (&(out))

#undef __File
#define __File "io.c"

_Check_return_ static int read_entire_file_m (_In_ char *_Stack text, _In_z_ const char *filename, _In_z_ const char *flags) {
    int result;
    FILE *fd;
    int size;

    fd = fopen (filename, flags);
    if (fd) {
        fseek (fd, 0, SEEK_END);
        size = ftell (fd);
        fseek (fd, 0, SEEK_SET);
        if (prepare_stack_noexcept (text, size)) {
            int readed;
            int error;
            char *memory;

            set_errno (0);
            readed = fread (push_stack (text, size), 1, size, fd);
            get_errno (error);
            if (0 == error) {
                result = 1;
            } else {
                Error ("cannot read entire file. readed: %d, size: %d, errno: %d (%x)", readed, size, error, error);
                pop_stack (text, size);
                result = 0;
            }
        } else {
            Error ("cannot allocate memory");
            result = 0;
        }
        fclose (fd);
    } else {
        Error ("cannot open file");
        result = 0;
    }
    return (result);
}

_Check_return_ static int read_entire_text_file (_In_ char *_Stack text, _In_z_ const char *filename) {
    return (read_entire_file_m (text, filename, "r"));
}

_Check_return_ static int read_entire_binary_file (_In_ char *_Stack text, _In_z_ const char *filename) {
    return (read_entire_file_m (text, filename, "rb"));
}

_Check_return_ static int write_entire_file_m (_In_ char *_Stack text, _In_z_ const char *filename, _In_z_ const char *flags) {
    int result;
    FILE *fd;

    fd = fopen (filename, flags);
    if (fd) {
        int written;
        int error;

        set_errno (0);
        written = fwrite (text, 1, get_stack_size (text), fd);
        get_errno (error);
        if (0 == error) {
            result = 1;
        } else {
            Error ("cannot read entire file. written: %d, size: %d, errno: %d (%x)", written, get_stack_size (text), error, error);
            result = 0;
        }
        fclose (fd);
    } else {
        Error ("cannot open file");
        result = 0;
    }
    return (result);
}

_Check_return_ static int write_entire_text_file (_In_ char *_Stack text, _In_z_ const char *filename) {
    return (write_entire_file_m (text, filename, "w"));
}

_Check_return_ static int write_entire_binary_file (_In_ char *_Stack text, _In_z_ const char *filename) {
    return (write_entire_file_m (text, filename, "wb"));
}

static int is_file_exists (_In_z_ const char *filename) {
#if __Platform == __Platform_x64_Windows_10_SDK
    int result;
    HANDLE file;

    file = CreateFileA (filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if ((result = file != INVALID_HANDLE_VALUE)) {
        CloseHandle (file);
    }
    return (result);
#else
#   warning it is recommended to implement platform specific code for is_file_exists.
    int result;
    FILE *fd;

    fd = fopen (filename, "r");
    if ((result = fd != 0)) {
        fclose (fd);
    }
    return (result);
#endif
}

static unsigned long long get_file_lmt (_In_z_ const char *filename) {
#if __Platform == __Platform_x64_Windows_10_SDK
    unsigned long long result;
    HANDLE file;

    result = 0;
    file = CreateFileA (filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        FILETIME        ft;
        ULARGE_INTEGER  lint;

        if (GetFileTime (file, 0, 0, &ft)) {
            lint.u.LowPart = ft.dwLowDateTime;
            lint.u.HighPart = ft.dwHighDateTime;
            result = lint.QuadPart;
        }
        CloseHandle (file);
    }
    return (result);
#else
#   error unknown platform. please, define functionality for target platform.
#endif
}

static void set_file_lmt (_In_z_ const char *filename, unsigned long long lmt) {
#if __Platform == __Platform_x64_Windows_10_SDK
    HANDLE  file;

    file = CreateFileA (filename, FILE_GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        FILETIME        ft;
        ULARGE_INTEGER  lint;

        lint.QuadPart = lmt;
        ft.dwLowDateTime = lint.u.LowPart;
        ft.dwHighDateTime = lint.u.HighPart;
        if (!SetFileTime (file, 0, 0, &ft)) {
            Error ("could not set file lmt for '%s', last error: 0x%x", filename, GetLastError ());
        }
        CloseHandle (file);
    }
#else
#   error unknown platform. please, define functionality for target platform.
#endif
}

static int check_lmt (_In_z_ const char *left, _In_z_ const char *right, _In_opt_ FILE *print_file) {
    int result;

    if (get_file_lmt (left) < get_file_lmt (right)) {
        if (print_file) {
            fprintf (print_file, " %s < %s\n", left, right);
        }
        result = 1;
    } else {
        result = 0;
    }
    return (result);
}

_Check_return_ static int move_file (_In_z_ const char *filename, _In_z_ const char *destination) {
#if __Platform == __Platform_x64_Windows_10_SDK
    return (!!MoveFileA (filename, destination));
#else
#   error unknown platform. please, define functionality for target platform.
#endif
}

_Check_return_ static int delete_file (_In_z_ const char *filename) {
#if __Platform == __Platform_x64_Windows_10_SDK
    return (!!DeleteFileA (filename));
#else
#   error unknown platform. please, define functionality for target platform.
#endif
}


