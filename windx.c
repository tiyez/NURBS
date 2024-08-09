
/* deps: __Platform_Windows_10_SDK essentials.c */

#include <windowsx.h>
#include <uxtheme.h>
#include <objbase.h>
#include <shlwapi.h>

#undef __File
#define __File "windx.c"

#pragma comment (lib, "Gdi32.lib")
#pragma comment (lib, "User32.lib")
#pragma comment (lib, "Ole32.lib")
#pragma comment (lib, "UxTheme.lib")
#pragma comment (lib, "dwmapi.lib")
#pragma comment (lib, "Shlwapi.lib")
#pragma comment (lib, "Shell32.lib")

#define Max_Mouse_Pressure 0x7FFF
struct mouseinfo {
    int     pos[2];
    int     delta[2];
    int     wheel_delta;
    int     pressure;
};

#define KeyComm(name) KeyComm_##name
enum keycommand {
    // KeyComm (left),
    // KeyComm (right),
    // KeyComm (up),
    // KeyComm (down),
    KeyComm (left_select),
    KeyComm (right_select),
    KeyComm (up_select),
    KeyComm (down_select),
    KeyComm (left_word),
    KeyComm (right_word),
    KeyComm (left_word_select),
    KeyComm (right_word_select),
    KeyComm (left_subword),
    KeyComm (right_subword),
    KeyComm (up_scroll),
    KeyComm (down_scroll),
    KeyComm (enter),
    KeyComm (delete),
    KeyComm (home),
    KeyComm (home_select),
    KeyComm (end),
    KeyComm (end_select),
    KeyComm (pageup),
    KeyComm (pagedown),
    KeyComm (insert),
    KeyComm (backspace),
    KeyComm (tab),
    KeyComm (f1),
    KeyComm (f2),
    KeyComm (f3),
    KeyComm (f4),
    KeyComm (copy),
    KeyComm (paste),
    KeyComm (undo),
    KeyComm (cut),
    KeyComm (find),
    KeyComm (goto),
    KeyComm (selectall),
    KeyComm (save),
    KeyComm (open),
    KeyComm (new),
    KeyComm (switch_tab_left),
    KeyComm (switch_tab_right),
};

#define ControlKey(name) ControlKey_##name
enum controlkey {
    ControlKey (control) = 0x1,
    ControlKey (shift) = 0x2,
    ControlKey (alt) = 0x4,
};

#define MouseKey(name) MouseKey_##name
enum mousekey {
    MouseKey (left) = 0x1,
    MouseKey (right) = 0x2,
    MouseKey (middle) = 0x4,
    MouseKey (extra1) = 0x8,
    MouseKey (extra2) = 0x10,
};

struct window_handlers {
    int     (*initialize) (struct window *window);
    void    (*release) (struct window *window);
    unsigned long long      (*on_resize) (struct window *window, int is_dpi_changed);
    unsigned long long      (*on_paint) (struct window *window);
    unsigned long long      (*on_char) (struct window *window, int codepoint);
    unsigned long long      (*on_keycommand) (struct window *window, enum keycommand command);
    unsigned long long      (*on_key_down) (struct window *window, int virtkey, enum controlkey control);
    unsigned long long      (*on_key_up) (struct window *window, int virtkey, enum controlkey control);
    unsigned long long      (*on_mouse_move) (struct window *window, struct mouseinfo *mouseinfo);
    unsigned long long      (*on_mouse_wheel) (struct window *window, struct mouseinfo *mouseinfo);
    unsigned long long      (*on_activate) (struct window *window, int is_active, int is_active_by_click, int mousepos[2]);
    unsigned long long      (*on_mouse_capture) (struct window *window, int is_captured, struct mouseinfo *mouseinfo, enum mousekey mousekeys, enum controlkey controlkeys);
    unsigned long long      (*on_mouse_button) (struct window *window, struct mouseinfo *mouseinfo, enum mousekey mousekeys, enum controlkey controlkeys);
    unsigned long long      (*on_file_change) (struct window *window, const char *filename);
};

struct dir_watcher {
    DWORD   main_thread_id;
    char    path[128];
};
#define MSG_File_Change (WM_USER + 0)

struct window {
    HWND                    handle;
    HACCEL                  accel;
    int                     dpi;
    int                     width;
    int                     height;
    int                     client_width;
    int                     client_height;
    float                   client_width_96;
    float                   client_height_96;
    unsigned long long      last_frame_ms;
    float                   dt;
    int                     alive;
    int                     mousepos[2];
    int                     is_mouse_captured;
    int                     captured_mouse_button;
    unsigned long long      frame_version;
    struct dx               dx;
    void                    *userdata;
    struct window_handlers  handlers;
    struct dir_watcher      dir_watcher;
};

struct keycommand_shortcut {
    int     cmd;
    int     virtkey;
    int     is_control;
    int     is_shift;
    int     is_alt;
};

static const struct keycommand_shortcut g_keycommand_shortcuts[] = {
    // { KeyComm (left),            VK_LEFT, 0, 0, 0, },
    // { KeyComm (right),           VK_RIGHT, 0, 0, 0, },
    // { KeyComm (up),              VK_UP, 0, 0, 0, },
    // { KeyComm (down),            VK_DOWN, 0, 0, 0, },

    { KeyComm (left_select),    VK_LEFT, 0, 1, 0, },
    { KeyComm (right_select),   VK_RIGHT, 0, 1, 0, },
    { KeyComm (up_select),      VK_UP, 0, 1, 0, },
    { KeyComm (down_select),    VK_DOWN, 0, 1, 0, },

    { KeyComm (left_word),      VK_LEFT, 1, 0, 0, },
    { KeyComm (right_word),     VK_RIGHT, 1, 0, 0, },

    { KeyComm (left_word_select),   VK_LEFT, 1, 1, 0, },
    { KeyComm (right_word_select),  VK_RIGHT, 1, 1, 0, },

    { KeyComm (left_subword),   VK_LEFT, 0, 0, 1, },
    { KeyComm (right_subword),  VK_RIGHT, 0, 0, 1, },

    { KeyComm (up_scroll),      VK_UP, 1, 0, 0, },
    { KeyComm (down_scroll),    VK_DOWN, 1, 0, 0, },

    { KeyComm (enter),          VK_RETURN, 0, 0, 0, },
    { KeyComm (delete),         VK_DELETE, 0, 0, 0, },
    { KeyComm (home),           VK_HOME, 0, 0, 0, },
    { KeyComm (home_select),    VK_HOME, 0, 1, 0, },
    { KeyComm (end),            VK_END, 0, 0, 0, },
    { KeyComm (end_select),     VK_END, 0, 1, 0, },
    { KeyComm (pageup),         VK_PRIOR, 0, 0, 0, },
    { KeyComm (pagedown),       VK_NEXT, 0, 0, 0, },
    { KeyComm (insert),         VK_INSERT, 0, 0, 0, },
    { KeyComm (backspace),      VK_BACK, 0, 0, 0, },
    { KeyComm (tab),            VK_TAB, 0, 0, 0, },

    { KeyComm (f1),             VK_F1, 0, 0, 0, },
    { KeyComm (f2),             VK_F2, 0, 0, 0, },
    { KeyComm (f3),             VK_F3, 0, 0, 0, },
    { KeyComm (f4),             VK_F4, 0, 0, 0, },

    { KeyComm (copy),           'C', 1, 0, 0, },
    { KeyComm (paste),          'V', 1, 0, 0, },
    { KeyComm (undo),           'Z', 1, 0, 0, },
    { KeyComm (cut),            'X', 1, 0, 0, },
    { KeyComm (find),           'F', 1, 0, 0, },
    { KeyComm (goto),           'G', 1, 0, 0, },
    { KeyComm (selectall),      'A', 1, 0, 0, },
    { KeyComm (save),           'S', 1, 0, 0, },
    { KeyComm (open),           'O', 1, 0, 0, },
    { KeyComm (new),            'N', 1, 0, 0, },

    { KeyComm (switch_tab_left), VK_LEFT, 1, 0, 1, },
    { KeyComm (switch_tab_right), VK_RIGHT, 1, 0, 1, },
};

static inline int       is_key_down (int virtkey) {
    return (!!(GetAsyncKeyState (virtkey) >> 8));
}

static inline int       scale_dpi (int value, int dpi) {
    return (MulDiv (value, dpi, 96));
}

static inline int       unscale_dpi (int value, int dpi) {
    return (MulDiv (value, 96, dpi));
}

static inline float     scale_dpi_f (float value, int dpi) {
    return (value * (dpi / 96.f));
}

static inline float     unscale_dpi_f (float value, int dpi) {
    return (value * (96.f / dpi));
}

static inline float     to_96 (float value, int dpi) {
    return (unscale_dpi_f (value, dpi));
}

static inline float     from_96 (float value, int dpi) {
    return (scale_dpi_f (value, dpi));
}

static inline int       get_normalized_pressure (int pressure, int max_pressure) {
    Assert (pressure >= 0 && pressure <= max_pressure);
    return (((float) pressure / max_pressure) * Max_Mouse_Pressure);
}

static inline float     get_pressure_01 (int pressure) {
    Assert (pressure >= 0 && pressure <= Max_Mouse_Pressure);
    return ((float) pressure / Max_Mouse_Pressure);
}

static void get_mousekeys_and_controlkeys_from_mouse_wparam (struct window *window, WPARAM w_param, enum mousekey *mousekeys, enum controlkey *controlkeys) {
    *mousekeys = 0;
    *controlkeys = 0;
    if (w_param & MK_CONTROL) {
        *controlkeys |= ControlKey (control);
    }
    if (w_param & MK_SHIFT) {
        *controlkeys |= ControlKey (shift);
    }
    if (is_key_down (VK_MENU)) {
        *controlkeys |= ControlKey (alt);
    }
    if (w_param & MK_LBUTTON) {
        *mousekeys |= MouseKey (left);
    }
    if (w_param & MK_RBUTTON) {
        *mousekeys |= MouseKey (right);
    }
    if (w_param & MK_MBUTTON) {
        *mousekeys |= MouseKey (middle);
    }
    if (w_param & MK_XBUTTON1) {
        *mousekeys |= MouseKey (extra1);
    }
    if (w_param & MK_XBUTTON2) {
        *mousekeys |= MouseKey (extra2);
    }
}

static enum controlkey  get_control_keys (void) {
    enum controlkey control;

    control = 0;
    if (GetKeyState (VK_CONTROL) & 0x8000) {
        control |= ControlKey (control);
    }
    if (GetKeyState (VK_SHIFT) & 0x8000) {
        control |= ControlKey (shift);
    }
    if (GetKeyState (VK_MENU) & 0x8000) {
        control |= ControlKey (alt);
    }
    return (control);
}

static void get_controlkeys_from_pointerinfo (struct window *window, POINTER_INFO *ptinfo, enum controlkey *controlkeys) {
    *controlkeys = 0;
    if (ptinfo->dwKeyStates & POINTER_MOD_CTRL) {
        *controlkeys |= ControlKey (control);
    }
    if (ptinfo->dwKeyStates & POINTER_MOD_SHIFT) {
        *controlkeys |= ControlKey (shift);
    }
    if (is_key_down (VK_MENU)) {
        *controlkeys |= ControlKey (alt);
    }
}

static int      get_button_from_pointer_w_param (WPARAM w_param) {
    int     index;

    if (IS_POINTER_FIRSTBUTTON_WPARAM (w_param)) {
        index = 0;
    } else if (IS_POINTER_SECONDBUTTON_WPARAM (w_param)) {
        index = 1;
    } else if (IS_POINTER_THIRDBUTTON_WPARAM (w_param)) {
        index = 2;
    } else if (IS_POINTER_FOURTHBUTTON_WPARAM (w_param)) {
        index = 3;
    } else if (IS_POINTER_FIFTHBUTTON_WPARAM (w_param)) {
        index = 4;
    } else {
        index = 9000;
    }
    return (index);
}

static int      get_mouse_pointer_button_index_from_message (UINT message, WPARAM w_param) {
    int     index;

    if (message == WM_POINTERDOWN || message == WM_POINTERUP) {
        index = GET_POINTERID_WPARAM (w_param);
    } else if (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_LBUTTONDBLCLK) {
        index = 0;
    } else if (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP || message == WM_RBUTTONDBLCLK) {
        index = 1;
    } else if (message == WM_MBUTTONDOWN || message == WM_MBUTTONUP || message == WM_MBUTTONDBLCLK) {
        index = 2;
    } else if (message == WM_XBUTTONDOWN || message == WM_XBUTTONUP || message == WM_XBUTTONDBLCLK) {
        index = 2 + GET_XBUTTON_WPARAM (w_param);
    } else {
        index = 9000;
    }
    return (index);
}

static void get_mouseinfo_from_mouse (struct window *window, struct mouseinfo *mouseinfo) {
    POINT   mousepos;

    GetCursorPos (&mousepos);
    ScreenToClient (window->handle, &mousepos);
    mouseinfo->pos[0] = mousepos.x;
    mouseinfo->pos[1] = mousepos.y;
    mouseinfo->delta[0] = mouseinfo->pos[0] - window->mousepos[0];
    mouseinfo->delta[1] = mouseinfo->pos[1] - window->mousepos[1];
    mouseinfo->wheel_delta = 0;
    mouseinfo->pressure = 0xFFFF;
}

static void get_mouseinfo_from_pointer (struct window *window, unsigned pointer_id, LPARAM l_param, struct mouseinfo *mouseinfo) {
    POINTER_INFO    ptinfo;
    POINT           mousepos;

    mousepos.x = GET_X_LPARAM (l_param);
    mousepos.y = GET_Y_LPARAM (l_param);
    ScreenToClient (window->handle, &mousepos);
    mouseinfo->pos[0] = mousepos.x;
    mouseinfo->pos[1] = mousepos.y;
    mouseinfo->delta[0] = mouseinfo->pos[0] - window->mousepos[0];
    mouseinfo->delta[1] = mouseinfo->pos[1] - window->mousepos[1];
    mouseinfo->wheel_delta = 0;
    GetPointerInfo (pointer_id, &ptinfo);
    if (ptinfo.pointerType == PT_PEN) {
        POINTER_PEN_INFO    peninfo;

        GetPointerPenInfo (pointer_id, &peninfo);
        mouseinfo->pressure = get_normalized_pressure (peninfo.pressure, 1024);
    } else {
        mouseinfo->pressure = Max_Mouse_Pressure;
    }
}

#pragma warning (suppress : 6262)
DWORD WINAPI    directory_change_watcher (_In_ LPVOID param) {
    struct dir_watcher  *watcher;
    DWORD               buffer[4 * 1024];
    HANDLE              dir;
    DWORD               count;

    watcher = param;
    dir = CreateFileA (watcher->path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if (INVALID_HANDLE_VALUE == dir) { Error ("could not open directory '%s' for monitoring", watcher->path); return 0; }
    do {
        count = 0;
        if (ReadDirectoryChangesW (dir, buffer, sizeof buffer, 0, FILE_NOTIFY_CHANGE_LAST_WRITE, &count, 0, 0)) {
            FILE_NOTIFY_INFORMATION *fni;

            fni = (FILE_NOTIFY_INFORMATION *) buffer;
            if (fni->Action == FILE_ACTION_MODIFIED) {
                char    *filename;
                int     index;

                filename = malloc (128);
                if (filename) {
                    index = 0;
                    while (index < fni->FileNameLength / 2) {
                        filename[index] = fni->FileName[index];
                        index += 1;
                    }
                    filename[index] = 0;
                    if (!PostThreadMessageW (watcher->main_thread_id, MSG_File_Change, (WPARAM) filename, 0)) {
                        Error ("could not push message to window. last error 0x%x", GetLastError ());
                    }
                }
            }
        } else {
            Error ("could not read directory changes for '%s'", watcher->path);
        }
    } while (1);
    return (0);
}

static unsigned long long   get_ms (void) {
    return (GetTickCount64 ());
}

static float    get_seconds_from_ms (unsigned long long ms) {
    return (ms / 1000.f);
}

static LRESULT CALLBACK window_proc (HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param) {
    LRESULT     result;

    if (message == WM_CREATE) {
        LPCREATESTRUCT  create = (LPCREATESTRUCT) l_param;
        struct window   *window = (struct window *) create->lpCreateParams;
        POINT       mousepos;

        Assert (window);
        window->handle = window_handle;
        SetWindowLongPtrW (window_handle, GWLP_USERDATA, (LONG_PTR) window);
        GetCursorPos (&mousepos);
        window->mousepos[0] = mousepos.x;
        window->mousepos[1] = mousepos.y;
        if (window->handlers.on_file_change) {
            window->dir_watcher.main_thread_id = GetCurrentThreadId ();
            strcpy (window->dir_watcher.path, ".\\");
            CreateThread (0, 0, directory_change_watcher, &window->dir_watcher, STACK_SIZE_PARAM_IS_A_RESERVATION, 0);
        }
        result = 0;
    } else {
        struct window *window;
        unsigned long long new_frame_version;

        window = (struct window *) (LONG_PTR) GetWindowLongPtrW (window_handle, GWLP_USERDATA);
        if (window) {
            new_frame_version = window->frame_version;
            switch (message) {
                case WM_SIZE: {
                    UINT    width, height;
                    RECT    rect;

                    window->client_width = LOWORD (l_param);
                    window->client_height = HIWORD (l_param);
                    window->client_width_96 = unscale_dpi_f (window->client_width, window->dpi);
                    window->client_height_96 = unscale_dpi_f (window->client_height, window->dpi);
                    GetWindowRect (window->handle, &rect);
                    window->width = rect.right - rect.left;
                    window->height = rect.bottom - rect.top;
                    if (window->dx.is_allocated) {
                        resize_dx (&window->dx, window->dpi);
                        if (window->handlers.on_resize) {
                            new_frame_version = window->handlers.on_resize (window, 0);
                        }
                    }
                    result = 0;
                }
                break ;
                case WM_DPICHANGED: {
                    RECT    *rect;
                    RECT    client_rect;

                    window->dpi = HIWORD (w_param);
                    rect = (RECT *) l_param;
                    window->width = unscale_dpi (rect->right - rect->left, window->dpi);
                    window->height = unscale_dpi (rect->bottom - rect->top, window->dpi);
                    GetClientRect (window->handle, &client_rect);
                    window->client_width = client_rect.right - client_rect.left;
                    window->client_height = client_rect.bottom - client_rect.top;
                    window->client_width_96 = unscale_dpi_f (window->client_width, window->dpi);
                    window->client_height_96 = unscale_dpi_f (window->client_height, window->dpi);
                    SetWindowPos (window->handle, 0, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
                    // Debug ("width %d height %d client_width %d client_height %d dpi %d", window->width, window->height, window->client_width, window->client_height, window->dpi);
                    if (window->dx.is_allocated) {
                        resize_dx (&window->dx, window->dpi);
                        if (window->handlers.on_resize) {
                            new_frame_version = window->handlers.on_resize (window, 1);
                        }
                    }
                    result = 0;
                }
                break ;
                case WM_DISPLAYCHANGE: {
                    new_frame_version += 1;
                    result = 0;
                }
                break ;
                case WM_PAINT: {
                    if (window->dx.is_allocated) {
                        unsigned long long  ms;

                        ms = get_ms ();
                        if (0 == window->last_frame_ms) {
                            window->last_frame_ms = ms;
                            window->dt = 0.016f;
                        } else {
                            window->dt = get_seconds_from_ms (ms - window->last_frame_ms);
                        }
                        if (window->dt > 1) {
                            window->dt = 1;
                        }
                        if (window->dt > 0.016f * 0.5f) {
                            if (window->handlers.on_paint) {
                                new_frame_version = window->handlers.on_paint (window);
                            }
                            if (window->frame_version == new_frame_version) {
                                window->last_frame_ms = 0;
                                ValidateRect (window->handle, 0);
                            } else {
                                window->last_frame_ms = ms;
                            }
                        }
                    }
                    result = 0;
                }
                break ;
                case WM_CHAR: {
                    if (window->handlers.on_char) {
                        new_frame_version = window->handlers.on_char (window, w_param);
                    }
                    result = 0;
                }
                break ;
                case WM_COMMAND: {
                    if (window->handlers.on_keycommand) {
                        new_frame_version = window->handlers.on_keycommand (window, LOWORD (w_param));
                    }
                    result = 0;
                }
                break ;
                case WM_KEYDOWN: {
                    if (window->handlers.on_key_down) {
                        new_frame_version = window->handlers.on_key_down (window, w_param, get_control_keys ());
                    }
                    result = 0;
                }
                break ;
                case WM_KEYUP: {
                    if (window->handlers.on_key_up) {
                        new_frame_version = window->handlers.on_key_up (window, w_param, get_control_keys ());
                    }
                    result = 0;
                }
                break ;
                case WM_MOUSEMOVE: case WM_POINTERUPDATE: {
                    struct mouseinfo    mouseinfo;

                    if (message == WM_POINTERUPDATE) {
                        get_mouseinfo_from_pointer (window, GET_POINTERID_WPARAM (w_param), l_param, &mouseinfo);
                    } else {
                        get_mouseinfo_from_mouse (window, &mouseinfo);
                    }
                    memcpy (window->mousepos, mouseinfo.pos, sizeof mouseinfo.pos);
                    // Debug ("mouse move %d %d", mouseinfo.pos[0], mouseinfo.pos[1]);
                    if (window->handlers.on_mouse_move) {
                        new_frame_version = window->handlers.on_mouse_move (window, &mouseinfo);
                    }
                    result = 0;
                }
                break ;
                case WM_MOUSEWHEEL: case WM_POINTERWHEEL: {
                    int     delta;
                    struct mouseinfo    mouseinfo;

                    if (message == WM_POINTERWHEEL) {
                        get_mouseinfo_from_pointer (window, GET_POINTERID_WPARAM (w_param), l_param, &mouseinfo);
                    } else {
                        get_mouseinfo_from_mouse (window, &mouseinfo);
                    }
                    mouseinfo.wheel_delta = GET_WHEEL_DELTA_WPARAM (w_param);
                    memcpy (window->mousepos, mouseinfo.pos, sizeof mouseinfo.pos);
                    if (window->handlers.on_mouse_wheel) {
                        new_frame_version = window->handlers.on_mouse_wheel (window, &mouseinfo);
                    }
                    result = 0;
                }
                break ;
                case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN: case WM_XBUTTONDOWN: case WM_POINTERDOWN: {
                    int             pos[2];
                    POINT           mousepos;
                    enum mousekey   mousekeys;
                    enum controlkey controlkeys;
                    struct mouseinfo    mouseinfo;
                    int             button;

                    button = get_mouse_pointer_button_index_from_message (message, w_param);
                    if (message == WM_POINTERDOWN) {
                        get_mouseinfo_from_pointer (window, GET_POINTERID_WPARAM (w_param), l_param, &mouseinfo);
                        // Debug ("WM_POINTERDOWN button:%d", button);
                    } else {
                        get_mouseinfo_from_mouse (window, &mouseinfo);
                    }
                    memcpy (window->mousepos, mouseinfo.pos, sizeof mouseinfo.pos);
                    if (message == WM_POINTERDOWN) {
                        POINTER_INFO    ptinfo;
                        int             index;

                        GetPointerInfo (GET_POINTERID_WPARAM (w_param), &ptinfo);
                        index = get_button_from_pointer_w_param (w_param);
                        mousekeys = 1 << index;
                        get_controlkeys_from_pointerinfo (window, &ptinfo, &controlkeys);
                        // if (ptinfo.pointerType == PT_PEN) {
                        //  Debug ("PEN! %x", mousekeys);
                        // } else {
                        //  Debug ("NOT A PEN! %x", mousekeys);
                        // }
                    } else {
                        get_mousekeys_and_controlkeys_from_mouse_wparam (window, w_param, &mousekeys, &controlkeys);
                    }
                    if (0 == window->is_mouse_captured) {
                        // Debug ("CAPTURE_ACQUIRED");
                        window->is_mouse_captured = 1;
                        window->captured_mouse_button = button;
                        if (message != WM_POINTERDOWN) {
                            SetCapture (window->handle);
                        }
                        if (window->handlers.on_mouse_capture) {
                            new_frame_version = window->handlers.on_mouse_capture (window, 1, &mouseinfo, mousekeys, controlkeys);
                        }
                    } else {
                        if (window->handlers.on_mouse_button) {
                            new_frame_version = window->handlers.on_mouse_button (window, &mouseinfo, mousekeys, controlkeys);
                        }
                    }
                    result = 0;
                }
                break ;
                case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP: case WM_XBUTTONUP: case WM_POINTERUP: {
                    int                 button;
                    struct mouseinfo    mouseinfo;

                    button = get_mouse_pointer_button_index_from_message (message, w_param);
                    if (message == WM_POINTERUP) {
                        get_mouseinfo_from_pointer (window, GET_POINTERID_WPARAM (w_param), l_param, &mouseinfo);
                        // Debug ("WM_POINTERUP button:%d", button);
                    } else {
                        get_mouseinfo_from_mouse (window, &mouseinfo);
                    }
                    memcpy (window->mousepos, mouseinfo.pos, sizeof mouseinfo.pos);
                    if (window->is_mouse_captured && window->captured_mouse_button == button) {
                        if (message == WM_POINTERUP) {
                            // Debug ("CAPTURE_RELEASED");
                            window->is_mouse_captured = 0;
                            if (window->handlers.on_mouse_capture) {
                                new_frame_version = window->handlers.on_mouse_capture (window, 0, &mouseinfo, 0, 0);
                            }
                        } else {
                            ReleaseCapture ();
                        }
                    } else {
                        enum mousekey   mousekeys;
                        enum controlkey controlkeys;

                        get_mousekeys_and_controlkeys_from_mouse_wparam (window, w_param, &mousekeys, &controlkeys);
                        if (window->handlers.on_mouse_button) {
                            new_frame_version = window->handlers.on_mouse_button (window, &mouseinfo, mousekeys, controlkeys);
                        }
                    }
                    result = 0;
                }
                break ;
                case WM_CAPTURECHANGED: case WM_POINTERCAPTURECHANGED: {
                    if (window->is_mouse_captured) {
                        struct mouseinfo    mouseinfo;

                        // Debug ("CAPTURE_RELEASED");
                        if (message == WM_POINTERCAPTURECHANGED) {
                            get_mouseinfo_from_pointer (window, GET_POINTERID_WPARAM (w_param), l_param, &mouseinfo);
                        } else {
                            get_mouseinfo_from_mouse (window, &mouseinfo);
                        }
                        memcpy (window->mousepos, mouseinfo.pos, sizeof mouseinfo.pos);
                        window->is_mouse_captured = 0;
                        if (window->handlers.on_mouse_capture) {
                            new_frame_version = window->handlers.on_mouse_capture (window, 0, &mouseinfo, 0, 0);
                        }
                    }
                    result = 0;
                }
                break ;
                case WM_POINTERACTIVATE: {
                    // Debug ("WM_POINTERACTIVATE");
                    result = PA_ACTIVATE;
                }
                break ;
                case WM_ACTIVATE: {
                    int     is_active, is_active_by_click;
                    struct mouseinfo    mouseinfo;

                    get_mouseinfo_from_mouse (window, &mouseinfo);
                    is_active_by_click = 0;
                    if (w_param == WA_ACTIVE) {
                        is_active = 1;
                    } else if (w_param == WA_CLICKACTIVE) {
                        is_active = 1;
                        is_active_by_click = 1;
                    } else {
                        is_active = 0;
                    }
                    // Debug ("WM_ACTIVATE %d %d", mouseinfo.pos[0], mouseinfo.pos[0]);
                    memcpy (window->mousepos, mouseinfo.pos, sizeof mouseinfo.pos);
                    if (window->handlers.on_activate) {
                        new_frame_version = window->handlers.on_activate (window, is_active, is_active_by_click, mouseinfo.pos);
                    }
                    result = 0;
                }
                break ;
                case WM_POINTERDEVICECHANGE:
                case WM_POINTERDEVICEINRANGE:
                case WM_POINTERDEVICEOUTOFRANGE:
                case WM_POINTERENTER:
                case WM_POINTERLEAVE:
                case WM_POINTERROUTEDAWAY:
                case WM_POINTERROUTEDRELEASED:
                case WM_POINTERROUTEDTO:
                case WM_POINTERHWHEEL: {
                    result = 0;
                }
                break ;
                case WM_DESTROY: {
                    PostQuitMessage (0);
                    result = 1;
                }
                break ;
                default: {
                    result = DefWindowProc (window_handle, message, w_param, l_param);
                }
                break ;
            }
            if (new_frame_version != window->frame_version) {
                window->frame_version = new_frame_version;
                InvalidateRect (window->handle, 0, FALSE);
            }
        } else {
            result = DefWindowProc (window_handle, message, w_param, l_param);
        }
    }
    return (result);
}

// #pragma comment (lib, "UxTheme.lib")
// #pragma comment (lib, "dwmapi.lib")
static void enable_dark_theme_if_needed (HWND window_handle) {
    typedef int (*TYPE_AllowDarkModeForWindow) (HWND a_HWND, BOOL a_Allow);
    TYPE_AllowDarkModeForWindow AllowDarkModeForWindow;
    HANDLE huxtheme, dwm;

    huxtheme = GetModuleHandleW(L"uxtheme.dll");
    if (!huxtheme) {
        static HANDLE   guxtheme = 0;

        if (!guxtheme) {
            guxtheme = LoadLibraryA ("uxtheme.dll");
        }
        huxtheme = guxtheme;
    }
    dwm = GetModuleHandleW(L"dwmapi.dll");
    if (!dwm) {
        static HANDLE   gdwm = 0;

        if (!gdwm) {
            gdwm = LoadLibraryA ("dwmapi.dll");
        }
        dwm = gdwm;
    }
    if (huxtheme && dwm) {
        BOOLEAN (WINAPI * pShouldAppsUseDarkMode)(void) = (void *) GetProcAddress (huxtheme, MAKEINTRESOURCEA(132));
        AllowDarkModeForWindow = (TYPE_AllowDarkModeForWindow) GetProcAddress (huxtheme, MAKEINTRESOURCEA(133));
        HRESULT (WINAPI *pDwmSetWindowAttribute) (HWND, DWORD, LPCVOID, DWORD) = (void *) GetProcAddress (dwm, "DwmSetWindowAttribute");
        if (pShouldAppsUseDarkMode && AllowDarkModeForWindow && pDwmSetWindowAttribute) {
            if (pShouldAppsUseDarkMode ()) {
                AllowDarkModeForWindow (window_handle, 1);
                // SetWindowTheme (window_handle, L"Explorer", 0);
                SetWindowTheme (window_handle, L"DarkMode_Explorer", 0);
                BOOL dark = 1;
                pDwmSetWindowAttribute (window_handle, 20, &dark, sizeof dark);
            }
        } else {
            Error ("no pShouldAppsUseDarkMode or AllowDarkModeForWindow or pDwmSetWindowAttribute");
        }
    } else {
        Error ("no uxtheme or dwm dll %p %p", (void *) huxtheme, (void *) dwm);
    }
}

static void init_console_output (int allocate_if_needed) {
    if (AttachConsole (ATTACH_PARENT_PROCESS) || (allocate_if_needed && AllocConsole ())) {
        (void) freopen ("CON", "w", stdout);
        (void) freopen ("CON", "w", stderr);
        printf ("\noutput attached to this console\n\n");
    }
}

static int      create_window (struct window *window, HINSTANCE instance, const short *title) {
    int         result;
    WNDCLASSEXW wc;

    Zero (&wc);
    wc.cbSize = sizeof wc;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND);
    wc.lpszClassName = title ? title : L"-";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor = LoadCursor (0, IDC_ARROW);
    if (RegisterClassExW (&wc)) {
        if (CreateWindowW (wc.lpszClassName, title ? title : L"-", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0, instance, window)) {
            RECT    client_rect;
            ACCEL   accels[Array_Count (g_keycommand_shortcuts)];
            int     index;

            index = 0;
            while (index < Array_Count (accels)) {
                accels[index].fVirt = FVIRTKEY;
                if (g_keycommand_shortcuts[index].is_control) {
                    accels[index].fVirt |= FCONTROL;
                }
                if (g_keycommand_shortcuts[index].is_shift) {
                    accels[index].fVirt |= FSHIFT;
                }
                if (g_keycommand_shortcuts[index].is_alt) {
                    accels[index].fVirt |= FALT;
                }
                accels[index].key = g_keycommand_shortcuts[index].virtkey;
                accels[index].cmd = g_keycommand_shortcuts[index].cmd;
                index += 1;
            }
            window->accel = CreateAcceleratorTableA (accels, Array_Count (accels));
            if (window->accel) {
                if (window->width == 0 || window->height == 0) {
                    window->width = 800;
                    window->height = 600;
                }
                window->dpi = GetDpiForWindow (window->handle);
                SetWindowPos (window->handle, 0, 0, 0, scale_dpi (window->width, window->dpi), scale_dpi (window->height, window->dpi), SWP_NOMOVE);
                GetClientRect (window->handle, &client_rect);
                window->client_width = client_rect.right - client_rect.left;
                window->client_height = client_rect.bottom - client_rect.top;
                window->client_width_96 = unscale_dpi_f (window->client_width, window->dpi);
                window->client_height_96 = unscale_dpi_f (window->client_height, window->dpi);
                enable_dark_theme_if_needed (window->handle);
                if (allocate_dx (&window->dx, window->dpi, window->handle)) {
                    result = 1;
                } else {
                    Error ("cannot initialize DirectX 11 context");
                    result = 0;
                }
            } else {
                Error ("cannot create accelerator table");
                result = 0;
            }
        } else {
            Error ("cannot create window");
            result = 0;
        }
    } else {
        Error ("cannot register class");
        result = 0;
    }
    return (result);
}

static void release_window (struct window *window) {
    if (window->handle) {
        DestroyWindow (window->handle);
    }
    if (window->dx.is_allocated) {
        release_dx (&window->dx);
    }
    Zero (window);
}

static int      dxwinmain (HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdline, int showcmd, const short *title, struct window_handlers *handlers, void *userdata) {
    int     result;

    init_console_output (0);
    if (SUCCEEDED (CoInitialize (0))) {
        struct window   window;

        Zero (&window);
        SetProcessDpiAwarenessContext (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        EnableMouseInPointer (TRUE);
        window.userdata = userdata;
        if (handlers) {
            window.handlers = *handlers;
        }
        if (create_window (&window, instance, title) && (0 == handlers || 0 == handlers->initialize || handlers->initialize (&window))) {
            MSG         msg;

            ShowWindow (window.handle, SW_SHOWNORMAL);
            UpdateWindow (window.handle);
            Zero (&msg);
            window.alive = 1;
            while (window.alive && GetMessage (&msg, 0, 0, 0) > 0) {
                if (msg.message == MSG_File_Change) {
                    if (window.handlers.on_file_change) {
                        unsigned long long      new_frame_version;

                        new_frame_version = window.handlers.on_file_change (&window, (char *) msg.wParam);
                        if (new_frame_version != window.frame_version) {
                            window.frame_version = new_frame_version;
                            InvalidateRect (window.handle, 0, FALSE);
                        }
                    }
                    free ((char *) msg.wParam);
                    result = 0;
                } else if (0 == TranslateAccelerator (window.handle, window.accel, &msg)) {
                    TranslateMessage (&msg);
                    DispatchMessage (&msg);
                }
            }
            result = 1;
            if (handlers && handlers->release) {
                handlers->release (&window);
            }
        } else {
            result = 0;
        }
        release_window (&window);
        CoUninitialize ();
    } else {
        Error ("cannot initialize COM");
        result = 0;
    }
    return (result);
}



struct continuous_keys {
    int     left, right, up, down, forward, backward;
};

static int  update_continuous_keys (struct continuous_keys *keys, int virtkey, int value) {
    int     result;

    result = 1;
    if (virtkey == VK_LEFT || virtkey == 'A') {
        keys->left = value;
    } else if (virtkey == VK_RIGHT || virtkey == 'D') {
        keys->right = value;
    } else if (virtkey == 'E') {
        keys->up = value;
    } else if (virtkey == 'Q') {
        keys->down = value;
    } else if (virtkey == VK_UP || virtkey == 'W') {
        keys->forward = value;
    } else if (virtkey == VK_DOWN || virtkey == 'S') {
        keys->backward = value;
    } else {
        result = 0;
    }
    return (result);
}

static void clear_continuous_keys (struct continuous_keys *keys) {
    memset (keys, 0, sizeof *keys);
}

static int  is_continuous_keys (struct continuous_keys *keys) {
    return (keys->left || keys->right || keys->up || keys->down || keys->forward || keys->backward);
}

static int  reload_continuous_keys (struct continuous_keys *keys) {
    int     result;

    result = 0;
    clear_continuous_keys (keys);
    if (!is_key_down (VK_CONTROL) && !is_key_down (VK_SHIFT) && !is_key_down (VK_MENU)) {
        if (is_key_down (VK_LEFT) || is_key_down ('A')) {
            keys->left = 1;
        }
        if (is_key_down (VK_RIGHT) || is_key_down ('D')) {
            keys->right = 1;
        }
        if (is_key_down ('E')) {
            keys->up = 1;
        }
        if (is_key_down ('Q')) {
            keys->down = 1;
        }
        if (is_key_down (VK_UP) || is_key_down ('W')) {
            keys->forward = 1;
        }
        if (is_key_down (VK_DOWN) || is_key_down ('S')) {
            keys->backward = 1;
        }
        result = is_continuous_keys (keys);
    }
    return (result);
}
