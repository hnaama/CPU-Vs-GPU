// NAME: Ripple Waves
// DESC: Concentric ripple effect from center

// Creates animated ripple waves emanating from the center
// Parameters:
//   param1: Ripple frequency
//   param2: Wave speed
//   param3: Color variation
//   param4: Amplitude

// Shader type: RIPPLE
void main() {
    float dist = sqrt(x * x + y * y);
    float ripple = sin(dist * 20.0 * param1 - time * 5.0);
    
    float r = ripple * 0.5 + 0.5;
    float g = sin(dist * 15.0 * param2 - time * 3.0) * 0.5 + 0.5;
    float b = cos(dist * 10.0 * param3 - time * 4.0) * 0.5 + 0.5;
    
    return rgb(r, g, b);
}

// Why GPU is Better:
// Each pixel is completely independent - perfect for parallel processing!
// CPU must calculate 480,000 pixels sequentially
// GPU can calculate thousands of pixels simultaneously
// Result: 50-100x faster on GPU for this type of shader
