#pragma once

#include <SDL.h>
#include <string>
#include <vector>
#include <memory>
#include "utils.h"

// Shader-based GPU renderer for comparison with CPU rendering
class ShaderRenderer {
public:
    ShaderRenderer(SDL_Renderer* renderer, int width, int height);
    ~ShaderRenderer();
    
    bool initialize();
    void resize(int width, int height);
    
    // Render different modes using GPU shaders
    void renderWeirdEntities(const std::vector<Triangle3D>& triangles, float time);
    void renderFractals(int fractalType, float time, float zoom, float centerX, float centerY);
    void renderBouncingBalls(const std::vector<Vec3>& positions, const std::vector<float>& radii, const std::vector<uint32_t>& colors);
    void renderOBJModel(const std::vector<std::pair<float, Triangle3D>>& sortedTriangles, const Matrix4x4& mvp);
    void renderGPUDemo(int demoMode, float time);
    
    // Render custom shader playground effects
    void renderShaderPlayground(int shaderType, float time, float p1, float p2, float p3, float p4);
    
    // Performance metrics
    float getLastFrameTime() const { return lastFrameTime; }
    int getTrianglesRendered() const { return trianglesRendered; }
    
    void clear(uint32_t color = 0xFF000000);
    void present();

private:
    SDL_Renderer* sdlRenderer;
    SDL_Texture* renderTarget;
    int width;
    int height;
    
    float lastFrameTime;
    int trianglesRendered;
    uint32_t lastTime;
    
    // Helper functions for GPU-accelerated rendering
    void drawTriangleGPU(const Triangle3D& tri);
    void drawFilledTriangle(const Vec3& v0, const Vec3& v1, const Vec3& v2, uint32_t color);
    void setPixel(int x, int y, uint32_t color);
    
    // Shader simulation functions (GPU-accelerated via SDL)
    void fractalShader(int x, int y, int fractalType, float time, float zoom, float centerX, float centerY, uint32_t* pixels);
    void entityShader(int x, int y, float time, uint32_t* pixels);
};

// Global shader renderer instance
extern ShaderRenderer* g_shaderRenderer;
