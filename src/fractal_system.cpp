#include "fractal_system.h"
#include "fractals.h"
#include "pixelbuffer.h"
#include <cmath>
#include <algorithm>

// Global fractal system instance
FractalSystem* g_fractalSystem = nullptr;

FractalSystem::FractalSystem(int w, int h) : width(w), height(h), time(0), fractalType(0), 
    zoomLevel(1.0f), center(0, 0, 0), warpIntensity(1.0f), colorShift(0), 
    pulseSpeed(1.0f), chaosLevel(0.5f), isTripping(false) {
    
    colorGrid.resize(height, std::vector<uint32_t>(width, 0xFF000000));
    initialize();
}

void FractalSystem::initialize() {
    // Initialize with random fractal parameters
    fractalType = randomInt(0, 8);
    zoomLevel = randomFloat(0.5f, 3.0f);
    center = Vec3(randomFloat(-2, 2), randomFloat(-2, 2), 0);
    warpIntensity = randomFloat(0.5f, 3.0f);
    pulseSpeed = randomFloat(0.5f, 2.0f);
    chaosLevel = randomFloat(0.3f, 0.8f);
    
    attractors.clear();
    int numAttractors = randomInt(2, 5);
    for (int i = 0; i < numAttractors; i++) {
        attractors.push_back(Vec3(randomFloat(-2, 2), randomFloat(-2, 2), randomFloat(-1, 1)));
    }
}

void FractalSystem::update(float deltaTime) {
    time += deltaTime * pulseSpeed;
    
    // Occasionally change visual state
    if (randomFloat(0, 1) < 0.02f) {
        isTripping = !isTripping;
        if (isTripping) {
            warpIntensity = randomFloat(3.0f, 8.0f);
            pulseSpeed = randomFloat(2.0f, 5.0f);
            chaosLevel = randomFloat(0.7f, 1.0f);
            fractalType = randomInt(0, 8);
            zoomLevel *= randomFloat(0.5f, 2.0f);
        } else {
            warpIntensity = randomFloat(1.0f, 4.0f);
            pulseSpeed = randomFloat(1.0f, 3.0f);
            chaosLevel = randomFloat(0.4f, 0.7f);
        }
    }
    
    // Animate parameters
    colorShift += deltaTime * randomFloat(50.0f, 150.0f);
    zoomLevel *= 1.0f + sin(time * 1.5f) * 0.05f * chaosLevel;
    
    // Move center in smooth patterns
    center.x += sin(time * 0.7f + colorShift * 0.01f) * chaosLevel * 0.02f;
    center.y += cos(time * 0.5f + colorShift * 0.007f) * chaosLevel * 0.02f;
    
    // Update attractors for dynamic effects
    for (auto& attractor : attractors) {
        attractor.x += sin(time * randomFloat(0.5f, 1.5f)) * chaosLevel * 0.05f;
        attractor.y += cos(time * randomFloat(0.5f, 1.5f)) * chaosLevel * 0.05f;
        attractor.z += sin(time * randomFloat(0.3f, 1.0f)) * chaosLevel * 0.03f;
    }
    
    // Render fractals to the color grid
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Map pixel to fractal space
            float fx = (x - width * 0.5f) / (width * 0.5f) * zoomLevel + center.x;
            float fy = (y - height * 0.5f) / (height * 0.5f) * zoomLevel + center.y;
            
            // Apply warp distortion
            float warpX = fx + sin(time * 2.0f + fy * 3.0f) * warpIntensity * 0.3f;
            float warpY = fy + cos(time * 1.5f + fx * 2.0f) * warpIntensity * 0.3f;
            
            // Compute fractal value
            float fractalValue = 0;
            switch (fractalType % 9) {
                case 0: fractalValue = computeMandelbrot(warpX, warpY); break;
                case 1: fractalValue = computeJulia(warpX, warpY, sin(time * 0.5f), cos(time * 0.7f)); break;
                case 2: fractalValue = computeBurningShip(warpX, warpY); break;
                case 3: fractalValue = computeTricorn(warpX, warpY); break;
                case 4: fractalValue = computePhoenix(warpX, warpY); break;
                case 5: fractalValue = computeNova(warpX, warpY); break;
                case 6: fractalValue = computePsychedelicWaves(warpX, warpY); break;
                case 7: fractalValue = computeStrangeAttractor(warpX, warpY); break;
                case 8: fractalValue = computeChaosFractal(warpX, warpY); break;
            }
            
            // Add attractor influences
            for (const auto& attractor : attractors) {
                float dx = fx - attractor.x;
                float dy = fy - attractor.y;
                float distance = sqrt(dx * dx + dy * dy) + 0.001f;
                float influence = (1.0f / distance) * 0.05f * chaosLevel;
                fractalValue += influence * sin(time * 3.0f + distance * 10.0f);
            }
            
            // Clamp value
            fractalValue = std::max(0.0f, std::min(2.0f, fractalValue));
            
            // Generate psychedelic colors
            float hue = fmod(fractalValue * 180.0f + colorShift + fx * 50.0f + fy * 30.0f, 360.0f);
            float saturation = 0.7f + sin(time * 2.0f + fractalValue * 5.0f) * 0.3f;
            float brightness = std::min(1.0f, fractalValue * (0.5f + sin(time * 3.0f) * 0.3f));
            
            // Enhanced colors when tripping
            if (isTripping) {
                hue += sin(time * 8.0f + x * 0.1f) * 60.0f;
                saturation = 1.0f;
                brightness *= (0.7f + sin(time * 12.0f + y * 0.2f) * 0.3f);
            }
            
            colorGrid[y][x] = hsvToRgb(hue, saturation, brightness);
        }
    }
    
    // Occasionally randomize parameters
    if (randomFloat(0, 1) < 0.015f) {
        fractalType = randomInt(0, 9);
        zoomLevel = randomFloat(0.1f, 5.0f);
        center = Vec3(randomFloat(-3, 3), randomFloat(-3, 3), randomFloat(-1, 1));
        warpIntensity = randomFloat(0.5f, 10.0f);
        
        // Add new attractors
        if (randomFloat(0, 1) < 0.5f) {
            attractors.push_back(Vec3(randomFloat(-2, 2), randomFloat(-2, 2), randomFloat(-1, 1)));
            if (attractors.size() > 6) {
                attractors.erase(attractors.begin());
            }
        }
    }
}

void FractalSystem::render(PixelBuffer& pixelBuffer) {
    int bufferWidth = pixelBuffer.getWidth();
    int bufferHeight = pixelBuffer.getHeight();
    
    for (int y = 0; y < bufferHeight && y < (int)colorGrid.size(); y++) {
        for (int x = 0; x < bufferWidth && x < (int)colorGrid[y].size(); x++) {
            pixelBuffer.setPixel(x, y, colorGrid[y][x]);
        }
    }
}

void FractalSystem::resize(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;
    colorGrid.resize(height, std::vector<uint32_t>(width, 0xFF000000));
    initialize();
}

std::string FractalSystem::getCurrentModeName() const {
    switch (fractalType % 9) {
        case 0: return "Mandelbrot Set";
        case 1: return "Julia Set";
        case 2: return "Burning Ship";
        case 3: return "Tricorn";
        case 4: return "Phoenix";
        case 5: return "Nova Fractal";
        case 6: return "Psychedelic Waves";
        case 7: return "Strange Attractor";
        case 8: return "Chaos Fractal";
        default: return "Fractal";
    }
}

void FractalSystem::generateFractalLevel(std::vector<Triangle3D>&, Vec3, float, int, int) const {
    // Optional: Can be used for 3D fractal generation
    // Currently unused but kept for potential future use
}