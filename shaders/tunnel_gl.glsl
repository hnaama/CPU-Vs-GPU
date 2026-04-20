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
    vec2 uv = (fragCoord * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
    
    // Convert to polar coordinates
    float dist = length(uv);
    float angle = atan(uv.y, uv.x);
    
    // Tunnel effect
    float r = sin(dist * 10.0 * iParam1 - iTime * 2.0) * 0.5 + 0.5;
    float g = sin(angle * 5.0 * iParam2 + iTime) * 0.5 + 0.5;
    float b = sin(dist * 5.0 * iParam3 + angle * 3.0 - iTime) * 0.5 + 0.5;
    
    FragColor = vec4(r, g, b, 1.0);
}
