
/* deps: essentials.c memory.c io.c dx.c fxc.h */

struct shader {
    DXt (ID3D11InputLayout) *input_layout;
    DXt (ID3D11VertexShader) *vs;
    DXt (ID3D11PixelShader) *ps;
    DXt (ID3D11DomainShader) *ds;
    DXt (ID3D11HullShader) *hs;
};

static void release_shader (_In_ struct shader *shader) {
    release_input_layout (&shader->input_layout);
    release_vertex_shader (&shader->vs);
    release_pixel_shader (&shader->ps);
    release_domain_shader (&shader->ds);
    release_hull_shader (&shader->hs);
}

static int make_shader_from_fxc_bundle (_In_ DXt (ID3D11Device) *device, _In_ char *_Stack file, _Out_ struct shader *shader) {
    int result;
    int is_ds, is_hs;
    union fxc_header *header;

    is_ds = is_hs = 0;
    header = (void *) file;
    shader->vs = make_vertex_shader (device, file + header->range [Shader (vs)].offset, header->range [Shader (vs)].size);
    shader->ps = make_pixel_shader (device, file + header->range [Shader (ps)].offset, header->range [Shader (ps)].size);
    if (header->range [Shader (ds)].offset) {
        is_ds = 1;
        shader->ds = make_domain_shader (device, file + header->range [Shader (ds)].offset, header->range [Shader (ds)].size);
    }
    if (header->range [Shader (hs)].offset) {
        is_hs = 1;
        shader->hs = make_hull_shader (device, file + header->range [Shader (hs)].offset, header->range [Shader (hs)].size);
    }
    if (shader->vs) {
        shader->input_layout = make_input_layout (device, file + header->range [Shader (vs)].offset, header->range [Shader (vs)].size);
    }
    if (shader->vs && shader->ps && (0 == is_ds || shader->ds) && (0 == is_hs || shader->hs)) {
        result = 1;
    } else {
        release_shader (shader);
        result = 0;
    }
    return (result);
}

static int read_shader_from_fxc_bundle (_In_ DXt (ID3D11Device) *device, _In_z_ const char *filename, _Out_ struct shader *shader) {
    int result;
    char *_Stack file;

    Zero (shader);
    file = allocate_stack (0);
    if (read_entire_binary_file (file, filename)) {
        result = make_shader_from_fxc_bundle (device, file, shader);
        release_stack (&file);
    } else {
        result = 0;
    }
    return (result);
}
