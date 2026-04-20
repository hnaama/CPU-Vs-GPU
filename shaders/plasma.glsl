// NAME: Plasma Waves
// DESC: Classic plasma effect with animated sine waves

// This shader creates a plasma effect using multiple sine waves
// Parameters:
//   param1: Wave frequency for red channel
//   param2: Wave frequency for green channel
//   param3: Wave frequency for blue channel
//   param4: Animation speed multiplier

// The shader runs on every pixel and calculates color based on position and time
// On a GPU, all pixels would be calculated in parallel
// On CPU, they're calculated sequentially (much slower!)

// Shader type: PLASMA
void main() {
    // x and y are normalized coordinates from -1 to 1
    // time is the elapsed time in seconds
    
    float r = sin(x * 10.0 * param1 + time) * 0.5 + 0.5;
    float g = sin(y * 10.0 * param2 + time * 1.3) * 0.5 + 0.5;
    float b = sin((x + y) * 5.0 * param3 + time * 0.7) * 0.5 + 0.5;
    
    // Output RGB color
    return rgb(r, g, b);
}

// Educational Note:
// This effect requires calculating sin() 3 times per pixel
// At 800x600 resolution, that's 1.44 MILLION sin() calls per frame!
// At 60 FPS, that's 86.4 MILLION sin() calls per second!
// GPUs handle this easily because they have thousands of cores working in parallel
