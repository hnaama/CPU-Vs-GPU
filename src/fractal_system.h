#pragma once

#include <vector>
#include <string>
#include "utils.h"

// Forward declaration
class PixelBuffer;

// FRACTAL SYSTEM
class FractalSystem {
private:
    int width, height;
    std::vector<std::vector<uint32_t>> colorGrid;
    float time;
    int fractalType;
    float zoomLevel;
    Vec3 center;
    float warpIntensity;
    float colorShift;
    float pulseSpeed;
    float chaosLevel;
    bool isTripping;
    std::vector<Vec3> attractors;
    
public:
    FractalSystem(int w, int h);
    
    void initialize();
    void update(float deltaTime);
    void render(PixelBuffer& pixelBuffer);
    void resize(int newWidth, int newHeight);
    
    std::string getCurrentModeName() const;
    
private:
    void generateFractalLevel(std::vector<Triangle3D>& triangles, Vec3 center, float scale, int level, int maxLevel) const;
};

// Global fractal system instance
extern FractalSystem* g_fractalSystem;