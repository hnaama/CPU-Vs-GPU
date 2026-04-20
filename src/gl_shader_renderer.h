#pragma once

// Platform-specific OpenGL/GLEW includes
#ifdef __APPLE__
    #include <GL/glew.h>  // Homebrew installs GLEW headers here
    #include <OpenGL/gl.h>
#else
    #include <GL/glew.h>
    #include <GL/gl.h>
#endif

#include <SDL.h>
#include <string>
#include <vector>

// OpenGL-based shader renderer for true GPU shader execution
class GLShaderRenderer {
public:
    GLShaderRenderer(SDL_Window* window, int width, int height);
    ~GLShaderRenderer();
    
    bool initialize();
    void resize(int width, int height);
    
    // Load and compile a GLSL fragment shader from file
    bool loadShader(const std::string& fragmentShaderPath);
    
    // Render using the loaded shader
    void render(float time, float param1, float param2, float param3, float param4);
    
    // Get shader info
    std::string getShaderName() const { return shaderName; }
    std::string getLastError() const { return lastError; }
    bool isShaderLoaded() const { return shaderProgram != 0; }
    
    // Performance metrics
    float getLastFrameTime() const { return lastFrameTime; }
    
private:
    SDL_Window* window;
    SDL_GLContext glContext;
    int width;
    int height;
    
    GLuint shaderProgram;
    GLuint vao, vbo;
    
    std::string shaderName;
    std::string lastError;
    float lastFrameTime;
    
    // Default vertex shader (full-screen quad)
    static const char* vertexShaderSource;
    
    // Helper functions
    bool compileShader(GLuint shader, const char* source);
    bool linkProgram(GLuint program);
    std::string readFile(const std::string& path);
    GLuint createShaderProgram(const std::string& fragmentSource);
    void setupQuad();
};

// Global instance
extern GLShaderRenderer* g_glShaderRenderer;
