
/* deps: no */

#undef __File
#define __File "math.c"

#define Pi 3.141592653589793238462643383279502884197169399375105820974944
#define Pif 3.141592653589793238462643383279502884197169399375105820974944f
#define ToRad (Pi / 180)
#define ToRadf (Pif / 180)

#define M(m,r,c) (m)[(c) * 4 + (r)] /* column-major */
#define Msize (16 * sizeof (float))
#define V3size (3 * sizeof (float))
#define V4size (4 * sizeof (float))
#define Qsize V4size

static void     init_orthographic_projection (_Out_writes_ (16) float *out, float left, float right, float bottom, float top, float _near, float _far) {
    memset (out, 0, Msize);
    M (out, 0, 0) = 2 / (right - left);
    M (out, 3, 0) = -(right + left) / (right - left);
    M (out, 1, 1) = -2 / (top - bottom);
    M (out, 3, 1) = (top + bottom) / (top - bottom);
    M (out, 2, 2) = 1 / (_far - _near);
    M (out, 3, 2) = -_near / (_far - _near);
    M (out, 3, 3) = 1;
}

static void     init_symmetric_orthographic_projection (_Out_writes_ (16) float *out, float right, float top, float _near, float _far) {
    memset (out, 0, Msize);
    M (out, 0, 0) = 1 / right;
    M (out, 1, 1) = -1 / top;
    M (out, 2, 2) = 1 / (_near - _far);
    M (out, 3, 2) = _near / (_near - _far);
    M (out, 3, 3) = 1;
}

static void     init_symmetric_perspective_projection (_Out_writes_ (16) float *out, float right, float top, float _near, float _far) {
    memset (out, 0, Msize);
    M (out, 0, 0) = _near / right;
    M (out, 1, 1) = -_near / top;
    M (out, 2, 2) = _far / (_near - _far);
    M (out, 3, 2) = (_far * _near) / (_near - _far);
    M (out, 2, 3) = -1;
}

static void     init_fov_perspective_projection (_Out_writes_ (16) float *out, float width, float height, float fovy, float _near, float _far) {
    float   tangent;

    tangent = tanf (fovy / 2);
    memset (out, 0, Msize);
    M (out, 0, 0) = height / (tangent * width);
    M (out, 1, 1) = 1.f / tangent;
    M (out, 2, 2) = _far / (_near - _far);
    M (out, 3, 2) = (_far * _near) / (_near - _far);
    M (out, 2, 3) = -1;
}

static void     init_perspective_projection (_Out_writes_ (16) float *out, float left, float right, float bottom, float top, float _near, float _far) {
    /* todo: test */
    memset (out, 0, Msize);
    M (out, 0, 0) = 2 * _near / (right - left);
    M (out, 2, 0) = -(right + left) / (right - left);
    M (out, 1, 1) = 2 * _near / (top - bottom);
    M (out, 2, 1) = (top + bottom) / (top - bottom);
    M (out, 2, 2) = _far / (_far - _near);
    M (out, 3, 2) = (_far * _near) / (_far - _near);
    M (out, 2, 3) = -1;
}

static inline float v2_dot (_In_reads_ (2) const float *restrict left, _In_reads_ (2) const float *restrict right) {
    return (left[0] * right[0] + left[1] * right[1]);
}

static inline float v2_dotp (_In_reads_ (2) const float *restrict leftp, _In_reads_ (2) const float *restrict right) {
    return (leftp[1] * right[0] - leftp[0] * right[1]);
}

static inline float v2_dotv (_In_reads_ (2) const float *vec) {
    return (vec[0] * vec[0] + vec[1] * vec[1]);
}

static inline void  v3_sub (_Out_writes_ (3) float *restrict out, _In_reads_ (3) const float *restrict left, _In_reads_ (3) const float *restrict right) {
    out[0] = left[0] - right[0];
    out[1] = left[1] - right[1];
    out[2] = left[2] - right[2];
}

static inline float v3_dotv (_In_reads_ (3) const float *vec) {
    return (vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

static inline float v3_dot (_In_reads_ (3) const float *restrict left, _In_reads_ (3) const float *restrict right) {
    return (left[0] * right[0] + left[1] * right[1] + left[2] * right[2]);
}

static inline void  v3_lerp (_Out_writes_ (3) float *restrict out, _In_reads_ (3) const float *restrict left, _In_reads_ (3) const float *restrict right, float t) {
    float   mt;

    mt = 1 - t;
    out[0] = left[0] * mt + right[0] * t;
    out[1] = left[1] * mt + right[1] * t;
    out[2] = left[2] * mt + right[2] * t;
}

static inline void  v3_neg (_Out_writes_ (3) float *restrict out, _In_reads_ (3) const float *restrict v) {
    out[0] = -v[0];
    out[1] = -v[1];
    out[2] = -v[2];
}

static inline void  v3_negv (_Inout_updates_ (3) float *restrict out) {
    out[0] = -out[0];
    out[1] = -out[1];
    out[2] = -out[2];
}

static inline void  v3_scale (_Out_writes_ (3) float *restrict out, _In_reads_ (3) const float *restrict v, float scl) {
    out[0] = v[0] * scl;
    out[1] = v[1] * scl;
    out[2] = v[2] * scl;
}

static inline void  v3_scalev (_Inout_updates_ (3) float *out, float scl) {
    out[0] *= scl;
    out[1] *= scl;
    out[2] *= scl;
}

static inline void  v3_add (_Out_writes_ (3) float *restrict out, _In_reads_ (3) const float *restrict l, _In_reads_ (3) const float *restrict r) {
    out[0] = l[0] + r[0];
    out[1] = l[1] + r[1];
    out[2] = l[2] + r[2];
}

static inline void  v3_addv (_Inout_updates_ (3) float *restrict out, _In_reads_ (3) const float *restrict r) {
    out[0] += r[0];
    out[1] += r[1];
    out[2] += r[2];
}

static inline void  v3_scale_add (_Out_writes_ (3) float *restrict out, _In_reads_ (3) const float *restrict l, float scl, _In_reads_ (3) const float *restrict r) {
    out[0] = l[0] * scl + r[0];
    out[1] = l[1] * scl + r[1];
    out[2] = l[2] * scl + r[2];
}

static inline void  v3_scale_addv (_Inout_updates_ (3) float *restrict out, float scl, _In_reads_ (3) const float *restrict r) {
    out[0] = out[0] * scl + r[0];
    out[1] = out[1] * scl + r[1];
    out[2] = out[2] * scl + r[2];
}

static inline void  v3_add_scale (_Out_writes_ (3) float *restrict out, _In_reads_ (3) const float *restrict l, _In_reads_ (3) const float *restrict r, float scl) {
    out[0] = l[0] + r[0] * scl;
    out[1] = l[1] + r[1] * scl;
    out[2] = l[2] + r[2] * scl;
}

static inline void  v3_add_scalev (_Inout_updates_ (3) float *restrict out, _In_reads_ (3) const float *restrict r, float scl) {
    out[0] += r[0] * scl;
    out[1] += r[1] * scl;
    out[2] += r[2] * scl;
}

static inline float v4_dotv (_In_reads_ (4) const float *vec) {
    return (vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] + vec[3] * vec[3]);
}

static inline float v4_dot (_In_reads_ (4) const float *restrict left, _In_reads_ (4) const float *restrict right) {
    return (left[0] * right[0] + left[1] * right[1] + left[2] * right[2] + left[3] * right[3]);
}

static inline void  v4_lerp (_Out_writes_ (4) float *restrict out, _In_reads_ (4) const float *restrict left, _In_reads_ (4) const float *restrict right, float t) {
    float   mt;

    mt = 1 - t;
    out[0] = left[0] * mt + right[0] * t;
    out[1] = left[1] * mt + right[1] * t;
    out[2] = left[2] * mt + right[2] * t;
    out[3] = left[3] * mt + right[3] * t;
}

static inline void  v4_neg (_Out_writes_ (4) float *restrict out, _In_reads_ (4) const float *restrict v) {
    out[0] = -v[0];
    out[1] = -v[1];
    out[2] = -v[2];
    out[3] = -v[3];
}

static inline void  v4_negv (_Inout_updates_ (4) float *restrict out) {
    out[0] = -out[0];
    out[1] = -out[1];
    out[2] = -out[2];
    out[3] = -out[3];
}

static inline void  v4_scale (_Out_writes_ (4) float *restrict out, _In_reads_ (4) const float *restrict v, float scl) {
    out[0] = v[0] * scl;
    out[1] = v[1] * scl;
    out[2] = v[2] * scl;
    out[3] = v[3] * scl;
}

static inline void  v4_scalev (_Inout_updates_ (4) float *out, float scl) {
    out[0] *= scl;
    out[1] *= scl;
    out[2] *= scl;
    out[3] *= scl;
}

static inline void  v4_add (_Out_writes_ (4) float *restrict out, _In_reads_ (4) const float *restrict l, _In_reads_ (4) const float *restrict r) {
    out[0] = l[0] + r[0];
    out[1] = l[1] + r[1];
    out[2] = l[2] + r[2];
    out[3] = l[3] + r[3];
}

static inline void  v4_addv (_Inout_updates_ (4) float *restrict out, _In_reads_ (4) const float *restrict r) {
    out[0] += r[0];
    out[1] += r[1];
    out[2] += r[2];
    out[3] += r[3];
}

static inline void  v2_normalize (_Out_writes_ (2) float *restrict out, _In_reads_ (2) const float *restrict vec) {
    float   length;

    length = v2_dotv (vec);
    if (length > 0) {
        length = sqrtf (length);
        out[0] = vec[0] / length;
        out[1] = vec[1] / length;
    } else {
        out[0] = out[1] = 0;
    }
}

static inline void  v2_normalizev (_Inout_updates_ (2) float *out) {
    float   length;

    length = v2_dotv (out);
    if (length > 0) {
        length = sqrtf (length);
        out[0] = out[0] / length;
        out[1] = out[1] / length;
    }
}

static inline void  v3_normalize (_Out_writes_ (3) float *restrict out, _In_reads_ (3) const float *restrict vec) {
    float   length;

    length = v3_dotv (vec);
    if (length > 0) {
        length = sqrtf (length);
        out[0] = vec[0] / length;
        out[1] = vec[1] / length;
        out[2] = vec[2] / length;
    } else {
        out[0] = out[1] = out[2] = 0;
    }
}

static inline void  v3_normalizev (_Inout_updates_ (3) float *out) {
    float   length;

    length = v3_dotv (out);
    if (length > 0) {
        length = sqrtf (length);
        out[0] = out[0] / length;
        out[1] = out[1] / length;
        out[2] = out[2] / length;
    }
}

static inline void  v4_normalize (_Out_writes_ (4) float *restrict out, _In_reads_ (4) const float *restrict vec) {
    float   length;

    length = v4_dotv (vec);
    if (length > 0) {
        length = sqrtf (length);
        out[0] = vec[0] / length;
        out[1] = vec[1] / length;
        out[2] = vec[2] / length;
        out[3] = vec[3] / length;
    } else {
        out[0] = out[1] = out[2] = out[3] = 0;
    }
}

static inline void  v4_normalizev (_Inout_updates_ (4) float *out) {
    float   length;

    length = v4_dotv (out);
    if (length > 0) {
        length = sqrtf (length);
        out[0] = out[0] / length;
        out[1] = out[1] / length;
        out[2] = out[2] / length;
        out[3] = out[3] / length;
    }
}

static inline void  v3_cross (_Out_writes_ (3) float *restrict out, _In_reads_ (3) const float *restrict left, _In_reads_ (3) const float *restrict right) {
    out[0] = left[1] * right[2] - left[2] * right[1];
    out[1] = left[2] * right[0] - left[0] * right[2];
    out[2] = left[0] * right[1] - left[1] * right[0];
}

static inline void  v3_crossn (_Out_writes_ (3) float *restrict out, _In_reads_ (3) const float *restrict left, _In_reads_ (3) const float *restrict right) {
    out[0] = left[1] * right[2] - left[2] * right[1];
    out[1] = left[2] * right[0] - left[0] * right[2];
    out[2] = left[0] * right[1] - left[1] * right[0];
    v3_normalizev (out);
}

static void     init_lookat_view (_Out_writes_ (16) float *restrict out, _In_reads_ (3) const float *restrict position, _In_reads_ (3) const float *restrict target) {
    float   forward[3], up[3], right[3], world_up[3];

    world_up[0] = 0;
    world_up[1] = -1;
    world_up[2] = 0;
    // Debug ("position: %f %f %f", position[0], position[1], position[2]);
    // Debug ("target: %f %f %f", target[0], target[1], target[2]);
    v3_sub (forward, target, position);
    // Debug ("forward: %f %f %f", forward[0], forward[1], forward[2]);
    v3_normalizev (forward);
    // Debug ("forwardn: %f %f %f", forward[0], forward[1], forward[2]);
    v3_cross (right, forward, world_up);
    v3_normalizev (right);
    // Debug ("right: %f %f %f", right[0], right[1], right[2]);
    v3_cross (up, right, forward);
    // Debug ("up: %f %f %f", up[0], up[1], up[2]);
    M (out, 0, 0) = right[0];
    M (out, 1, 0) = right[1];
    M (out, 2, 0) = right[2];
    M (out, 3, 0) = -v3_dot (position, right);
    M (out, 0, 1) = up[0];
    M (out, 1, 1) = up[1];
    M (out, 2, 1) = up[2];
    M (out, 3, 1) = -v3_dot (position, up);
    M (out, 0, 2) = forward[0];
    M (out, 1, 2) = forward[1];
    M (out, 2, 2) = forward[2];
    M (out, 3, 2) = -v3_dot (position, forward);
    M (out, 0, 3) = 0;
    M (out, 1, 3) = 0;
    M (out, 2, 3) = 0;
    M (out, 3, 3) = 1;
}

static void     m4_multiply (_Out_writes_ (16) float *restrict out, _In_reads_ (16) const float *restrict a, _In_reads_ (16) const float *restrict b) {
    float a00 = M (a, 0, 0), a01 = M (a, 0, 1), a02 = M (a, 0, 2), a03 = M (a, 0, 3),
        a10 = M (a, 1, 0), a11 = M (a, 1, 1), a12 = M (a, 1, 2), a13 = M (a, 1, 3),
        a20 = M (a, 2, 0), a21 = M (a, 2, 1), a22 = M (a, 2, 2), a23 = M (a, 2, 3),
        a30 = M (a, 3, 0), a31 = M (a, 3, 1), a32 = M (a, 3, 2), a33 = M (a, 3, 3),
        b00 = M (b, 0, 0), b01 = M (b, 0, 1), b02 = M (b, 0, 2), b03 = M (b, 0, 3),
        b10 = M (b, 1, 0), b11 = M (b, 1, 1), b12 = M (b, 1, 2), b13 = M (b, 1, 3),
        b20 = M (b, 2, 0), b21 = M (b, 2, 1), b22 = M (b, 2, 2), b23 = M (b, 2, 3),
        b30 = M (b, 3, 0), b31 = M (b, 3, 1), b32 = M (b, 3, 2), b33 = M (b, 3, 3);
    M (out, 0, 0) = a00 * b00 + a01 * b10 + a02 * b20 + a03 * b30;
    M (out, 0, 1) = a00 * b01 + a01 * b11 + a02 * b21 + a03 * b31;
    M (out, 0, 2) = a00 * b02 + a01 * b12 + a02 * b22 + a03 * b32;
    M (out, 0, 3) = a00 * b03 + a01 * b13 + a02 * b23 + a03 * b33;
    M (out, 1, 0) = a10 * b00 + a11 * b10 + a12 * b20 + a13 * b30;
    M (out, 1, 1) = a10 * b01 + a11 * b11 + a12 * b21 + a13 * b31;
    M (out, 1, 2) = a10 * b02 + a11 * b12 + a12 * b22 + a13 * b32;
    M (out, 1, 3) = a10 * b03 + a11 * b13 + a12 * b23 + a13 * b33;
    M (out, 2, 0) = a20 * b00 + a21 * b10 + a22 * b20 + a23 * b30;
    M (out, 2, 1) = a20 * b01 + a21 * b11 + a22 * b21 + a23 * b31;
    M (out, 2, 2) = a20 * b02 + a21 * b12 + a22 * b22 + a23 * b32;
    M (out, 2, 3) = a20 * b03 + a21 * b13 + a22 * b23 + a23 * b33;
    M (out, 3, 0) = a30 * b00 + a31 * b10 + a32 * b20 + a33 * b30;
    M (out, 3, 1) = a30 * b01 + a31 * b11 + a32 * b21 + a33 * b31;
    M (out, 3, 2) = a30 * b02 + a31 * b12 + a32 * b22 + a33 * b32;
    M (out, 3, 3) = a30 * b03 + a31 * b13 + a32 * b23 + a33 * b33;
}

static void     m4_multiplyv (_Inout_updates_ (16) float *restrict out, _In_reads_ (16) const float *restrict b) {
    float   a[16];

    memcpy (a, out, Msize);
    m4_multiply (out, a, b);
}

static void     m4_transpose (_Out_writes_ (16) float *restrict out, _In_reads_ (16) const float *restrict m) {
    M (out, 0, 0) = M (m, 0, 0); M (out, 1, 0) = M (m, 0, 1);
    M (out, 0, 1) = M (m, 1, 0); M (out, 1, 1) = M (m, 1, 1);
    M (out, 0, 2) = M (m, 2, 0); M (out, 1, 2) = M (m, 2, 1);
    M (out, 0, 3) = M (m, 3, 0); M (out, 1, 3) = M (m, 3, 1);
    M (out, 2, 0) = M (m, 0, 2); M (out, 3, 0) = M (m, 0, 3);
    M (out, 2, 1) = M (m, 1, 2); M (out, 3, 1) = M (m, 1, 3);
    M (out, 2, 2) = M (m, 2, 2); M (out, 3, 2) = M (m, 2, 3);
    M (out, 2, 3) = M (m, 3, 2); M (out, 3, 3) = M (m, 3, 3);
}

static inline void  m4_transposev (_Inout_updates_ (16) float *out) {
    float   t[16];

    m4_transpose (t, out);
    memcpy (out, t, Msize);
}

static inline void  m4_scale (_Out_writes_ (16) float *restrict out, _In_reads_ (16) float *restrict m, float s) {
    M (out, 0, 0) = M (m, 0, 0) * s;
    M (out, 0, 1) = M (m, 0, 1) * s;
    M (out, 0, 2) = M (m, 0, 2) * s;
    M (out, 0, 3) = M (m, 0, 3) * s;
    M (out, 1, 0) = M (m, 1, 0) * s;
    M (out, 1, 1) = M (m, 1, 1) * s;
    M (out, 1, 2) = M (m, 1, 2) * s;
    M (out, 1, 3) = M (m, 1, 3) * s;
    M (out, 2, 0) = M (m, 2, 0) * s;
    M (out, 2, 1) = M (m, 2, 1) * s;
    M (out, 2, 2) = M (m, 2, 2) * s;
    M (out, 2, 3) = M (m, 2, 3) * s;
    M (out, 3, 0) = M (m, 3, 0) * s;
    M (out, 3, 1) = M (m, 3, 1) * s;
    M (out, 3, 2) = M (m, 3, 2) * s;
    M (out, 3, 3) = M (m, 3, 3) * s;
}

static inline void  m4_scalev (_Inout_updates_ (16) float *out, float s) {
    M (out, 0, 0) *= s;
    M (out, 0, 1) *= s;
    M (out, 0, 2) *= s;
    M (out, 0, 3) *= s;
    M (out, 1, 0) *= s;
    M (out, 1, 1) *= s;
    M (out, 1, 2) *= s;
    M (out, 1, 3) *= s;
    M (out, 2, 0) *= s;
    M (out, 2, 1) *= s;
    M (out, 2, 2) *= s;
    M (out, 2, 3) *= s;
    M (out, 3, 0) *= s;
    M (out, 3, 1) *= s;
    M (out, 3, 2) *= s;
    M (out, 3, 3) *= s;
}

static inline void  m4_invert (_Out_writes_ (16) float *restrict out, _In_reads_ (16) const float *restrict mat) {
    float   t[6], det, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;

    // a = M (mat, 0, 0); b = M (mat, 0, 1); c = M (mat, 0, 2); d = M (mat, 0, 3);
    // e = M (mat, 1, 0); f = M (mat, 1, 1); g = M (mat, 1, 2); h = M (mat, 1, 3);
    // i = M (mat, 2, 0); j = M (mat, 2, 1); k = M (mat, 2, 2); l = M (mat, 2, 3);
    // m = M (mat, 3, 0); n = M (mat, 3, 1); o = M (mat, 3, 2); p = M (mat, 3, 3);
    a = M (mat, 0, 0); b = M (mat, 1, 0); c = M (mat, 2, 0); d = M (mat, 3, 0);
    e = M (mat, 0, 1); f = M (mat, 1, 1); g = M (mat, 2, 1); h = M (mat, 3, 1);
    i = M (mat, 0, 2); j = M (mat, 1, 2); k = M (mat, 2, 2); l = M (mat, 3, 2);
    m = M (mat, 0, 3); n = M (mat, 1, 3); o = M (mat, 2, 3); p = M (mat, 3, 3);
    t[0] = k * p - o * l; t[1] = j * p - n * l; t[2] = j * o - n * k;
    t[3] = i * p - m * l; t[4] = i * o - m * k; t[5] = i * n - m * j;
    // M (out, 0, 0) =  f * t[0] - g * t[1] + h * t[2];
    // M (out, 1, 0) =-(e * t[0] - g * t[3] + h * t[4]);
    // M (out, 2, 0) =  e * t[1] - f * t[3] + h * t[5];
    // M (out, 3, 0) =-(e * t[2] - f * t[4] + g * t[5]);
    // M (out, 0, 1) =-(b * t[0] - c * t[1] + d * t[2]);
    // M (out, 1, 1) =  a * t[0] - c * t[3] + d * t[4];
    // M (out, 2, 1) =-(a * t[1] - b * t[3] + d * t[5]);
    // M (out, 3, 1) =  a * t[2] - b * t[4] + c * t[5];
    M (out, 0, 0) =  f * t[0] - g * t[1] + h * t[2];
    M (out, 0, 1) =-(e * t[0] - g * t[3] + h * t[4]);
    M (out, 0, 2) =  e * t[1] - f * t[3] + h * t[5];
    M (out, 0, 3) =-(e * t[2] - f * t[4] + g * t[5]);
    M (out, 1, 0) =-(b * t[0] - c * t[1] + d * t[2]);
    M (out, 1, 1) =  a * t[0] - c * t[3] + d * t[4];
    M (out, 1, 2) =-(a * t[1] - b * t[3] + d * t[5]);
    M (out, 1, 3) =  a * t[2] - b * t[4] + c * t[5];
    t[0] = g * p - o * h; t[1] = f * p - n * h; t[2] = f * o - n * g;
    t[3] = e * p - m * h; t[4] = e * o - m * g; t[5] = e * n - m * f;
    // M (out, 0, 2) =  b * t[0] - c * t[1] + d * t[2];
    // M (out, 1, 2) =-(a * t[0] - c * t[3] + d * t[4]);
    // M (out, 2, 2) =  a * t[1] - b * t[3] + d * t[5];
    // M (out, 3, 2) =-(a * t[2] - b * t[4] + c * t[5]);
    M (out, 2, 0) =  b * t[0] - c * t[1] + d * t[2];
    M (out, 2, 1) =-(a * t[0] - c * t[3] + d * t[4]);
    M (out, 2, 2) =  a * t[1] - b * t[3] + d * t[5];
    M (out, 2, 3) =-(a * t[2] - b * t[4] + c * t[5]);
    t[0] = g * l - k * h; t[1] = f * l - j * h; t[2] = f * k - j * g;
    t[3] = e * l - i * h; t[4] = e * k - i * g; t[5] = e * j - i * f;
    // M (out, 0, 3) =-(b * t[0] - c * t[1] + d * t[2]);
    // M (out, 1, 3) =  a * t[0] - c * t[3] + d * t[4];
    // M (out, 2, 3) =-(a * t[1] - b * t[3] + d * t[5]);
    // M (out, 3, 3) =  a * t[2] - b * t[4] + c * t[5];
    M (out, 3, 0) =-(b * t[0] - c * t[1] + d * t[2]);
    M (out, 3, 1) =  a * t[0] - c * t[3] + d * t[4];
    M (out, 3, 2) =-(a * t[1] - b * t[3] + d * t[5]);
    M (out, 3, 3) =  a * t[2] - b * t[4] + c * t[5];
    // det = 1.0f / (a * M (out, 0, 0) + b * M (out, 1, 0) + c * M (out, 2, 0) + d * M (out, 3, 0));
    det = 1.0f / (a * M (out, 0, 0) + b * M (out, 0, 1) + c * M (out, 0, 2) + d * M (out, 0, 3));
    m4_scalev (out, det);
}

static inline void  m4_multiply_v4 (_Out_writes_ (4) float *restrict out, _In_reads_ (16) const float *restrict m, _In_reads_ (4) const float *restrict v) {
    out[0] = v4_dot (m + 0, v);
    out[1] = v4_dot (m + 4, v);
    out[2] = v4_dot (m + 8, v);
    out[3] = v4_dot (m + 12, v);
}

static inline void  m4_multiply_v4v (_Inout_updates_ (4) float *restrict out, _In_reads_ (16) const float *restrict m) {
    float   v[4];

    memcpy (v, out, V4size);
    m4_multiply_v4 (out, m, v);
}

static inline void  m4_multiply_v3 (_Out_writes_ (3) float *restrict out, _In_reads_ (16) const float *restrict m, _In_reads_ (3) const float *restrict v) {
    out[0] = v3_dot (m + 0, v) + m[0 + 3];
    out[1] = v3_dot (m + 4, v) + m[4 + 3];
    out[2] = v3_dot (m + 8, v) + m[8 + 3];
    v3_scalev (out, 1.f / (v3_dot (m + 12, v) + m[12 + 3]));
}

static inline void  m4_multiply_v3v (_Inout_updates_ (3) float *restrict out, _In_reads_ (16) const float *restrict m) {
    float   v[3];

    memcpy (v, out, V3size);
    m4_multiply_v3 (out, m, v);
}

static inline void  m4_multiply_v3_rot (_Out_writes_ (3) float *restrict out, _In_reads_ (16) const float *restrict m, _In_reads_ (3) const float *restrict v) {
    float   scl;

    out[0] = v3_dot (m + 0, v);
    out[1] = v3_dot (m + 4, v);
    out[2] = v3_dot (m + 8, v);
    scl = v3_dot (m + 12, v);
    if (scl) {
        v3_scalev (out, 1.f / scl);
    }
}

static inline void  m4_multiply_v3_rotv (_Inout_updates_ (3) float *restrict out, _In_reads_ (16) const float *restrict m) {
    float   v[3];

    memcpy (v, out, V3size);
    m4_multiply_v3_rot (out, m, v);
}

static inline void  m4_multiply_v3r (_Out_writes_ (3) float *restrict out, _In_reads_ (16) const float *restrict m, _In_reads_ (3) const float *restrict v) {
    out[0] = v3_dot (m + 0, v);
    out[1] = v3_dot (m + 4, v);
    out[2] = v3_dot (m + 8, v);
    v3_scalev (out, 1.f / v3_dot (m + 12, v));
}

static inline void  m4_multiply_v3rv (_Inout_updates_ (3) float *restrict out, _In_reads_ (16) const float *restrict m) {
    float   v[3];

    memcpy (v, out, V3size);
    m4_multiply_v3r (out, m, v);
}

static inline void  m4_basis (_In_reads_ (16) const float *restrict m, _Out_writes_ (3) float *restrict x, _Out_writes_ (3) float *restrict y, _Out_writes_ (3) float *restrict z) {
    x[0] = M (m, 0, 0);
    y[0] = M (m, 1, 0);
    z[0] = M (m, 2, 0);
    x[1] = M (m, 0, 1);
    y[1] = M (m, 1, 1);
    z[1] = M (m, 2, 1);
    x[2] = M (m, 0, 2);
    y[2] = M (m, 1, 2);
    z[2] = M (m, 2, 2);
}

static inline void  m4_translatev (_Inout_updates_ (16) float *restrict out, _In_reads_ (3) const float *restrict v) {
    float   v0[4], v1[4], v2[4];

    /* todo: test */
    m4_basis (out, v0, v1, v2);
    v3_scalev (v0, v[0]);
    v3_scalev (v1, v[1]);
    v3_scalev (v2, v[2]);
    M (out, 3, 0) += v0[0] + v1[0] + v2[0];
    M (out, 3, 1) += v0[1] + v1[1] + v2[1];
    M (out, 3, 2) += v0[2] + v1[2] + v2[2];
}

static inline void  m4_translate (_Out_writes_ (16) float *restrict out, _In_reads_ (16) const float *restrict m, _In_reads_ (3) const float *restrict v) {
    memcpy (out, m, Msize);
    m4_translatev (out, v);
}


static void v4_print (_In_reads_ (4) const float *v) {
    Print ("[ %f\t%f\t%f\t%f ]", v[0], v[1], v[2], v[3]);
}

static void v3_print (_In_reads_ (3) const float *v) {
    Print ("[ %f\t%f\t%f ]", v[0], v[1], v[2]);
}

static void v2_print (_In_reads_ (2) const float *v) {
    Print ("[ %f\t%f ]", v[0], v[1]);
}

static void m4_print (_In_reads_ (16) const float *m) {
    Print ("\n[ [ %f\t%f\t%f\t%f ]\n  [ %f\t%f\t%f\t%f ]\n  [ %f\t%f\t%f\t%f ]\n  [ %f\t%f\t%f\t%f ] ]\n", M (m, 0, 0), M (m, 0, 1), M (m, 0, 2), M (m, 0, 3), M (m, 1, 0), M (m, 1, 1), M (m, 1, 2), M (m, 1, 3), M (m, 2, 0), M (m, 2, 1), M (m, 2, 2), M (m, 2, 3), M (m, 3, 0), M (m, 3, 1), M (m, 3, 2), M (m, 3, 3));
}

static inline void      make_m4_identity (_Out_writes_ (16) float *out) {
    static const float identity[16] = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1, };

    memcpy (out, identity, Msize);
}

static inline void      make_m4_translation (_Out_writes_ (16) float *out, float x, float y, float z) {
    make_m4_identity (out);
    M (out, 3, 0) = x;
    M (out, 3, 1) = y;
    M (out, 3, 2) = z;
}

static inline void  make_q_identity (_Out_writes_ (4) float *out) {
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    out[3] = 1;
}

static void make_q_v3angle (_Out_writes_ (4) float *restrict out, _In_reads_ (3) const float *restrict v, float angle) {
    float l;

    l = v3_dotv (v);
    if (l > 0) {
        float   s;

        l = 1.f / sqrtf (l);
        angle *= 0.5f;
        s = sinf (angle);
        out[0] = s * v[0] * l;
        out[1] = s * v[1] * l;
        out[2] = s * v[2] * l;
        out[3] = cosf (angle);
    } else {
        make_q_identity (out);
    }
}

static inline void  make_q_v3angle_ (_Out_writes_ (4) float *out, float x, float y, float z, float angle) {
    float   v[3];

    v[0] = x;
    v[1] = y;
    v[2] = z;
    make_q_v3angle (out, v, angle);
}

static inline void  q_normalize (_Out_writes_ (4) float *restrict out, _In_reads_ (4) const float *restrict q) {
    float   l;

    l = v4_dotv (q);
    if (l > 0) {
        l = 1.f / sqrtf (l);
        out[0] = q[0] * l;
        out[1] = q[1] * l;
        out[2] = q[2] * l;
        out[3] = q[3] * l;
    } else {
        make_q_identity (out);
    }
}

static inline void  q_normalizev (_Inout_updates_ (4) float *out) {
    float   l;

    l = v4_dotv (out);
    if (l > 0) {
        l = 1.f / sqrtf (l);
        out[0] *= l;
        out[1] *= l;
        out[2] *= l;
        out[3] *= l;
    } else {
        make_q_identity (out);
    }
}

static inline void  q_conjugate (_Out_writes_ (4) float *restrict out, _In_reads_ (4) const float *restrict q) {
    out[0] = -q[0];
    out[1] = -q[1];
    out[2] = -q[2];
    out[3] = q[3];
}

static inline void  q_conjugatev (_Inout_updates_ (4) float *out) {
    out[0] = -out[0];
    out[1] = -out[1];
    out[2] = -out[2];
}

static inline void  q_inv (_Out_writes_ (4) float *restrict out, _In_reads_ (4) const float *restrict q) {
    float   s;

    q_conjugate (out, q);
    s = 1.f / v4_dotv (q);
    out[0] *= s;
    out[1] *= s;
    out[2] *= s;
    out[3] *= s;
}

static inline float q_ilen (_In_reads_ (4) const float *q) {
    return (sqrtf (v3_dotv (q)));
}

static inline float q_angle (_In_reads_ (4) const float *q) {
    return (2 * atan2f (q_ilen (q), q[3]));
}

static inline float q_norm (_In_reads_ (4) const float *q) {
    return (sqrtf (v4_dotv (q)));
}

static inline void  q_axis (_Out_writes_ (3) float *restrict out, _In_reads_ (4) const float *restrict q) {
    v3_normalize (out, q);
}

static inline void  q_multiply (_Out_writes_ (4) float *restrict out, _In_reads_ (4) const float *restrict q0, _In_reads_ (4) const float *restrict q1) {
    out[0] = q0[3] * q1[0] + q0[0] * q1[3] + q0[1] * q1[2] - q0[2] * q1[1];
    out[1] = q0[3] * q1[1] - q0[0] * q1[2] + q0[1] * q1[3] + q0[2] * q1[0];
    out[2] = q0[3] * q1[2] + q0[0] * q1[1] - q0[1] * q1[0] + q0[2] * q1[3];
    out[3] = q0[3] * q1[3] - q0[0] * q1[0] - q0[1] * q1[1] - q0[2] * q1[2];
}

static inline void  q_multiplyv (_Inout_updates_ (4) float *restrict out, _In_reads_ (4) const float *restrict q1) {
    float   q0[4];

    /* todo: test */
    memcpy (q0, out, Qsize);
    q_multiply (out, q0, q1);
}

static inline void  q_to_m4 (_Out_writes_ (16) float *restrict out, _In_reads_ (4) const float *restrict q) {
    float w, x, y, z, xx, yy, zz, xy, yz, xz, wx, wy, wz, norm, s;

    norm = q_norm (q);
    s = norm > 0 ? 2 / norm : 0;
    x = q[0];
    y = q[1];
    z = q[2];
    w = q[3];
    xx = s * x * x;
    xy = s * x * y;
    xz = s * x * z;
    yy = s * y * y;
    yz = s * y * z;
    zz = s * z * z;
    wx = s * w * x;
    wy = s * w * y;
    wz = s * w * z;
    M (out, 0, 0) = 1 - yy - zz;
    M (out, 0, 1) = xy + wz;
    M (out, 0, 2) = xz - wy;
    M (out, 0, 3) = 0;
    M (out, 1, 0) = xy - wz;
    M (out, 1, 1) = 1 - xx - zz;
    M (out, 1, 2) = yz + wx;
    M (out, 1, 3) = 0;
    M (out, 2, 0) = xz + wy;
    M (out, 2, 1) = yz - wx;
    M (out, 2, 2) = 1 - xx - yy;
    M (out, 2, 3) = 0;
    M (out, 3, 0) = 0;
    M (out, 3, 1) = 0;
    M (out, 3, 2) = 0;
    M (out, 3, 3) = 1;
}

static inline void  q_to_m4t (_Out_writes_ (16) float *restrict out, _In_reads_ (4) const float *restrict q) {
    float w, x, y, z, xx, yy, zz, xy, yz, xz, wx, wy, wz, norm, s;

    norm = q_norm (q);
    s = norm > 0 ? 2 / norm : 0;
    x = q[0];
    y = q[1];
    z = q[2];
    w = q[3];
    xx = s * x * x;
    xy = s * x * y;
    xz = s * x * z;
    yy = s * y * y;
    yz = s * y * z;
    zz = s * z * z;
    wx = s * w * x;
    wy = s * w * y;
    wz = s * w * z;
    M (out, 0, 0) = 1 - yy - zz;
    M (out, 1, 0) = xy + wz;
    M (out, 2, 0) = xz - wy;
    M (out, 3, 0) = 0;
    M (out, 0, 1) = xy - wz;
    M (out, 1, 1) = 1 - xx - zz;
    M (out, 2, 1) = yz + wx;
    M (out, 3, 1) = 0;
    M (out, 0, 2) = xz + wy;
    M (out, 1, 2) = yz - wx;
    M (out, 2, 2) = 1 - xx - yy;
    M (out, 3, 2) = 0;
    M (out, 0, 3) = 0;
    M (out, 1, 3) = 0;
    M (out, 2, 3) = 0;
    M (out, 3, 3) = 1;
}

static inline void  q_lerp (_Out_writes_ (4) float *restrict out, _In_reads_ (4) const float *restrict left, _In_reads_ (4) const float *restrict right, float t) {
    /* todo: test */
    v4_lerp (out, left, right, t);
}

static inline void  q_slerp (_Out_writes_ (4) float *restrict out, _In_reads_ (4) const float *restrict left, _In_reads_ (4) const float *restrict right, float t) {
    float   q1[4], q2[4];
    float   cost, sint, angle, scale;

    /* todo: test */
    cost = v4_dot (left, right);
    memcpy (q1, left, Qsize);
    if (fabsf (cost) >= 1) {
        memcpy (out, q1, Qsize);
    } else {
        if (cost < 0) {
            v4_negv (q1);
            cost = -cost;
        }
        sint = sqrtf (1 - cost * cost);
        if (fabsf (sint) < 0.001f) {
            q_lerp (out, left, right, t);
        } else {
            angle = acosf (cost);
            scale = sinf ((1 - t) * angle);
            v4_scalev (q1, scale);
            scale = sinf (t * angle);
            v4_scale (q2, right, scale);
            v4_addv (q1, q2);
            scale = 1 / sint;
            v4_scale (out, q1, scale);
        }
    }
}

static inline void  q_look (_Out_writes_ (16) float *restrict out, _In_reads_ (4) const float *restrict q, _In_reads_ (3) const float *restrict position) {
    float   rpos[3];

    /* todo: test */
    q_to_m4t (out, q);
    v3_neg (rpos, position);
    m4_translatev (out, rpos);
}

static inline void  q_rotatev3 (_Out_writes_ (3) float *restrict out, _In_reads_ (4) const float *restrict q, _In_reads_ (3) const float *restrict v) {
    float   u[4], v1[3], v2[3];

    q_normalize (u, q);
    v3_scale (v1, u, 2 * v3_dot (u, v));
    v3_scale (v2, v, u[3] * u[3] - v3_dotv (u));
    v3_addv (v1, v2);
    v3_cross (v2, u, v);
    v3_scalev (v2, 2 * u[3]);
    v3_add (out, v1, v2);
}

static inline void  q_rotatev3v (_Inout_updates_ (3) float *restrict out, _In_reads_ (4) const float *restrict q) {
    float   p[4], u[3], v1[3], v2[3], s;

    q_normalize (p, q);
    memcpy (u, p, V3size);
    s = p[3];
    v3_scale (v1, u, 2 * v3_dot (u, out));
    v3_scale (v2, out, s * s - v3_dotv (u));
    v3_addv (v1, v2);
    v3_cross (v2, u, out);
    v3_scalev (v2, 2 * s);
    v3_add (out, v1, v2);
}

static inline void  q_rotatem4 (_Out_writes_ (16) float *restrict out, _In_reads_ (16) const float *restrict m, _In_reads_ (4) const float *restrict q) {
    float   rot[16];

    /* todo: test */
    q_to_m4t (rot, q);
    m4_multiply (out, m, rot);
}

static inline void  q_rotatem4v (_Inout_updates_ (16) float *restrict out, _In_reads_ (4) const float *restrict q) {
    float   rot[16];

    /* todo: test */
    q_to_m4t (rot, q);
    m4_multiplyv (out, rot);
}

static inline void  q_rotatem4_at (_Out_writes_ (16) float *restrict out, _In_reads_ (16) const float *restrict m, _In_reads_ (4) const float *restrict q, _In_reads_ (3) const float *restrict p) {
    float   invp[3];

    /* todo: test */
    v3_neg (invp, p);
    m4_translate (out, m, p);
    q_rotatem4v (out, q);
    m4_translatev (out, invp);
}

static inline void  q_rotatem4_atv (_Inout_updates_ (16) float *restrict out, _In_reads_ (4) const float *restrict q, _In_reads_ (3) const float *restrict p) {
    float   invp[3];

    /* todo: test */
    v3_neg (invp, p);
    m4_translatev (out, p);
    q_rotatem4v (out, q);
    m4_translatev (out, invp);
}

static inline void  q_basis (_In_reads_ (4) const float *restrict q, _Out_writes_ (3) float *restrict right, _Out_writes_ (3) float *restrict up, _Out_writes_ (3) float *restrict forward) {
    static const float  forward0[3] = { 0, 0, 1, }, up0[3] = { 0, -1, 0, };

    q_rotatev3 (forward, q, forward0);
    v3_normalizev (forward);
    v3_cross (right, forward, up0);
    v3_cross (up, right, forward);
}

static inline void  q_plane_basis (_In_reads_ (4) const float *restrict q, _Out_writes_ (3) float *restrict right, _Out_writes_ (3) float *restrict forward) {
    static const float  forward0[3] = { 0, 0, 1, }, up0[3] = { 0, -1, 0, };

    q_rotatev3 (forward, q, forward0);
    v3_normalizev (forward);
    v3_cross (right, forward, up0);
    v3_normalizev (right);
    v3_cross (forward, up0, right);
    v3_normalizev (forward);
}

static inline void  make_q_pitch_yaw (_Out_writes_ (4) float *out, float pitch, float yaw) {
    static float    up[3] = { 0, 1, 0, }, right[3] = { 1, 0, 0, };
    float   q[2][4];

    make_q_v3angle (q[0], up, pitch);
    make_q_v3angle (q[1], right, yaw);
    make_q_identity (out);
    q_multiplyv (out, q[0]);
    q_multiplyv (out, q[1]);
}

