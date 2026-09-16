#version 120
varying vec3 eyePosition;
varying vec3 eyeNormal;
varying vec3 localPosition;
varying vec3 tint;

void main()
{
    eyePosition = (gl_ModelViewMatrix * gl_Vertex).xyz;
    eyeNormal = gl_NormalMatrix * gl_Normal;
    localPosition = gl_Vertex.xyz;
    tint = gl_Color.rgb;
    gl_Position = gl_ProjectionMatrix * vec4(eyePosition, 1.0);
}
