#if BGFX_SHADER_LANGUAGE_HLSL
float4 vec4_splat(float x) { return float4(x, x, x, x); }
#else
#include <bgfx_shader.sh>
#endif

void main()
{
    gl_FragColor = vec4(0.2, 1.0, 0.4, 1.0);
}
