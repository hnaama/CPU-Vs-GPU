# Supports: macOS, Linux, Windows (MinGW), FreeBSD

# Project configuration
PROJECT_NAME = software_renderer
SRC_DIR = src
BUILD_DIR = build
IMGUI_DIR = imgui
TARGET = $(BUILD_DIR)/$(PROJECT_NAME)

# Source files
SOURCES = $(SRC_DIR)/main.cpp \
          $(SRC_DIR)/pixelbuffer.cpp \
          $(SRC_DIR)/utils.cpp \
          $(SRC_DIR)/weird_entities.cpp \
          $(SRC_DIR)/fractals.cpp \
          $(SRC_DIR)/fractal_system.cpp \
          $(SRC_DIR)/bouncing_balls.cpp \
          $(SRC_DIR)/obj_loader.cpp \
          $(SRC_DIR)/gpu_demo.cpp \
          $(SRC_DIR)/gui.cpp \
          $(SRC_DIR)/shader_renderer.cpp \
          $(SRC_DIR)/shader_playground.cpp \
          $(SRC_DIR)/gl_shader_renderer.cpp
IMGUI_SOURCES = $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_widgets.cpp $(IMGUI_DIR)/imgui_tables.cpp \
                $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer2.cpp

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
IMGUI_OBJECTS = $(BUILD_DIR)/imgui.o $(BUILD_DIR)/imgui_draw.o $(BUILD_DIR)/imgui_widgets.o $(BUILD_DIR)/imgui_tables.o \
                $(BUILD_DIR)/imgui_impl_sdl2.o $(BUILD_DIR)/imgui_impl_sdlrenderer2.o
ALL_OBJECTS = $(OBJECTS) $(IMGUI_OBJECTS)

# Compiler settings
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -I$(SRC_DIR) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -DNDEBUG

# Detect operating system
UNAME_S := $(shell uname -s 2>/dev/null || echo "Windows")
UNAME_M := $(shell uname -m 2>/dev/null || echo "unknown")

# Initialize variables
SDL_CFLAGS =
SDL_LIBS =
PLATFORM_FLAGS =
LDFLAGS = -lSDL2 -lGLEW -framework OpenGL
EXECUTABLE_EXT =

# ============================================================================
# Platform-specific configuration
# ============================================================================

# macOS Configuration
ifeq ($(UNAME_S), Darwin)
    PLATFORM = macOS
    
    # Try Homebrew first (most common on macOS)
    ifneq ($(shell which brew 2>/dev/null),)
        HOMEBREW_PREFIX := $(shell brew --prefix 2>/dev/null)
        ifneq ($(HOMEBREW_PREFIX),)
            # Check if SDL2 is installed via Homebrew
            ifneq ($(shell brew list sdl2 2>/dev/null),)
                SDL_CFLAGS = -I$(HOMEBREW_PREFIX)/include/SDL2
                SDL_LIBS = -L$(HOMEBREW_PREFIX)/lib -lSDL2 -lSDL2main
                SDL_FOUND = 1
            endif
        endif
    endif
    
    # Try MacPorts if Homebrew didn't work
    ifeq ($(SDL_FOUND),)
        ifneq ($(shell which port 2>/dev/null),)
            ifneq ($(wildcard /opt/local/include/SDL2),)
                SDL_CFLAGS = -I/opt/local/include/SDL2
                SDL_LIBS = -L/opt/local/lib -lSDL2 -lSDL2main
                SDL_FOUND = 1
            endif
        endif
    endif
    
    # Try system installation
    ifeq ($(SDL_FOUND),)
        ifneq ($(wildcard /usr/local/include/SDL2),)
            SDL_CFLAGS = -I/usr/local/include/SDL2
            SDL_LIBS = -L/usr/local/lib -lSDL2 -lSDL2main
            SDL_FOUND = 1
        endif
    endif
    
    # macOS specific linker flags (not compilation flags)
    LDFLAGS += -framework Cocoa -framework IOKit -framework CoreVideo
    
# Linux Configuration
else ifeq ($(UNAME_S), Linux)
    PLATFORM = Linux
    
    # Try pkg-config first (standard on Linux)
    ifneq ($(shell which pkg-config 2>/dev/null),)
        ifneq ($(shell pkg-config --exists sdl2 && echo "yes"),)
            SDL_CFLAGS = $(shell pkg-config --cflags sdl2)
            SDL_LIBS = $(shell pkg-config --libs sdl2)
            SDL_FOUND = 1
        endif
    endif
    
    # Try common Linux paths
    ifeq ($(SDL_FOUND),)
        # Ubuntu/Debian
        ifneq ($(wildcard /usr/include/SDL2),)
            SDL_CFLAGS = -I/usr/include/SDL2
            SDL_LIBS = -lSDL2 -lSDL2main
            SDL_FOUND = 1
        # Fedora/CentOS
        else ifneq ($(wildcard /usr/local/include/SDL2),)
            SDL_CFLAGS = -I/usr/local/include/SDL2
            SDL_LIBS = -L/usr/local/lib -lSDL2 -lSDL2main
            SDL_FOUND = 1
        endif
    endif
    
    LDFLAGS += -lpthread -ldl

# Windows Configuration (MinGW)
else ifeq ($(UNAME_S), MINGW32_NT)
    PLATFORM = Windows_MinGW32
    EXECUTABLE_EXT = .exe
    include Makefile.windows
else ifeq ($(UNAME_S), MINGW64_NT)
    PLATFORM = Windows_MinGW64  
    EXECUTABLE_EXT = .exe
    include Makefile.windows
else ifneq ($(findstring MINGW,$(UNAME_S)),)
    PLATFORM = Windows_MinGW
    EXECUTABLE_EXT = .exe
    include Makefile.windows
else ifneq ($(findstring MSYS,$(UNAME_S)),)
    PLATFORM = Windows_MSYS
    EXECUTABLE_EXT = .exe
    include Makefile.windows

# FreeBSD Configuration
else ifeq ($(UNAME_S), FreeBSD)
    PLATFORM = FreeBSD
    
    # Try pkg-config
    ifneq ($(shell which pkg-config 2>/dev/null),)
        ifneq ($(shell pkg-config --exists sdl2 && echo "yes"),)
            SDL_CFLAGS = $(shell pkg-config --cflags sdl2)
            SDL_LIBS = $(shell pkg-config --libs sdl2)
            SDL_FOUND = 1
        endif
    endif
    
    # Try FreeBSD ports
    ifeq ($(SDL_FOUND),)
        ifneq ($(wildcard /usr/local/include/SDL2),)
            SDL_CFLAGS = -I/usr/local/include/SDL2
            SDL_LIBS = -L/usr/local/lib -lSDL2 -lSDL2main
            SDL_FOUND = 1
        endif
    endif
    
    LDFLAGS += -lpthread

# Unknown platform
else
    PLATFORM = Unknown
    $(warning Unknown platform: $(UNAME_S))
endif

# ============================================================================
# Detect GLEW
# ============================================================================

ifeq ($(UNAME_S), Darwin)
    # macOS - check Homebrew first
    GLEW_HOMEBREW := $(shell brew --prefix glew 2>/dev/null)
    ifneq ($(GLEW_HOMEBREW),)
        $(info ✓ GLEW Found (Homebrew))
        $(info   Include: $(GLEW_HOMEBREW)/include)
        CFLAGS += -I$(GLEW_HOMEBREW)/include
        SDL_CFLAGS += -I$(GLEW_HOMEBREW)/include
        LDFLAGS += -L$(GLEW_HOMEBREW)/lib -lGLEW -framework OpenGL
    else
        $(warning ⚠ GLEW not found via Homebrew. Install with: brew install glew)
    endif
else ifeq ($(UNAME_S), Linux)
    # Linux - use pkg-config
    GLEW_CFLAGS := $(shell pkg-config --cflags glew 2>/dev/null)
    GLEW_LIBS := $(shell pkg-config --libs glew 2>/dev/null)
    ifneq ($(GLEW_LIBS),)
        $(info ✓ GLEW Found)
        CFLAGS += $(GLEW_CFLAGS)
        SDL_CFLAGS += $(GLEW_CFLAGS)
        LDFLAGS += $(GLEW_LIBS) -lGL
    else
        $(warning ⚠ GLEW not found. Install with: sudo apt-get install libglew-dev)
    endif
else ifeq ($(UNAME_S), FreeBSD)
    # FreeBSD
    SDL_CFLAGS += -I/usr/local/include
    LDFLAGS += -L/usr/local/lib -lGLEW -lGL
else
    # Windows/MinGW
    LDFLAGS += -lglew32 -lopengl32
endif

# ============================================================================
# Finalize configuration
# ============================================================================

# Add executable extension
TARGET := $(TARGET)$(EXECUTABLE_EXT)

# Combine all flags
ALL_CXXFLAGS = $(CXXFLAGS) $(SDL_CFLAGS) $(PLATFORM_FLAGS)
ALL_LIBS = $(SDL_LIBS) $(LDFLAGS)

# Build mode selection
ifdef DEBUG
    ALL_CXXFLAGS += $(DEBUG_FLAGS)
    BUILD_MODE = Debug
else
    ALL_CXXFLAGS += $(RELEASE_FLAGS)  
    BUILD_MODE = Release
endif

# ============================================================================
# Targets
# ============================================================================

.PHONY: all clean fclean re install uninstall info debug release help

# Default target
all: info $(BUILD_DIR) $(TARGET)

# Show configuration information
info:
	@echo "=========================================="
	@echo "Software Renderer Build Configuration"
	@echo "=========================================="
	@echo "Platform:     $(PLATFORM) ($(UNAME_M))"
	@echo "Build Mode:   $(BUILD_MODE)"
	@echo "Compiler:     $(CXX)"
	@echo "Target:       $(TARGET)"
	@echo "Build Dir:    $(BUILD_DIR)"
	@echo "=========================================="
ifdef SDL_FOUND
	@echo "✓ SDL2 Found"
	@echo "  CFLAGS:     $(SDL_CFLAGS)"
	@echo "  LIBS:       $(SDL_LIBS)"
else
	@echo "✗ SDL2 NOT FOUND"
	@echo ""
	@echo "Please install SDL2:"
ifeq ($(PLATFORM), macOS)
	@echo "  brew install sdl2"
	@echo "  # or"  
	@echo "  sudo port install libsdl2"
else ifeq ($(PLATFORM), Linux)
	@echo "  # Ubuntu/Debian:"
	@echo "  sudo apt-get install libsdl2-dev"
	@echo "  # Fedora/CentOS:"
	@echo "  sudo dnf install SDL2-devel"
	@echo "  # Arch Linux:"
	@echo "  sudo pacman -S sdl2"
else ifeq ($(findstring Windows,$(PLATFORM)), Windows)
	@echo "  # Download SDL2 development libraries from:"
	@echo "  # https://www.libsdl.org/download-2.0.php"
else ifeq ($(PLATFORM), FreeBSD)
	@echo "  sudo pkg install sdl2"
endif
	@echo ""
	@$(error SDL2 is required to build this project)
endif
	@echo "=========================================="

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
ifdef SDL_FOUND
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(ALL_CXXFLAGS) -c $< -o $@
else
	@$(error SDL2 not found. Run 'make info' for installation instructions)
endif

# Compile ImGui core files
$(BUILD_DIR)/imgui.o: $(IMGUI_DIR)/imgui.cpp
ifdef SDL_FOUND
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(ALL_CXXFLAGS) -c $< -o $@
else
	@$(error SDL2 not found. Run 'make info' for installation instructions)
endif

$(BUILD_DIR)/imgui_draw.o: $(IMGUI_DIR)/imgui_draw.cpp
ifdef SDL_FOUND
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(ALL_CXXFLAGS) -c $< -o $@
else
	@$(error SDL2 not found. Run 'make info' for installation instructions)
endif

$(BUILD_DIR)/imgui_widgets.o: $(IMGUI_DIR)/imgui_widgets.cpp
ifdef SDL_FOUND
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(ALL_CXXFLAGS) -c $< -o $@
else
	@$(error SDL2 not found. Run 'make info' for installation instructions)
endif

$(BUILD_DIR)/imgui_tables.o: $(IMGUI_DIR)/imgui_tables.cpp
ifdef SDL_FOUND
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(ALL_CXXFLAGS) -c $< -o $@
else
	@$(error SDL2 not found. Run 'make info' for installation instructions)
endif

# Compile ImGui backend files
$(BUILD_DIR)/imgui_impl_sdl2.o: $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp
ifdef SDL_FOUND
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(ALL_CXXFLAGS) -c $< -o $@
else
	@$(error SDL2 not found. Run 'make info' for installation instructions)
endif

$(BUILD_DIR)/imgui_impl_sdlrenderer2.o: $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer2.cpp
ifdef SDL_FOUND
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(ALL_CXXFLAGS) -c $< -o $@
else
	@$(error SDL2 not found. Run 'make info' for installation instructions)
endif

# Link executable
$(TARGET): $(ALL_OBJECTS)
ifdef SDL_FOUND
	@echo "Linking $(TARGET)..."
	@$(CXX) $(ALL_OBJECTS) $(ALL_LIBS) -o $@
	@echo "Build complete: $(TARGET)"
else
	@$(error SDL2 not found. Run 'make info' for installation instructions)
endif

# Debug build
debug:
	@$(MAKE) DEBUG=1 all

# Release build (default)
release:
	@$(MAKE) all

# Run the program
run: $(TARGET)
	@echo "Running $(TARGET)..."
	@./$(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Full clean - remove build directory and executable
fclean: clean
	@echo "Removing executable..."
	@rm -f $(TARGET)
	@echo "Full clean complete."

# Rebuild everything from scratch
re: fclean all

# Install (copy to /usr/local/bin on Unix-like systems)
install: $(TARGET)
ifeq ($(findstring Windows,$(PLATFORM)), Windows)
	@echo "Install not implemented for Windows. Copy $(TARGET) to desired location."
else
	@echo "Installing $(PROJECT_NAME) to /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/$(PROJECT_NAME)
	@echo "Installation complete."
endif

# Uninstall
uninstall:
ifeq ($(findstring Windows,$(PLATFORM)), Windows)
	@echo "Uninstall not implemented for Windows."
else
	@echo "Removing $(PROJECT_NAME) from /usr/local/bin..."
	@sudo rm -f /usr/local/bin/$(PROJECT_NAME)
	@echo "Uninstall complete."
endif

# Help
help:
	@echo "Available targets:"
	@echo "  all        - Build the project (default)"
	@echo "  debug      - Build with debug flags"
	@echo "  release    - Build with release flags"
	@echo "  run        - Build and run the program"
	@echo "  clean      - Remove build artifacts"
	@echo "  fclean     - Full clean (remove build directory and executable)"
	@echo "  re         - Rebuild everything from scratch"
	@echo "  install    - Install to system (Unix-like only)"
	@echo "  uninstall  - Remove from system (Unix-like only)"
	@echo "  info       - Show build configuration"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Build modes:"
	@echo "  make              # Release build"
	@echo "  make DEBUG=1      # Debug build"
	@echo "  make debug        # Debug build (shorthand)"

# Dependency tracking
-include $(ALL_OBJECTS:.o=.d)

# Generate dependency files
$(BUILD_DIR)/%.d: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@$(CXX) $(ALL_CXXFLAGS) -MM -MT '$(BUILD_DIR)/$*.o' $< > $@