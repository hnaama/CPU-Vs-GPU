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
    
    // Mandelbrot set
    vec2 c = uv * iParam1;
    vec2 z = vec2(0.0);
    
    int maxIter = int(50.0 * iParam2);
    int iter = 0;
    
    for (int i = 0; i < 200; i++) {
        if (i >= maxIter) break;
        if (dot(z, z) > 4.0) break;
        
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        iter++;
    }
    
    // Color based on iteration count
    float value = float(iter) / float(maxIter);
    float hue = value + iTime * 0.1 * iParam3;
    
    // Simple HSV to RGB
    vec3 rgb = 0.5 + 0.5 * cos(6.28318 * (hue + vec3(0.0, 0.33, 0.67)));
    
    FragColor = vec4(rgb * iParam4, 1.0);
}
