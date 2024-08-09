
/* dep: 3plib/srgb-transform.c */

static float    ubyte_color_to_float (int color, int is_to_linear) {
    return (is_to_linear ? srgb_to_linear_float (color / 255.f) : color / 255.f);
}

static float    ushort_color_to_float (int color, int is_to_linear) {
    return (is_to_linear ? srgb_to_linear_float (color / (float) 0xFFFF) : color / (float) 0xFFFF);
}

static void convert_uint_color_to_floats (unsigned color, float out[4], int is_to_linear) {
    out[0] = ubyte_color_to_float (color & 0xFF, is_to_linear);
    out[1] = ubyte_color_to_float ((color >> 8) & 0xFF, is_to_linear);
    out[2] = ubyte_color_to_float ((color >> 16) & 0xFF, is_to_linear);
    out[3] = ubyte_color_to_float (color >> 24, 0);
}

static void convert_ulong_color_to_floats (unsigned long long color, float out[4], int is_to_linear) {
    out[0] = ushort_color_to_float (color & 0xFFFF, is_to_linear);
    out[1] = ushort_color_to_float ((color >> 16) & 0xFFFF, is_to_linear);
    out[2] = ushort_color_to_float ((color >> 32) & 0xFFFF, is_to_linear);
    out[3] = ushort_color_to_float (color >> 48, 0);
}
