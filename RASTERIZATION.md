# Triangle Rasterization in Software Rendering

## Table of Contents
1. [Overview](#overview)
2. [Fundamental Concepts](#fundamental-concepts)
3. [Algorithm 1: Barycentric Coordinate Rasterization](#algorithm-1-barycentric-coordinate-rasterization)
4. [Algorithm 2: Scanline Rasterization](#algorithm-2-scanline-rasterization)
5. [Vertex Color Interpolation](#vertex-color-interpolation)
6. [3D Rotation and Transformation](#3d-rotation-and-transformation)
7. [Implementation Details](#implementation-details)
8. [Performance Comparison](#performance-comparison)
9. [Mathematical Deep Dive](#mathematical-deep-dive)
10. [Code Examples](#code-examples)

---

## Overview

Triangle rasterization is the process of converting geometric triangle data (3 vertices) into pixels on a 2D screen. This is a fundamental operation in computer graphics that determines which pixels inside a triangle should be colored, and what color they should be.

Our software renderer implements **two different rasterization algorithms**:

1. **Barycentric Coordinate Method** - Mathematically precise, supports advanced features
2. **Scanline Method** - Performance optimized, cache-friendly

Both algorithms support **vertex color interpolation**, allowing each vertex to have its own color that blends smoothly across the triangle surface.

---

## Fundamental Concepts

### What is Rasterization?

Rasterization converts continuous geometric shapes (triangles) into discrete pixels. Given three vertices with positions and colors:

```
Vertex A: (x₀, y₀) → Color₀
Vertex B: (x₁, y₁) → Color₁  
Vertex C: (x₂, y₂) → Color₂
```

The rasterizer must determine:
1. **Which pixels** are inside the triangle
2. **What color** each pixel should be (interpolated from vertex colors)

### Key Challenges

- **Inside/Outside Test**: Determining if a pixel is within triangle boundaries
- **Color Interpolation**: Smoothly blending colors from vertices
- **Edge Cases**: Handling degenerate triangles, clipping, precision issues
- **Performance**: Processing thousands of triangles per frame efficiently

---

## Algorithm 1: Barycentric Coordinate Rasterization

### Theory

Barycentric coordinates express any point inside a triangle as a weighted combination of the three vertices. For a point P inside triangle ABC:

```
P = w₀ × A + w₁ × B + w₂ × C
```

Where:
- `w₀ + w₁ + w₂ = 1` (weights sum to 1)
- If all weights ≥ 0, point P is inside the triangle
- Weights represent "how much" each vertex contributes to point P

### Implementation Steps

#### Step 1: Calculate Bounding Box
```cpp
int min_x = std::min({x0, x1, x2});
int max_x = std::max({x0, x1, x2});
int min_y = std::min({y0, y1, y2});  
int max_y = std::max({y0, y1, y2});
```

This restricts our search to only pixels that could possibly be inside the triangle.

#### Step 2: Calculate Triangle Area
```cpp
float area = sign(x0, y0, x1, y1, x2, y2);
```

The `sign` function computes the signed area using the cross product:
```cpp
auto sign = [](int x0, int y0, int x1, int y1, int x2, int y2) -> float {
    return (x0 - x2) * (y1 - y2) - (x1 - x2) * (y0 - y2);
};
```

#### Step 3: For Each Pixel in Bounding Box
```cpp
for (int y = min_y; y <= max_y; y++) {
    for (int x = min_x; x <= max_x; x++) {
        // Calculate barycentric coordinates
        float w0 = sign(x, y, x1, y1, x2, y2) / area;
        float w1 = sign(x0, y0, x, y, x2, y2) / area;
        float w2 = sign(x0, y0, x1, y1, x, y) / area;
        
        // Inside test
        if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
            // Interpolate color
            Color finalColor = c0 * w0 + c1 * w1 + c2 * w2;
            setPixel(x, y, finalColor.toARGB());
        }
    }
}
```

### Advantages
- ✅ **Mathematically precise** - No edge cases or rounding errors
- ✅ **Natural color interpolation** - Barycentric weights directly give interpolation factors
- ✅ **Handles all triangle orientations** - Works regardless of vertex order
- ✅ **Easy to extend** - Simple to add texture coordinates, normals, etc.

### Disadvantages  
- ❌ **Potentially slower** - Tests every pixel in bounding box
- ❌ **Cache unfriendly** - Random memory access pattern for large triangles

---

## Algorithm 2: Scanline Rasterization

### Theory

Scanline rasterization fills triangles by processing horizontal lines (scanlines) from top to bottom. For each scanline, it calculates where the line intersects the triangle edges, then fills between those intersection points.

### Implementation Steps

#### Step 1: Sort Vertices by Y-Coordinate
```cpp
// Ensure v0 is topmost, v2 is bottommost
if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); std::swap(color0, color1); }
if (y0 > y2) { std::swap(x0, x2); std::swap(y0, y2); std::swap(color0, color2); }
if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); std::swap(color1, color2); }
```

This creates a consistent vertex ordering for the algorithm.

#### Step 2: For Each Scanline Y
```cpp
for (int y = y0; y <= y2; y++) {
    // Calculate left and right edge intersections
    float t_main = (float)(y - y0) / (y2 - y0);
    float x_left = lerp(x0, x2, t_main);
    Color color_left = lerpColor(c0, c2, t_main);
    
    // Right edge depends on upper/lower triangle section
    float x_right, color_right;
    if (y <= y1) {
        // Upper triangle: interpolate from v0 to v1
        float t_upper = (float)(y - y0) / (y1 - y0);
        x_right = lerp(x0, x1, t_upper);
        color_right = lerpColor(c0, c1, t_upper);
    } else {
        // Lower triangle: interpolate from v1 to v2  
        float t_lower = (float)(y - y1) / (y2 - y1);
        x_right = lerp(x1, x2, t_lower);
        color_right = lerpColor(c1, c2, t_lower);
    }
}
```

#### Step 3: Fill Horizontal Scanline
```cpp
for (int x = x_start; x <= x_end; x++) {
    float t_horizontal = (float)(x - x_start) / (x_end - x_start);
    Color finalColor = lerpColor(color_left, color_right, t_horizontal);
    setPixel(x, y, finalColor.toARGB());
}
```

### Advantages
- ✅ **Performance optimized** - Only processes pixels actually inside triangle
- ✅ **Cache friendly** - Sequential memory access (row by row)
- ✅ **Minimal computations** - Linear interpolation along edges
- ✅ **Predictable cost** - Processing time proportional to triangle area

### Disadvantages
- ❌ **More complex edge cases** - Horizontal edges, degenerate triangles
- ❌ **Requires vertex sorting** - Additional preprocessing step

---

## Vertex Color Interpolation

### Color Representation

Our color system uses a floating-point intermediate representation for precise interpolation:

```cpp
struct Color {
    float r, g, b, a;  // Range [0.0, 1.0]
    
    Color(uint32_t argb) {
        a = ((argb >> 24) & 0xFF) / 255.0f;
        r = ((argb >> 16) & 0xFF) / 255.0f; 
        g = ((argb >> 8) & 0xFF) / 255.0f;
        b = (argb & 0xFF) / 255.0f;
    }
    
    uint32_t toARGB() const {
        return ((uint8_t)(a * 255) << 24) |
               ((uint8_t)(r * 255) << 16) |
               ((uint8_t)(g * 255) << 8) |
               ((uint8_t)(b * 255));
    }
};
```

### Linear Interpolation

Both algorithms use **linear interpolation** to blend colors:

```cpp
Color lerp(const Color& a, const Color& b, float t) {
    return Color(
        a.r + t * (b.r - a.r),
        a.g + t * (b.g - a.g), 
        a.b + t * (b.b - a.b),
        a.a + t * (a.a - a.a)
    );
}
```

Where `t ∈ [0,1]`:
- `t = 0` → color A
- `t = 1` → color B  
- `t = 0.5` → 50/50 blend

### Barycentric Color Interpolation

With barycentric coordinates, color interpolation is natural:

```cpp
Color interpolated = c0 * w0 + c1 * w1 + c2 * w2;
```

This gives **perspective-correct interpolation** - colors blend correctly even with 3D perspective projection.

---

## 3D Rotation and Transformation

### Overview of 3D Pipeline

To achieve true 3D rotating triangles, our software renderer implements a complete 3D graphics pipeline:

1. **3D Model Space** - Define triangles in 3D coordinates
2. **World Transformation** - Position and orient objects in 3D world
3. **View Transformation** - Apply camera positioning
4. **Projection** - Convert 3D coordinates to 2D screen space
5. **Rasterization** - Fill triangles with colors and lighting

### 3D Vector Mathematics

#### Vec3 Class Implementation

The foundation of 3D graphics is robust vector mathematics:

```cpp
struct Vec3 {
    float x, y, z;
    
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    // Basic arithmetic operations
    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }
    
    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }
    
    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }
    
    // Dot product for lighting calculations
    float dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    
    // Cross product for surface normals
    Vec3 cross(const Vec3& other) const {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
    
    // Vector normalization for unit vectors
    Vec3 normalize() const {
        float len = sqrt(x * x + y * y + z * z);
        if (len > 0.001f) {
            return Vec3(x / len, y / len, z / len);
        }
        return Vec3(0, 0, 0);
    }
};
```

### 4x4 Transformation Matrices

#### Matrix4x4 Class

3D transformations are represented using 4x4 homogeneous coordinate matrices:

```cpp
struct Matrix4x4 {
    float m[4][4];
    
    Matrix4x4() {
        // Initialize as identity matrix
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                m[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }
    
    // Matrix multiplication for combining transformations
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; k++) {
                    result.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return result;
    }
    
    // Transform a 3D point with perspective division
    Vec3 transform(const Vec3& v) const {
        float w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3];
        if (abs(w) < 0.001f) w = 1.0f; // Avoid division by zero
        
        return Vec3(
            (m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3]) / w,
            (m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3]) / w,
            (m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3]) / w
        );
    }
};
```

#### Rotation Matrices

Each axis rotation is implemented as a separate matrix factory method:

**X-Axis Rotation (Pitch):**
```cpp
static Matrix4x4 rotationX(float angle) {
    Matrix4x4 mat;
    float c = cos(angle);
    float s = sin(angle);
    mat.m[1][1] = c; mat.m[1][2] = -s;
    mat.m[2][1] = s; mat.m[2][2] = c;
    return mat;
}
```

**Y-Axis Rotation (Yaw):**
```cpp
static Matrix4x4 rotationY(float angle) {
    Matrix4x4 mat;
    float c = cos(angle);
    float s = sin(angle);
    mat.m[0][0] = c;  mat.m[0][2] = s;
    mat.m[2][0] = -s; mat.m[2][2] = c;
    return mat;
}
```

**Z-Axis Rotation (Roll):**
```cpp
static Matrix4x4 rotationZ(float angle) {
    Matrix4x4 mat;
    float c = cos(angle);
    float s = sin(angle);
    mat.m[0][0] = c; mat.m[0][1] = -s;
    mat.m[1][0] = s; mat.m[1][1] = c;
    return mat;
}
```

#### Translation Matrix

Position objects in 3D space:

```cpp
static Matrix4x4 translation(float x, float y, float z) {
    Matrix4x4 mat;
    mat.m[0][3] = x;
    mat.m[1][3] = y;
    mat.m[2][3] = z;
    return mat;
}
```

#### Perspective Projection Matrix

Convert 3D coordinates to screen space with proper depth perception:

```cpp
static Matrix4x4 perspective(float fov, float aspect, float near, float far) {
    Matrix4x4 mat;
    float tanHalfFov = tan(fov / 2.0f);
    
    // Initialize to zero
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            mat.m[i][j] = 0.0f;
        }
    }
    
    // Set projection matrix elements
    mat.m[0][0] = 1.0f / (aspect * tanHalfFov);  // X scaling
    mat.m[1][1] = 1.0f / tanHalfFov;             // Y scaling
    mat.m[2][2] = -(far + near) / (far - near);  // Z mapping
    mat.m[2][3] = -(2.0f * far * near) / (far - near); // Z offset
    mat.m[3][2] = -1.0f;                         // Perspective divide
    
    return mat;
}
```

### 3D Triangle Structure

#### Triangle3D Class

Represents a triangle in 3D space with vertex colors:

```cpp
struct Triangle3D {
    Vec3 vertices[3];
    uint32_t colors[3];
    
    Triangle3D(Vec3 v0, Vec3 v1, Vec3 v2, uint32_t c0, uint32_t c1, uint32_t c2) {
        vertices[0] = v0; vertices[1] = v1; vertices[2] = v2;
        colors[0] = c0; colors[1] = c1; colors[2] = c2;
    }
    
    // Calculate surface normal for lighting
    Vec3 getNormal() const {
        Vec3 edge1 = vertices[1] - vertices[0];
        Vec3 edge2 = vertices[2] - vertices[0];
        return edge1.cross(edge2).normalize();
    }
    
    // Apply transformation matrix to all vertices
    Triangle3D transform(const Matrix4x4& matrix) const {
        return Triangle3D(
            matrix.transform(vertices[0]),
            matrix.transform(vertices[1]),
            matrix.transform(vertices[2]),
            colors[0], colors[1], colors[2]
        );
    }
};
```

### 3D to 2D Projection

#### Screen Space Conversion

After perspective projection, normalized device coordinates are converted to screen pixels:

```cpp
std::pair<int, int> project3DTo2D(const Vec3& point, int screenWidth, int screenHeight) {
    // Point is already in normalized device coordinates [-1, 1]
    int x = (int)((point.x + 1.0f) * 0.5f * screenWidth);
    int y = (int)((1.0f - point.y) * 0.5f * screenHeight); // Flip Y axis
    return {x, y};
}
```

### Lighting System

#### Directional Lighting

Simple directional lighting calculates brightness based on surface normal:

```cpp
void render3DTriangle(const Triangle3D& triangle, int screenWidth, int screenHeight) {
    // Project vertices to screen space
    auto p0 = project3DTo2D(triangle.vertices[0], screenWidth, screenHeight);
    auto p1 = project3DTo2D(triangle.vertices[1], screenWidth, screenHeight);
    auto p2 = project3DTo2D(triangle.vertices[2], screenWidth, screenHeight);
    
    // Calculate lighting intensity
    Vec3 lightDir = Vec3(0.3f, -0.5f, -0.7f).normalize();
    Vec3 normal = triangle.getNormal();
    float lightIntensity = std::max(0.2f, -normal.dot(lightDir));
    
    // Apply lighting to vertex colors
    auto applyLighting = [lightIntensity](uint32_t color) -> uint32_t {
        uint8_t a = (color >> 24) & 0xFF;
        uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * lightIntensity);
        uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * lightIntensity);
        uint8_t b = (uint8_t)((color & 0xFF) * lightIntensity);
        return (a << 24) | (r << 16) | (g << 8) | b;
    };
    
    // Render with lit colors
    fillTriangleGradient(
        p0.first, p0.second, applyLighting(triangle.colors[0]),
        p1.first, p1.second, applyLighting(triangle.colors[1]),
        p2.first, p2.second, applyLighting(triangle.colors[2])
    );
}
```

### Animation System

#### Time-Based Rotation

Smooth animation uses delta time for frame-rate independent motion:

```cpp
// Animation variables
float rotation_time = 0.0f;
uint32_t last_time = SDL_GetTicks();

// In main loop:
uint32_t current_time = SDL_GetTicks();
float delta_time = (current_time - last_time) / 1000.0f;
last_time = current_time;
rotation_time += delta_time;
```

#### Complex Transformation Chains

Multiple transformations are combined through matrix multiplication:

```cpp
// Example: Tumbling triangle (rotates around all three axes)
Matrix4x4 transform = Matrix4x4::translation(-1.5f, 0.0f, 0.0f) *
                     Matrix4x4::rotationX(rotation_time * 1.2f) *
                     Matrix4x4::rotationY(rotation_time * 0.8f) *
                     Matrix4x4::rotationZ(rotation_time * 0.5f) *
                     projection;
```

#### Oscillating Motion

Sine and cosine functions create organic, non-linear motion:

```cpp
// Wobbling triangle with complex motion
float wobble_x = sin(rotation_time * 2.0f) * 0.3f;
float wobble_y = cos(rotation_time * 1.7f) * 0.3f;
float wobble_z = sin(rotation_time * 0.9f) * cos(rotation_time * 1.1f) * 0.2f;

Matrix4x4 wobble_transform = Matrix4x4::translation(1.5f, 0.0f, 0.0f) *
                            Matrix4x4::rotationX(wobble_x) *
                            Matrix4x4::rotationY(wobble_y) *
                            Matrix4x4::rotationZ(wobble_z) *
                            projection;
```

### Backface Culling

#### Visibility Optimization

Only render triangles facing toward the camera:

```cpp
// After transformation to screen space
Vec3 normal = transformed_triangle.getNormal();
if (normal.z > 0) { // Triangle facing camera
    pixelBuffer.render3DTriangle(transformed_triangle, WINDOW_WIDTH, WINDOW_HEIGHT);
}
```

The Z-component of the normal vector indicates whether the triangle faces toward (positive Z) or away from (negative Z) the camera.

### 3D Scene Construction

#### Creating Complex Objects

Multiple triangles can form complex 3D objects like pyramids:

```cpp
// Pyramid construction
Vec3 pyramid_center = Vec3(0.0f, 0.0f, -7.0f);
float pyramid_size = 0.6f;

// Base vertices (square base)
Vec3 base1 = pyramid_center + Vec3(-pyramid_size, -pyramid_size, pyramid_size);
Vec3 base2 = pyramid_center + Vec3(pyramid_size, -pyramid_size, pyramid_size);
Vec3 base3 = pyramid_center + Vec3(pyramid_size, -pyramid_size, -pyramid_size);
Vec3 base4 = pyramid_center + Vec3(-pyramid_size, -pyramid_size, -pyramid_size);
Vec3 apex = pyramid_center + Vec3(0.0f, pyramid_size, 0.0f);

// Four triangular faces
std::vector<Triangle3D> pyramid_faces = {
    Triangle3D(apex, base1, base2, 0xFFFF6666, 0xFFFF0000, 0xFFFF3333), // Red
    Triangle3D(apex, base2, base3, 0xFF66FF66, 0xFF00FF00, 0xFF33FF33), // Green
    Triangle3D(apex, base3, base4, 0xFF6666FF, 0xFF0000FF, 0xFF3333FF), // Blue
    Triangle3D(apex, base4, base1, 0xFFFFFF66, 0xFFFFFF00, 0xFFFFFF33)  // Yellow
};
```

### Performance Considerations

#### Matrix Multiplication Optimization

Matrix operations are computationally expensive. Optimizations include:

1. **Pre-calculate common matrices** - Store projection matrix
2. **Combine transformations** - Multiply matrices once, not per vertex
3. **SIMD instructions** - Use vectorized math libraries for large datasets
4. **Level-of-detail** - Use fewer triangles for distant objects

#### Memory Layout

Efficient memory access patterns improve cache performance:

```cpp
// Structure of Arrays (SoA) vs Array of Structures (AoS)
struct TriangleSoA {
    std::vector<Vec3> vertices;     // Better cache locality
    std::vector<uint32_t> colors;   // for bulk operations
};
```

---

## Implementation Details

### Bounds Checking

All algorithms include proper bounds checking:

```cpp
min_x = std::max(0, min_x);
max_x = std::min(width - 1, max_x);
min_y = std::max(0, min_y);  
max_y = std::min(height - 1, max_y);
```

This prevents drawing outside the screen buffer.

### Degenerate Triangle Handling

```cpp
if (std::abs(area) < 0.001f) return;  // Skip zero-area triangles
if (y0 == y2) return;                 // Skip flat triangles
```

These checks prevent division by zero and other numerical issues.

### Precision Considerations

- **Floating-point arithmetic** for color interpolation prevents banding
- **Integer coordinates** for pixel addressing avoids subpixel issues
- **Epsilon comparisons** for area calculations handle floating-point precision

---

## Performance Comparison

### Barycentric Method
- **Best for**: Small triangles, precise rendering, feature-rich shading
- **Complexity**: O(bounding_box_area)
- **Memory access**: Random (potentially cache-unfriendly)
- **Typical use**: High-quality rendering, when correctness > speed

### Scanline Method  
- **Best for**: Large triangles, performance-critical applications
- **Complexity**: O(triangle_area) 
- **Memory access**: Sequential (cache-friendly)
- **Typical use**: Real-time rendering, software rasterizers

### Benchmark Results (Typical)

| Triangle Size | Barycentric | Scanline | Winner |
|---------------|-------------|----------|---------|
| Small (< 100px²) | Fast | Fast | Tie |
| Medium (100-1000px²) | Good | Faster | Scanline |
| Large (> 1000px²) | Slow | Much Faster | Scanline |

---

## Mathematical Deep Dive

### Barycentric Coordinate Derivation

For triangle with vertices A, B, C and point P:

```
P = uA + vB + wC  where u + v + w = 1
```

Solving the system:
```
Px = u*Ax + v*Bx + w*Cx
Py = u*Ay + v*By + w*Cy  
1  = u    + v    + w
```

Using Cramer's rule and cross products:

```cpp
float area = (Bx - Ax) * (Cy - Ay) - (Cx - Ax) * (By - Ay);
float u = ((Bx - Px) * (Cy - Py) - (Cx - Px) * (By - Py)) / area;
float v = ((Cx - Px) * (Ay - Py) - (Ax - Px) * (Cy - Py)) / area;  
float w = 1.0f - u - v;
```

### Edge Function Method

Our `sign` function implements the **edge function**:

```cpp
float edge(Vec2 a, Vec2 b, Vec2 c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}
```

This has geometric meaning:
- **Positive**: Point C is to the left of line AB
- **Zero**: Point C lies on line AB  
- **Negative**: Point C is to the right of line AB

### Color Space Considerations

Linear interpolation in RGB space can produce unexpected results. For more accurate color blending, consider:

1. **Gamma correction**: Convert to linear space before interpolation
2. **Perceptual color spaces**: Use LAB or HSV for more natural blending
3. **HDR rendering**: Use floating-point colors throughout pipeline

---

## Code Examples

### Basic Triangle Fill

```cpp
// Solid color triangle (no interpolation)
pixelBuffer.fillTriangleBarycentric(100, 100, 200, 100, 150, 200, 0xFFFF0000);
```

### Gradient Triangle

```cpp  
// Red-Green-Blue gradient triangle
pixelBuffer.fillTriangleGradient(
    100, 100, 0xFFFF0000,  // Red vertex
    200, 100, 0xFF00FF00,  // Green vertex
    150, 200, 0xFF0000FF   // Blue vertex
);
```

### Custom Color Interpolation

```cpp
// Create custom gradient effects
pixelBuffer.fillTriangleGradient(
    400, 300, 0xFFFFFFFF,  // White (highlight)
    350, 400, 0xFF808080,  // Gray (midtone)  
    450, 400, 0xFF000000   // Black (shadow)
);
```

### Performance Optimization

```cpp
// Use scanline for large triangles
if (triangle_area > 1000) {
    fillTriangleGradientScanline(x0, y0, c0, x1, y1, c1, x2, y2, c2);
} else {
    fillTriangleGradient(x0, y0, c0, x1, y1, c1, x2, y2, c2);  
}
```

---

## Advanced Topics

### Texture Mapping Extension

The barycentric coordinate system naturally extends to texture mapping:

```cpp
// Interpolate UV coordinates
float u = u0 * w0 + u1 * w1 + u2 * w2;
float v = v0 * w0 + v1 * w1 + v2 * w2;
Color textureColor = sampleTexture(u, v);
```

### Depth Testing

Add Z-coordinate interpolation for 3D rendering:

```cpp
float z = z0 * w0 + z1 * w1 + z2 * w2;
if (z < depthBuffer[y * width + x]) {
    setPixel(x, y, color);
    depthBuffer[y * width + x] = z;
}
```

### Sub-pixel Accuracy

For higher quality, sample multiple points per pixel:

```cpp
// 4x MSAA sampling
for (int sy = 0; sy < 2; sy++) {
    for (int sx = 0; sx < 2; sx++) {
        float sample_x = x + (sx + 0.5f) / 2.0f;
        float sample_y = y + (sy + 0.5f) / 2.0f;
        // Test sample point...
    }
}
```

---

## Conclusion

Triangle rasterization forms the foundation of 3D computer graphics. Our implementation provides:

- **Two complementary algorithms** optimized for different use cases
- **High-quality color interpolation** with mathematical precision  
- **Robust edge case handling** for production use
- **Clean, extensible code** ready for advanced features

The barycentric coordinate method excels at correctness and extensibility, while the scanline method optimizes for performance. Together, they provide a complete solution for software triangle rasterization.

Understanding these algorithms deeply enables implementation of advanced graphics features like texture mapping, lighting models, and anti-aliasing - all building blocks of modern 3D rendering pipelines.