#pragma once

#include <vector>
#include <cstdint>
#include "utils.h"

class PixelBuffer {
private:
    std::vector<uint32_t> pixels;
    int width, height;
    
    // Color utility functions for vertex color interpolation
    struct Color {
        float r, g, b, a;
        
        Color(uint32_t argb = 0xFF000000);
        Color(float red, float green, float blue, float alpha = 1.0f);
        
        uint32_t toARGB() const;
        Color operator+(const Color& other) const;
        Color operator*(float scalar) const;
    };

public:
    PixelBuffer(int w, int h);
    
    void clear(uint32_t color = 0xFF000000);
    void setPixel(int x, int y, uint32_t color);
    uint32_t getPixel(int x, int y) const;
    
    const uint32_t* getData() const;
    int getWidth() const;
    int getHeight() const;
    
    // Basic drawing functions
    void drawLine(int x0, int y0, int x1, int y1, uint32_t color);
    void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
    void fillTriangleBarycentric(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
    void drawTriangleWireframe(int x0, int y0, int x1, int y1, int x2, int y2, 
                              uint32_t wireframe_color, uint32_t fill_color, bool filled = true);
    void fillRectangle(int x, int y, int w, int h, uint32_t color);
    void drawRectangle(int x, int y, int w, int h, uint32_t color);
    void drawCircle(int cx, int cy, int radius, uint32_t color);
    void fillCircle(int cx, int cy, int radius, uint32_t color);
    
    // Advanced triangle rendering with gradients
    void fillTriangleGradient(int x0, int y0, uint32_t color0,
                             int x1, int y1, uint32_t color1,
                             int x2, int y2, uint32_t color2);
    void fillTriangleGradientScanline(int x0, int y0, uint32_t color0,
                                     int x1, int y1, uint32_t color1,
                                     int x2, int y2, uint32_t color2);
    void fillTriangleRainbow(int x0, int y0, int x1, int y1, int x2, int y2);
    
    // 3D rendering functions
    std::pair<int, int> project3DTo2D(const Vec3& point, int screenWidth, int screenHeight);
    
    // Render a 3D triangle with proper screen space coordinates and depth buffer
    void render3DTriangle(const Triangle3D& tri, int screenWidth, int screenHeight) {
        // Extract vertices
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
        int maxX = std::min(screenWidth - 1, std::max({x0, x1, x2}));
        int minY = std::max(0, std::min({y0, y1, y2}));
        int maxY = std::min(screenHeight - 1, std::max({y0, y1, y2}));
        
        // Calculate area for barycentric coordinates
        float area = (float)((x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0));
        
        // Skip degenerate or backfacing triangles
        if (std::abs(area) < 0.001f) return;
        
        // Rasterize using barycentric coordinates
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                // Calculate barycentric coordinates
                float w0 = ((x1 - x0) * (y - y0) - (y1 - y0) * (x - x0)) / area;
                float w1 = ((x2 - x1) * (y - y1) - (y2 - y1) * (x - x1)) / area;
                float w2 = 1.0f - w0 - w1;
                
                // Check if point is inside triangle
                if (w0 >= -0.001f && w1 >= -0.001f && w2 >= -0.001f) {
                    // Interpolate color
                    uint32_t c0 = tri.colors[0];
                    uint32_t c1 = tri.colors[1];
                    uint32_t c2 = tri.colors[2];
                    
                    uint32_t r = (uint32_t)(((c0 >> 16) & 0xFF) * w2 + ((c1 >> 16) & 0xFF) * w0 + ((c2 >> 16) & 0xFF) * w1);
                    uint32_t g = (uint32_t)(((c0 >> 8) & 0xFF) * w2 + ((c1 >> 8) & 0xFF) * w0 + ((c2 >> 8) & 0xFF) * w1);
                    uint32_t b = (uint32_t)((c0 & 0xFF) * w2 + (c1 & 0xFF) * w0 + (c2 & 0xFF) * w1);
                    
                    setPixel(x, y, 0xFF000000 | (r << 16) | (g << 8) | b);
                }
            }
        }
    }
};
