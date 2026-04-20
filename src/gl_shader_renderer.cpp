#include "gl_shader_renderer.h"
#include <fstream>
#include <sstream>
#include <iostream>

GLShaderRenderer* g_glShaderRenderer = nullptr;

// Default vertex shader - creates a full-screen quad
const char* GLShaderRenderer::vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 fragCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    fragCoord = aTexCoord;
}
)";

GLShaderRenderer::GLShaderRenderer(SDL_Window* window, int width, int height)
    : window(window), glContext(nullptr), width(width), height(height),
      shaderProgram(0), vao(0), vbo(0), lastFrameTime(0.0f) {
}

GLShaderRenderer::~GLShaderRenderer() {
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }
    if (vao) {
        glDeleteVertexArrays(1, &vao);
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
    }
    if (glContext) {
        SDL_GL_DeleteContext(glContext);
    }
}

bool GLShaderRenderer::initialize() {
    // Set OpenGL attributes before creating context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    // Create OpenGL context
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        lastError = std::string("Failed to create OpenGL context: ") + SDL_GetError();
        std::cerr << lastError << "\n";
        return false;
    }
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        lastError = std::string("GLEW initialization failed: ") + 
                    (const char*)glewGetErrorString(glewError);
        std::cerr << lastError << "\n";
        return false;
    }
    
    std::cout << "OpenGL initialized successfully\n";
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
    
    // Setup full-screen quad
    setupQuad();
    
    // Set viewport
    glViewport(0, 0, width, height);
    
    return true;
}

void GLShaderRenderer::setupQuad() {
    // Full-screen quad vertices (position + texcoord)
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

void GLShaderRenderer::resize(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;
    glViewport(0, 0, width, height);
}

std::string GLShaderRenderer::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        lastError = "Failed to open file: " + path;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool GLShaderRenderer::compileShader(GLuint shader, const char* source) {
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        lastError = std::string("Shader compilation failed:\n") + infoLog;
        return false;
    }
    
    return true;
}

bool GLShaderRenderer::linkProgram(GLuint program) {
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        lastError = std::string("Program linking failed:\n") + infoLog;
        return false;
    }
    
    return true;
}

GLuint GLShaderRenderer::createShaderProgram(const std::string& fragmentSource) {
    // Create vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!compileShader(vertexShader, vertexShaderSource)) {
        glDeleteShader(vertexShader);
        return 0;
    }
    
    // Create fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!compileShader(fragmentShader, fragmentSource.c_str())) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }
    
    // Link program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    
    if (!linkProgram(program)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return 0;
    }
    
    // Clean up shaders (they're linked into the program now)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

bool GLShaderRenderer::loadShader(const std::string& fragmentShaderPath) {
    // Read shader file
    std::string fragmentSource = readFile(fragmentShaderPath);
    if (fragmentSource.empty()) {
        std::cerr << lastError << "\n";
        return false;
    }
    
    // Extract shader name from file
    size_t lastSlash = fragmentShaderPath.find_last_of("/\\");
    shaderName = (lastSlash != std::string::npos) ? 
                 fragmentShaderPath.substr(lastSlash + 1) : fragmentShaderPath;
    
    // Delete old shader program if exists
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    
    // Create new shader program
    shaderProgram = createShaderProgram(fragmentSource);
    if (shaderProgram == 0) {
        std::cerr << lastError << "\n";
        return false;
    }
    
    std::cout << "Successfully loaded shader: " << shaderName << "\n";
    return true;
}

void GLShaderRenderer::render(float time, float param1, float param2, float param3, float param4) {
    if (!isShaderLoaded()) {
        return;
    }
    
    uint32_t startTime = SDL_GetTicks();
    
    // Clear screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Use shader program
    glUseProgram(shaderProgram);
    
    // Set uniform variables
    GLint timeLoc = glGetUniformLocation(shaderProgram, "iTime");
    GLint resolutionLoc = glGetUniformLocation(shaderProgram, "iResolution");
    GLint param1Loc = glGetUniformLocation(shaderProgram, "iParam1");
    GLint param2Loc = glGetUniformLocation(shaderProgram, "iParam2");
    GLint param3Loc = glGetUniformLocation(shaderProgram, "iParam3");
    GLint param4Loc = glGetUniformLocation(shaderProgram, "iParam4");
    
    if (timeLoc != -1) glUniform1f(timeLoc, time);
    if (resolutionLoc != -1) glUniform2f(resolutionLoc, (float)width, (float)height);
    if (param1Loc != -1) glUniform1f(param1Loc, param1);
    if (param2Loc != -1) glUniform1f(param2Loc, param2);
    if (param3Loc != -1) glUniform1f(param3Loc, param3);
    if (param4Loc != -1) glUniform1f(param4Loc, param4);
    
    // Draw full-screen quad
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Swap buffers
    SDL_GL_SwapWindow(window);
    
    uint32_t endTime = SDL_GetTicks();
    lastFrameTime = (endTime - startTime);
}
