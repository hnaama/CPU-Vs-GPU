#include "shader_playground.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>
#include <dirent.h>

ShaderPlayground::ShaderPlayground(int w, int h)
    : width(w), height(h), shaderLoaded(false) {
}

bool ShaderPlayground::loadShader(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        lastError = "Failed to open shader file: " + filepath;
        shaderLoaded = false;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    shaderCode = buffer.str();
    
    // Parse metadata from comments
    shaderName = "Custom Shader";
    shaderDescription = "User-defined shader effect";
    
    size_t namePos = shaderCode.find("// NAME:");
    if (namePos != std::string::npos) {
        size_t lineEnd = shaderCode.find('\n', namePos);
        shaderName = shaderCode.substr(namePos + 8, lineEnd - namePos - 8);
    }
    
    size_t descPos = shaderCode.find("// DESC:");
    if (descPos != std::string::npos) {
        size_t lineEnd = shaderCode.find('\n', descPos);
        shaderDescription = shaderCode.substr(descPos + 8, lineEnd - descPos - 8);
    }
    
    shaderLoaded = compileShader(shaderCode);
    return shaderLoaded;
}

bool ShaderPlayground::compileShader(const std::string& code) {
    // For simplicity, we support predefined shader patterns
    // In a real implementation, you'd parse and compile the shader
    
    // Check for different shader types based on keywords
    if (code.find("plasma") != std::string::npos || code.find("PLASMA") != std::string::npos) {
        compiledShader = [](float x, float y, float time, float p1, float p2, float p3, float p4) -> uint32_t {
            float r = sin(x * 10.0f * p1 + time) * 0.5f + 0.5f;
            float g = sin(y * 10.0f * p2 + time * 1.3f) * 0.5f + 0.5f;
            float b = sin((x + y) * 5.0f * p3 + time * 0.7f) * 0.5f + 0.5f;
            return 0xFF000000 | ((int)(r * 255) << 16) | ((int)(g * 255) << 8) | (int)(b * 255);
        };
    }
    else if (code.find("tunnel") != std::string::npos || code.find("TUNNEL") != std::string::npos) {
        compiledShader = [](float x, float y, float time, float p1, float p2, float p3, float p4) -> uint32_t {
            float dist = sqrt(x * x + y * y);
            float angle = atan2(y, x);
            float r = sin(dist * 10.0f * p1 - time * 2.0f) * 0.5f + 0.5f;
            float g = sin(angle * 5.0f * p2 + time) * 0.5f + 0.5f;
            float b = sin(dist * 5.0f * p3 + angle * 3.0f - time) * 0.5f + 0.5f;
            return 0xFF000000 | ((int)(r * 255) << 16) | ((int)(g * 255) << 8) | (int)(b * 255);
        };
    }
    else if (code.find("ripple") != std::string::npos || code.find("RIPPLE") != std::string::npos) {
        compiledShader = [](float x, float y, float time, float p1, float p2, float p3, float p4) -> uint32_t {
            float dist = sqrt(x * x + y * y);
            float ripple = sin(dist * 20.0f * p1 - time * 5.0f);
            float r = ripple * 0.5f + 0.5f;
            float g = sin(dist * 15.0f * p2 - time * 3.0f) * 0.5f + 0.5f;
            float b = cos(dist * 10.0f * p3 - time * 4.0f) * 0.5f + 0.5f;
            return 0xFF000000 | ((int)(r * 255) << 16) | ((int)(g * 255) << 8) | (int)(b * 255);
        };
    }
    else if (code.find("mandelbrot") != std::string::npos || code.find("MANDELBROT") != std::string::npos) {
        compiledShader = [](float x, float y, float time, float p1, float p2, float p3, float p4) -> uint32_t {
            float cx = x * p1;
            float cy = y * p1;
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
            float r = sin(hue * 6.28f) * 0.5f + 0.5f;
            float g = sin(hue * 6.28f + 2.09f) * 0.5f + 0.5f;
            float b = sin(hue * 6.28f + 4.19f) * 0.5f + 0.5f;
            
            return 0xFF000000 | ((int)(r * 255) << 16) | ((int)(g * 255) << 8) | (int)(b * 255);
        };
    }
    else if (code.find("voronoi") != std::string::npos || code.find("VORONOI") != std::string::npos) {
        compiledShader = [](float x, float y, float time, float p1, float p2, float p3, float p4) -> uint32_t {
            float minDist = 10.0f;
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    float cellX = i + sin(time * p1 + i * j) * 0.5f;
                    float cellY = j + cos(time * p1 + i * j) * 0.5f;
                    float dx = (x * p2) - cellX;
                    float dy = (y * p2) - cellY;
                    float dist = sqrt(dx * dx + dy * dy);
                    minDist = std::min(minDist, dist);
                }
            }
            
            float value = minDist * p3;
            float r = sin(value + time) * 0.5f + 0.5f;
            float g = sin(value + time + 2.0f) * 0.5f + 0.5f;
            float b = sin(value + time + 4.0f) * 0.5f + 0.5f;
            
            return 0xFF000000 | ((int)(r * 255) << 16) | ((int)(g * 255) << 8) | (int)(b * 255);
        };
    }
    else {
        // Default: simple gradient shader
        compiledShader = [](float x, float y, float time, float p1, float p2, float p3, float p4) -> uint32_t {
            float r = x * 0.5f + 0.5f;
            float g = y * 0.5f + 0.5f;
            float b = sin(time) * 0.5f + 0.5f;
            return 0xFF000000 | ((int)(r * 255) << 16) | ((int)(g * 255) << 8) | (int)(b * 255);
        };
    }
    
    return true;
}

void ShaderPlayground::render(PixelBuffer& buffer, float time, float p1, float p2, float p3, float p4) {
    if (!shaderLoaded || !compiledShader) return;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Normalize coordinates to [-1, 1]
            float fx = (x / (float)width) * 2.0f - 1.0f;
            float fy = (y / (float)height) * 2.0f - 1.0f;
            
            // Execute shader
            uint32_t color = compiledShader(fx, fy, time, p1, p2, p3, p4);
            buffer.setPixel(x, y, color);
        }
    }
}

std::vector<std::string> ShaderPlayground::getAvailableShaders() {
    std::vector<std::string> shaders;
    
    DIR* dir = opendir("shaders");
    if (dir == nullptr) {
        return shaders;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        
        if (filename == "." || filename == "..") continue;
        
        if (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".glsl") {
            shaders.push_back(filename);
        }
    }
    
    closedir(dir);
    std::sort(shaders.begin(), shaders.end());
    
    return shaders;
}

// GLSL-like utility functions
float ShaderPlayground::fract(float x) {
    return x - floor(x);
}

float ShaderPlayground::mix(float a, float b, float t) {
    return a + t * (b - a);
}

float ShaderPlayground::smoothstep(float edge0, float edge1, float x) {
    float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
    return t * t * (3.0f - 2.0f * t);
}

float ShaderPlayground::length(float x, float y) {
    return sqrt(x * x + y * y);
}

float ShaderPlayground::atan2(float y, float x) {
    return ::atan2(y, x);
}
