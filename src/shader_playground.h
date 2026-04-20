#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "pixelbuffer.h"

// Simple GLSL shader interpreter for educational purposes
class ShaderPlayground {
public:
    ShaderPlayground(int width, int height);
    
    // Load and compile a shader from file
    bool loadShader(const std::string& filepath);
    
    // Execute the shader and render to pixel buffer
    void render(PixelBuffer& buffer, float time, float param1, float param2, float param3, float param4);
    
    // Get shader info
    std::string getShaderName() const { return shaderName; }
    std::string getShaderDescription() const { return shaderDescription; }
    std::string getLastError() const { return lastError; }
    bool isShaderLoaded() const { return shaderLoaded; }
    
    // Get available shaders
    static std::vector<std::string> getAvailableShaders();

private:
    int width;
    int height;
    bool shaderLoaded;
    std::string shaderName;
    std::string shaderDescription;
    std::string shaderCode;
    std::string lastError;
    
    // Shader function type: (x, y, time, params...) -> color
    using ShaderFunc = std::function<uint32_t(float, float, float, float, float, float, float)>;
    ShaderFunc compiledShader;
    
    // Parse and compile the shader code
    bool compileShader(const std::string& code);
    
    // Built-in shader functions (GLSL-like)
    static float fract(float x);
    static float mix(float a, float b, float t);
    static float smoothstep(float edge0, float edge1, float x);
    static float length(float x, float y);
    static float atan2(float y, float x);
};
