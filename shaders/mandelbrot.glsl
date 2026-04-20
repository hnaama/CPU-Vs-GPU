// NAME: Mandelbrot Fractal
// DESC: Real-time animated Mandelbrot set

// Classic Mandelbrot fractal with color animation
// Parameters:
//   param1: Zoom level
//   param2: Max iterations (quality)
//   param3: Color animation speed
//   param4: Unused

// Shader type: MANDELBROT
void main() {
    float cx = x * param1;
    float cy = y * param1;
    float zx = 0;
    float zy = 0;
    int iter = 0;
    int maxIter = int(50 * param2);
    
    while (zx * zx + zy * zy < 4.0 && iter < maxIter) {
        float temp = zx * zx - zy * zy + cx;
        zy = 2.0 * zx * zy + cy;
        zx = temp;
        iter++;
    }
    
    float value = float(iter) / float(maxIter);
    float hue = value + time * 0.1 * param3;
    
    float r = sin(hue * 6.28) * 0.5 + 0.5;
    float g = sin(hue * 6.28 + 2.09) * 0.5 + 0.5;
    float b = sin(hue * 6.28 + 4.19) * 0.5 + 0.5;
    
    return rgb(r, g, b);
}

// Computational Complexity:
// Each pixel requires up to 50 iterations of complex math
// That's potentially 24 MILLION iterations per frame!
// CPU: Sequential processing = very slow
// GPU: Parallel processing = smooth real-time rendering
// This demonstrates why GPUs revolutionized graphics and scientific computing
