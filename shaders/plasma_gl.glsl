#version 330 core

in vec2 fragCoord;
out vec4 FragColor;

uniform float iTime;
uniform vec2 iResolution;
uniform float iParam1;
uniform float iParam2;
uniform float iParam3;
uniform float iParam4;

void main() {
    // Normalize coordinates to [-1, 1]
    vec2 uv = (fragCoord * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
    
    // Plasma effect using interference patterns
    float r = sin(uv.x * 10.0 * iParam1 + iTime) * 0.5 + 0.5;
    float g = sin(uv.y * 10.0 * iParam2 + iTime * 1.3) * 0.5 + 0.5;
    float b = sin((uv.x + uv.y) * 5.0 * iParam3 + iTime * 0.7) * 0.5 + 0.5;
    
    FragColor = vec4(r, g, b, 1.0);
}
