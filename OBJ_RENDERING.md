# OBJ File Rendering Implementation

This document provides a comprehensive technical overview of the OBJ file loading, mesh processing, triangulation, and rendering pipeline in the Software Renderer.

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [OBJ File Format Support](#obj-file-format-support)
4. [Data Structures](#data-structures)
5. [File Loading Pipeline](#file-loading-pipeline)
6. [Mesh Processing](#mesh-processing)
7. [Triangulation Algorithms](#triangulation-algorithms)
8. [Rendering Pipeline](#rendering-pipeline)
9. [Color Assignment](#color-assignment)
10. [Mathematical Foundations](#mathematical-foundations)
11. [Usage Examples](#usage-examples)
12. [Limitations & Future Work](#limitations--future-work)

---

## Overview

The OBJ rendering system provides a complete pipeline for loading Wavefront OBJ files and rendering them using software rasterization. Key features include:

- **Robust OBJ parsing** with support for multiple face formats
- **Automatic mesh normalization** to fit models in a unit cube
- **Ear clipping triangulation** for proper handling of concave polygons
- **Non-coplanar face detection** with graceful fallback
- **Depth-based lighting** simulation
- **Gradient color interpolation** across triangle faces

---

## Architecture

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   OBJ File      │────▶│   ObjLoader     │────▶│     Mesh        │
│   (.obj)        │     │   (Parser)      │     │   (Storage)     │
└─────────────────┘     └─────────────────┘     └────────┬────────┘
                                                         │
                                                         ▼
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│  PixelBuffer    │◀────│  Triangle3D     │◀────│  Triangulation  │
│  (Rasterizer)   │     │  (Primitives)   │     │  (Ear Clipping) │
└─────────────────┘     └─────────────────┘     └─────────────────┘
```

### Source Files

| File | Description |
|------|-------------|
| `obj_loader.h` | Mesh and ObjLoader class declarations |
| `obj_loader.cpp` | OBJ parsing, triangulation algorithms, mesh processing |
| `utils.h/cpp` | Vec3, Matrix4x4, Triangle3D primitives |
| `pixelbuffer.h/cpp` | Software rasterization and rendering |

---

## OBJ File Format Support

### Supported Elements

| Element | Syntax | Description |
|---------|--------|-------------|
| Vertex | `v x y z` | 3D vertex position |
| Normal | `vn x y z` | Vertex normal vector |
| Face | `f v1 v2 v3 ...` | Polygon face (3+ vertices) |
| Comment | `# text` | Ignored |

### Face Format Variations

The parser handles all standard OBJ face formats:

```obj
# Vertex indices only
f 1 2 3

# Vertex/Texture coordinates
f 1/1 2/2 3/3

# Vertex/Texture/Normal
f 1/1/1 2/2/2 3/3/3

# Vertex//Normal (no texture)
f 1//1 2//2 3//3

# N-gons (polygons with more than 3 vertices)
f 1 2 3 4 5 6
```

### Index Handling

```cpp
// OBJ uses 1-based indices, converted to 0-based internally
// Negative indices are relative to current vertex count
if (vIdx > 0) {
    index = vIdx - 1;                    // Positive: convert to 0-based
} else {
    index = mesh.vertices.size() + vIdx; // Negative: relative index
}
```

---

## Data Structures

### Vec3 - 3D Vector

```cpp
struct Vec3 {
    float x, y, z;
    
    Vec3 operator+(const Vec3& other) const;
    Vec3 operator-(const Vec3& other) const;
    Vec3 operator*(float scalar) const;
    
    float dot(const Vec3& other) const;
    Vec3 cross(const Vec3& other) const;
    float length() const;
    Vec3 normalize() const;
};
```

### Mesh - 3D Model Container

```cpp
struct Mesh {
    std::vector<Vec3> vertices;           // Vertex positions
    std::vector<Vec3> normals;            // Vertex normals
    std::vector<std::vector<int>> faces;  // Face vertex indices (n-gons)
    std::vector<std::vector<int>> faceNormals; // Face normal indices
    std::vector<uint32_t> faceColors;     // Per-face colors (ARGB)
    
    Vec3 center;   // Bounding box center
    float scale;   // Bounding box max dimension
    
    void calculateBounds();
    void centerAndNormalize();
    std::vector<Triangle3D> toTriangles() const;
};
```

### Triangle3D - Renderable Primitive

```cpp
struct Triangle3D {
    Vec3 vertices[3];      // Three corner positions
    uint32_t colors[3];    // Per-vertex colors (ARGB)
    
    Vec3 getNormal() const;
    Triangle3D transform(const Matrix4x4& matrix) const;
};
```

---

## File Loading Pipeline

### ObjLoader::load()

```cpp
bool ObjLoader::load(const std::string& filename, Mesh& mesh);
```

**Process Flow:**

```
1. Open file and validate
        │
        ▼
2. Parse line-by-line
   ├── "v"  → Add vertex to mesh.vertices
   ├── "vn" → Add normal to mesh.normals (normalized)
   ├── "f"  → Parse face indices, add to mesh.faces
   └── "#"  → Skip comments
        │
        ▼
3. Center and normalize mesh
        │
        ▼
4. Assign colors to faces
        │
        ▼
5. Return success
```

**Face Parsing Logic:**

```cpp
while (iss >> vertexData) {
    // Replace '/' with ' ' for easy parsing
    std::replace(vertexData.begin(), vertexData.end(), '/', ' ');
    std::istringstream viss(vertexData);
    
    int vIdx = 0, vtIdx = 0, vnIdx = 0;
    viss >> vIdx;           // Vertex index (required)
    
    if (viss.peek() != EOF)
        viss >> vtIdx;      // Texture coord index (optional)
    
    if (viss.peek() != EOF)
        viss >> vnIdx;      // Normal index (optional)
    
    // Convert 1-based to 0-based, handle negative indices
    faceVertices.push_back(vIdx > 0 ? vIdx - 1 : vertices.size() + vIdx);
}
```

---

## Mesh Processing

### Bounding Box Calculation

```cpp
void Mesh::calculateBounds() {
    Vec3 minBound = vertices[0];
    Vec3 maxBound = vertices[0];
    
    for (const auto& v : vertices) {
        minBound.x = std::min(minBound.x, v.x);
        minBound.y = std::min(minBound.y, v.y);
        minBound.z = std::min(minBound.z, v.z);
        maxBound.x = std::max(maxBound.x, v.x);
        maxBound.y = std::max(maxBound.y, v.y);
        maxBound.z = std::max(maxBound.z, v.z);
    }
    
    // Center is midpoint of bounding box
    center = (minBound + maxBound) * 0.5f;
    
    // Scale is the largest dimension
    scale = std::max({
        maxBound.x - minBound.x,
        maxBound.y - minBound.y,
        maxBound.z - minBound.z
    });
}
```

### Mesh Normalization

All loaded meshes are automatically centered at origin and scaled to fit in a [-1, 1] cube:

```cpp
void Mesh::centerAndNormalize() {
    calculateBounds();
    
    for (auto& v : vertices) {
        v.x = (v.x - center.x) / scale * 2.0f;
        v.y = (v.y - center.y) / scale * 2.0f;
        v.z = (v.z - center.z) / scale * 2.0f;
    }
    
    center = Vec3(0, 0, 0);
    scale = 1.0f;
}
```

**Why Normalize?**
- Ensures consistent rendering regardless of original model scale
- Simplifies camera and projection setup
- Models from different sources work without manual adjustment

---

## Triangulation Algorithms

### Overview

OBJ files can contain n-gon faces (polygons with any number of vertices). The renderer only handles triangles, so polygons must be triangulated.

```
┌─────────────────────────────────────────────────────────────┐
│                    Triangulation Decision                   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌─────────────────┐
                    │  Face has 3     │───Yes──▶ Return as-is
                    │  vertices?      │
                    └────────┬────────┘
                             │ No
                             ▼
                    ┌─────────────────┐
                    │  Calculate      │
                    │  face normal    │
                    │  (Newell's)     │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │  Face is        │───No──▶ Fan Triangulation
                    │  coplanar?      │         (with warning)
                    └────────┬────────┘
                             │ Yes
                             ▼
                    ┌─────────────────┐
                    │  Ear Clipping   │
                    │  Triangulation  │
                    └─────────────────┘
```

### Newell's Method for Normal Calculation

Computes a robust normal for any polygon (convex or concave):

```cpp
Vec3 Mesh::calculateFaceNormal(
    const std::vector<int>& faceIndices,
    const std::vector<Vec3>& vertices) 
{
    Vec3 normal(0, 0, 0);
    
    for (size_t i = 0; i < faceIndices.size(); i++) {
        const Vec3& current = vertices[faceIndices[i]];
        const Vec3& next = vertices[faceIndices[(i + 1) % faceIndices.size()]];
        
        // Newell's formula
        normal.x += (current.y - next.y) * (current.z + next.z);
        normal.y += (current.z - next.z) * (current.x + next.x);
        normal.z += (current.x - next.x) * (current.y + next.y);
    }
    
    return normal.normalize();
}
```

**Newell's Method Formula:**
```
Nx = Σ (Yi - Yi+1)(Zi + Zi+1)
Ny = Σ (Zi - Zi+1)(Xi + Xi+1)
Nz = Σ (Xi - Xi+1)(Yi + Yi+1)
```

### Coplanarity Check

Detects if all face vertices lie on the same plane:

```cpp
bool Mesh::isFaceCoplanar(
    const std::vector<int>& faceIndices,
    const std::vector<Vec3>& vertices,
    float tolerance) 
{
    if (faceIndices.size() <= 3) return true;  // Triangles always coplanar
    
    // Calculate plane from first 3 vertices
    Vec3 p0 = vertices[faceIndices[0]];
    Vec3 v1 = vertices[faceIndices[1]] - p0;
    Vec3 v2 = vertices[faceIndices[2]] - p0;
    Vec3 normal = v1.cross(v2).normalize();
    
    // Check remaining vertices against plane
    for (size_t i = 3; i < faceIndices.size(); i++) {
        Vec3 diff = vertices[faceIndices[i]] - p0;
        float dist = std::abs(normal.dot(diff));
        
        if (dist > tolerance) {
            return false;  // Vertex off-plane
        }
    }
    
    return true;
}
```

### Ear Clipping Algorithm

The ear clipping algorithm correctly triangulates any simple polygon, including concave shapes:

```
    ┌───────────────────────────────────────────────────────┐
    │                  Ear Clipping Process                 │
    └───────────────────────────────────────────────────────┘
    
    Original Concave Polygon:          After finding ears:
    
         2                                  2
        /│\                               /│ 
       / │ \                             / │  
      /  │  \                           /  │   
     1   │   3                         1   │   3  ← Ear (removed)
      \  │  /                           \  │  
       \ │ /                             \ │ 
        \│/                               \│
         4                                 4
    
    An "ear" is a triangle formed by three consecutive vertices where:
    1. The middle vertex is convex (turns the same way as face normal)
    2. No other polygon vertices are inside the triangle
```

**Implementation:**

```cpp
std::vector<std::array<int, 3>> Mesh::earClipTriangulate(
    const std::vector<int>& faceIndices,
    const std::vector<Vec3>& vertices,
    const Vec3& normal) 
{
    std::vector<std::array<int, 3>> triangles;
    std::vector<int> polygon = faceIndices;  // Working copy
    
    while (polygon.size() > 3) {
        bool earFound = false;
        
        for (size_t i = 0; i < polygon.size(); i++) {
            if (isEar(polygon, i, vertices, normal)) {
                int n = polygon.size();
                int prev = (i - 1 + n) % n;
                int next = (i + 1) % n;
                
                // Add ear triangle
                triangles.push_back({
                    polygon[prev],
                    polygon[i],
                    polygon[next]
                });
                
                // Remove ear vertex
                polygon.erase(polygon.begin() + i);
                earFound = true;
                break;
            }
        }
        
        if (!earFound) {
            // Fallback to fan triangulation for degenerate cases
            break;
        }
    }
    
    // Add final triangle
    if (polygon.size() == 3) {
        triangles.push_back({polygon[0], polygon[1], polygon[2]});
    }
    
    return triangles;
}
```

### Ear Detection

```cpp
bool Mesh::isEar(
    const std::vector<int>& polygon,
    int earIndex,
    const std::vector<Vec3>& vertices,
    const Vec3& normal) 
{
    int n = polygon.size();
    int prevIdx = (earIndex - 1 + n) % n;
    int nextIdx = (earIndex + 1) % n;
    
    Vec3 prev = vertices[polygon[prevIdx]];
    Vec3 curr = vertices[polygon[earIndex]];
    Vec3 next = vertices[polygon[nextIdx]];
    
    // 1. Must be a convex vertex
    if (!isConvexVertex(prev, curr, next, normal)) {
        return false;
    }
    
    // 2. No other vertices inside the potential ear triangle
    for (int i = 0; i < n; i++) {
        if (i == prevIdx || i == earIndex || i == nextIdx) continue;
        
        if (pointInTriangle(vertices[polygon[i]], prev, curr, next, normal)) {
            return false;
        }
    }
    
    return true;
}
```

### Convexity Test

```cpp
bool Mesh::isConvexVertex(
    const Vec3& prev, const Vec3& curr, const Vec3& next,
    const Vec3& normal) 
{
    Vec3 edge1 = curr - prev;
    Vec3 edge2 = next - curr;
    Vec3 cross = edge1.cross(edge2);
    
    // Convex if cross product aligns with face normal
    return cross.dot(normal) >= 0;
}
```

### Point-in-Triangle Test

Uses barycentric coordinates with 2D projection for robustness:

```cpp
bool Mesh::pointInTriangle(
    const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c,
    const Vec3& normal) 
{
    // Project onto 2D plane (drop axis with largest normal component)
    int axis1, axis2;
    float absX = std::abs(normal.x);
    float absY = std::abs(normal.y);
    float absZ = std::abs(normal.z);
    
    if (absZ >= absX && absZ >= absY) {
        axis1 = 0; axis2 = 1;  // Project onto XY
    } else if (absY >= absX) {
        axis1 = 0; axis2 = 2;  // Project onto XZ
    } else {
        axis1 = 1; axis2 = 2;  // Project onto YZ
    }
    
    // Barycentric coordinate test in 2D
    // ... (compute u, v, w barycentric coords)
    
    return u >= 0 && v >= 0 && w >= 0;
}
```

---

## Rendering Pipeline

### Mesh to Triangles Conversion

```cpp
std::vector<Triangle3D> Mesh::toTriangles() const {
    std::vector<Triangle3D> triangles;
    
    for (size_t i = 0; i < faces.size(); i++) {
        const auto& face = faces[i];
        uint32_t color = faceColors[i];
        
        // Triangulate the face
        auto faceTriangles = triangulateFace(face, vertices);
        
        // Create Triangle3D for each sub-triangle
        for (const auto& tri : faceTriangles) {
            triangles.push_back(Triangle3D(
                vertices[tri[0]],
                vertices[tri[1]],
                vertices[tri[2]],
                color, color, color
            ));
        }
    }
    
    return triangles;
}
```

### 3D to 2D Projection

```cpp
std::pair<int, int> PixelBuffer::project3DTo2D(
    const Vec3& point, 
    int screenWidth, 
    int screenHeight) 
{
    // Normalized Device Coordinates to screen space
    int x = (int)((point.x + 1.0f) * 0.5f * screenWidth);
    int y = (int)((1.0f - point.y) * 0.5f * screenHeight);  // Flip Y
    return {x, y};
}
```

### Triangle Rasterization

The renderer uses **barycentric coordinate rasterization** with gradient color interpolation:

```cpp
void PixelBuffer::fillTriangleGradient(
    int x0, int y0, uint32_t color0,
    int x1, int y1, uint32_t color1,
    int x2, int y2, uint32_t color2) 
{
    // 1. Calculate bounding box
    int min_x = std::min({x0, x1, x2});
    int max_x = std::max({x0, x1, x2});
    int min_y = std::min({y0, y1, y2});
    int max_y = std::max({y0, y1, y2});
    
    // 2. Clamp to screen bounds
    min_x = std::max(0, min_x);
    max_x = std::min(width - 1, max_x);
    // ... same for Y
    
    // 3. Calculate triangle area for barycentric coords
    float area = (x0 - x2) * (y1 - y2) - (x1 - x2) * (y0 - y2);
    
    // 4. For each pixel in bounding box
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            // Calculate barycentric coordinates
            float w0 = /* ... */;
            float w1 = /* ... */;
            float w2 = /* ... */;
            
            // Check if inside triangle
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                // Interpolate color
                Color interpolated = c0 * w0 + c1 * w1 + c2 * w2;
                setPixel(x, y, interpolated.toARGB());
            }
        }
    }
}
```

### Depth-Based Lighting

Simple lighting simulation based on Z-depth:

```cpp
void PixelBuffer::render3DTriangle(const Triangle3D& triangle, ...) {
    // Average Z depth of triangle
    float avgZ = (triangle.vertices[0].z + 
                  triangle.vertices[1].z + 
                  triangle.vertices[2].z) / 3.0f;
    
    // Map Z from [-1, 1] to light intensity
    // Near (Z = -1) is bright, far (Z = 1) is dark
    float depthFactor = (1.0f - avgZ) * 0.5f;
    depthFactor = std::clamp(depthFactor, 0.3f, 1.0f);
    
    // Apply lighting to colors
    float lightIntensity = depthFactor * 0.7f + 0.3f;  // Ambient + depth
    
    uint32_t litColor = applyLighting(color, lightIntensity);
}
```

---

## Color Assignment

### Random Hue-Based Colors

```cpp
void ObjLoader::assignRandomColors(Mesh& mesh) {
    for (size_t i = 0; i < mesh.faces.size(); i++) {
        // Distribute hues evenly with slight randomness
        float hue = (float)i / mesh.faces.size() * 360.0f + randomFloat(0, 30);
        
        // HSV to RGB: high saturation (0.7), high value (0.9)
        uint32_t color = hsvToRgb(fmod(hue, 360.0f), 0.7f, 0.9f);
        mesh.faceColors.push_back(color);
    }
}
```

### Gradient Colors

```cpp
void ObjLoader::assignGradientColors(Mesh& mesh, uint32_t color1, uint32_t color2) {
    for (size_t i = 0; i < mesh.faces.size(); i++) {
        float t = (float)i / (mesh.faces.size() - 1);  // 0 to 1
        uint32_t color = blendColors(color1, color2, t);
        mesh.faceColors.push_back(color);
    }
}
```

---

## Mathematical Foundations

### Barycentric Coordinates

For a point P inside triangle ABC:

```
P = w₀·A + w₁·B + w₂·C

where w₀ + w₁ + w₂ = 1
```

**Calculation:**
```
       (B.y - C.y)(P.x - C.x) + (C.x - B.x)(P.y - C.y)
w₀ = ─────────────────────────────────────────────────────
       (B.y - C.y)(A.x - C.x) + (C.x - B.x)(A.y - C.y)

       (C.y - A.y)(P.x - C.x) + (A.x - C.x)(P.y - C.y)
w₁ = ─────────────────────────────────────────────────────
                          (same denominator)

w₂ = 1 - w₀ - w₁
```

Point is inside triangle if: `w₀ ≥ 0 AND w₁ ≥ 0 AND w₂ ≥ 0`

### Cross Product (Winding Order)

```cpp
Vec3 cross = edge1.cross(edge2);

// If cross.z > 0: Counter-clockwise winding (front-facing)
// If cross.z < 0: Clockwise winding (back-facing)
```

---

## Usage Examples

### Loading and Rendering an OBJ File

```cpp
#include "obj_loader.h"
#include "pixelbuffer.h"

int main() {
    // Load mesh
    Mesh mesh;
    if (!ObjLoader::load("model.obj", mesh)) {
        return -1;
    }
    
    // Convert to triangles
    std::vector<Triangle3D> triangles = mesh.toTriangles();
    
    // Create rotation matrix
    Matrix4x4 rotation = Matrix4x4::rotationY(angle);
    
    // Render each triangle
    PixelBuffer buffer(800, 600);
    for (const auto& tri : triangles) {
        Triangle3D transformed = tri.transform(rotation);
        buffer.render3DTriangle(transformed, 800, 600);
    }
}
```

### Custom Color Schemes

```cpp
// Gradient from red to blue
ObjLoader::assignGradientColors(mesh, 0xFFFF0000, 0xFF0000FF);

// Or modify colors manually
for (size_t i = 0; i < mesh.faceColors.size(); i++) {
    mesh.faceColors[i] = myCustomColorFunction(i);
}

// Re-generate triangles with new colors
triangles = mesh.toTriangles();
```

---

## Limitations & Future Work

### Current Limitations

| Limitation | Description |
|------------|-------------|
| No texture support | `vt` texture coordinates are parsed but not used |
| No smooth shading | Per-face colors only, no vertex normal interpolation |
| No Z-buffering | Painter's algorithm requires manual triangle sorting |
| Single mesh only | No support for object groups (`o`, `g`) |
| No MTL support | Material files (`.mtl`) are ignored |

### Potential Improvements

1. **Z-Buffer Implementation**
   - Per-pixel depth testing for correct occlusion

2. **Texture Mapping**
   - UV coordinate interpolation
   - Texture sampling

3. **Smooth Shading**
   - Gouraud shading with vertex normal interpolation
   - Phong shading for per-pixel lighting

4. **Material Support**
   - Parse `.mtl` files
   - Ambient, diffuse, specular colors

5. **Optimizations**
   - Spatial partitioning (BVH, octree)
   - Backface culling
   - View frustum culling

---

## References

- [Wavefront OBJ Specification](http://www.martinreddy.net/gfx/3d/OBJ.spec)
- [Ear Clipping Triangulation](https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf)
- [Newell's Method for Polygon Normals](https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal)
- [Barycentric Coordinates](https://ceng2.ktu.edu.tr/~cakir/files/grafikler/Barycentric_Coordinates.pdf)
