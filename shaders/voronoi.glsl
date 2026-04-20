// NAME: Voronoi Cells
// DESC: Animated Voronoi diagram with moving cell centers

// Creates a Voronoi pattern with animated cell centers
// Parameters:
//   param1: Animation speed
//   param2: Cell density
//   param3: Color intensity
//   param4: Unused

// Shader type: VORONOI
void main() {
    float minDist = 10.0;
    
    // Check 9 neighboring cells
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            float cellX = float(i) + sin(time * param1 + float(i * j)) * 0.5;
            float cellY = float(j) + cos(time * param1 + float(i * j)) * 0.5;
            float dx = (x * param2) - cellX;
            float dy = (y * param2) - cellY;
            float dist = sqrt(dx * dx + dy * dy);
            minDist = min(minDist, dist);
        }
    }
    
    float value = minDist * param3;
    float r = sin(value + time) * 0.5 + 0.5;
    float g = sin(value + time + 2.0) * 0.5 + 0.5;
    float b = sin(value + time + 4.0) * 0.5 + 0.5;
    
    return rgb(r, g, b);
}

// Performance Analysis:
// Each pixel requires 9 distance calculations
// That's 9 × 480,000 = 4.32 MILLION distance calculations per frame!
// Each distance calculation involves sqrt() - an expensive operation
// CPU: Struggles to maintain even 10 FPS
// GPU: Easily maintains 60+ FPS
// This shows the power of parallel processing for independent computations
