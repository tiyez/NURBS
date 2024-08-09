

#define Shader(name) Shader_##name
enum { Shader (vs), Shader (ps), Shader (ds), Shader (hs), Shader (_count) };

union fxc_header {
    int     _fields [64];
    struct {
        struct {
            int offset;
            int size;
        } range [Shader (_count)];
    };
};

