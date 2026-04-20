# Software Renderer - Technical Documentation

## Overview

This is a **pure software 3D renderer** written in C++ that renders graphics entirely on the CPU without using GPU acceleration (like OpenGL or DirectX). It demonstrates the fundamental concepts of computer graphics from scratch.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Components](#core-components)
3. [The Rendering Pipeline](#the-rendering-pipeline)
4. [OBJ File Loading](#obj-file-loading)
5. [3D Mathematics](#3d-mathematics)
6. [Rasterization](#rasterization)
7. [Visual Modes](#visual-modes)

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         main.cpp                                 │
│                    (Application Loop)                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │  OBJ Loader  │  │ Weird Entities│  │ Fractal System      │  │
│  │ (obj_loader) │  │(weird_entities)│  │ (fractal_system)    │  │
│  └──────┬───────┘  └──────┬───────┘  └──────────┬───────────┘  │
│         │                 │                      │               │
│         └─────────────────┴──────────────────────┘               │
│                           │                                      │
│                           ▼                                      │
│                  ┌─────────────────┐                            │
│                  │   PixelBuffer   │                            │
│                  │ (Rasterization) │                            │
│                  └────────┬────────┘                            │
│                           │                                      │
│                           ▼                                      │
│                  ┌─────────────────┐                            │
│                  │   SDL2 Window   │                            │
│                  │   (Display)     │                            │
│                  └─────────────────┘                            │
└─────────────────────────────────────────────────────────────────┘
```

### File Structure

| File | Purpose |
|------|---------|
| `main.cpp` | Application entry point, event loop, mode switching |
| `utils.h/cpp` | Math utilities (Vec3, Matrix4x4, Triangle3D) |
| `pixelbuffer.h/cpp` | Software rasterizer, triangle filling, line drawing |
| `obj_loader.h/cpp` | Wavefront OBJ file parser |
| `fractals.h/cpp` | Fractal computation functions |
| `fractal_system.h/cpp` | Cellular automaton + fractal visualization |
| `weird_entities.h/cpp` | Procedural 3D entity generation |

---

## Core Components

### 1. Vec3 - 3D Vector

The fundamental building block for 3D graphics.

```cpp
struct Vec3 {
    float x, y, z;
    
    // Operations
    Vec3 operator+(const Vec3& other);  // Vector addition
    Vec3 operator-(const Vec3& other);  // Vector subtraction
    Vec3 operator*(float scalar);       // Scalar multiplication
    float dot(const Vec3& other);       // Dot product
    Vec3 cross(const Vec3& other);      // Cross product
    Vec3 normalize();                   // Unit vector
    float length();                     // Magnitude
};
```

**Example Usage:**
```cpp
Vec3 a(1, 0, 0);  // X-axis unit vector
Vec3 b(0, 1, 0);  // Y-axis unit vector
Vec3 c = a.cross(b);  // Result: (0, 0, 1) - Z-axis
```

### 2. Matrix4x4 - Transformation Matrix

A 4x4 matrix for 3D transformations (translation, rotation, scaling, projection).

```cpp
struct Matrix4x4 {
    float m[4][4];
    
    // Factory methods
    static Matrix4x4 identity();
    static Matrix4x4 translation(float x, float y, float z);
    static Matrix4x4 rotationX(float angle);
    static Matrix4x4 rotationY(float angle);
    static Matrix4x4 rotationZ(float angle);
    static Matrix4x4 scale(float sx, float sy, float sz);
    static Matrix4x4 perspective(float fov, float aspect, float near, float far);
    
    // Operations
    Matrix4x4 operator*(const Matrix4x4& other);  // Matrix multiplication
    Vec3 transform(const Vec3& v);                // Transform a point
};
```

**How Perspective Projection Works:**

```
                    Near Plane    Far Plane
       Camera          │             │
          ◉───────────▶│             │
         /│\           │             │
        / │ \          │    Object   │
       /  │  \         │      ●      │
      /   │   \        │             │
     /    │    \       │             │
    ──────┴──────      │             │
      View Frustum
```

The perspective matrix transforms 3D world coordinates into 2D screen coordinates, making distant objects appear smaller.

### 3. Triangle3D - The Basic Rendering Primitive

```cpp
struct Triangle3D {
    Vec3 vertices[3];   // Three corner points
    uint32_t colors[3]; // Color at each vertex (for gradient)
    
    Vec3 getNormal();                           // Surface normal
    Triangle3D transform(const Matrix4x4& m);   // Apply transformation
};
```

---

## The Rendering Pipeline

The rendering pipeline transforms 3D objects into 2D pixels on screen:

```
┌─────────────────┐
│  1. VERTICES    │  Raw 3D coordinates from OBJ file
│  (Model Space)  │  e.g., (0.23, -1.21, 1.13)
└────────┬────────┘
         │
         ▼ Model Matrix (rotation, scale, position)
┌─────────────────┐
│  2. WORLD SPACE │  Object positioned in 3D world
└────────┬────────┘
         │
         ▼ View Matrix (camera position/orientation)
┌─────────────────┐
│  3. VIEW SPACE  │  Relative to camera
│  (Camera Space) │
└────────┬────────┘
         │
         ▼ Projection Matrix (perspective)
┌─────────────────┐
│  4. CLIP SPACE  │  Normalized device coordinates
│     (NDC)       │  Range: -1 to +1
└────────┬────────┘
         │
         ▼ Viewport Transform
┌─────────────────┐
│ 5. SCREEN SPACE │  Actual pixel coordinates
│    (2D)         │  e.g., (400, 300)
└────────┬────────┘
         │
         ▼ Rasterization
┌─────────────────┐
│  6. PIXELS      │  Final colored pixels
│  (Framebuffer)  │
└─────────────────┘
```

### Pipeline in Code

```cpp
// 1. Set up transformation matrices
Matrix4x4 projection = Matrix4x4::perspective(fov, aspect, 0.1f, 100.0f);
Matrix4x4 rotX = Matrix4x4::rotationX(angleX);
Matrix4x4 rotY = Matrix4x4::rotationY(angleY);
Matrix4x4 translation = Matrix4x4::translation(0, 0, -distance);

// 2. Combine transformations (order matters!)
Matrix4x4 modelView = translation * rotX * rotY;
Matrix4x4 mvp = projection * modelView;

// 3. Transform each triangle
Triangle3D transformed = triangle.transform(mvp);

// 4. Backface culling (skip triangles facing away)
Vec3 normal = transformed.getNormal();
if (normal.z > 0) {
    // 5. Rasterize (convert to pixels)
    pixelBuffer.render3DTriangle(transformed, width, height);
}
```

---

## OBJ File Loading

### OBJ File Format

The Wavefront OBJ format is a simple text-based 3D model format:

```obj
# Comment
v 0.232406 -1.216630 1.133818    # Vertex position (x, y, z)
v 0.232406 -0.745504 2.843098
vn 0.0 1.0 0.0                    # Vertex normal (nx, ny, nz)
vt 0.5 0.5                        # Texture coordinate (u, v)
f 1 2 3                           # Face using vertex indices
f 1/1/1 2/2/1 3/3/1               # Face with vertex/texture/normal
```

### Parsing Process

```cpp
bool ObjLoader::load(const std::string& filename, Mesh& mesh) {
    std::ifstream file(filename);
    std::string line;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            // Parse vertex: v x y z
            float x, y, z;
            iss >> x >> y >> z;
            mesh.vertices.push_back(Vec3(x, y, z));
        }
        else if (prefix == "f") {
            // Parse face: f v1 v2 v3 ... (can be polygon)
            std::vector<int> faceVertices;
            // ... parse vertex indices (1-based in OBJ)
            mesh.faces.push_back(faceVertices);
        }
    }
    
    mesh.centerAndNormalize();  // Fit to unit cube
    return true;
}
```

### Mesh Normalization

After loading, the mesh is centered and scaled to fit in a unit cube:

```cpp
void Mesh::centerAndNormalize() {
    // 1. Find bounding box
    Vec3 min, max;
    for (auto& v : vertices) {
        min = Vec3::min(min, v);
        max = Vec3::max(max, v);
    }
    
    // 2. Calculate center and size
    Vec3 center = (min + max) * 0.5f;
    float size = std::max({max.x-min.x, max.y-min.y, max.z-min.z});
    
    // 3. Transform vertices to fit in [-1, 1] range
    for (auto& v : vertices) {
        v = (v - center) / size * 2.0f;
    }
}
```

---

## 3D Mathematics

### Coordinate Systems

```
        Y (up)
        │
        │
        │
        └───────── X (right)
       /
      /
     Z (towards viewer)
```

### Rotation Matrices

**Rotation around X-axis:**
```
┌ 1    0        0    ┐
│ 0    cos(θ)  -sin(θ)│
└ 0    sin(θ)   cos(θ)┘
```

**Rotation around Y-axis:**
```
┌  cos(θ)  0   sin(θ) ┐
│    0     1     0    │
└ -sin(θ)  0   cos(θ) ┘
```

### Perspective Projection Matrix

```cpp
Matrix4x4 Matrix4x4::perspective(float fov, float aspect, float near, float far) {
    float tanHalfFov = tan(fov / 2.0f);
    
    Matrix4x4 m;
    m.m[0][0] = 1.0f / (aspect * tanHalfFov);
    m.m[1][1] = 1.0f / tanHalfFov;
    m.m[2][2] = -(far + near) / (far - near);
    m.m[2][3] = -1.0f;
    m.m[3][2] = -(2.0f * far * near) / (far - near);
    return m;
}
```

### Backface Culling

Triangles facing away from the camera are skipped for performance:

```cpp
Vec3 Triangle3D::getNormal() {
    Vec3 edge1 = vertices[1] - vertices[0];
    Vec3 edge2 = vertices[2] - vertices[0];
    return edge1.cross(edge2).normalize();
}

// If normal.z > 0, triangle faces the camera
if (normal.z > 0) {
    render(triangle);
}
```

---

## Rasterization

Rasterization converts geometric primitives (triangles) into pixels.

### Triangle Rasterization (Barycentric Method)

```cpp
void PixelBuffer::fillTriangleGradient(
    int x0, int y0, uint32_t color0,
    int x1, int y1, uint32_t color1,
    int x2, int y2, uint32_t color2
) {
    // 1. Find bounding box
    int minX = min(x0, x1, x2);
    int maxX = max(x0, x1, x2);
    int minY = min(y0, y1, y2);
    int maxY = max(y0, y1, y2);
    
    // 2. For each pixel in bounding box
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            // 3. Calculate barycentric coordinates
            float w0, w1, w2;
            computeBarycentric(x, y, x0, y0, x1, y1, x2, y2, w0, w1, w2);
            
            // 4. Check if point is inside triangle
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                // 5. Interpolate color
                Color c = color0 * w0 + color1 * w1 + color2 * w2;
                setPixel(x, y, c.toARGB());
            }
        }
    }
}
```

### Barycentric Coordinates Visualization

```
        v0 (color0)
        /\
       /  \
      /    \
     / P    \    P = w0*v0 + w1*v1 + w2*v2
    /        \   where w0 + w1 + w2 = 1
   /__________\
 v1            v2
(color1)    (color2)
```

### Depth Sorting (Painter's Algorithm)

Since we don't have a Z-buffer, we sort triangles back-to-front:

```cpp
// Collect triangles with depth
std::vector<std::pair<float, Triangle3D>> sortedTriangles;
for (auto& tri : triangles) {
    float avgZ = (tri.vertices[0].z + tri.vertices[1].z + tri.vertices[2].z) / 3.0f;
    sortedTriangles.push_back({avgZ, tri});
}

// Sort back-to-front (farthest first)
std::sort(sortedTriangles.begin(), sortedTriangles.end(),
    [](auto& a, auto& b) { return a.first > b.first; });

// Render in order
for (auto& pair : sortedTriangles) {
    render(pair.second);
}
```

---

## Visual Modes

### Mode 1: Weird Chaos (Procedural Entities)

Generates procedural 3D entities with various shapes:

```cpp
// Entity types
enum EntityType {
    SPIKY_STAR,      // Radiating spikes
    MORPHING_BLOB,   // Pulsing organic shape
    FRACTAL_SPIKES,  // Recursive fractal pattern
    TWISTED_RIBBON,  // Helical ribbon
    PULSING_ORB,     // Sphere with rings
    CHAOTIC_FRAGMENTS, // Floating pieces
    WEIRD_POLYHEDRON   // Deformed cube
};
```

### Mode 2: Fractal/Game of Life

Combines cellular automaton with fractal mathematics:

```cpp
// Cellular automaton rules
float neighbors = countNeighbors(x, y);
if (cell > 0) {
    newCell = (neighbors >= 2 && neighbors <= 3) ? cell * 1.1f : cell * 0.8f;
} else {
    newCell = (neighbors >= 2.8 && neighbors <= 3.2) ? 1.0f : 0;
}

// Blend with fractal value
float fractalValue = computeMandelbrot(fx, fy);
newCell = newCell * 0.6f + fractalValue * 0.4f;
```

### Mode 3: OBJ Viewer

Renders loaded 3D models with:
- Auto-rotation
- Manual rotation (arrow keys)
- Zoom control (+/-)
- Color randomization (C key)

---

## Color System

### HSV to RGB Conversion

Colors are often specified in HSV (Hue, Saturation, Value) for easier manipulation:

```cpp
uint32_t hsvToRgb(float h, float s, float v) {
    // h: 0-360 (color wheel position)
    // s: 0-1 (color intensity)
    // v: 0-1 (brightness)
    
    int hi = (int)(h / 60.0f) % 6;
    float f = h / 60.0f - hi;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    
    // ... convert to RGB based on hi sector
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}
```

### Color Format

Colors are stored as 32-bit ARGB values:

```
┌────────┬────────┬────────┬────────┐
│ Alpha  │  Red   │ Green  │  Blue  │
│ 8 bits │ 8 bits │ 8 bits │ 8 bits │
└────────┴────────┴────────┴────────┘
  Byte 3   Byte 2   Byte 1   Byte 0

Example: 0xFFFF0000 = Opaque Red
         0xFF00FF00 = Opaque Green
         0x80FFFFFF = 50% transparent White
```

---

## Performance Considerations

### Current Approach
- **CPU-based rendering**: All calculations done on CPU
- **Painter's algorithm**: Simple depth sorting (O(n log n))
- **Barycentric rasterization**: Accurate but not the fastest

### Potential Optimizations
1. **Z-Buffer**: Per-pixel depth testing (more accurate than sorting)
2. **Edge walking**: Scanline-based rasterization (faster)
3. **SIMD**: Use SSE/AVX for parallel pixel processing
4. **Multi-threading**: Parallel triangle processing
5. **Spatial partitioning**: Octree/BSP for culling

---

## Controls Summary

| Key | Action |
|-----|--------|
| `ESC` | Exit |
| `F11` | Toggle fullscreen |
| `M` | Cycle modes |
| `R` | Reset current mode |
| `SPACE` | Inject chaos (fractal mode) |
| `↑/↓/←/→` | Rotate model (OBJ mode) |
| `+/-` | Zoom in/out (OBJ mode) |
| `C` | Randomize colors (OBJ mode) |

---

## Building

```bash
# macOS/Linux
make

# Run normal mode
./build/software_renderer

# Run with OBJ file
./build/software_renderer path/to/model.obj
```

---

## Further Reading

- [Scratchapixel - Rasterization](https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation)
- [Learn OpenGL - Coordinate Systems](https://learnopengl.com/Getting-started/Coordinate-Systems)
- [Wavefront OBJ Format](https://en.wikipedia.org/wiki/Wavefront_.obj_file)
- [Barycentric Coordinates](https://en.wikipedia.org/wiki/Barycentric_coordinate_system)