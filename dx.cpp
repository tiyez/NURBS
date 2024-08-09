
/* deps: __Platform_Windows_10_SDK */

#include <sstream>
#include <iostream>

#include <d2d1.h>
#include <d2d1_1.h>
#include <dxgi1_2.h>
#include <dxgi1_6.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <dwrite.h>
#include <wincodec.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

using namespace DirectX;

#pragma comment (lib, "d2d1.lib")
#pragma comment (lib, "D3D11.lib")
#pragma comment (lib, "DXGI.lib")
#pragma comment (lib, "dwrite.lib")
#pragma comment (lib, "Gdi32.lib")
#pragma comment (lib, "User32.lib")
#pragma comment (lib, "d3dcompiler.lib")

extern "C" {
#include "essentials.c"
#include "memory.c"
#include "dx.c"
#include <3plib/srgb-transform.c>
#include "color.c"
}

#undef __File
#define __File "dx.cpp"

#define Throw_If_Failed(...) throw_if_failed ((__VA_ARGS__), __File, __LINE__, #__VA_ARGS__)
inline void     throw_if_failed (HRESULT hr, const char *filename, int line, const char *message) {
    if (FAILED (hr)) {
        std::ostringstream  ss;

        ss << message << " (" << filename << ":" << line << ") [0x" << std::hex << std::uppercase << hr << "]";
        throw std::runtime_error (ss.str());
    }
}
#define Crash_If_Failed(...) crash_if_failed ((__VA_ARGS__), __File, __LINE__, #__VA_ARGS__)
inline void     crash_if_failed (HRESULT hr, const char *filename, int line, const char *message) {
    if (FAILED (hr)) {
        std::cerr << message << " (" << filename << ":" << line << ") [0x" << std::hex << std::uppercase << hr << "]" << std::endl;
        Assert (0);
    }
}
#define Report_If_Failed(...) report_if_failed ((__VA_ARGS__), __File, __LINE__, #__VA_ARGS__)
inline int      report_if_failed (HRESULT hr, const char *filename, int line, const char *message) {
    if (FAILED (hr)) {
        std::cerr << filename << "(" << line << "): " << message << " [0x" << std::hex << std::uppercase << hr << "]" << std::endl;
    }
    return (SUCCEEDED (hr));
}
#define Safe_Release(acc) do if (acc) { (acc)->Release (); (acc) = 0; } while (0)

static const DXGI_FORMAT    g_pixel_dxformats[Pixel (_count)] = {
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_R16G16B16A16_UNORM,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
};

extern void release_dx (_In_ struct dx *dx) {
    Safe_Release (dx->d2dfactory);
    Safe_Release (dx->d2ddevice);
    Safe_Release (dx->d2dcontext);
    Safe_Release (dx->dwfactory);
    Safe_Release (dx->d3ddevice);
    Safe_Release (dx->d3dcontext);
    Safe_Release (dx->target_bitmap);
    Safe_Release (dx->render_target);
    Safe_Release (dx->swapchain);
    Zero (dx);
}

static inline int   compute_intersection_area (int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2) {
    return max (0, min (ax2, bx2) - max (ax1, bx1)) * max (0, min (ay2, by2) - max (ay1, by1));
}

static HRESULT  enumerate_dxgi_adapter_for_output (_In_ IDXGIAdapter1 *adapter, HWND window_handle, _Out_ IDXGIOutput **poutput) {
    IDXGIOutput *current_output;
    int         index;
    HRESULT     hr;
    RECT        winrect, outrect;
    int         best_intersect_area;

    GetWindowRect (window_handle, &winrect);
    best_intersect_area = -1;
    hr = S_OK;
    current_output = 0;
    *poutput = 0;
    index = 0;
    while (adapter->EnumOutputs (index, &current_output) != DXGI_ERROR_NOT_FOUND) {
        int     intersect_area;
        int     ax1, ay1, ax2, ay2;
        int     bx1, by1, bx2, by2;
        DXGI_OUTPUT_DESC    desc;

        ax1 = winrect.left;
        ay1 = winrect.top;
        ax2 = winrect.right;
        ay2 = winrect.bottom;
        if (FAILED (hr = current_output->GetDesc (&desc))) {
            Safe_Release (current_output);
            return (hr);
        }
        outrect = desc.DesktopCoordinates;
        bx1 = outrect.left;
        by1 = outrect.top;
        bx2 = outrect.right;
        by2 = outrect.bottom;
        intersect_area = compute_intersection_area (ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);
        if (intersect_area > best_intersect_area) {
            Safe_Release (*poutput);
            *poutput = current_output;
            current_output = 0;
            best_intersect_area = intersect_area;
        } else {
            Safe_Release (current_output);
        }
        index += 1;
    }
    if (0 == *poutput) {
        hr = E_FAIL;
    }
    return (hr);
}

extern int      allocate_dx (_Out_ struct dx *dx, int dpi, HWND window_handle) {
    int                     result;
    UINT                    creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL   feature_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
    };
    IDXGIDevice1            *dxgi_device = 0;
    IDXGIAdapter1           *adapter = 0;
    IDXGIFactory2           *factory2 = 0;
    IDXGISurface            *dxgi_back_buffer = 0;
    ID3D11Texture2D         *d3d_back_buffer = 0;
    D2D1_BITMAP_PROPERTIES1 bitmap_props;
    IDXGIOutput             *output = 0;
    IDXGIOutput6            *output6 = 0;
    DXGI_SWAP_CHAIN_DESC1   desc;

    result = 1;
    Zero (dx);
    try {
        /* d3d setup */
        Throw_If_Failed (D3D11CreateDevice (0, D3D_DRIVER_TYPE_HARDWARE, 0, creation_flags, feature_levels, ARRAYSIZE (feature_levels), D3D11_SDK_VERSION, &dx->d3ddevice, &dx->d3dfeature_level, &dx->d3dcontext));
        Throw_If_Failed (dx->d3ddevice->QueryInterface(__uuidof (IDXGIDevice1), (void **) &dxgi_device));
        Throw_If_Failed (dxgi_device->SetMaximumFrameLatency (1));
        Throw_If_Failed (dxgi_device->GetParent (__uuidof (IDXGIAdapter1), (void **) &adapter));
        Throw_If_Failed (adapter->GetParent (__uuidof (IDXGIFactory2), (void **) &factory2));
        Throw_If_Failed (enumerate_dxgi_adapter_for_output (adapter, window_handle, &output));
        Zero (&desc);
        desc.Width = 0;
        desc.Height = 0;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.Stereo = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.Scaling = DXGI_SCALING_NONE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.Flags = 0;
        if (SUCCEEDED (output->QueryInterface (__uuidof (IDXGIOutput6), (void **) &output6))) {
            DXGI_OUTPUT_DESC1   desc1;

            Throw_If_Failed (output6->GetDesc1 (&desc1));
            if (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
                desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                dx->is_hdr = 1;
            }
        }
        dx->format = desc.Format;
        Throw_If_Failed (factory2->CreateSwapChainForHwnd (dx->d3ddevice, window_handle, &desc, 0, 0, &dx->swapchain));
        Throw_If_Failed (dx->swapchain->GetBuffer (0, __uuidof (IDXGISurface), (void **) &dxgi_back_buffer));
        Throw_If_Failed (dxgi_back_buffer->QueryInterface (__uuidof (ID3D11Texture2D), (void **) &d3d_back_buffer));
        Throw_If_Failed (dx->d3ddevice->CreateRenderTargetView (d3d_back_buffer, 0, &dx->render_target));

        /* d2d setup */
        Throw_If_Failed (D2D1CreateFactory (D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof (ID2D1Factory1), (void **) &dx->d2dfactory));
        Throw_If_Failed (dx->d2dfactory->CreateDevice (dxgi_device, &dx->d2ddevice));
        Throw_If_Failed (dx->d2ddevice->CreateDeviceContext (D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &dx->d2dcontext));
        bitmap_props = D2D1::BitmapProperties1 (D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, D2D1::PixelFormat (dx->format, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);
        Throw_If_Failed (dx->d2dcontext->CreateBitmapFromDxgiSurface (dxgi_back_buffer, &bitmap_props, &dx->target_bitmap));
        dx->d2dcontext->SetTarget (dx->target_bitmap);
        Throw_If_Failed (dx->d2dcontext->CreateSolidColorBrush (D2D1::ColorF (1.f, 1.f, 1.f, 1.f), &dx->default_text_brush));

        /* dw setup */
        Throw_If_Failed (DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (IDWriteFactory), (IUnknown **) &dx->dwfactory));

        dx->is_allocated = 1;
    } catch (std::exception &exception) {
        Error ("cannot allocate dx resources: %s", exception.what ());
        result = 0;
    }
    Safe_Release (dxgi_device);
    Safe_Release (adapter);
    Safe_Release (factory2);
    Safe_Release (dxgi_back_buffer);
    Safe_Release (d3d_back_buffer);
    Safe_Release (output);
    Safe_Release (output6);
    return (result);
}

extern int      resize_dx (_In_ struct dx *dx, int dpi) {
    int         result;

    if (dx->is_allocated) {
        IDXGISurface    *dxgi_back_buffer = 0;
        ID3D11Texture2D *d3d_back_buffer = 0;
        D2D1_BITMAP_PROPERTIES1 bitmap_props;

        try {
            Safe_Release (dx->default_text_brush);
            Safe_Release (dx->target_bitmap);
            Safe_Release (dx->render_target);
            dx->d2dcontext->SetTarget (0);
            Throw_If_Failed (dx->swapchain->ResizeBuffers (0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));
            Throw_If_Failed (dx->swapchain->GetBuffer (0, __uuidof (IDXGISurface), (void **) &dxgi_back_buffer));
            Throw_If_Failed (dxgi_back_buffer->QueryInterface (__uuidof (ID3D11Texture2D), (void **) &d3d_back_buffer));
            Throw_If_Failed (dx->d3ddevice->CreateRenderTargetView (d3d_back_buffer, 0, &dx->render_target));
            bitmap_props = D2D1::BitmapProperties1 (D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, D2D1::PixelFormat (dx->format, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);
            Throw_If_Failed (dx->d2dcontext->CreateBitmapFromDxgiSurface (dxgi_back_buffer, &bitmap_props, &dx->target_bitmap));
            dx->d2dcontext->SetTarget (dx->target_bitmap);
            Throw_If_Failed (dx->d2dcontext->CreateSolidColorBrush (D2D1::ColorF (1.f, 1.f, 1.f, 1.f), &dx->default_text_brush));
            result = 1;
        } catch (std::exception exception) {
            Error ("cannot resize dx resources: %s", exception.what ());
            result = 0;
        }
        Safe_Release (d3d_back_buffer);
        Safe_Release (dxgi_back_buffer);
    } else {
        result = 0;
    }
    return (result);
}

extern int      get_dx_dpi (_In_ struct dx *dx) {
    FLOAT   dpi, vdpi;

    dx->target_bitmap->GetDpi (&dpi, &vdpi);
    return (dpi);
}

extern int      begin_dx (_In_ struct dx *dx) {
    int             result;

    if (dx->is_allocated) {
        int             dpi;

        dpi = get_dx_dpi (dx);
        dx->d2dcontext->SetDpi (dpi, dpi);
        dx->d2dcontext->BeginDraw ();
        result = 1;
    } else {
        Error ("DX is not allocated");
        result = 0;
    }
    return (result);
}

extern void setup_dx_render_target (_In_ struct dx *dx, _In_ DXt (ID3D11RenderTargetView) *target, _In_opt_ DXt (ID3D11DepthStencilView) *depth) {
    dx->d3dcontext->OMSetRenderTargets (1, &target, depth);
}

extern void setup_dx_input_assembler (_In_ struct dx *dx, _In_opt_ DXt (ID3D11InputLayout) *input_layout, DXi (D3D11_PRIMITIVE_TOPOLOGY) topology, _In_opt_ DXt (ID3D11Buffer) *vertex_buffer, int stride, int offset) {
    unsigned    strides[] = { (unsigned) stride };
    unsigned    offsets[] = { (unsigned) offset };

    dx->d3dcontext->IASetInputLayout (input_layout);
    dx->d3dcontext->IASetPrimitiveTopology (topology);
    if (input_layout) {
        Assert (vertex_buffer);
        dx->d3dcontext->IASetVertexBuffers (0, 1, &vertex_buffer, strides, offsets);
    }
}

extern void setup_dx_rasterizer_state (_In_ struct dx *dx, _In_ DXt (ID3D11RasterizerState) *rasterizer) {
    dx->d3dcontext->RSSetState (rasterizer);
}

extern void setup_dx_blend_state (_In_ struct dx *dx, _In_ DXt (ID3D11BlendState) *blend) {
    dx->d3dcontext->OMSetBlendState (blend, 0, 0xFFFFFFFF);
}

extern void setup_dx_depth_state (_In_ struct dx *dx, _In_ DXt (ID3D11DepthStencilState) *depth_state) {
    dx->d3dcontext->OMSetDepthStencilState (depth_state, 1);
}

extern void setup_dx_viewport (_In_ struct dx *dx, int x, int y, int width, int height) {
    D3D11_VIEWPORT  vp;

    vp.TopLeftX = x;
    vp.TopLeftY = y;
    vp.Width = width;
    vp.Height = height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    dx->d3dcontext->RSSetViewports (1, &vp);
}

extern void setup_dx_vertex_shader (_In_ struct dx *dx, _In_ DXt (ID3D11VertexShader) *shader) {
    dx->d3dcontext->VSSetShader (shader, 0, 0);
}

extern void setup_dx_pixel_shader (_In_ struct dx *dx, _In_ DXt (ID3D11PixelShader) *shader) {
    dx->d3dcontext->PSSetShader (shader, 0, 0);
}

extern void setup_dx_domain_shader (_In_ struct dx *dx, _In_ DXt (ID3D11DomainShader) *shader) {
    dx->d3dcontext->DSSetShader (shader, 0, 0);
}

extern void setup_dx_hull_shader (_In_ struct dx *dx, _In_ DXt (ID3D11HullShader) *shader) {
    dx->d3dcontext->HSSetShader (shader, 0, 0);
}

extern void setup_dx_constant_buffer_for_vertex_shader (_In_ struct dx *dx, int index, _In_ DXt (ID3D11Buffer) *buffer) {
    dx->d3dcontext->VSSetConstantBuffers (index, 1, &buffer);
}

extern void setup_dx_constant_buffer_for_pixel_shader (_In_ struct dx *dx, int index, _In_ DXt (ID3D11Buffer) *buffer) {
    dx->d3dcontext->PSSetConstantBuffers (index, 1, &buffer);
}

extern void setup_dx_constant_buffer_for_domain_shader (_In_ struct dx *dx, int index, _In_ DXt (ID3D11Buffer) *buffer) {
    dx->d3dcontext->DSSetConstantBuffers (index, 1, &buffer);
}

extern void setup_dx_constant_buffer_for_hull_shader (_In_ struct dx *dx, int index, _In_ DXt (ID3D11Buffer) *buffer) {
    dx->d3dcontext->HSSetConstantBuffers (index, 1, &buffer);
}

extern void dx_clear_render_target (_In_ struct dx *dx, _In_ DXt (ID3D11RenderTargetView) *target, unsigned clear_color) {
    float bgcolorf[4];

    convert_uint_color_to_floats (clear_color, bgcolorf, dx->is_hdr);
    dx->d3dcontext->ClearRenderTargetView (target, bgcolorf);
}

extern void dx_clear_depth_view (_In_ struct dx *dx, _In_ DXt (ID3D11DepthStencilView) *view, float value) {
    dx->d3dcontext->ClearDepthStencilView (view, D3D11_CLEAR_DEPTH, value, 0);
}

extern void dx_update_buffer (_In_ struct dx *dx, _In_ DXt (ID3D11Buffer) *buffer, _In_reads_bytes_ (size) const void *data, int size) {
    D3D11_MAPPED_SUBRESOURCE    subres;
    HRESULT                     hr;

    Zero (&subres);
    if (SUCCEEDED (hr = dx->d3dcontext->Map (buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subres))) {
        memcpy (subres.pData, data, size);
        dx->d3dcontext->Unmap (buffer, 0);
    } else {
        Error ("dx->d3dcontext->Map failed with 0x%x", hr);
    }
}

extern void dx_draw (_In_ struct dx *dx, int count, int offset) {
    dx->d3dcontext->Draw (count, offset);
}

extern void dx_draw_rectangle (_In_ struct dx *dx, int x, int y, int width, int height, unsigned color) {
    ID2D1SolidColorBrush    *brush;

    brush = make_solid_brush (dx->d2dcontext, color, dx->is_hdr);
    if (brush) {
        dx->d2dcontext->FillRectangle (D2D1_RECT_F { (float) x, (float) y, (float) x + width, (float) y + height }, brush);
        brush->Release ();
    }
}

extern void dx_draw_rectangle_border (_In_ struct dx *dx, int x, int y, int width, int height, unsigned color) {
    ID2D1SolidColorBrush    *brush;

    brush = make_solid_brush (dx->d2dcontext, color, dx->is_hdr);
    if (brush) {
        dx->d2dcontext->DrawRectangle (D2D1_RECT_F { (float) x, (float) y, (float) x + width, (float) y + height }, brush);
        brush->Release ();
    }
}

extern void dx_draw_text_layout (_In_ struct dx *dx, _In_ DXt (IDWriteTextLayout) *layout, float x, float y, unsigned color, DXi (D2D1_DRAW_TEXT_OPTIONS) options) {
    D2D1_POINT_2F   origin = { x, y, };
    ID2D1SolidColorBrush    *brush;

    if (color) {
        brush = make_solid_brush (dx->d2dcontext, color, dx->is_hdr);
    } else {
        brush = dx->default_text_brush;
    }
    if (brush) {
        dx->d2dcontext->DrawTextLayout (origin, layout, brush, options);
        if (color) {
            brush->Release ();
        }
    }
}

extern void dx_draw_2d_line (_In_ struct dx *dx, _In_reads_ (2) float start [2], _In_reads_ (2) float end [2], float width, unsigned color) {
    ID2D1SolidColorBrush    *brush;

    if (color) {
        brush = make_solid_brush (dx->d2dcontext, color, dx->is_hdr);
    } else {
        brush = dx->default_text_brush;
    }
    if (brush) {
        D2D1_POINT_2F   point0, point1;

        point0.x = start [0];
        point0.y = start [1];
        point1.x = end [0];
        point1.y = end [1];
        dx->d2dcontext->DrawLine (point0, point1, brush, width, 0);
    }
}

extern int  end_dx (_In_ struct dx *dx) {
    int         result;

    result = Report_If_Failed (dx->d2dcontext->EndDraw ());
    result = Report_If_Failed (dx->swapchain->Present (1, 0)) && result;
    return (result);
}

_Ret_maybenull_ extern ID3DBlob *compile_shader (_In_ const char *text, int text_size, _In_z_ const char *filename, _In_z_ const char *entrypoint, _In_z_ const char *profile, _Inout_ char *_Stack *perror) {
    int         flags;
    ID3DBlob    *shader_blob;
    ID3DBlob    *error_blob;

    shader_blob = 0;
    error_blob = 0;
    flags = D3DCOMPILE_ENABLE_STRICTNESS;
    if (FAILED (D3DCompile (text, text_size, filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entrypoint, profile, flags, 0, &shader_blob, &error_blob))) {
        if (error_blob) {
            if (0 == *perror) {
                *perror = (char *) allocate_stack (1);
            }
            push_stack_data (*perror, error_blob->GetBufferPointer (), error_blob->GetBufferSize ());
            error_blob->Release();
        }
        if (shader_blob) {
            shader_blob->Release();
            shader_blob = 0;
        }
    }
    return (shader_blob);
}

extern void release_blob (_In_ DXt (ID3DBlob) **pblob) {
    (*pblob)->Release ();
    *pblob = 0;
}

_Ret_notnull_ extern void   *get_blob_data (_In_ DXt (ID3DBlob) *blob) {
    return (blob->GetBufferPointer ());
}

extern size_t   get_blob_size (_In_ DXt (ID3DBlob) *blob) {
    return (blob->GetBufferSize ());
}

_Ret_maybenull_ extern DXt (ID3D11PixelShader)  *make_pixel_shader (_In_ DXt (ID3D11Device) *device, _In_reads_ (data_size) const void *data, size_t data_size) {
    ID3D11PixelShader *shader;
    HRESULT hr;

    shader = 0;
    if (FAILED (hr = device->CreatePixelShader (data, data_size, 0, &shader))) {
        Error ("device->CreatePixelShader failed with 0x%x", hr);
    }
    return (shader);
}

_Ret_maybenull_ extern DXt (ID3D11VertexShader) *make_vertex_shader (_In_ DXt (ID3D11Device) *device, _In_reads_ (data_size) const void *data, size_t data_size) {
    ID3D11VertexShader *shader;
    HRESULT hr;

    shader = 0;
    if (FAILED (hr = device->CreateVertexShader (data, data_size, 0, &shader))) {
        Error ("device->CreateVertexShader failed with 0x%x", hr);
    }
    return (shader);
}

_Ret_maybenull_ extern DXt (ID3D11DomainShader) *make_domain_shader (_In_ DXt (ID3D11Device) *device, _In_reads_ (data_size) const void *data, size_t data_size) {
    ID3D11DomainShader *shader;
    HRESULT hr;

    shader = 0;
    if (FAILED (hr = device->CreateDomainShader (data, data_size, 0, &shader))) {
        Error ("device->CreateDomainShader failed with 0x%x", hr);
    }
    return (shader);
}

_Ret_maybenull_ extern DXt (ID3D11HullShader) *make_hull_shader (_In_ DXt (ID3D11Device) *device, _In_reads_ (data_size) const void *data, size_t data_size) {
    ID3D11HullShader *shader;
    HRESULT hr;

    shader = 0;
    if (FAILED (hr = device->CreateHullShader (data, data_size, 0, &shader))) {
        Error ("device->CreateHullShader failed with 0x%x", hr);
    }
    return (shader);
}

extern void release_pixel_shader (_In_ DXt (ID3D11PixelShader) **pshader) {
    if (*pshader) {
        (*pshader)->Release ();
        *pshader = 0;
    }
}

extern void release_vertex_shader (_In_ DXt (ID3D11VertexShader) **pshader) {
    if (*pshader) {
        (*pshader)->Release ();
        *pshader = 0;
    }
}

extern void release_domain_shader (_In_ DXt (ID3D11DomainShader) **pshader) {
    if (*pshader) {
        (*pshader)->Release ();
        *pshader = 0;
    }
}

extern void release_hull_shader (_In_ DXt (ID3D11HullShader) **pshader) {
    if (*pshader) {
        (*pshader)->Release ();
        *pshader = 0;
    }
}

_Ret_maybenull_ extern DXt (ID3D11InputLayout)  *make_input_layout (_In_ DXt (ID3D11Device) *device, _In_reads_ (data_size) const void *data, size_t data_size) {
    ID3D11InputLayout *layout;
    ID3D11ShaderReflection *refl;
    D3D11_SHADER_DESC desc;
    D3D11_INPUT_ELEMENT_DESC elements [32];
    HRESULT hr;
    int index, offset;

    offset = 0;
    refl = 0;
    if (FAILED (D3DReflect (data, data_size, __uuidof (ID3D11ShaderReflection), (void **) &refl))) {
        return (0);
    }
    refl->GetDesc (&desc);
    index = 0;
    while (index < desc.InputParameters) {
        D3D11_SIGNATURE_PARAMETER_DESC param;
        D3D11_INPUT_ELEMENT_DESC *element;

        refl->GetInputParameterDesc (index, &param);
        Assert (index < sizeof elements / sizeof *elements);
        element = elements + index;
        element->SemanticName = param.SemanticName;
        element->SemanticIndex = param.SemanticIndex;
        element->InputSlot = 0;
        element->AlignedByteOffset = offset;
        element->InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        element->InstanceDataStepRate = 0;
        if (param.Mask == 1) {
            if (param.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element->Format = DXGI_FORMAT_R32_UINT;
            else if (param.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element->Format = DXGI_FORMAT_R32_SINT;
            else if (param.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element->Format = DXGI_FORMAT_R32_FLOAT;
            offset += 4;
        } else if (param.Mask <= 3) {
            if (param.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element->Format = DXGI_FORMAT_R32G32_UINT;
            else if (param.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element->Format = DXGI_FORMAT_R32G32_SINT;
            else if (param.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element->Format = DXGI_FORMAT_R32G32_FLOAT;
            offset += 8;
        } else if (param.Mask <= 7) {
            if (param.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element->Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (param.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element->Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (param.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element->Format = DXGI_FORMAT_R32G32B32_FLOAT;
            offset += 12;
        } else if (param.Mask <= 15) {
            if (param.ComponentType == D3D_REGISTER_COMPONENT_UINT32) element->Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (param.ComponentType == D3D_REGISTER_COMPONENT_SINT32) element->Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (param.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) element->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            offset += 16;
        }
        index += 1;
    }
    layout = 0;
    Assert (index <= sizeof elements / sizeof *elements);
    if (FAILED (hr = device->CreateInputLayout (elements, index, data, data_size, &layout))) {
        Error ("device->CreateInputLayout failed with 0x%x", hr);
    }
    refl->Release ();
    return (layout);
}

_Ret_maybenull_ extern DXt (ID3D11Buffer)   *make_vertex_buffer (_In_ DXt (ID3D11Device) *device, _In_reads_ (byte_size) const void *data, int byte_size) {
    D3D11_BUFFER_DESC desc;
    D3D11_SUBRESOURCE_DATA init;
    ID3D11Buffer *buffer;
    HRESULT hr;

    Zero (&desc);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = byte_size;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    init.pSysMem = data;
    init.SysMemPitch = 0;
    init.SysMemSlicePitch = 0;
    buffer = 0;
    if (FAILED (hr = device->CreateBuffer (&desc, &init, &buffer))) {
        Error ("device->CreateBuffer failed with 0x%x", hr);
    }
    return (buffer);
}

_Ret_maybenull_ extern DXt (ID3D11Buffer)   *make_constant_buffer (_In_ DXt (ID3D11Device) *device, _In_reads_ (byte_size) const void *data, int byte_size) {
    D3D11_BUFFER_DESC desc;
    D3D11_SUBRESOURCE_DATA init;
    ID3D11Buffer *buffer;
    HRESULT hr;

    Zero (&desc);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = byte_size;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;
    init.pSysMem = data;
    init.SysMemPitch = 0;
    init.SysMemSlicePitch = 0;
    buffer = 0;
    if (FAILED (hr = device->CreateBuffer (&desc, &init, &buffer))) {
        Error ("device->CreateBuffer failed with 0x%x", hr);
    }
    return (buffer);
}

_Ret_maybenull_ extern DXt (ID3D11Buffer)   *make_constant_buffer_empty (_In_ DXt (ID3D11Device) *device, int byte_size) {
    D3D11_BUFFER_DESC desc;
    ID3D11Buffer *buffer;
    HRESULT hr;

    Zero (&desc);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = byte_size;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;
    buffer = 0;
    if (FAILED (hr = device->CreateBuffer (&desc, 0, &buffer))) {
        Error ("device->CreateBuffer failed with 0x%x", hr);
    }
    return (buffer);
}

extern void release_buffer (_In_ DXt (ID3D11Buffer) **pbuffer) {
    if (*pbuffer) {
        (*pbuffer)->Release ();
        *pbuffer = 0;
    }
}

_Ret_maybenull_ extern DXt (ID3D11RasterizerState)  *make_rasterizer_state (_In_ DXt (ID3D11Device) *device, int is_solid, int is_cull) {
    ID3D11RasterizerState   *state;
    D3D11_RASTERIZER_DESC   raster_desc;
    HRESULT                 hr;

    Zero (&raster_desc);
    raster_desc.FillMode = is_solid ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
    raster_desc.CullMode = is_cull ? D3D11_CULL_BACK : D3D11_CULL_NONE;
    raster_desc.FrontCounterClockwise = 1;
    raster_desc.DepthClipEnable = 1;
    state = 0;
    if (FAILED (hr = device->CreateRasterizerState (&raster_desc, &state))) {
        Error ("device->CreateRasterizerState failed with 0x%x", hr);
    }
    return (state);
}

_Ret_maybenull_ extern DXt (ID3D11BlendState)   *make_blend_state (_In_ DXt (ID3D11Device) *device) {
    ID3D11BlendState    *blend;
    D3D11_BLEND_DESC    desc;
    HRESULT                 hr;

    Zero (&desc);
    desc.AlphaToCoverageEnable = 0;
    desc.IndependentBlendEnable = 0;
    desc.RenderTarget[0].BlendEnable = 1;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    blend = 0;
    if (FAILED (hr = device->CreateBlendState (&desc, &blend))) {
        Error ("device->CreateBlendState failed with 0x%x", hr);
    }
    return (blend);
}

_Ret_maybenull_ extern DXt (ID3D11DepthStencilState)    *make_depth_state (_In_ DXt (ID3D11Device) *device) {
    D3D11_DEPTH_STENCIL_DESC    desc;
    ID3D11DepthStencilState     *state;
    HRESULT                     hr;

    Zero (&desc);
    desc.DepthEnable = 1;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_LESS; // for left-handed
    // desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    desc.StencilEnable = 0;
    desc.StencilReadMask = 0xFF;
    desc.StencilWriteMask = 0xFF;
    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    state = 0;
    if (FAILED (hr = device->CreateDepthStencilState (&desc, &state))) {
        Error ("device->CreateDepthStencilState failed with 0x%x", hr);
    }
    return (state);
}

_Ret_maybenull_ extern DXt (ID3D11Texture2D)    *make_depth_buffer (_In_ DXt (ID3D11Device) *device, int width, int height) {
    DXt (ID3D11Texture2D)   *buffer;
    D3D11_TEXTURE2D_DESC    desc;
    HRESULT                 hr;

    Zero (&desc);
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    buffer = 0;
    if (FAILED (hr = device->CreateTexture2D (&desc, 0, &buffer))) {
        Error ("device->CreateTexture2D failed with 0x%x", hr);
    }
    return (buffer);
}

_Ret_maybenull_ extern DXt (ID3D11DepthStencilView) *make_depth_view (_In_ DXt (ID3D11Device) *device, DXt (ID3D11Texture2D) *buffer) {
    DXt (ID3D11DepthStencilView)    *view;
    D3D11_DEPTH_STENCIL_VIEW_DESC   desc;
    HRESULT                         hr;

    Zero (&desc);
    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    view = 0;
    if (FAILED (hr = device->CreateDepthStencilView (buffer, &desc, &view))) {
        Error ("device->CreateDepthStencilView failed with 0x%x", hr);
    }
    return (view);
}

extern void release_input_layout (_In_ DXt (ID3D11InputLayout) **playout) {
    if (*playout) {
        (*playout)->Release ();
        *playout = 0;
    }
}

extern void release_rasterizer_state (_In_ DXt (ID3D11RasterizerState) **pras) {
    if (*pras) {
        (*pras)->Release ();
        *pras = 0;
    }
}

extern void release_blend_state (_In_ DXt (ID3D11BlendState) **pblend) {
    if (*pblend) {
        (*pblend)->Release ();
        *pblend = 0;
    }
}

extern void release_depth_state (_In_ DXt (ID3D11DepthStencilState) **pdepth) {
    if (*pdepth) {
        (*pdepth)->Release ();
        *pdepth = 0;
    }
}

extern void release_depth_view (_In_ DXt (ID3D11DepthStencilView) **pview) {
    if (*pview) {
        (*pview)->Release ();
        *pview = 0;
    }
}

extern void release_depth_buffer (_In_ DXt (ID3D11Texture2D) **pbuffer) {
    if (*pbuffer) {
        (*pbuffer)->Release ();
        *pbuffer = 0;
    }
}

_Ret_maybenull_ extern DXt (IDWriteTextFormat)  *make_text_format (_In_ DXt (IDWriteFactory) *factory, _In_z_ const WCHAR *family, DXi (DWRITE_FONT_WEIGHT) weight, DXi (DWRITE_FONT_STYLE) style, float size) {
    IDWriteTextFormat   *format;
    HRESULT             hr;

    format = 0;
    if (FAILED (hr = factory->CreateTextFormat (family, 0, weight, style, DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &format))) {
        Error ("factory->CreateTextFormat failed with 0x%x", hr);
    }
    return (format);
}

_Ret_maybenull_ extern DXt (IDWriteTextLayout)  *make_text_layout (_In_ DXt (IDWriteFactory) *factory, _In_z_ const WCHAR *text, int text_length, DXt (IDWriteTextFormat) *format, float width, float height) {
    IDWriteTextLayout   *layout;
    HRESULT             hr;

    layout = 0;
    if (FAILED (hr = factory->CreateTextLayout (text, text_length, format, width, height, &layout))) {
        Error ("factory->CreateTextLayout failed with 0x%x", hr);
    }
    return (layout);
}

extern void release_text_format (_In_ DXt (IDWriteTextFormat) *format) {
    format->Release ();
}

extern void release_text_layout (_In_ DXt (IDWriteTextLayout) *layout) {
    layout->Release ();
}

extern struct dwrite_text_metrics   get_text_layout_metrics (_In_ DXt (IDWriteTextLayout) *layout) {
    struct dwrite_text_metrics  metrics;

    layout->GetMetrics ((DWRITE_TEXT_METRICS *) &metrics);
    return (metrics);
}

_Ret_maybenull_ extern DXt (ID2D1SolidColorBrush)   *make_solid_brush (_In_ DXt (ID2D1DeviceContext) *context, unsigned color, int is_to_linear) {
    ID2D1SolidColorBrush    *brush;
    float                   fcolors[4];
    HRESULT                 hr;

    brush = 0;
    convert_uint_color_to_floats (color, fcolors, is_to_linear);
    if (FAILED (hr = context->CreateSolidColorBrush (D2D1::ColorF (fcolors[0], fcolors[1], fcolors[2], fcolors[3]), &brush))) {
        Error ("context->CreateSolidColorBrush failed with 0x%x", hr);
    }
    return (brush);
}

