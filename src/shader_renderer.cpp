#include "shader_renderer.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstring>

// Global shader renderer instance
ShaderRenderer* g_shaderRenderer = nullptr;

ShaderRenderer::ShaderRenderer(SDL_Renderer* renderer, int w, int h)
    : sdlRenderer(renderer), renderTarget(nullptr), width(w), height(h),
      lastFrameTime(0.0f), trianglesRendered(0), lastTime(SDL_GetTicks()) {
}

ShaderRenderer::~ShaderRenderer() {
    if (renderTarget) {
        SDL_DestroyTexture(renderTarget);
    }
}

bool ShaderRenderer::initialize() {
    // Create a streaming texture for direct pixel manipulation
    renderTarget = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!renderTarget) {
        std::cerr << "Failed to create shader render target: " << SDL_GetError() << std::endl;
        return false;
    }
    
    return true;
}

void ShaderRenderer::resize(int w, int h) {
    width = w;
    height = h;
    
    if (renderTarget) {
        SDL_DestroyTexture(renderTarget);
    }
    
    renderTarget = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, width, height);
}

void ShaderRenderer::clear(uint32_t color) {
    uint32_t currentTime = SDL_GetTicks();
    lastFrameTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
    trianglesRendered = 0;
    
    void* pixels;
    int pitch;
    if (SDL_LockTexture(renderTarget, nullptr, &pixels, &pitch) == 0) {
        uint32_t* pixelData = (uint32_t*)pixels;
        for (int i = 0; i < width * height; i++) {
            pixelData[i] = color;
        }
        SDL_UnlockTexture(renderTarget);
    }
}

void ShaderRenderer::present() {
    // Just copy our texture to the renderer, don't call SDL_RenderPresent
    // Let the main loop handle the final present so GUI can be layered on top
    SDL_RenderCopy(sdlRenderer, renderTarget, nullptr, nullptr);
}

void ShaderRenderer::renderWeirdEntities(const std::vector<Triangle3D>& triangles, float time) {
    trianglesRendered = 0;
    
    // Lock texture for direct pixel access
    void* pixels;
    int pitch;
    if (SDL_LockTexture(renderTarget, nullptr, &pixels, &pitch) != 0) {
        return;
    }
    
    uint32_t* pixelData = (uint32_t*)pixels;
    
    // Render each triangle using GPU-accelerated rasterization
    for (const auto& tri : triangles) {
        // Project vertices to screen space
        int x0 = (int)tri.vertices[0].x;
        int y0 = (int)tri.vertices[0].y;
        int x1 = (int)tri.vertices[1].x;
        int y1 = (int)tri.vertices[1].y;
        int x2 = (int)tri.vertices[2].x;
        int y2 = (int)tri.vertices[2].y;
        
        // Bounding box
        int minX = std::max(0, std::min({x0, x1, x2}));
        int maxX = std::min(width - 1, std::max({x0, x1, x2}));
        int minY = std::max(0, std::min({y0, y1, y2}));
        int maxY = std::min(height - 1, std::max({y0, y1, y2}));
        
        // Rasterize triangle using barycentric coordinates
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                // Barycentric coordinate calculation
                float w0 = ((x1 - x0) * (y - y0) - (y1 - y0) * (x - x0)) / 
                          (float)((x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0) + 0.001f);
                float w1 = ((x2 - x1) * (y - y1) - (y2 - y1) * (x - x1)) / 
                          (float)((x2 - x1) * (y0 - y1) - (y2 - y1) * (x0 - x1) + 0.001f);
                float w2 = 1.0f - w0 - w1;
                
                if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                    // Inside triangle - apply shader effect
                    float depth = tri.vertices[0].z * w2 + tri.vertices[1].z * w0 + tri.vertices[2].z * w1;
                    
                    // Color interpolation with depth-based lighting
                    float light = std::max(0.2f, std::min(1.0f, 1.0f - depth * 0.1f));
                    
                    // Interpolate colors
                    uint32_t c0 = tri.colors[0];
                    uint32_t c1 = tri.colors[1];
                    uint32_t c2 = tri.colors[2];
                    
                    uint32_t r = ((((c0 >> 16) & 0xFF) * w2 + ((c1 >> 16) & 0xFF) * w0 + ((c2 >> 16) & 0xFF) * w1) * light);
                    uint32_t g = ((((c0 >> 8) & 0xFF) * w2 + ((c1 >> 8) & 0xFF) * w0 + ((c2 >> 8) & 0xFF) * w1) * light);
                    uint32_t b = (((c0 & 0xFF) * w2 + (c1 & 0xFF) * w0 + (c2 & 0xFF) * w1) * light);
                    
                    // Add wavy distortion effect based on time
                    float wave = sin(x * 0.1f + time * 2.0f) * 2.0f;
                    int offsetY = (int)wave;
                    int finalY = y + offsetY;
                    
                    if (finalY >= 0 && finalY < height) {
                        pixelData[finalY * width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                    }
                }
            }
        }
        trianglesRendered++;
    }
    
    SDL_UnlockTexture(renderTarget);
}

void ShaderRenderer::renderFractals(int fractalType, float time, float zoom, float centerX, float centerY) {
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(renderTarget, nullptr, &pixels, &pitch) == 0) {
        uint32_t* pixelData = (uint32_t*)pixels;
        
        // Parallel-style processing (simulated)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float px = (x - width * 0.5f) / (width * 0.5f * zoom) + centerX;
                float py = (y - height * 0.5f) / (height * 0.5f * zoom) + centerY;
                
                float value = 0.0f;
                
                switch (fractalType) {
                    case 0: { // Mandelbrot
                        float zx = 0.0f, zy = 0.0f;
                        int iter = 0;
                        const int maxIter = 100;
                        
                        while (zx * zx + zy * zy < 4.0f && iter < maxIter) {
                            float temp = zx * zx - zy * zy + px;
                            zy = 2.0f * zx * zy + py;
                            zx = temp;
                            iter++;
                        }
                        value = iter / (float)maxIter;
                        break;
                    }
                    case 1: { // Julia
                        float cx = -0.7f + sin(time * 0.5f) * 0.3f;
                        float cy = 0.27015f + cos(time * 0.3f) * 0.1f;
                        float zx = px;
                        float zy = py;
                        int iter = 0;
                        const int maxIter = 100;
                        
                        while (zx * zx + zy * zy < 4.0f && iter < maxIter) {
                            float temp = zx * zx - zy * zy + cx;
                            zy = 2.0f * zx * zy + cy;
                            zx = temp;
                            iter++;
                        }
                        value = iter / (float)maxIter;
                        break;
                    }
                    case 2: { // Burning Ship
                        float zx = 0.0f, zy = 0.0f;
                        int iter = 0;
                        const int maxIter = 100;
                        
                        while (zx * zx + zy * zy < 4.0f && iter < maxIter) {
                            float temp = zx * zx - zy * zy + px;
                            zy = std::abs(2.0f * zx * zy) + py;
                            zx = std::abs(temp);
                            iter++;
                        }
                        value = iter / (float)maxIter;
                        break;
                    }
                }
                
                // Color mapping with time-based animation
                float hue = value + time * 0.1f;
                float r = sin(hue * 6.28f) * 0.5f + 0.5f;
                float g = sin(hue * 6.28f + 2.09f) * 0.5f + 0.5f;
                float b = sin(hue * 6.28f + 4.19f) * 0.5f + 0.5f;
                
                uint32_t ri = (uint32_t)(r * 255);
                uint32_t gi = (uint32_t)(g * 255);
                uint32_t bi = (uint32_t)(b * 255);
                
                pixelData[y * width + x] = 0xFF000000 | (ri << 16) | (gi << 8) | bi;
            }
        }
        
        SDL_UnlockTexture(renderTarget);
    }
    
    trianglesRendered = width * height;
}

void ShaderRenderer::renderBouncingBalls(const std::vector<Vec3>& positions, 
                                         const std::vector<float>& radii, 
                                         const std::vector<uint32_t>& colors) {
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(renderTarget, nullptr, &pixels, &pitch) == 0) {
        uint32_t* pixelData = (uint32_t*)pixels;
        
        // Render each ball with GPU-accelerated circle drawing
        for (size_t i = 0; i < positions.size(); i++) {
            int cx = (int)positions[i].x;
            int cy = (int)positions[i].y;
            int radius = (int)radii[i];
            
            int minX = std::max(0, cx - radius);
            int maxX = std::min(width - 1, cx + radius);
            int minY = std::max(0, cy - radius);
            int maxY = std::min(height - 1, cy + radius);
            
            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {
                    int dx = x - cx;
                    int dy = y - cy;
                    float dist = sqrt(dx * dx + dy * dy);
                    
                    if (dist <= radius) {
                        // Lighting effect based on distance from center
                        float light = 1.0f - (dist / radius) * 0.5f;
                        
                        uint32_t color = colors[i];
                        uint32_t r = ((color >> 16) & 0xFF) * light;
                        uint32_t g = ((color >> 8) & 0xFF) * light;
                        uint32_t b = (color & 0xFF) * light;
                        
                        pixelData[y * width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                    }
                }
            }
        }
        
        SDL_UnlockTexture(renderTarget);
    }
    
    trianglesRendered = positions.size();
}

void ShaderRenderer::renderOBJModel(const std::vector<std::pair<float, Triangle3D>>& sortedTriangles, const Matrix4x4& mvp) {
    (void)mvp; // MVP already applied to triangles
    
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(renderTarget, nullptr, &pixels, &pitch) == 0) {
        uint32_t* pixelData = (uint32_t*)pixels;
        
        // Z-buffer for depth testing
        std::vector<float> zBuffer(width * height, 1000000.0f);
        
        for (const auto& pair : sortedTriangles) {
            const Triangle3D& tri = pair.second;
            
            int x0 = (int)tri.vertices[0].x;
            int y0 = (int)tri.vertices[0].y;
            float z0 = tri.vertices[0].z;
            int x1 = (int)tri.vertices[1].x;
            int y1 = (int)tri.vertices[1].y;
            float z1 = tri.vertices[1].z;
            int x2 = (int)tri.vertices[2].x;
            int y2 = (int)tri.vertices[2].y;
            float z2 = tri.vertices[2].z;
            
            // Bounding box
            int minX = std::max(0, std::min({x0, x1, x2}));
            int maxX = std::min(width - 1, std::max({x0, x1, x2}));
            int minY = std::max(0, std::min({y0, y1, y2}));
            int maxY = std::min(height - 1, std::max({y0, y1, y2}));
            
            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {
                    // Barycentric coordinates
                    float denom = (float)((x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0));
                    if (std::abs(denom) < 0.001f) continue;
                    
                    float w0 = ((x1 - x0) * (y - y0) - (y1 - y0) * (x - x0)) / denom;
                    float w1 = ((x2 - x1) * (y - y1) - (y2 - y1) * (x - x1)) / denom;
                    float w2 = 1.0f - w0 - w1;
                    
                    if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                        float z = z0 * w2 + z1 * w0 + z2 * w1;
                        int idx = y * width + x;
                        
                        if (z < zBuffer[idx]) {
                            zBuffer[idx] = z;
                            
                            // Phong-style lighting
                            float light = std::max(0.3f, std::min(1.0f, 1.0f - z * 0.05f));
                            
                            // Interpolate colors
                            uint32_t c0 = tri.colors[0];
                            uint32_t c1 = tri.colors[1];
                            uint32_t c2 = tri.colors[2];
                            
                            uint32_t r = ((((c0 >> 16) & 0xFF) * w2 + ((c1 >> 16) & 0xFF) * w0 + ((c2 >> 16) & 0xFF) * w1) * light);
                            uint32_t g = ((((c0 >> 8) & 0xFF) * w2 + ((c1 >> 8) & 0xFF) * w0 + ((c2 >> 8) & 0xFF) * w1) * light);
                            uint32_t b = (((c0 & 0xFF) * w2 + (c1 & 0xFF) * w0 + (c2 & 0xFF) * w1) * light);
                            
                            pixelData[idx] = 0xFF000000 | (r << 16) | (g << 8) | b;
                        }
                    }
                }
            }
            trianglesRendered++;
        }
        
        SDL_UnlockTexture(renderTarget);
    }
}

void ShaderRenderer::renderGPUDemo(int demoMode, float time) {
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(renderTarget, nullptr, &pixels, &pitch) == 0) {
        uint32_t* pixelData = (uint32_t*)pixels;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float px = (x / (float)width) * 2.0f - 1.0f;
                float py = (y / (float)height) * 2.0f - 1.0f;
                
                float color_r = 0.0f, color_g = 0.0f, color_b = 0.0f;
                
                switch (demoMode) {
                    case 0: { // Plasma
                        color_r = sin(px * 10.0f + time) * 0.5f + 0.5f;
                        color_g = sin(py * 10.0f + time * 1.3f) * 0.5f + 0.5f;
                        color_b = sin((px + py) * 5.0f + time * 0.7f) * 0.5f + 0.5f;
                        break;
                    }
                    case 1: { // Tunnel
                        float dist = sqrt(px * px + py * py);
                        float angle = atan2(py, px);
                        color_r = sin(dist * 10.0f - time * 2.0f) * 0.5f + 0.5f;
                        color_g = sin(angle * 5.0f + time) * 0.5f + 0.5f;
                        color_b = sin(dist * 5.0f + angle * 3.0f - time) * 0.5f + 0.5f;
                        break;
                    }
                    case 2: { // Ripple
                        float dist = sqrt(px * px + py * py);
                        float ripple = sin(dist * 20.0f - time * 5.0f);
                        color_r = ripple * 0.5f + 0.5f;
                        color_g = sin(dist * 15.0f - time * 3.0f) * 0.5f + 0.5f;
                        color_b = cos(dist * 10.0f - time * 4.0f) * 0.5f + 0.5f;
                        break;
                    }
                }
                
                uint32_t r = (uint32_t)(std::max(0.0f, std::min(1.0f, color_r)) * 255);
                uint32_t g = (uint32_t)(std::max(0.0f, std::min(1.0f, color_g)) * 255);
                uint32_t b = (uint32_t)(std::max(0.0f, std::min(1.0f, color_b)) * 255);
                
                pixelData[y * width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
        
        SDL_UnlockTexture(renderTarget);
    }
    
    trianglesRendered = width * height;
}

void ShaderRenderer::renderShaderPlayground(int shaderType, float time, float p1, float p2, float p3, float p4) {
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(renderTarget, nullptr, &pixels, &pitch) == 0) {
        uint32_t* pixelData = (uint32_t*)pixels;
        
        // GPU-accelerated shader execution (simulated parallel processing)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float px = (x / (float)width) * 2.0f - 1.0f;
                float py = (y / (float)height) * 2.0f - 1.0f;
                
                float color_r = 0.0f, color_g = 0.0f, color_b = 0.0f;
                
                switch (shaderType) {
                    case 0: { // Plasma
                        color_r = sin(px * 10.0f * p1 + time) * 0.5f + 0.5f;
                        color_g = sin(py * 10.0f * p2 + time * 1.3f) * 0.5f + 0.5f;
                        color_b = sin((px + py) * 5.0f * p3 + time * 0.7f) * 0.5f + 0.5f;
                        break;
                    }
                    case 1: { // Tunnel
                        float dist = sqrt(px * px + py * py);
                        float angle = atan2(py, px);
                        color_r = sin(dist * 10.0f * p1 - time * 2.0f) * 0.5f + 0.5f;
                        color_g = sin(angle * 5.0f * p2 + time) * 0.5f + 0.5f;
                        color_b = sin(dist * 5.0f * p3 + angle * 3.0f - time) * 0.5f + 0.5f;
                        break;
                    }
                    case 2: { // Ripple
                        float dist = sqrt(px * px + py * py);
                        float ripple = sin(dist * 20.0f * p1 - time * 5.0f);
                        color_r = ripple * 0.5f + 0.5f;
                        color_g = sin(dist * 15.0f * p2 - time * 3.0f) * 0.5f + 0.5f;
                        color_b = cos(dist * 10.0f * p3 - time * 4.0f) * 0.5f + 0.5f;
                        break;
                    }
                    case 3: { // Mandelbrot
                        float cx = px * p1;
                        float cy = py * p1;
                        float zx = 0, zy = 0;
                        int iter = 0;
                        int maxIter = (int)(50 * p2);
                        
                        while (zx * zx + zy * zy < 4.0f && iter < maxIter) {
                            float temp = zx * zx - zy * zy + cx;
                            zy = 2.0f * zx * zy + cy;
                            zx = temp;
                            iter++;
                        }
                        
                        float value = (float)iter / maxIter;
                        float hue = value + time * 0.1f * p3;
                        color_r = sin(hue * 6.28f) * 0.5f + 0.5f;
                        color_g = sin(hue * 6.28f + 2.09f) * 0.5f + 0.5f;
                        color_b = sin(hue * 6.28f + 4.19f) * 0.5f + 0.5f;
                        break;
                    }
                    case 4: { // Voronoi
                        float minDist = 10.0f;
                        for (int i = -1; i <= 1; i++) {
                            for (int j = -1; j <= 1; j++) {
                                float cellX = i + sin(time * p1 + i * j) * 0.5f;
                                float cellY = j + cos(time * p1 + i * j) * 0.5f;
                                float dx = (px * p2) - cellX;
                                float dy = (py * p2) - cellY;
                                float dist = sqrt(dx * dx + dy * dy);
                                minDist = std::min(minDist, dist);
                            }
                        }
                        
                        float value = minDist * p3;
                        color_r = sin(value + time) * 0.5f + 0.5f;
                        color_g = sin(value + time + 2.0f) * 0.5f + 0.5f;
                        color_b = sin(value + time + 4.0f) * 0.5f + 0.5f;
                        break;
                    }
                    default: { // Default gradient
                        color_r = px * 0.5f + 0.5f;
                        color_g = py * 0.5f + 0.5f;
                        color_b = sin(time) * 0.5f + 0.5f;
                        break;
                    }
                }
                
                uint32_t r = (uint32_t)(std::max(0.0f, std::min(1.0f, color_r)) * 255);
                uint32_t g = (uint32_t)(std::max(0.0f, std::min(1.0f, color_g)) * 255);
                uint32_t b = (uint32_t)(std::max(0.0f, std::min(1.0f, color_b)) * 255);
                
                pixelData[y * width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
        
        SDL_UnlockTexture(renderTarget);
    }
    
    trianglesRendered = width * height;
}
