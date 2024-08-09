
/* used in hs and ds */
cbuffer gdata {
    matrix MVP;
    float g_tess_factor;
    float pad[3];
};

struct vs_in {
    float4 wposition: POSITION;
};

struct vs_out {
    float4 wposition: POSITION;
};

vs_out vs_main (vs_in input) {
    vs_out output;

    output.wposition = input.wposition;
    return (output);
}

struct hs_constant_data {
    float   edges[4]: SV_TessFactor;
    float   inside[2]: SV_InsideTessFactor;
    uint    colind: TEXCOORD;
};

struct hs_out {
    float4  wposition: POSITION;
};

hs_constant_data    hs_constant_data_function (InputPatch<vs_out, 16> ip, uint patch_id: SV_PrimitiveID) {
    hs_constant_data    data;

    data.edges[0] = data.edges[1] = data.edges[2] = data.edges[3] = g_tess_factor;
    data.inside[0] = data.inside[1] = g_tess_factor;
    data.colind = ((patch_id % 3) & 1) == ((patch_id / 3) & 1);
    return (data);
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(16)]
[patchconstantfunc("hs_constant_data_function")]
hs_out  hs_main (InputPatch<vs_out, 16> points, uint index: SV_OutputControlPointID, uint patch_id: SV_PrimitiveID) {
    hs_out  output;

    output.wposition = points[index].wposition;
    return (output);
}

struct ds_out {
    float4  position: SV_POSITION;
    uint    colind: TEXCOORD;
};

float4  interpolate4 (float4 a, float4 b, float4 c, float4 d, float t) {
    float4  bc = lerp (b, c, t);
    return (lerp (lerp (lerp (a, b, t), bc, t), lerp (bc, lerp (c, d, t), t), t));
}

[domain("quad")]
ds_out  ds_main (hs_constant_data input, float2 uv: SV_DomainLocation, const OutputPatch<hs_out, 16> controls) {
    ds_out  output;
    float4  column[4];
    float4  position;

    column[0] = interpolate4 (controls[0].wposition, controls[1].wposition, controls[2].wposition, controls[3].wposition, uv.x);
    column[1] = interpolate4 (controls[4].wposition, controls[5].wposition, controls[6].wposition, controls[7].wposition, uv.x);
    column[2] = interpolate4 (controls[8].wposition, controls[9].wposition, controls[10].wposition, controls[11].wposition, uv.x);
    column[3] = interpolate4 (controls[12].wposition, controls[13].wposition, controls[14].wposition, controls[15].wposition, uv.x);
    position = interpolate4 (column[0], column[1], column[2], column[3], uv.y);
    output.position = mul (position, MVP);
    output.colind = input.colind;
    return (output);
}

float4  ps_main (ds_out input): SV_TARGET {
    float   gb;

    gb = 0.6 + 0.4 * input.colind;
    return float4 (1, gb, gb, 1);
}


