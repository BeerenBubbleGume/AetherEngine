$input a_position, a_normal
$output v_worldPos, v_normal

#include <bgfx_shader.sh>

void main()
{
    vec4 worldPos = mul(u_model[0], vec4(a_position, 1.0));
    vec4 worldNormal = mul(u_model[0], vec4(a_normal, 0.0));

    gl_Position = mul(u_viewProj, worldPos);
    v_worldPos = worldPos.xyz;
    v_normal = normalize(worldNormal.xyz);
}
