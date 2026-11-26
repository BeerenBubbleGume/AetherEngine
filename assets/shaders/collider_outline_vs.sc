$input a_position

#if BGFX_SHADER_LANGUAGE_HLSL
uniform float4x4 u_modelViewProj;
#else
#include <bgfx_shader.sh>
#endif

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}
