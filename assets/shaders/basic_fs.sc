$input v_worldPos, v_normal

#include <bgfx_shader.sh>

void main()
{
    vec3 normal = normalize(v_normal);
    vec3 lightDir = normalize(vec3(-0.35, 0.65, -0.70));

    float diffuse = max(dot(normal, lightDir), 0.0);
    float rim = pow(1.0 - saturate(abs(normal.z)), 2.0);

    vec3 baseColor = vec3(0.24, 0.48, 0.90);
    vec3 warmLight = vec3(1.00, 0.92, 0.78);
    vec3 coolAmbient = vec3(0.10, 0.14, 0.20);

    vec3 color = baseColor * coolAmbient;
    color += baseColor * warmLight * diffuse;
    color += vec3(0.35, 0.58, 1.00) * rim * 0.20;
    color *= 0.85 + saturate(v_worldPos.y + 0.5) * 0.25;

    gl_FragColor = vec4(color, 1.0);
}
