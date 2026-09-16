#version 120
varying vec3 eyePosition;
varying vec3 eyeNormal;
varying vec3 localPosition;
varying vec3 tint;
uniform float metallic;
uniform float roughness;
uniform float emission;
uniform float fogEnabled;
uniform float flashlightOn;
const float PI = 3.14159265;

vec3 safeNormalize(vec3 v)
{
    return v * inversesqrt(max(dot(v, v), 0.000001));
}

vec3 lightBRDF(vec3 position, vec3 radiance, vec3 N, vec3 V, vec3 base, vec3 F0, float rough)
{
    vec3 delta = position - eyePosition;
    float distanceSquared = max(dot(delta, delta), 0.01);
    vec3 L = delta * inversesqrt(distanceSquared);
    vec3 H = safeNormalize(V + L);
    float NoL = clamp(dot(N, L), 0.0, 1.0), NoV = clamp(dot(N, V), 0.001, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0), VoH = clamp(dot(V, H), 0.0, 1.0);
    float a = rough * rough, a2 = a * a;
    float d = NoH * NoH * (a2 - 1.0) + 1.0;
    float D = a2 / max(PI * d * d, 0.0001);
    float k = (rough + 1.0) * (rough + 1.0) / 8.0;
    float G = (NoV / (NoV * (1.0 - k) + k)) * (NoL / (NoL * (1.0 - k) + k));
    // A camera-mounted light makes V and H almost identical. Roundoff can
    // otherwise push their dot product above 1 and give pow a negative base.
    float fresnel = 1.0 - VoH;
    float fresnel2 = fresnel * fresnel;
    vec3 F = F0 + (1.0 - F0) * fresnel2 * fresnel2 * fresnel;
    vec3 specular = D * G * F / max(4.0 * NoV * NoL, 0.001);
    vec3 diffuse = (1.0 - F) * (1.0 - metallic) * base / PI;
    return (diffuse + specular) * radiance * NoL / (1.0 + 0.12 * distanceSquared);
}

void main()
{
    vec3 N = safeNormalize(eyeNormal), V = safeNormalize(-eyePosition);
    // Fine directional variation approximates brushed metal without external textures.
    float brush = sin(localPosition.y * 260.0 + sin(localPosition.z * 31.0) * 2.0);
    float filterWidth = fwidth(localPosition.y * 260.0);
    brush *= 1.0 - smoothstep(0.8, 3.0, filterWidth);
    vec3 base = pow(max(tint, vec3(0.0)), vec3(2.2));
    base *= 1.0 + 0.07 * brush * metallic;
    float rough = clamp(roughness + 0.035 * brush * metallic, 0.12, 0.95);
    vec3 F0 = mix(vec3(0.04), base, metallic);
    // Low indirect fill keeps unlit metal legible; this is not a reflected scene map.
    vec3 color = base * 0.10 + F0 * 0.025;
    color +=
        lightBRDF(gl_LightSource[0].position.xyz, gl_LightSource[0].diffuse.rgb * 8.0, N, V, base, F0, rough);
    color +=
        lightBRDF(gl_LightSource[1].position.xyz, gl_LightSource[1].diffuse.rgb * 8.0, N, V, base, F0, rough);
    color +=
        lightBRDF(gl_LightSource[2].position.xyz, gl_LightSource[2].diffuse.rgb * 8.0, N, V, base, F0, rough);
    color +=
        lightBRDF(gl_LightSource[3].position.xyz, gl_LightSource[3].diffuse.rgb * 8.0, N, V, base, F0, rough);
    color +=
        lightBRDF(gl_LightSource[4].position.xyz, gl_LightSource[4].diffuse.rgb * 8.0, N, V, base, F0, rough);
    color +=
        lightBRDF(gl_LightSource[5].position.xyz, gl_LightSource[5].diffuse.rgb * 8.0, N, V, base, F0, rough);
    if (flashlightOn > 0.5)
    {
        float cone = smoothstep(0.86, 0.96, dot(-V, vec3(0.0, 0.0, -1.0)));
        if (cone > 0.0)
        {
            color += lightBRDF(vec3(0.0), vec3(18.0, 19.0, 20.0) * cone, N, V, base, F0, rough);
        }
    }
    if (emission > 0.0)
    {
        color = base * emission;
    }
    float fog = smoothstep(12.0, 48.0, length(eyePosition)) * fogEnabled;
    color = mix(color, vec3(0.003, 0.005, 0.009), fog);
    gl_FragColor = vec4(color, 1.0);
}
