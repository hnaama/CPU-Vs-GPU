#pragma once
#include <SDL.h>
#include <functional>
#include <string>
#include <vector>

struct GUIState {
    int currentMode;
    
    // Fractal controls
    int fractalType;
    float fractalZoom;
    float fractalCenterX;
    float fractalCenterY;
    bool fractalAutoAnimate;
    
    // Bouncing balls controls
    int ballCount;
    float gravity;
    float bounceDamping;
    
    // OBJ viewer controls
    float modelRotationX;
    float modelRotationY;
    float modelRotationZ;
    float modelScale;
    bool autoRotate;
    std::string selectedObjFile;
    bool objFileChanged;
    
    // Weird entities controls
    int maxEntities;
    float spawnRate;
    int entityComplexity;
    
    // GPU demo controls
    int gpuDemoMode;
    
    // Shader playground controls
    std::string selectedShaderFile;
    bool shaderFileChanged;
    float shaderTime;
    float shaderParam1;
    float shaderParam2;
    float shaderParam3;
    float shaderParam4;
    bool useGPUShaderPlayground;
    bool useOpenGLShaders;  // New: toggle between SDL2 and OpenGL
    
    // Performance metrics
    float fps;
    int triangleCount;
    
    // GPU rendering toggles for each mode
    bool useGPUWeirdEntities;
    bool useGPUFractals;
    bool useGPUBalls;
    bool useGPUOBJ;
    bool useGPUDemo;
    
    // Educational panel visibility
    bool showEducationalPanel;
    
    GUIState() 
        : currentMode(0),
          fractalType(0), fractalZoom(1.0f), fractalCenterX(0.0f), fractalCenterY(0.0f),
          fractalAutoAnimate(true),
          ballCount(50), gravity(500.0f), bounceDamping(0.8f),
          modelRotationX(0.0f), modelRotationY(0.0f), modelRotationZ(0.0f),
          modelScale(1.0f), autoRotate(true), selectedObjFile(""), objFileChanged(false),
          maxEntities(15), spawnRate(1.0f), entityComplexity(5),
          gpuDemoMode(0),
          selectedShaderFile(""), shaderFileChanged(false), shaderTime(0.0f),
          shaderParam1(0.5f), shaderParam2(0.5f), shaderParam3(0.5f), shaderParam4(0.5f),
          fps(0.0f), triangleCount(0),
          useGPUWeirdEntities(false), useGPUFractals(false), 
          useGPUBalls(false), useGPUOBJ(false), useGPUDemo(false),
          useGPUShaderPlayground(false), useOpenGLShaders(false),
          showEducationalPanel(true) {}
};

class GUI {
public:
    static bool initialize(SDL_Window* window, SDL_Renderer* renderer);
    static void shutdown();
    static void beginFrame();
    static void endFrame(SDL_Renderer* renderer);
    static void processEvent(SDL_Event* event);
    
    // Helper to show a demo window
    static void showDemoWindow(bool* show);
    
    // Helper to create a simple settings window
    static void showSettingsWindow(bool* show, std::function<void()> renderCallback);
    
    // Helper to show control panel for the renderer
    static void showControlPanel(bool* show, 
                                   int* currentMode, 
                                   bool* modeEnabled,
                                   std::function<void(const std::string&)> onObjFileSelected,
                                   std::function<void()> onResetMode,
                                   class GPUDemoSystem* gpuDemo = nullptr);
    
    // Helper to list files in a directory with a specific extension
    static std::vector<std::string> listFilesInDirectory(const std::string& directory, const std::string& extension);
};

void initGUI(SDL_Window* window, SDL_Renderer* renderer);
void shutdownGUI();
void renderGUI(GUIState& state);
bool handleGUIEvent(const SDL_Event& event);
