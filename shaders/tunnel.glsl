// NAME: Tunnel Effect
// DESC: Psychedelic tunnel with rotating colors

// This creates a classic demoscene tunnel effect
// Parameters:
//   param1: Radial frequency
//   param2: Angular frequency
//   param3: Color mix factor
//   param4: Speed multiplier

// Shader type: TUNNEL
void main() {
    float dist = sqrt(x * x + y * y);
    float angle = atan2(y, x);
    
    float r = sin(dist * 10.0 * param1 - time * 2.0) * 0.5 + 0.5;
    float g = sin(angle * 5.0 * param2 + time) * 0.5 + 0.5;
    float b = sin(dist * 5.0 * param3 + angle * 3.0 - time) * 0.5 + 0.5;
    
    return rgb(r, g, b);
}

// Performance Note:
// This shader uses sqrt() and atan2() - expensive operations!
// sqrt(): ~15-30 CPU cycles
// atan2(): ~40-100 CPU cycles
// At 800x600 = 480,000 pixels, this is VERY demanding on CPU
// GPU: No problem! Hardware accelerated math units handle this easily
