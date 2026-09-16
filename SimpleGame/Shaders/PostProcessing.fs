#version 120
uniform sampler2D scene;
uniform float exposure;
uniform float strength;
uniform float bloomStrength;
uniform vec2 texel;
varying vec2 uv;

float finiteRadiance(float v)
{
    // GLSL 1.20 has no isnan/isinf: unordered or negative values fail this test.
    if (!(v >= 0.0))
    {
        return 0.0;
    }
    return min(v, 60000.0);
}

vec3 readScene(vec2 p)
{
    vec3 c = texture2D(scene, p).rgb;
    return vec3(finiteRadiance(c.r), finiteRadiance(c.g), finiteRadiance(c.b));
}

vec3 bright(vec2 p)
{
    vec3 c = readScene(p);
    float peak = max(c.r, max(c.g, c.b));
    return c * max(peak - 1.0, 0.0) / max(peak, 0.0001);
}

vec3 toneMap(vec3 c)
{
    // ACES-style fitted filmic curve, followed by display gamma conversion.
    return clamp((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14), 0.0, 1.0);
}

void main()
{
    vec3 color = readScene(uv);
    vec3 bloom = vec3(0.0);
    for (int y = -2; y <= 2; ++y)
    {
        for (int x = -2; x <= 2; ++x)
        {
            vec2 offset = vec2(float(x), float(y));
            float weight = (3.0 - abs(float(x))) * (3.0 - abs(float(y)));
            bloom += bright(uv + offset * texel * 2.5) * weight / 81.0;
        }
    }
    color = toneMap((color + bloom * bloomStrength) * exposure);
    // Apply the mask after display conversion so gamma does not wash out the edges.
    vec3 displayColor = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
    vec2 p = (uv - 0.5) * 2.0;
    float edge = smoothstep(0.30, 1.40, length(p));
    float vignette = 1.0 - clamp(strength, 0.0, 0.95) * edge;
    gl_FragColor = vec4(displayColor * vignette, 1.0);
}
