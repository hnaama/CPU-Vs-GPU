#include <SDL.h>
#include <iostream>
#include <memory>
#include <cstring>
#include <cmath>
#include <vector>

// Include our modularized headers
#include "utils.h"
#include "fractals.h"
#include "pixelbuffer.h"
#include "weird_entities.h"
#include "fractal_system.h"
#include "obj_loader.h"
#include "bouncing_balls.h"
#include "gpu_demo.h"
#include "gui.h"
#include "imgui.h"  // Add ImGui header for ImGuiIO type
#include "shader_renderer.h"
#include "shader_playground.h"
#include "gl_shader_renderer.h"

// No need to declare g_shaderRenderer here, it's extern in shader_renderer.h

int main(int argc, char** argv) {
    // Check for OBJ file argument
    std::string objFilePath = "";
    bool objMode = false;
    
    if (argc >= 2) {
        objFilePath = argv[1];
        // Check if it's an .obj file
        if (objFilePath.size() >= 4 && 
            objFilePath.substr(objFilePath.size() - 4) == ".obj") {
            objMode = true;
            std::cout << "OBJ file mode: " << objFilePath << "\n";
        }
    }
    
    std::cout << "Starting SDL initialization..." << std::flush;
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return -1;
    }
    std::cout << "SDL initialized successfully\n" << std::flush;

    // Get display information for fullscreen
    SDL_DisplayMode displayMode;
    if (SDL_GetCurrentDisplayMode(0, &displayMode) != 0) {
        std::cerr << "SDL_GetCurrentDisplayMode failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return -1;
    }
    
    std::cout << "Display resolution: " << displayMode.w << "x" << displayMode.h << "\n" << std::flush;

    // Use display resolution for fullscreen, or default for windowed
    int WINDOW_WIDTH = displayMode.w;
    int WINDOW_HEIGHT = displayMode.h;
    bool isFullscreen = true;  // Start in fullscreen mode
    
    // For windowed mode (can be toggled later)
    const int WINDOWED_WIDTH = 800;
    const int WINDOWED_HEIGHT = 600;

    std::cout << "Creating fullscreen window..." << std::flush;
    SDL_Window* window = SDL_CreateWindow(
        "Software Renderer - Weird Visual Chaos Engine",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return -1;
    }
    std::cout << "Fullscreen window created (" << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << ")\n" << std::flush;

    std::cout << "Creating renderer..." << std::flush;
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    std::cout << "Renderer created with VSync enabled\n" << std::flush;

    // Initialize ImGui
    std::cout << "Initializing ImGui..." << std::flush;
    if (!GUI::initialize(window, renderer)) {
        std::cerr << "Failed to initialize ImGui\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    std::cout << "ImGui initialized successfully\n" << std::flush;

    // Initialize shader renderer
    g_shaderRenderer = new ShaderRenderer(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!g_shaderRenderer->initialize()) {
        std::cerr << "Failed to initialize shader renderer\n";
        delete g_shaderRenderer;
        g_shaderRenderer = nullptr;
    }

    // Initialize OpenGL shader renderer (separate from SDL renderer)
    if (g_glShaderRenderer == nullptr) {
        g_glShaderRenderer = new GLShaderRenderer(window, WINDOW_WIDTH, WINDOW_HEIGHT);
        if (!g_glShaderRenderer->initialize()) {
            std::cerr << "Warning: OpenGL shader renderer failed to initialize\n";
            std::cerr << g_glShaderRenderer->getLastError() << "\n";
            delete g_glShaderRenderer;
            g_glShaderRenderer = nullptr;
        }
    }

    std::cout << "Creating texture..." << std::flush;
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!texture) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    std::cout << "Texture created (" << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << ")\n" << std::flush;

    // Create our pixel buffer with current resolution
    std::cout << "Creating pixel buffer..." << std::flush;
    PixelBuffer pixelBuffer(WINDOW_WIDTH, WINDOW_HEIGHT);
    std::cout << "Pixel buffer created\n" << std::flush;

    std::cout << "Software Renderer initialized in fullscreen!\n";
    std::cout << "Current resolution: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << "\n" << std::flush;
    std::cout << "Controls:\n";
    std::cout << "  ESC - Exit\n";
    std::cout << "  G - Toggle GUI\n";
    std::cout << "  F11 - Toggle fullscreen/windowed\n";
    std::cout << "  F - Toggle fullscreen/windowed\n";
    std::cout << "  M - Toggle between modes\n";
    std::cout << "  SPACE - New fractal (in fractal mode)\n";
    std::cout << "  R - Reset current mode\n" << std::flush;

    bool running = true;
    bool needsRedraw = true;
    SDL_Event e;
    
    // GUI state
    bool showGUI = true;
    bool renderingEnabled = false;  // Start with rendering disabled
    GUIState guiState;
    
    // Animation variables
    float rotation_time = 0.0f;
    uint32_t last_time = SDL_GetTicks();
    
    std::cout << "Creating visual systems..." << std::flush;
    
    // Create weird visual manager
    WeirdVisualManager weirdVisualManager;

    // Create fractal system
    FractalSystem fractalSystem(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // Set global pointer for injection functions
    g_fractalSystem = &fractalSystem;

    // Load OBJ mesh if in OBJ mode
    Mesh objMesh;
    std::vector<Triangle3D> objTriangles;
    float objRotationX = 0.0f;
    float objRotationY = 0.0f;
    float objZoom = 3.0f;
    bool objLoaded = false;
    
    if (objMode) {
        if (!ObjLoader::load(objFilePath, objMesh)) {
            std::cerr << "Failed to load OBJ file: " << objFilePath << "\n";
            objMode = false;
        } else {
            objTriangles = objMesh.toTriangles();
            objLoaded = true;
            std::cout << "Loaded " << objTriangles.size() << " triangles from OBJ file\n";
        }
    }

    // Bouncing Balls system
    BouncingBallSystem ballSystem;
    
    // GPU Demo system
    GPUDemoSystem gpuDemo(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Initialize shader playground
    ShaderPlayground shaderPlayground(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Mode: 0=Weird Chaos, 1=Fractals, 2=Bouncing Balls, 3=OBJ Viewer, 4=GPU Demo, 5=Shader Playground
    int currentMode = 0;  // Start with mode 0 but disabled

    std::cout << "Initialized visual systems!\n";
    std::cout << "Starting with blank screen - press G to open GUI\n";
    std::cout << std::flush;

    // Function to toggle fullscreen
    auto toggleFullscreen = [&]() {
        if (isFullscreen) {
            // Switch to windowed mode
            SDL_SetWindowFullscreen(window, 0);
            SDL_SetWindowSize(window, WINDOWED_WIDTH, WINDOWED_HEIGHT);
            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            
            // Recreate texture for new size
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      WINDOWED_WIDTH, WINDOWED_HEIGHT);
            
            // Recreate pixel buffer for new size
            pixelBuffer = PixelBuffer(WINDOWED_WIDTH, WINDOWED_HEIGHT);
            
            // Resize fractal system to match new resolution
            fractalSystem.resize(WINDOWED_WIDTH, WINDOWED_HEIGHT);
            
            WINDOW_WIDTH = WINDOWED_WIDTH;
            WINDOW_HEIGHT = WINDOWED_HEIGHT;
            isFullscreen = false;
            
            std::cout << "Switched to windowed mode (" << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << ")\n" << std::flush;
        } else {
            // Switch to fullscreen mode
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            
            // Recreate texture for new size
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      displayMode.w, displayMode.h);
            
            // Recreate pixel buffer for new size
            pixelBuffer = PixelBuffer(displayMode.w, displayMode.h);
            
            // Resize fractal system to match new resolution
            fractalSystem.resize(displayMode.w, displayMode.h);
            
            WINDOW_WIDTH = displayMode.w;
            WINDOW_HEIGHT = displayMode.h;
            isFullscreen = true;
            
            std::cout << "Switched to fullscreen mode (" << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << ")\n" << std::flush;
        }
        needsRedraw = true;
    };

    // Function to draw the scene
    auto drawScene = [&]() {
        // Update animation time
        uint32_t current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        rotation_time += delta_time;
        
        // Check if a new OBJ file was selected via GUI
        if (guiState.objFileChanged) {
            std::cout << "Loading OBJ file from GUI: " << guiState.selectedObjFile << "\n" << std::flush;
            Mesh newMesh;
            if (ObjLoader::load(guiState.selectedObjFile, newMesh)) {
                objMesh = newMesh;
                objTriangles = objMesh.toTriangles();
                objLoaded = true;
                objRotationX = 0.0f;
                objRotationY = 0.0f;
                objZoom = 3.0f;
                currentMode = 3;  // Switch to OBJ viewer
                std::cout << "Loaded " << objTriangles.size() << " triangles\n" << std::flush;
            } else {
                std::cerr << "Failed to load OBJ file: " << guiState.selectedObjFile << "\n" << std::flush;
            }
            guiState.objFileChanged = false;
        }

        // Check if a new shader file was selected via GUI
        if (guiState.shaderFileChanged && currentMode == 5) {
            std::cout << "Loading shader file: " << guiState.selectedShaderFile << "\n" << std::flush;
            if (shaderPlayground.loadShader(guiState.selectedShaderFile)) {
                std::cout << "Shader loaded: " << shaderPlayground.getShaderName() << "\n" << std::flush;
                guiState.shaderTime = 0.0f;  // Reset time when loading new shader
            } else {
                std::cerr << "Failed to load shader: " << shaderPlayground.getLastError() << "\n" << std::flush;
            }
            guiState.shaderFileChanged = false;
        }
        
        // Render based on current mode
        switch (currentMode) {
            case 0: { // Weird Entities
                if (guiState.useGPUWeirdEntities && g_shaderRenderer) {
                    // GPU rendering path
                    g_shaderRenderer->clear(0xFF000000);
                    weirdVisualManager.update(delta_time);
                    std::vector<Triangle3D> allTriangles = weirdVisualManager.getAllTriangles();
                    g_shaderRenderer->renderWeirdEntities(allTriangles, SDL_GetTicks() / 1000.0f);
                    guiState.triangleCount = g_shaderRenderer->getTrianglesRendered();
                } else {
                    // CPU rendering path
                    pixelBuffer.clear(0xFF000000);
                    weirdVisualManager.update(delta_time);
                    std::vector<Triangle3D> allTriangles = weirdVisualManager.getAllTriangles();
                    for (const auto& tri : allTriangles) {
                        pixelBuffer.render3DTriangle(tri, WINDOW_WIDTH, WINDOW_HEIGHT);
                    }
                    guiState.triangleCount = allTriangles.size();
                }
                break;
            }
            case 1: { // Fractals
                if (guiState.useGPUFractals && g_shaderRenderer) {
                    // GPU rendering path
                    g_shaderRenderer->clear(0xFF000000);
                    float time = SDL_GetTicks() / 1000.0f;
                    g_shaderRenderer->renderFractals(guiState.fractalType, time, 
                                                    guiState.fractalZoom,
                                                    guiState.fractalCenterX, 
                                                    guiState.fractalCenterY);
                    guiState.triangleCount = g_shaderRenderer->getTrianglesRendered();
                } else {
                    // CPU rendering path
                    pixelBuffer.clear(0xFF000000);
                    fractalSystem.update(delta_time);
                    fractalSystem.render(pixelBuffer);
                    guiState.triangleCount = WINDOW_WIDTH * WINDOW_HEIGHT;
                }
                break;
            }
            case 2: { // Bouncing Balls
                if (guiState.useGPUBalls && g_shaderRenderer) {
                    // GPU rendering path
                    g_shaderRenderer->clear(0xFF000000);
                    ballSystem.update(delta_time, WINDOW_WIDTH, WINDOW_HEIGHT);
                    std::vector<Vec3> positions;
                    std::vector<float> radii;
                    std::vector<uint32_t> colors;
                    ballSystem.getBallData(positions, radii, colors);
                    g_shaderRenderer->renderBouncingBalls(positions, radii, colors);
                    guiState.triangleCount = positions.size();
                } else {
                    // CPU rendering path
                    pixelBuffer.clear(0xFF000000);
                    ballSystem.update(delta_time, WINDOW_WIDTH, WINDOW_HEIGHT);
                    ballSystem.render(pixelBuffer);
                    guiState.triangleCount = ballSystem.getBallCount();
                }
                break;
            }
            case 3: { // OBJ Viewer
                if (!objLoaded) {
                    // Show "no model loaded" message
                    pixelBuffer.clear(0xFF000000);
                    guiState.triangleCount = 0;
                    break;
                }
                
                // Update rotation if auto-rotate is enabled
                if (guiState.autoRotate) {
                    guiState.modelRotationY += delta_time * 30.0f; // 30 degrees per second
                }
                
                // Apply transformations
                Matrix4x4 modelMatrix = Matrix4x4::identity();
                modelMatrix = modelMatrix * Matrix4x4::rotationX(guiState.modelRotationX * M_PI / 180.0f);
                modelMatrix = modelMatrix * Matrix4x4::rotationY(guiState.modelRotationY * M_PI / 180.0f);
                modelMatrix = modelMatrix * Matrix4x4::rotationZ(guiState.modelRotationZ * M_PI / 180.0f);
                
                // Create projection matrix
                float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
                Matrix4x4 projMatrix = Matrix4x4::perspective(60.0f * M_PI / 180.0f, aspect, 0.1f, 100.0f);
                
                // View matrix (camera at z=5)
                Matrix4x4 viewMatrix = Matrix4x4::translation(0, 0, -5.0f * guiState.modelScale);
                
                // Combined MVP
                Matrix4x4 mvp = projMatrix * viewMatrix * modelMatrix;
                
                if (guiState.useGPUOBJ && g_shaderRenderer) {
                    // GPU rendering path
                    g_shaderRenderer->clear(0xFF000000);
                    
                    // Transform and sort triangles
                    std::vector<std::pair<float, Triangle3D>> sortedTriangles;
                    for (const auto& triangle : objTriangles) {
                        Triangle3D transformed = triangle.transform(mvp);
                        
                        // Project to screen space
                        for (int i = 0; i < 3; i++) {
                            transformed.vertices[i].x = (transformed.vertices[i].x + 1.0f) * WINDOW_WIDTH * 0.5f;
                            transformed.vertices[i].y = (1.0f - transformed.vertices[i].y) * WINDOW_HEIGHT * 0.5f;
                        }
                        
                        float avgZ = (transformed.vertices[0].z + 
                                      transformed.vertices[1].z + 
                                      transformed.vertices[2].z) / 3.0f;
                        sortedTriangles.push_back({avgZ, transformed});
                    }
                    
                    // Sort by depth (back to front)
                    std::sort(sortedTriangles.begin(), sortedTriangles.end(),
                        [](const auto& a, const auto& b) { return a.first > b.first; });
                    
                    g_shaderRenderer->renderOBJModel(sortedTriangles, Matrix4x4::identity());
                    guiState.triangleCount = g_shaderRenderer->getTrianglesRendered();
                } else {
                    // CPU rendering path
                    pixelBuffer.clear(0xFF000000);
                    
                    // Transform and sort triangles by depth
                    std::vector<std::pair<float, Triangle3D>> sortedTriangles;
                    for (const auto& triangle : objTriangles) {
                        Triangle3D transformed = triangle.transform(mvp);
                        
                        // Project to screen space
                        for (int i = 0; i < 3; i++) {
                            transformed.vertices[i].x = (transformed.vertices[i].x + 1.0f) * WINDOW_WIDTH * 0.5f;
                            transformed.vertices[i].y = (1.0f - transformed.vertices[i].y) * WINDOW_HEIGHT * 0.5f;
                        }
                        
                        float avgZ = (transformed.vertices[0].z + 
                                      transformed.vertices[1].z + 
                                      transformed.vertices[2].z) / 3.0f;
                        sortedTriangles.push_back({avgZ, transformed});
                    }
                    
                    // Sort by depth (back to front for proper rendering)
                    std::sort(sortedTriangles.begin(), sortedTriangles.end(),
                        [](const auto& a, const auto& b) { return a.first > b.first; });
                    
                    // Render triangles in sorted order
                    for (const auto& pair : sortedTriangles) {
                        pixelBuffer.render3DTriangle(pair.second, WINDOW_WIDTH, WINDOW_HEIGHT);
                    }
                    guiState.triangleCount = sortedTriangles.size();
                }
                break;
            }
            case 4: { // GPU Demo
                if (guiState.useGPUDemo && g_shaderRenderer) {
                    // GPU rendering path
                    g_shaderRenderer->clear(0xFF000000);
                    float time = SDL_GetTicks() / 1000.0f;
                    g_shaderRenderer->renderGPUDemo(guiState.gpuDemoMode, time);
                    guiState.triangleCount = g_shaderRenderer->getTrianglesRendered();
                } else {
                    // CPU rendering path
                    pixelBuffer.clear(0xFF000000);
                    gpuDemo.update(delta_time);
                    gpuDemo.render(pixelBuffer);
                    guiState.triangleCount = gpuDemo.getTriangleCount();
                }
                break;
            }
            case 5: { // Shader Playground
                guiState.shaderTime += delta_time;
                
                // Detect shader type from filename
                int shaderType = 0; // default
                if (guiState.selectedShaderFile.find("plasma") != std::string::npos) shaderType = 0;
                else if (guiState.selectedShaderFile.find("tunnel") != std::string::npos) shaderType = 1;
                else if (guiState.selectedShaderFile.find("ripple") != std::string::npos) shaderType = 2;
                else if (guiState.selectedShaderFile.find("mandelbrot") != std::string::npos) shaderType = 3;
                else if (guiState.selectedShaderFile.find("voronoi") != std::string::npos) shaderType = 4;
                
                if (guiState.useGPUShaderPlayground && g_shaderRenderer) {
                    // GPU rendering path - FAST!
                    g_shaderRenderer->clear(0xFF000000);
                    g_shaderRenderer->renderShaderPlayground(shaderType, guiState.shaderTime,
                                                            guiState.shaderParam1,
                                                            guiState.shaderParam2,
                                                            guiState.shaderParam3,
                                                            guiState.shaderParam4);
                    guiState.triangleCount = g_shaderRenderer->getTrianglesRendered();
                } else {
                    // CPU rendering path - SLOW!
                    pixelBuffer.clear(0xFF000000);
                    
                    if (shaderPlayground.isShaderLoaded()) {
                        shaderPlayground.render(pixelBuffer, guiState.shaderTime, 
                                              guiState.shaderParam1, 
                                              guiState.shaderParam2,
                                              guiState.shaderParam3, 
                                              guiState.shaderParam4);
                        guiState.triangleCount = WINDOW_WIDTH * WINDOW_HEIGHT;
                    } else {
                        // Show message when no shader is loaded
                        guiState.triangleCount = 0;
                    }
                }
                break;
            }
        }

        // Update FPS
        guiState.fps = 1.0f / delta_time;
        
        // Check if mode was changed via GUI BEFORE overwriting it
        if (guiState.currentMode != currentMode) {
            currentMode = guiState.currentMode;
            std::cout << "GUI changed mode to " << currentMode << "\n" << std::flush;
        }
        
        // Now sync back to GUI state
        guiState.currentMode = currentMode;

        // Clear renderer first
        SDL_RenderClear(renderer);

        // Present the rendered content to screen
        bool usedGPU = (currentMode == 0 && guiState.useGPUWeirdEntities) ||
                       (currentMode == 1 && guiState.useGPUFractals) ||
                       (currentMode == 2 && guiState.useGPUBalls) ||
                       (currentMode == 3 && guiState.useGPUOBJ) ||
                       (currentMode == 4 && guiState.useGPUDemo) ||
                       (currentMode == 5 && guiState.useGPUShaderPlayground);

        if (usedGPU && g_shaderRenderer) {
            // GPU path: copy shader texture to renderer
            g_shaderRenderer->present();
        } else {
            // CPU path: copy pixel buffer to texture then to renderer
            void* texturePixels;
            int pitch;
            if (SDL_LockTexture(texture, NULL, &texturePixels, &pitch) == 0) {
                memcpy(texturePixels, pixelBuffer.getData(), WINDOW_WIDTH * WINDOW_HEIGHT * 4);
                SDL_UnlockTexture(texture);
            }
            SDL_RenderCopy(renderer, texture, NULL, NULL);
        }

        // Render GUI on top - only call renderGUI, not showControlPanel
        GUI::beginFrame();
        renderGUI(guiState);
        GUI::endFrame(renderer);
        
        // Final present
        SDL_RenderPresent(renderer);
    };

    // Main loop
    while (running) {
        // Handle events
        while (SDL_PollEvent(&e)) {
            // Let ImGui handle events first
            GUI::processEvent(&e);
            ImGuiIO& io = ImGui::GetIO();
            
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN && !io.WantCaptureKeyboard) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_g:
                        showGUI = !showGUI;
                        needsRedraw = true;
                        break;
                    case SDLK_f:
                    case SDLK_F11:
                        toggleFullscreen();
                        break;
                    case SDLK_m:
                        currentMode = (currentMode + 1) % 6;
                        renderingEnabled = true;
                        needsRedraw = true;
                        std::cout << "Switched to mode " << currentMode << "\n" << std::flush;
                        break;
                    case SDLK_r:
                        if (currentMode == 0) {
                            weirdVisualManager = WeirdVisualManager();
                        } else if (currentMode == 1) {
                            fractalSystem.initialize();
                        } else if (currentMode == 2) {
                            ballSystem.reset();
                        }
                        needsRedraw = true;
                        break;
                    case SDLK_SPACE:
                        if (currentMode == 1) {
                            fractalSystem.initialize();
                            needsRedraw = true;
                        }
                        break;
                }
            }
        }
        
        // Draw if rendering is enabled
        if (renderingEnabled) {
            drawScene();
        } else {
            // Just draw GUI on black screen
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            
            GUI::beginFrame();
            GUI::showControlPanel(&showGUI, &currentMode, &renderingEnabled, [&](const std::string& filepath) {
                std::cout << "Loading OBJ file: " << filepath << "\n" << std::flush;
                Mesh newMesh;
                if (ObjLoader::load(filepath, newMesh)) {
                    objMesh = newMesh;
                    objTriangles = objMesh.toTriangles();
                    objLoaded = true;
                    objRotationX = 0.0f;
                    objRotationY = 0.0f;
                    objZoom = 3.0f;
                    currentMode = 3;
                    renderingEnabled = true;
                    std::cout << "Loaded " << objTriangles.size() << " triangles from " << filepath << "\n" << std::flush;
                } else {
                    std::cerr << "Failed to load OBJ file: " << filepath << "\n" << std::flush;
                }
                needsRedraw = true;
            }, [&]() {
                if (currentMode == 0) {
                    weirdVisualManager = WeirdVisualManager();
                } else if (currentMode == 1) {
                    fractalSystem.initialize();
                } else if (currentMode == 2) {
                    ballSystem.reset();
                }
                needsRedraw = true;
            }, &gpuDemo);
            GUI::endFrame(renderer);
            
            SDL_RenderPresent(renderer);
        }
    }

    std::cout << "Cleaning up...\n" << std::flush;
    if (g_shaderRenderer) {
        delete g_shaderRenderer;
        g_shaderRenderer = nullptr;
    }
    if (g_glShaderRenderer) {
        delete g_glShaderRenderer;
        g_glShaderRenderer = nullptr;
    }
    GUI::shutdown();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "Software Renderer terminated successfully!\n" << std::flush;
    return 0;
}
