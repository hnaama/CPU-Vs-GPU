# Software Renderer - CPU vs GPU Learning Course

This project is designed as a **hands-on educational course** for understanding the difference between **CPU rendering** and **GPU rendering**.  
You can run the same kinds of graphics workloads in different modes, switch CPU/GPU paths in the UI, and directly observe why modern graphics relies on massive parallelism.

## What this course teaches

1. **How software rasterization works** (pixel-level rendering on CPU)
2. **Where CPU pipelines bottleneck** (sequential per-pixel/per-triangle workloads)
3. **Why GPU architecture wins** for graphics (parallel processing)
4. **How shader workflows compare** across CPU simulation, SDL GPU path, and real OpenGL shaders
5. **How complexity affects FPS** in fractals, OBJ meshes, and full-screen effects

## Implemented modes (what you can test now)

| Mode | What it demonstrates | CPU vs GPU lesson |
|------|----------------------|-------------------|
| Weird Entities | Dynamic triangle morphing and rasterization | Parallel triangle work scales better on GPU |
| Fractals | Mandelbrot / Julia / Burning Ship style workloads | Independent pixel loops heavily favor GPU |
| Bouncing Balls | Physics + rendering split | CPU is strong for logic; GPU helps rendering throughput |
| OBJ Viewer | 3D transforms + rasterization/depth | Model rendering is a core GPU use case |
| GPU Demo | Progressive triangle/pixel stress tests | Clear visual proof of CPU limits vs GPU parallelism |
| Shader Playground | Custom `.glsl` shaders with params | Same shader idea: slow on CPU, fast on GPU/OpenGL |

## Course documentation (Markdown explanations)

Use these docs as the guided lessons:

- **[CPU_VS_GPU.md](CPU_VS_GPU.md)** - architecture differences and why GPUs exist
- **[RASTERIZATION.md](RASTERIZATION.md)** - how triangles become pixels
- **[OBJ_RENDERING.md](OBJ_RENDERING.md)** - OBJ pipeline and 3D model rendering path
- **[SHADER_PLAYGROUND.md](SHADER_PLAYGROUND.md)** - shader system, CPU/GPU execution model, performance
- **[TECHNICAL_DOCS.md](TECHNICAL_DOCS.md)** - core renderer internals and math pipeline

## Screenshots

![Software Renderer screenshot 1](<screenshots/Screenshot 2026-04-20 at 15.56.39.png>)
![Software Renderer screenshot 2](<screenshots/Screenshot 2026-04-20 at 15.57.05.png>)
![Software Renderer screenshot 3](<screenshots/Screenshot 2026-04-20 at 15.58.02.png>)
![Software Renderer screenshot 4](<screenshots/Screenshot 2026-04-20 at 15.58.18.png>)
![Software Renderer screenshot 5](<screenshots/Screenshot 2026-04-20 at 15.58.27.png>)
![Software Renderer screenshot 6](<screenshots/Screenshot 2026-04-20 at 15.58.35.png>)
![Software Renderer screenshot 7](<screenshots/Screenshot 2026-04-20 at 15.58.53.png>)

## Tech stack

- C++17
- SDL2
- OpenGL 3.3 Core
- GLEW
- Dear ImGui

## Prerequisites

### macOS
```bash
brew install sdl2 glew
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install libsdl2-dev libglew-dev
```

### Linux (Fedora/CentOS)
```bash
sudo dnf install SDL2-devel glew-devel
```

### FreeBSD
```bash
sudo pkg install sdl2 glew
```

### Windows (MinGW)
- https://www.libsdl.org/download-2.0.php
- http://glew.sourceforge.net/

## Build and run

```bash
git clone https://github.com/hnaama/CPU-Vs-GPU.git
cd Software-Renderer
make
./build/software_renderer
```

## Controls

- **ESC**: Exit
- **G**: Toggle GUI
- **F / F11**: Fullscreen toggle
- **M**: Next mode
- **R**: Reset mode
- **SPACE**: New fractal (Fractal mode)

## License

See [LICENSE](LICENSE).
