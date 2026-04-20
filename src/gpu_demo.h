#pragma once

#include "utils.h"
#include "pixelbuffer.h"
#include <vector>
#include <string>

// Demonstrates why GPUs are necessary by doing intensive rendering on CPU
class GPUDemoSystem {
public:
    enum DemoMode {
        SINGLE_TRIANGLE,      // 1 triangle - trivial
        FEW_TRIANGLES,        // ~100 triangles - easy
        MANY_TRIANGLES,       // ~1,000 triangles - getting slow
        LOTS_OF_TRIANGLES,    // ~10,000 triangles - very slow
        EXTREME_TRIANGLES,    // ~100,000 triangles - painfully slow
        FULL_SCREEN_PIXELS    // Every pixel computed - shows pixel fill rate
    };
    
private:
    int width, height;
    DemoMode currentMode;
    float time;
    std::vector<Triangle3D> triangles;
    
    // Performance metrics
    float lastFrameTime;
    float avgFrameTime;
    int frameCount;
    int triangleCount;
    int pixelsRendered;
    
    // Animation
    float rotationY;
    float rotationX;
    
public:
    GPUDemoSystem(int w, int h);
    
    void setMode(DemoMode mode);
    void update(float deltaTime);
    void render(PixelBuffer& pixelBuffer);
    void resize(int newWidth, int newHeight);
    
    // Getters for performance stats
    DemoMode getCurrentMode() const { return currentMode; }
    std::string getModeName() const;
    std::string getModeDescription() const;
    int getTriangleCount() const { return triangleCount; }
    int getPixelsRendered() const { return pixelsRendered; }
    float getLastFrameTime() const { return lastFrameTime; }
    float getAvgFrameTime() const { return avgFrameTime; }
    float getFPS() const { return avgFrameTime > 0 ? 1000.0f / avgFrameTime : 0; }
    
private:
    void generateTriangles();
    void generateSphere(Vec3 center, float radius, int segments, uint32_t color);
    void generateCube(Vec3 center, float size, uint32_t color);
    void generateGrid(int gridSize);
};
