
/* deps: __Platform_Windows_10_SDK essentials.c memory.c */

#undef __File
#define __File "dx.c"

#ifdef __cplusplus
#   define DXt(cpp_type) cpp_type
#   define DXi(cpp_type) cpp_type
#else
#   define DXt(cpp_type) struct __DX_##cpp_type
#   define DXi(cpp_type) int
#endif

#define Pixel(name) Pixel_##name
enum pixel {
    Pixel (ubyte_4),
    Pixel (ubyte_4_srgb),
    Pixel (ushort_4),
    Pixel (half_4),
    Pixel (float_4),
    Pixel (_count),
};

static const int    g_pixel_size[Pixel (_count)] = {
    4, 4, 8, 8, 16,
};
// #define Default_Scroll_Speed 50.f

#define DXVault(name) DXVault_##name
enum dxvault {
    DXVault (nil),
    DXVault (free_entry),
    DXVault (buffer),
    DXVault (texture),

    DXVault (_count),
};

struct dxbuffer {
    DXt (ID3D11Buffer)  *handle;
};

#define DXTexture_Type(name) DXTexture_Type_##name
enum dxtexture_type {
    DXTexture_Type (nil),
    DXTexture_Type (1d),
    DXTexture_Type (2d),
    DXTexture_Type (cube),
    DXTexture_Type (3d),
};

struct dxtexture {
    union {
        DXt (ID3D11Texture1D)   *handle1d;
        DXt (ID3D11Texture2D)   *handle2d;
        DXt (ID3D11Texture3D)   *handle3d;
        void                    *handle;
    };
    enum dxtexture_type     type;
    enum pixel              pixel;
    int                     width, height, depth;
};

#define Font_Weight(name) Font_Weight_##name
enum font_weight {
    Font_Weight (thin) = 100,
    Font_Weight (extra_light) = 200,
    Font_Weight (light) = 300,
    Font_Weight (semi_light) = 350,
    Font_Weight (regular) = 400,
    Font_Weight (medium) = 500,
    Font_Weight (semi_bold) = 600,
    Font_Weight (bold) = 700,
    Font_Weight (extra_bold) = 800,
    Font_Weight (heavy) = 900,
    Font_Weight (extra_heavy) = 950,
};

#define Font_Style(name) Font_Style_##name
enum font_style {
    Font_Style (normal),
    Font_Style (oblique),
    Font_Style (italic),
};

#define Topology(name) Topology_##name
enum topology {
    Topology (points) = 1,
    Topology (lines) = 2,
    Topology (line_strip) = 3,
    Topology (triangles) = 4,
    Topology (triangle_strip) = 5,
    Topology (lines_adj) = 10,
    Topology (line_strip_adj) = 11,
    Topology (triangles_adj) = 12,
    Topology (triangle_strip_adj) = 13,
    Topology (patch_1) = 33,
    Topology (patch_2) = 34,
    Topology (patch_3) = 35,
    Topology (patch_4) = 36,
    Topology (patch_5) = 37,
    Topology (patch_6) = 38,
    Topology (patch_7) = 39,
    Topology (patch_8) = 40,
    Topology (patch_9) = 41,
    Topology (patch_10) = 42,
    Topology (patch_11) = 43,
    Topology (patch_12) = 44,
    Topology (patch_13) = 45,
    Topology (patch_14) = 46,
    Topology (patch_15) = 47,
    Topology (patch_16) = 48,
    Topology (patch_17) = 49,
    Topology (patch_18) = 50,
    Topology (patch_19) = 51,
    Topology (patch_20) = 52,
    Topology (patch_21) = 53,
    Topology (patch_22) = 54,
    Topology (patch_23) = 55,
    Topology (patch_24) = 56,
    Topology (patch_25) = 57,
    Topology (patch_26) = 58,
    Topology (patch_27) = 59,
    Topology (patch_28) = 60,
    Topology (patch_29) = 61,
    Topology (patch_30) = 62,
    Topology (patch_31) = 63,
    Topology (patch_32) = 64,
};

#define Text_Draw_Option(name) Text_Draw_Option_##name
enum text_draw_option {
    Text_Draw_Option (no_snap) = 0x1,
    Text_Draw_Option (clip) = 0x2,
};

struct dwrite_text_metrics {
    float   left;
    float   top;
    float   width;
    float   widthIncludingTrailingWhitespace;
    float   height;
    float   layoutWidth;
    float   layoutHeight;
    int     maxBidiReorderingDepth;
    int     lineCount;
};

struct dx {
    int                             is_allocated;
    int                             is_hdr;
    DXt (ID3D11Device)              *d3ddevice;
    DXt (ID3D11DeviceContext)       *d3dcontext;
    DXi (D3D_FEATURE_LEVEL)         d3dfeature_level;
    DXt (ID3D11RenderTargetView)    *render_target;
    DXt (IDXGISwapChain1)           *swapchain;
    DXi (DXGI_FORMAT)               format;
    DXt (ID2D1Factory1)             *d2dfactory;
    DXt (ID2D1Device)               *d2ddevice;
    DXt (ID2D1DeviceContext)        *d2dcontext;
    DXt (ID2D1Bitmap1)              *target_bitmap;
    DXt (ID2D1SolidColorBrush)      *default_text_brush;
    DXt (IDWriteFactory)            *dwfactory;
};

extern int      allocate_dx (_Out_ struct dx *dx, int dpi, HWND window_handle);
extern void     release_dx (_In_ struct dx *dx);
extern int      resize_dx (_In_ struct dx *dx, int dpi);
extern int      get_dx_dpi (_In_ struct dx *dx);

extern int  begin_dx (_In_ struct dx *dx);
extern void setup_dx_render_target (_In_ struct dx *dx, _In_ DXt (ID3D11RenderTargetView) *target, _In_opt_ DXt (ID3D11DepthStencilView) *depth);
extern void setup_dx_input_assembler (_In_ struct dx *dx, _In_opt_ DXt (ID3D11InputLayout) *input_layout, DXi (D3D11_PRIMITIVE_TOPOLOGY) topology, _In_opt_ DXt (ID3D11Buffer) *vertex_buffer, int stride, int offset);
extern void setup_dx_rasterizer_state (_In_ struct dx *dx, _In_ DXt (ID3D11RasterizerState) *rasterizer);
extern void setup_dx_blend_state (_In_ struct dx *dx, _In_ DXt (ID3D11BlendState) *blend);
extern void setup_dx_depth_state (_In_ struct dx *dx, _In_ DXt (ID3D11DepthStencilState) *depth_state);
extern void setup_dx_viewport (_In_ struct dx *dx, int x, int y, int width, int height);
extern void setup_dx_vertex_shader (_In_ struct dx *dx, _In_ DXt (ID3D11VertexShader) *shader);
extern void setup_dx_pixel_shader (_In_ struct dx *dx, _In_ DXt (ID3D11PixelShader) *shader);
extern void setup_dx_domain_shader (_In_ struct dx *dx, _In_ DXt (ID3D11DomainShader) *shader);
extern void setup_dx_hull_shader (_In_ struct dx *dx, _In_ DXt (ID3D11HullShader) *shader);
extern void setup_dx_constant_buffer_for_vertex_shader (_In_ struct dx *dx, int index, _In_ DXt (ID3D11Buffer) *buffer);
extern void setup_dx_constant_buffer_for_pixel_shader (_In_ struct dx *dx, int index, _In_ DXt (ID3D11Buffer) *buffer);
extern void setup_dx_constant_buffer_for_domain_shader (_In_ struct dx *dx, int index, _In_ DXt (ID3D11Buffer) *buffer);
extern void setup_dx_constant_buffer_for_hull_shader (_In_ struct dx *dx, int index, _In_ DXt (ID3D11Buffer) *buffer);
extern void dx_clear_render_target (_In_ struct dx *dx, _In_ DXt (ID3D11RenderTargetView) *target, unsigned clear_color);
extern void dx_clear_depth_view (_In_ struct dx *dx, _In_ DXt (ID3D11DepthStencilView) *view, float value);
extern void dx_update_buffer (_In_ struct dx *dx, _In_ DXt (ID3D11Buffer) *buffer, _In_reads_bytes_ (size) const void *data, int size);
extern void dx_draw (_In_ struct dx *dx, int count, int offset);
extern void dx_draw_rectangle (_In_ struct dx *dx, int x, int y, int width, int height, unsigned color);
extern void dx_draw_rectangle_border (_In_ struct dx *dx, int x, int y, int width, int height, unsigned color);
extern void dx_draw_text_layout (_In_ struct dx *dx, _In_ DXt (IDWriteTextLayout) *layout, float x, float y, unsigned color, DXi (D2D1_DRAW_TEXT_OPTIONS) options);
extern void dx_draw_2d_line (_In_ struct dx *dx, _In_reads_ (2) float start [2], _In_reads_ (2) float end [2], float width, unsigned color);
extern int  end_dx (_In_ struct dx *dx);

_Ret_maybenull_ extern DXt (ID3DBlob)   *compile_shader (_In_ const char *text, int text_size, _In_z_ const char *filename, _In_z_ const char *entrypoint, _In_z_ const char *profile, _Inout_ char *_Stack *perror);
extern void                 release_blob (_In_ DXt (ID3DBlob) **pblob);
_Ret_notnull_ extern void   *get_blob_data (_In_ DXt (ID3DBlob) *blob);
extern size_t               get_blob_size (_In_ DXt (ID3DBlob) *blob);

_Ret_maybenull_ extern DXt (ID3D11PixelShader)  *make_pixel_shader (_In_ DXt (ID3D11Device) *device, _In_reads_ (data_size) const void *data, size_t data_size);
_Ret_maybenull_ extern DXt (ID3D11VertexShader) *make_vertex_shader (_In_ DXt (ID3D11Device) *device, _In_reads_ (data_size) const void *data, size_t data_size);
_Ret_maybenull_ extern DXt (ID3D11DomainShader) *make_domain_shader (_In_ DXt (ID3D11Device) *device, _In_reads_ (data_size) const void *data, size_t data_size);
_Ret_maybenull_ extern DXt (ID3D11HullShader) *make_hull_shader (_In_ DXt (ID3D11Device) *device, _In_reads_ (data_size) const void *data, size_t data_size);

extern void release_pixel_shader (_In_ DXt (ID3D11PixelShader) **pshader);
extern void release_vertex_shader (_In_ DXt (ID3D11VertexShader) **pshader);
extern void release_domain_shader (_In_ DXt (ID3D11DomainShader) **pshader);
extern void release_hull_shader (_In_ DXt (ID3D11HullShader) **pshader);

_Ret_maybenull_ extern DXt (ID3D11Buffer)   *make_vertex_buffer (_In_ DXt (ID3D11Device) *device, _In_reads_ (byte_size) const void *data, int byte_size);
_Ret_maybenull_ extern DXt (ID3D11Buffer)   *make_constant_buffer (_In_ DXt (ID3D11Device) *device, _In_reads_ (byte_size) const void *data, int byte_size);
_Ret_maybenull_ extern DXt (ID3D11Buffer)   *make_constant_buffer_empty (_In_ DXt (ID3D11Device) *device, int byte_size);

extern void release_buffer (_In_ DXt (ID3D11Buffer) **pbuffer);

_Ret_maybenull_ extern DXt (ID3D11InputLayout)          *make_input_layout (_In_ DXt (ID3D11Device) *device, _In_reads_ (data_size) const void *data, size_t data_size);
_Ret_maybenull_ extern DXt (ID3D11RasterizerState)      *make_rasterizer_state (_In_ DXt (ID3D11Device) *device, int is_solid, int is_cull);
_Ret_maybenull_ extern DXt (ID3D11BlendState)           *make_blend_state (_In_ DXt (ID3D11Device) *device);
_Ret_maybenull_ extern DXt (ID3D11DepthStencilState)    *make_depth_state (_In_ DXt (ID3D11Device) *device);
_Ret_maybenull_ extern DXt (ID3D11Texture2D)            *make_depth_buffer (_In_ DXt (ID3D11Device) *device, int width, int height);
_Ret_maybenull_ extern DXt (ID3D11DepthStencilView)     *make_depth_view (_In_ DXt (ID3D11Device) *device, DXt (ID3D11Texture2D) *buffer);

extern void release_input_layout (_In_ DXt (ID3D11InputLayout) **playout);
extern void release_rasterizer_state (_In_ DXt (ID3D11RasterizerState) **pras);
extern void release_blend_state (_In_ DXt (ID3D11BlendState) **pblend);
extern void release_depth_state (_In_ DXt (ID3D11DepthStencilState) **pdepth);
extern void release_depth_view (_In_ DXt (ID3D11DepthStencilView) **pview);
extern void release_depth_buffer (_In_ DXt (ID3D11Texture2D) **pbuffer);

_Ret_maybenull_ extern DXt (IDWriteTextFormat)  *make_text_format (_In_ DXt (IDWriteFactory) *factory, _In_z_ const WCHAR *family, DXi (DWRITE_FONT_WEIGHT) weight, DXi (DWRITE_FONT_STYLE) style, float size);
_Ret_maybenull_ extern DXt (IDWriteTextLayout)  *make_text_layout (_In_ DXt (IDWriteFactory) *factory, _In_z_ const WCHAR *text, int text_length, DXt (IDWriteTextFormat) *format, float width, float height);

extern void release_text_format (_In_ DXt (IDWriteTextFormat) *format);
extern void release_text_layout (_In_ DXt (IDWriteTextLayout) *layout);

extern struct dwrite_text_metrics   get_text_layout_metrics (_In_ DXt (IDWriteTextLayout) *layout);

_Ret_maybenull_ extern DXt (ID2D1SolidColorBrush)   *make_solid_brush (_In_ DXt (ID2D1DeviceContext) *context, unsigned color, int is_to_linear);

