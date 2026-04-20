#include "gui.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "gpu_demo.h"
#include <dirent.h>
#include <sys/stat.h>

bool GUI::initialize(SDL_Window* window, SDL_Renderer* renderer) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    if (!ImGui_ImplSDL2_InitForSDLRenderer(window, renderer)) {
        return false;
    }
    if (!ImGui_ImplSDLRenderer2_Init(renderer)) {
        ImGui_ImplSDL2_Shutdown();
        return false;
    }
    
    return true;
}

void GUI::shutdown() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void GUI::beginFrame() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void GUI::endFrame(SDL_Renderer* renderer) {
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
}

void GUI::processEvent(SDL_Event* event) {
    ImGui_ImplSDL2_ProcessEvent(event);
}

void GUI::showDemoWindow(bool* show) {
    // Simple demo window without imgui_demo.cpp
    if (show && *show) {
        ImGui::Begin("ImGui Demo", show);
        ImGui::Text("ImGui is working correctly!");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                    1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        if (ImGui::Button("Close")) {
            *show = false;
        }
        ImGui::End();
    }
}

void GUI::showSettingsWindow(bool* show, std::function<void()> renderCallback) {
    if (show && *show) {
        ImGui::Begin("Settings", show);
        if (renderCallback) {
            renderCallback();
        }
        ImGui::End();
    }
}

std::vector<std::string> GUI::listFilesInDirectory(const std::string& directory, const std::string& extension) {
    std::vector<std::string> files;
    
    DIR* dir = opendir(directory.c_str());
    if (dir == nullptr) {
        return files;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        
        // Skip . and ..
        if (filename == "." || filename == "..") {
            continue;
        }
        
        // Check if file has the right extension
        if (extension.empty() || 
            (filename.size() >= extension.size() && 
             filename.substr(filename.size() - extension.size()) == extension)) {
            
            // Check if it's a regular file
            std::string fullPath = directory + "/" + filename;
            struct stat statbuf;
            if (stat(fullPath.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
                files.push_back(filename);
            }
        }
    }
    
    closedir(dir);
    
    // Sort alphabetically
    std::sort(files.begin(), files.end());
    
    return files;
}

void GUI::showControlPanel(bool* show, 
                           int* currentMode, 
                           bool* modeEnabled,
                           std::function<void(const std::string&)> onObjFileSelected,
                           std::function<void()> onResetMode,
                           GPUDemoSystem* gpuDemo) {
    if (!show || !*show) return;
    
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Renderer Control Panel", show, ImGuiWindowFlags_None);
    
    ImGui::Text("Software Renderer Control");
    ImGui::Separator();
    
    // FPS Display
    ImGui::Text("FPS: %.1f (%.3f ms/frame)", 
                ImGui::GetIO().Framerate, 
                1000.0f / ImGui::GetIO().Framerate);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Mode Selection
    ImGui::Text("Rendering Mode:");
    ImGui::Checkbox("Enable Rendering", modeEnabled);
    
    if (*modeEnabled) {
        const char* modes[] = { "Weird Chaos", "Fractals", "Bouncing Balls", "OBJ Viewer", "GPU Demo" };
        ImGui::Combo("Mode", currentMode, modes, IM_ARRAYSIZE(modes));
        
        ImGui::Spacing();
        
        // Mode-specific controls
        switch (*currentMode) {
            case 0:
                ImGui::TextWrapped("Weird Chaos Mode: Random 3D shapes and effects");
                break;
            case 1:
                ImGui::TextWrapped("Fractals: Mandelbrot, Julia, and more");
                if (ImGui::Button("New Fractal (SPACE)")) {
                    onResetMode();
                }
                break;
            case 2:
                ImGui::TextWrapped("Bouncing Balls: Physics simulation in a box");
                break;
            case 3:
                ImGui::TextWrapped("OBJ Viewer: 3D model rendering");
                break;
            case 4:
                ImGui::TextWrapped("GPU Demo: Why We Need GPUs");
                ImGui::Spacing();
                
                if (gpuDemo) {
                    // Show current complexity level
                    ImGui::Text("Complexity Level:");
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", gpuDemo->getModeName().c_str());
                    
                    ImGui::Spacing();
                    
                    // Show performance stats
                    ImGui::Text("Performance Metrics:");
                    ImGui::Separator();
                    
                    if (gpuDemo->getCurrentMode() == GPUDemoSystem::FULL_SCREEN_PIXELS) {
                        ImGui::Text("Pixels Computed: %d", gpuDemo->getPixelsRendered());
                    } else {
                        ImGui::Text("Triangles: %d", gpuDemo->getTriangleCount());
                    }
                    
                    ImGui::Text("Render Time: %.2f ms", gpuDemo->getLastFrameTime());
                    ImGui::Text("Average Time: %.2f ms", gpuDemo->getAvgFrameTime());
                    
                    float fps = gpuDemo->getFPS();
                    ImVec4 fpsColor = fps > 30 ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : 
                                     fps > 10 ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) : 
                                               ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
                    ImGui::TextColored(fpsColor, "Est. FPS: %.1f", fps);
                    
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    // Description
                    ImGui::TextWrapped("%s", gpuDemo->getModeDescription().c_str());
                    
                    ImGui::Spacing();
                    
                    // Complexity controls
                    ImGui::Text("Controls:");
                    ImGui::BulletText("UP Arrow - Increase complexity");
                    ImGui::BulletText("DOWN Arrow - Decrease complexity");
                    
                    ImGui::Spacing();
                    
                    // Quick buttons for complexity levels
                    if (ImGui::Button("1 Triangle")) {
                        gpuDemo->setMode(GPUDemoSystem::SINGLE_TRIANGLE);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("100 Tris")) {
                        gpuDemo->setMode(GPUDemoSystem::FEW_TRIANGLES);
                    }
                    
                    if (ImGui::Button("1K Tris")) {
                        gpuDemo->setMode(GPUDemoSystem::MANY_TRIANGLES);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("10K Tris")) {
                        gpuDemo->setMode(GPUDemoSystem::LOTS_OF_TRIANGLES);
                    }
                    
                    if (ImGui::Button("100K Tris")) {
                        gpuDemo->setMode(GPUDemoSystem::EXTREME_TRIANGLES);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("All Pixels")) {
                        gpuDemo->setMode(GPUDemoSystem::FULL_SCREEN_PIXELS);
                    }
                }
                break;
        }
        
        ImGui::Spacing();
        
        if (ImGui::Button("Reset Current Mode (R)")) {
            onResetMode();
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // OBJ File Browser
    ImGui::Text("OBJ File Browser:");
    ImGui::TextWrapped("Select a 3D model from the assets folder:");
    
    ImGui::Spacing();
    
    static std::vector<std::string> objFiles;
    static bool filesLoaded = false;
    
    if (!filesLoaded || ImGui::Button("Refresh File List")) {
        objFiles = listFilesInDirectory("assets", ".obj");
        filesLoaded = true;
    }
    
    ImGui::Spacing();
    
    if (objFiles.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No .obj files found in assets/");
    } else {
        ImGui::BeginChild("OBJFiles", ImVec2(0, 150), true);
        
        for (const auto& file : objFiles) {
            if (ImGui::Selectable(file.c_str())) {
                std::string fullPath = "assets/" + file;
                if (onObjFileSelected) {
                    onObjFileSelected(fullPath);
                }
            }
            
            // Show file size hint on hover
            if (ImGui::IsItemHovered()) {
                struct stat statbuf;
                std::string fullPath = "assets/" + file;
                if (stat(fullPath.c_str(), &statbuf) == 0) {
                    float sizeMB = statbuf.st_size / (1024.0f * 1024.0f);
                    if (sizeMB < 1.0f) {
                        ImGui::SetTooltip("%.1f KB - Click to load", statbuf.st_size / 1024.0f);
                    } else {
                        ImGui::SetTooltip("%.1f MB - Click to load", sizeMB);
                    }
                }
            }
        }
        
        ImGui::EndChild();
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Controls help
    ImGui::Text("Keyboard Controls:");
    ImGui::BulletText("ESC - Exit");
    ImGui::BulletText("F11/F - Toggle fullscreen");
    ImGui::BulletText("G - Toggle this GUI");
    ImGui::BulletText("M - Next mode");
    ImGui::BulletText("R - Reset mode");
    if (*currentMode == 1) {
        ImGui::BulletText("SPACE - New fractal");
    } else if (*currentMode == 4) {
        ImGui::BulletText("UP/DOWN - Change complexity");
    }
    
    ImGui::End();
}

void renderGUI(GUIState& state) {
    // Main Control Panel
    ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("Rendering Status: Active");
    ImGui::Checkbox("Show Educational Info", &state.showEducationalPanel);
    
    // Mode selection
    const char* modes[] = { "Weird Entities", "Fractals", "Bouncing Balls", "OBJ Viewer", "GPU Demo", "Shader Playground" };
    if (ImGui::Combo("Render Mode", &state.currentMode, modes, 6)) {
        // Mode changed
    }
    
    ImGui::Separator();
    
    // Mode-specific controls
    switch (state.currentMode) {
        case 0: { // Weird Entities
            ImGui::Text("Weird Visual Chaos");
            ImGui::Checkbox("Use GPU Rendering", &state.useGPUWeirdEntities);
            if (state.useGPUWeirdEntities) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GPU Accelerated Mode");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU Software Rendering");
            }
            ImGui::Separator();
            ImGui::SliderInt("Max Entities", &state.maxEntities, 1, 50);
            ImGui::SliderFloat("Spawn Rate", &state.spawnRate, 0.1f, 3.0f);
            ImGui::SliderInt("Entity Complexity", &state.entityComplexity, 1, 10);
            break;
        }
        case 1: { // Fractals
            ImGui::Checkbox("Use GPU Rendering", &state.useGPUFractals);
            if (state.useGPUFractals) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GPU Shader-Based");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU Pixel-by-Pixel");
            }
            ImGui::Separator();
            const char* fractalTypes[] = { "Mandelbrot", "Julia Set", "Burning Ship" };
            ImGui::Combo("Fractal Type", &state.fractalType, fractalTypes, 3);
            ImGui::SliderFloat("Zoom", &state.fractalZoom, 0.1f, 10.0f);
            ImGui::SliderFloat("Center X", &state.fractalCenterX, -2.0f, 2.0f);
            ImGui::SliderFloat("Center Y", &state.fractalCenterY, -2.0f, 2.0f);
            ImGui::Checkbox("Auto Animate", &state.fractalAutoAnimate);
            break;
        }
        case 2: { // Bouncing Balls
            ImGui::Checkbox("Use GPU Rendering", &state.useGPUBalls);
            if (state.useGPUBalls) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GPU Circle Rendering");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU Scanline Rendering");
            }
            ImGui::Separator();
            ImGui::SliderInt("Ball Count", &state.ballCount, 1, 200);
            ImGui::SliderFloat("Gravity", &state.gravity, 0.0f, 1000.0f);
            ImGui::SliderFloat("Bounce Damping", &state.bounceDamping, 0.1f, 1.0f);
            break;
        }
        case 3: { // OBJ Viewer
            ImGui::Checkbox("Use GPU Rendering", &state.useGPUOBJ);
            if (state.useGPUOBJ) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GPU Z-Buffer + Lighting");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU Triangle Rasterization");
            }
            ImGui::Separator();
            
            // OBJ File Browser
            ImGui::Text("Load OBJ Model:");
            
            static std::vector<std::string> objFiles;
            static bool filesLoaded = false;
            static std::string selectedFile = "";
            
            if (!filesLoaded || ImGui::Button("Refresh Files")) {
                objFiles = GUI::listFilesInDirectory("assets", ".obj");
                filesLoaded = true;
            }
            
            ImGui::Spacing();
            
            if (objFiles.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No .obj files in assets/");
            } else {
                ImGui::BeginChild("OBJFileList", ImVec2(0, 120), true);
                
                for (const auto& file : objFiles) {
                    bool isSelected = (selectedFile == file);
                    if (ImGui::Selectable(file.c_str(), isSelected)) {
                        selectedFile = file;
                        state.selectedObjFile = "assets/" + file;
                        state.objFileChanged = true;
                    }
                }
                
                ImGui::EndChild();
            }
            
            ImGui::Separator();
            ImGui::SliderFloat("Rotation X", &state.modelRotationX, -180.0f, 180.0f);
            ImGui::SliderFloat("Rotation Y", &state.modelRotationY, -180.0f, 180.0f);
            ImGui::SliderFloat("Rotation Z", &state.modelRotationZ, -180.0f, 180.0f);
            ImGui::SliderFloat("Scale", &state.modelScale, 0.1f, 5.0f);
            ImGui::Checkbox("Auto Rotate", &state.autoRotate);
            break;
        }
        case 4: { // GPU Demo
            ImGui::Checkbox("Use GPU Rendering", &state.useGPUDemo);
            if (state.useGPUDemo) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Pure GPU Shader Effects");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU Math Emulation");
            }
            ImGui::Separator();
            const char* demoModes[] = { "Plasma", "Tunnel", "Ripple" };
            ImGui::Combo("Effect", &state.gpuDemoMode, demoModes, 3);
            break;
        }
        case 5: { // Shader Playground
            ImGui::Text("Live GLSL Shader Editor");
            ImGui::Checkbox("Use GPU Rendering", &state.useGPUShaderPlayground);
            
            if (state.useGPUShaderPlayground) {
                ImGui::SameLine();
                ImGui::Checkbox("Use OpenGL", &state.useOpenGLShaders);
                
                if (state.useOpenGLShaders) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OpenGL + GLEW - Real GLSL Shaders");
                } else {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GPU Hardware Accelerated (SDL2)");
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU Sequential Execution");
            }
            ImGui::Separator();
            
            // Shader file browser
            static std::vector<std::string> shaderFiles;
            static bool shadersLoaded = false;
            static std::string selectedShader = "";
            
            if (!shadersLoaded || ImGui::Button("Refresh Shaders")) {
                shaderFiles = GUI::listFilesInDirectory("shaders", ".glsl");
                shadersLoaded = true;
            }
            
            ImGui::Spacing();
            
            if (shaderFiles.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No .glsl files in shaders/");
                ImGui::TextWrapped("Create shader files in the 'shaders/' folder to get started!");
            } else {
                ImGui::BeginChild("ShaderFileList", ImVec2(0, 120), true);
                
                for (const auto& file : shaderFiles) {
                    bool isSelected = (selectedShader == file);
                    if (ImGui::Selectable(file.c_str(), isSelected)) {
                        selectedShader = file;
                        state.selectedShaderFile = "shaders/" + file;
                        state.shaderFileChanged = true;
                    }
                }
                
                ImGui::EndChild();
            }
            
            ImGui::Separator();
            ImGui::Text("Shader Parameters:");
            ImGui::SliderFloat("Param 1", &state.shaderParam1, 0.0f, 2.0f);
            ImGui::SliderFloat("Param 2", &state.shaderParam2, 0.0f, 2.0f);
            ImGui::SliderFloat("Param 3", &state.shaderParam3, 0.0f, 2.0f);
            ImGui::SliderFloat("Param 4", &state.shaderParam4, 0.0f, 2.0f);
            
            if (ImGui::Button("Reset Parameters")) {
                state.shaderParam1 = 0.5f;
                state.shaderParam2 = 0.5f;
                state.shaderParam3 = 0.5f;
                state.shaderParam4 = 0.5f;
            }
            break;
        }
    }
    
    ImGui::Separator();
    
    // Performance metrics
    ImGui::Text("Performance");
    ImGui::Text("FPS: %.1f", state.fps);
    ImGui::Text("Triangles/Pixels: %d", state.triangleCount);
    
    // Performance comparison hint
    if (state.currentMode == 0 && !state.useGPUWeirdEntities) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Try GPU mode for comparison!");
    } else if (state.currentMode == 1 && !state.useGPUFractals) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GPU fractals are MUCH faster!");
    }
    
    ImGui::Separator();
    
    // Keyboard controls
    ImGui::Text("Keyboard Shortcuts:");
    ImGui::BulletText("M - Next mode");
    ImGui::BulletText("R - Reset current mode");
    ImGui::BulletText("G - Toggle GUI");
    ImGui::BulletText("F/F11 - Toggle fullscreen");
    ImGui::BulletText("ESC - Exit");
    
    ImGui::End();
    
    // Educational Panel
    if (state.showEducationalPanel) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 520, 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(500, 700), ImGuiCond_FirstUseEver);
        ImGui::Begin("Educational Info", &state.showEducationalPanel);
        
        switch (state.currentMode) {
            case 0: // Weird Entities
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MODE: 3D Triangle Rendering");
                ImGui::Separator();
                
                ImGui::TextWrapped("This mode renders dynamic 3D triangles with morphing geometries and complex transformations.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "How It Works:");
                ImGui::BulletText("Each entity generates 10-50 triangles");
                ImGui::BulletText("Triangles transform using sin/cos functions");
                ImGui::BulletText("Barycentric coordinate rasterization");
                ImGui::BulletText("Color interpolation per pixel");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU Approach:");
                ImGui::BulletText("Sequential triangle processing");
                ImGui::BulletText("Software rasterization (1 pixel at a time)");
                ImGui::BulletText("No parallel processing");
                ImGui::TextWrapped("The CPU must calculate each pixel serially. With complex math per triangle, this becomes very slow.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GPU Advantage:");
                ImGui::BulletText("Parallel triangle rasterization");
                ImGui::BulletText("Hardware-accelerated interpolation");
                ImGui::BulletText("Thousands of pixels processed simultaneously");
                ImGui::TextWrapped("GPUs can process multiple triangles and pixels in parallel, making them 10-100x faster for this workload.");
                ImGui::Spacing();
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Try This:");
                ImGui::BulletText("Increase Entity Complexity to 10");
                ImGui::BulletText("Set Max Entities to 50");
                ImGui::BulletText("Toggle GPU on/off and watch FPS change!");
                ImGui::TextWrapped("You'll see CPU FPS drop dramatically while GPU stays smooth.");
                break;
                
            case 1: // Fractals
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MODE: Fractal Generation");
                ImGui::Separator();
                
                ImGui::TextWrapped("Fractals require complex iterative calculations for EVERY pixel on screen.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "How It Works:");
                ImGui::BulletText("Each pixel needs 10-100 iterations");
                ImGui::BulletText("Complex number arithmetic (z = z² + c)");
                ImGui::BulletText("Convergence testing per iteration");
                ImGui::Text("Total operations: %d pixels × ~50 iterations", state.triangleCount);
                ImGui::TextWrapped("= Millions of floating-point calculations!");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU Approach:");
                ImGui::BulletText("Process pixels one-by-one");
                ImGui::BulletText("1-8 cores working simultaneously");
                ImGui::BulletText("~8 pixels calculated at once (best case)");
                ImGui::TextWrapped("Even with multiple CPU cores, you're limited to processing a handful of pixels simultaneously.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GPU Advantage:");
                ImGui::BulletText("1000s of parallel shader cores");
                ImGui::BulletText("Each pixel calculated independently");
                ImGui::BulletText("10,000+ pixels processed simultaneously");
                ImGui::TextWrapped("Modern GPUs have specialized hardware for exactly this type of 'embarrassingly parallel' problem!");
                ImGui::Spacing();
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "SPEED DIFFERENCE:");
                ImGui::Text("CPU: ~5-30 FPS");
                ImGui::Text("GPU: ~500-2000 FPS");
                ImGui::TextWrapped("That's 50-100x faster! This is THE classic example of why GPUs exist.");
                break;
                
            case 2: // Bouncing Balls
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MODE: Physics Simulation");
                ImGui::Separator();
                
                ImGui::TextWrapped("Physics simulation has two parts: physics update and rendering.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "How It Works:");
                ImGui::BulletText("Physics: Update ball positions/velocities");
                ImGui::BulletText("Collision: Check ball-to-ball collisions");
                ImGui::BulletText("Rendering: Draw each ball as a circle");
                ImGui::Text("Current balls: %d", state.ballCount);
                ImGui::Text("Collision checks: %d per frame", (state.ballCount * (state.ballCount - 1)) / 2);
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU Approach:");
                ImGui::BulletText("Physics: GOOD - sequential, dependent");
                ImGui::BulletText("Collisions: OKAY - can parallelize somewhat");
                ImGui::BulletText("Rendering: SLOW - pixel-by-pixel drawing");
                ImGui::TextWrapped("CPU is actually great for physics because updates depend on each other. But rendering circles is still slow.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GPU Advantage:");
                ImGui::BulletText("Physics: Done on CPU (better there)");
                ImGui::BulletText("Rendering: FAST - parallel circle rasterization");
                ImGui::BulletText("Shading: Hardware-accelerated gradients");
                ImGui::TextWrapped("GPU renders all circles simultaneously with hardware anti-aliasing and lighting effects.");
                ImGui::Spacing();
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Try This:");
                ImGui::BulletText("Increase ball count to 200");
                ImGui::BulletText("Toggle GPU on/off");
                ImGui::TextWrapped("You'll notice GPU helps more with rendering than physics updates.");
                break;
                
            case 3: // OBJ Viewer
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MODE: 3D Model Rendering");
                ImGui::Separator();
                
                ImGui::TextWrapped("3D model rendering is the PRIMARY use case for GPUs!");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "How It Works:");
                ImGui::BulletText("Transform vertices (MVP matrix)");
                ImGui::BulletText("Project to 2D screen space");
                ImGui::BulletText("Rasterize triangles to pixels");
                ImGui::BulletText("Z-buffer for depth testing");
                ImGui::BulletText("Lighting calculations per pixel");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU Approach:");
                ImGui::BulletText("Sort triangles by depth (slow)");
                ImGui::BulletText("Render triangles sequentially");
                ImGui::BulletText("Software z-buffer (memory intensive)");
                ImGui::BulletText("No hardware acceleration");
                ImGui::TextWrapped("Without depth sorting, you get rendering artifacts. With sorting, it's extremely slow for complex models.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GPU Advantage:");
                ImGui::BulletText("Hardware z-buffer (no sorting needed!)");
                ImGui::BulletText("Parallel vertex processing");
                ImGui::BulletText("Parallel rasterization");
                ImGui::BulletText("Hardware texture mapping");
                ImGui::BulletText("Built-in anti-aliasing");
                ImGui::TextWrapped("GPUs were literally DESIGNED for this task. They have dedicated hardware for every step!");
                ImGui::Spacing();
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Real-World Impact:");
                ImGui::TextWrapped("Modern games render millions of triangles at 60+ FPS. This would be IMPOSSIBLE on CPU.");
                ImGui::TextWrapped("A GPU can render 10,000+ triangles in the time CPU renders 100.");
                break;
                
            case 4: // GPU Demo
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MODE: Shader Effects");
                ImGui::Separator();
                
                ImGui::TextWrapped("These are 'shader' effects - programs that run per pixel.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "How It Works:");
                ImGui::BulletText("Each pixel calculates independently");
                ImGui::BulletText("Complex math: sin, cos, sqrt per pixel");
                ImGui::BulletText("Distance fields and procedural textures");
                ImGui::Text("Calculations: %d pixels × ~20 operations", state.triangleCount);
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU Approach:");
                ImGui::BulletText("Sequential pixel processing");
                ImGui::BulletText("Limited by single-thread performance");
                ImGui::BulletText("Cache misses on large images");
                ImGui::TextWrapped("CPU must process each pixel in sequence. Even with SIMD instructions, it's painfully slow.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GPU Advantage:");
                ImGui::BulletText("'Embarrassingly parallel' problem");
                ImGui::BulletText("Each shader core handles 1 pixel");
                ImGui::BulletText("10,000+ shader cores working simultaneously");
                ImGui::TextWrapped("This is what GPUs do best - run the same program on different data in parallel.");
                ImGui::Spacing();
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Key Insight:");
                ImGui::TextWrapped("These effects would be impossible in real-time on CPU. At 1920×1080, that's 2 million pixels to calculate 60 times per second = 120 MILLION calculations/sec!");
                ImGui::TextWrapped("GPUs make this trivial because they're designed for massive parallelism.");
                break;

            case 5: // Shader Playground
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MODE: Custom Shader Programming");
                ImGui::Separator();
                
                ImGui::TextWrapped("This mode loads custom .glsl shader files and executes them on the CPU to demonstrate shader programming concepts.");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "How It Works:");
                ImGui::BulletText("Load .glsl files from shaders/ folder");
                ImGui::BulletText("Each pixel runs the shader function");
                ImGui::BulletText("4 adjustable parameters for live tweaking");
                ImGui::BulletText("Access to: position (x,y), time, params");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CPU-Only (Educational):");
                ImGui::BulletText("This mode runs ONLY on CPU");
                ImGui::BulletText("Each pixel calculated sequentially");
                ImGui::BulletText("No GPU option available (by design!)");
                ImGui::TextWrapped("Why? To show you exactly how slow shader code is when not parallelized. This is what GPUs were invented to solve!");
                ImGui::Spacing();
                
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Real GPU Shaders:");
                ImGui::BulletText("In real GPUs, this SAME code runs on 1000s of cores");
                ImGui::BulletText("All pixels calculated simultaneously");
                ImGui::BulletText("50-1000x faster than what you see here");
                ImGui::TextWrapped("Modern GPUs have 2000-10000+ shader cores. Each one runs your shader code on a different pixel at the same time!");
                ImGui::Spacing();
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shader Programming Concepts:");
                ImGui::BulletText("Pixel shader = program that runs per pixel");
                ImGui::BulletText("Uniforms = parameters (time, mouse, etc.)");
                ImGui::BulletText("Varying = interpolated values across surface");
                ImGui::BulletText("No state = each pixel is independent");
                ImGui::Spacing();
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Performance Impact:");
                ImGui::Text("At 800×600 resolution:");
                ImGui::BulletText("480,000 shader executions per frame!");
                ImGui::BulletText("At 60 FPS = 28.8 MILLION per second!");
                ImGui::TextWrapped("This is why GPUs are absolutely essential for real-time graphics. Watch your FPS drop as shaders get more complex!");
                ImGui::Spacing();
                
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Try This:");
                ImGui::BulletText("Load plasma.glsl - see how sin() affects FPS");
                ImGui::BulletText("Load mandelbrot.glsl - iterative loops = SLOW");
                ImGui::BulletText("Load voronoi.glsl - nested loops = VERY SLOW");
                ImGui::BulletText("Adjust parameters and watch FPS change");
                ImGui::TextWrapped("Complex shaders that run at 1000 FPS on GPU might only run at 5-10 FPS here. That's the power of parallel processing!");
                break;
        }
        
        ImGui::End();
    }
}
