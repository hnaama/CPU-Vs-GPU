#include "gpu_demo.h"
#include <cmath>
#include <chrono>

GPUDemoSystem::GPUDemoSystem(int w, int h) 
    : width(w), height(h), currentMode(SINGLE_TRIANGLE), time(0),
      lastFrameTime(0), avgFrameTime(0), frameCount(0),
      triangleCount(0), pixelsRendered(0),
      rotationY(0), rotationX(0) {
    generateTriangles();
}

void GPUDemoSystem::setMode(DemoMode mode) {
    currentMode = mode;
    frameCount = 0;
    avgFrameTime = 0;
    generateTriangles();
}

void GPUDemoSystem::update(float deltaTime) {
    time += deltaTime;
    
    // Smooth rotation
    rotationY += deltaTime * 0.5f;
    rotationX = sin(time * 0.3f) * 0.3f;
}

void GPUDemoSystem::render(PixelBuffer& pixelBuffer) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Clear to dark background
    pixelBuffer.clear(0xFF0a0a1a);
    
    pixelsRendered = 0;
    
    if (currentMode == FULL_SCREEN_PIXELS) {
        // Render every single pixel individually with computation
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // Simulate expensive per-pixel computation
                float fx = (x - width * 0.5f) / width;
                float fy = (y - height * 0.5f) / height;
                
                // Multiple operations per pixel
                float dist = sqrt(fx * fx + fy * fy);
                float angle = atan2(fy, fx);
                float wave1 = sin(dist * 10.0f - time * 2.0f);
                float wave2 = cos(angle * 8.0f + time);
                float wave3 = sin(fx * 15.0f + fy * 15.0f + time * 3.0f);
                
                float value = (wave1 + wave2 + wave3) / 3.0f;
                value = (value + 1.0f) * 0.5f; // Normalize to 0-1
                
                // Color calculation
                uint8_t r = (uint8_t)(sin(value * 3.14159f + time) * 127 + 128);
                uint8_t g = (uint8_t)(cos(value * 3.14159f + time * 1.3f) * 127 + 128);
                uint8_t b = (uint8_t)(sin(value * 6.28f + time * 0.7f) * 127 + 128);
                
                uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;
                pixelBuffer.setPixel(x, y, color);
                pixelsRendered++;
            }
        }
    } else {
        // Render triangles
        float fov = 60.0f * M_PI / 180.0f;
        float aspect = (float)width / height;
        
        // Set up transformation matrices
        Matrix4x4 rotX = Matrix4x4::rotationX(rotationX);
        Matrix4x4 rotY = Matrix4x4::rotationY(rotationY);
        Matrix4x4 model = rotX * rotY;
        
        Matrix4x4 view = Matrix4x4::translation(0, 0, -5.0f);
        Matrix4x4 projection = Matrix4x4::perspective(fov, aspect, 0.1f, 100.0f);
        Matrix4x4 mvp = projection * view * model;
        
        // Sort triangles by depth (painter's algorithm)
        std::vector<std::pair<float, Triangle3D>> sortedTriangles;
        
        for (const auto& tri : triangles) {
            Triangle3D transformed = tri.transform(mvp);
            
            // Calculate average Z for sorting
            float avgZ = (transformed.vertices[0].z + 
                         transformed.vertices[1].z + 
                         transformed.vertices[2].z) / 3.0f;
            
            // Skip triangles outside view frustum
            bool allBehind = transformed.vertices[0].z < -1.0f && 
                            transformed.vertices[1].z < -1.0f && 
                            transformed.vertices[2].z < -1.0f;
            bool allInFront = transformed.vertices[0].z > 1.0f && 
                             transformed.vertices[1].z > 1.0f && 
                             transformed.vertices[2].z > 1.0f;
            
            if (!allBehind && !allInFront) {
                sortedTriangles.push_back({avgZ, transformed});
            }
        }
        
        // Sort back to front
        std::sort(sortedTriangles.begin(), sortedTriangles.end(),
            [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Render all triangles
        for (const auto& pair : sortedTriangles) {
            pixelBuffer.render3DTriangle(pair.second, width, height);
            pixelsRendered += 50; // Approximate pixels per triangle
        }
    }
    
    // Calculate frame time
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    lastFrameTime = duration.count() / 1000.0f; // Convert to milliseconds
    
    // Calculate running average
    frameCount++;
    avgFrameTime = (avgFrameTime * (frameCount - 1) + lastFrameTime) / frameCount;
    if (frameCount > 100) {
        frameCount = 50; // Reset to prevent overflow and keep recent average
        avgFrameTime = lastFrameTime;
    }
}

void GPUDemoSystem::resize(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;
}

std::string GPUDemoSystem::getModeName() const {
    switch (currentMode) {
        case SINGLE_TRIANGLE: return "1 Triangle (Trivial)";
        case FEW_TRIANGLES: return "~100 Triangles (Easy)";
        case MANY_TRIANGLES: return "~1,000 Triangles (Slow)";
        case LOTS_OF_TRIANGLES: return "~10,000 Triangles (Very Slow)";
        case EXTREME_TRIANGLES: return "~100,000 Triangles (Extreme!)";
        case FULL_SCREEN_PIXELS: return "Every Pixel Computed";
        default: return "Unknown";
    }
}

std::string GPUDemoSystem::getModeDescription() const {
    switch (currentMode) {
        case SINGLE_TRIANGLE:
            return "A single triangle. Your CPU can handle this easily.";
        case FEW_TRIANGLES:
            return "A hundred triangles forming a sphere. Still manageable for CPU.";
        case MANY_TRIANGLES:
            return "A thousand triangles. CPU is starting to struggle!";
        case LOTS_OF_TRIANGLES:
            return "Ten thousand triangles. Your CPU is working very hard now.";
        case EXTREME_TRIANGLES:
            return "One hundred thousand triangles! This is why we need GPUs!";
        case FULL_SCREEN_PIXELS:
            return "Computing EVERY pixel individually. Shows why parallel processing matters.";
        default:
            return "";
    }
}

void GPUDemoSystem::generateTriangles() {
    triangles.clear();
    
    switch (currentMode) {
        case SINGLE_TRIANGLE: {
            // Just one triangle
            Triangle3D tri(
                Vec3(0, 1, 0),
                Vec3(-1, -1, 0),
                Vec3(1, -1, 0),
                0xFF00FF00, 0xFF00FF00, 0xFF00FF00
            );
            triangles.push_back(tri);
            triangleCount = 1;
            break;
        }
        
        case FEW_TRIANGLES: {
            // One sphere with ~100 triangles
            generateSphere(Vec3(0, 0, 0), 1.5f, 8, 0xFF00AAFF);
            triangleCount = triangles.size();
            break;
        }
        
        case MANY_TRIANGLES: {
            // Multiple spheres totaling ~1,000 triangles
            generateSphere(Vec3(0, 0, 0), 1.5f, 16, 0xFF00AAFF);
            generateSphere(Vec3(2, 0, 0), 0.8f, 12, 0xFFFF0080);
            generateSphere(Vec3(-2, 0, 0), 0.8f, 12, 0xFF80FF00);
            generateSphere(Vec3(0, 2, 0), 0.8f, 12, 0xFFFFAA00);
            generateSphere(Vec3(0, -2, 0), 0.8f, 12, 0xFF00FFAA);
            triangleCount = triangles.size();
            break;
        }
        
        case LOTS_OF_TRIANGLES: {
            // Many small spheres totaling ~10,000 triangles
            for (int i = 0; i < 20; i++) {
                for (int j = 0; j < 20; j++) {
                    float x = (i - 10) * 0.5f;
                    float y = (j - 10) * 0.5f;
                    float z = sin(i * 0.5f) * cos(j * 0.5f) * 2.0f;
                    uint32_t color = 0xFF000000 | 
                                    ((i * 12) << 16) | 
                                    ((j * 12) << 8) | 
                                    ((i + j) * 6);
                    generateSphere(Vec3(x, y, z), 0.2f, 4, color);
                }
            }
            triangleCount = triangles.size();
            break;
        }
        
        case EXTREME_TRIANGLES: {
            // Massive grid of tiny triangles - ~100,000+
            generateGrid(150);
            triangleCount = triangles.size();
            break;
        }
        
        case FULL_SCREEN_PIXELS: {
            // No triangles, just pixel rendering
            triangleCount = 0;
            break;
        }
    }
}

void GPUDemoSystem::generateSphere(Vec3 center, float radius, int segments, uint32_t color) {
    // Generate sphere using latitude/longitude
    for (int lat = 0; lat < segments; lat++) {
        for (int lon = 0; lon < segments; lon++) {
            float theta1 = lat * M_PI / segments;
            float theta2 = (lat + 1) * M_PI / segments;
            float phi1 = lon * 2.0f * M_PI / segments;
            float phi2 = (lon + 1) * 2.0f * M_PI / segments;
            
            // Four vertices for this quad
            Vec3 v1(sin(theta1) * cos(phi1), cos(theta1), sin(theta1) * sin(phi1));
            Vec3 v2(sin(theta1) * cos(phi2), cos(theta1), sin(theta1) * sin(phi2));
            Vec3 v3(sin(theta2) * cos(phi2), cos(theta2), sin(theta2) * sin(phi2));
            Vec3 v4(sin(theta2) * cos(phi1), cos(theta2), sin(theta2) * sin(phi1));
            
            v1 = v1 * radius + center;
            v2 = v2 * radius + center;
            v3 = v3 * radius + center;
            v4 = v4 * radius + center;
            
            // Create two triangles for this quad
            Triangle3D tri1(v1, v2, v3, color, color, color);
            triangles.push_back(tri1);
            
            Triangle3D tri2(v1, v3, v4, color, color, color);
            triangles.push_back(tri2);
        }
    }
}

void GPUDemoSystem::generateCube(Vec3 center, float size, uint32_t color) {
    float h = size * 0.5f;
    
    // 8 vertices of cube
    Vec3 vertices[8] = {
        Vec3(-h, -h, -h), Vec3(h, -h, -h), Vec3(h, h, -h), Vec3(-h, h, -h),
        Vec3(-h, -h, h), Vec3(h, -h, h), Vec3(h, h, h), Vec3(-h, h, h)
    };
    
    for (int i = 0; i < 8; i++) {
        vertices[i] = vertices[i] + center;
    }
    
    // 12 triangles (2 per face)
    int faces[12][3] = {
        {0,1,2}, {0,2,3}, // Front
        {4,6,5}, {4,7,6}, // Back
        {0,4,5}, {0,5,1}, // Bottom
        {2,6,7}, {2,7,3}, // Top
        {0,3,7}, {0,7,4}, // Left
        {1,5,6}, {1,6,2}  // Right
    };
    
    for (int i = 0; i < 12; i++) {
        Triangle3D tri(
            vertices[faces[i][0]],
            vertices[faces[i][1]],
            vertices[faces[i][2]],
            color, color, color
        );
        triangles.push_back(tri);
    }
}

void GPUDemoSystem::generateGrid(int gridSize) {
    // Generate a massive grid of triangles
    float spacing = 0.1f;
    float offset = gridSize * spacing * 0.5f;
    
    for (int y = 0; y < gridSize - 1; y++) {
        for (int x = 0; x < gridSize - 1; x++) {
            float x1 = x * spacing - offset;
            float x2 = (x + 1) * spacing - offset;
            float y1 = y * spacing - offset;
            float y2 = (y + 1) * spacing - offset;
            
            // Height variation for visual interest
            float z1 = sin(x * 0.3f) * cos(y * 0.3f) * 2.0f;
            float z2 = sin((x+1) * 0.3f) * cos(y * 0.3f) * 2.0f;
            float z3 = sin((x+1) * 0.3f) * cos((y+1) * 0.3f) * 2.0f;
            float z4 = sin(x * 0.3f) * cos((y+1) * 0.3f) * 2.0f;
            
            // Color based on position
            uint32_t color = 0xFF000000 | 
                           ((x * 255 / gridSize) << 16) | 
                           ((y * 255 / gridSize) << 8) | 
                           128;
            
            // Two triangles per quad
            Triangle3D tri1(
                Vec3(x1, y1, z1),
                Vec3(x2, y1, z2),
                Vec3(x2, y2, z3),
                color, color, color
            );
            triangles.push_back(tri1);
            
            Triangle3D tri2(
                Vec3(x1, y1, z1),
                Vec3(x2, y2, z3),
                Vec3(x1, y2, z4),
                color, color, color
            );
            triangles.push_back(tri2);
        }
    }
}
