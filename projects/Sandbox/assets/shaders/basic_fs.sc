$input v_worldPos, v_normal, v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_baseColor;
SAMPLER2D(s_albedo, 0);

void main()
{
    vec3 normal = normalize(v_normal);
    vec3 lightDir = normalize(vec3(-0.35, 0.65, -0.70));
    vec4 albedo = texture2D(s_albedo, v_texcoord0);

    float diffuse = max(dot(normal, lightDir), 0.0);
    float rim = pow(1.0 - saturate(abs(normal.z)), 2.0);

    vec3 baseColor = u_baseColor.rgb * albedo.rgb;
    vec3 warmLight = vec3(1.00, 0.92, 0.78);
    vec3 coolAmbient = vec3(0.10, 0.14, 0.20);

    vec3 color = baseColor * 0.15 + baseColor * diffuse;
    color += baseColor * warmLight * diffuse;
    color += vec3(0.35, 0.58, 1.00) * rim * 0.20;
    color *= 0.85 + saturate(v_worldPos.y + 0.5) * 0.25;

    gl_FragColor = vec4(color, u_baseColor.a * albedo.a);
}