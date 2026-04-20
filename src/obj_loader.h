#pragma once

#include <vector>
#include <string>
#include "utils.h"

// 3D Mesh structure to hold loaded OBJ data
struct Mesh {
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<std::vector<int>> faces;  // Each face is a list of vertex indices
    std::vector<std::vector<int>> faceNormals; // Normal indices for each face
    std::vector<uint32_t> faceColors;  // Color for each face
    
    Vec3 center;
    float scale;
    
    Mesh();
    
    void calculateBounds();
    void centerAndNormalize();
    std::vector<Triangle3D> toTriangles() const;
    
private:
    // Triangulation helpers
    static std::vector<std::array<int, 3>> triangulateFace(
        const std::vector<int>& faceIndices, 
        const std::vector<Vec3>& vertices);
    
    static std::vector<std::array<int, 3>> earClipTriangulate(
        const std::vector<int>& faceIndices,
        const std::vector<Vec3>& vertices,
        const Vec3& normal);
    
    static bool isEar(
        const std::vector<int>& polygon,
        int earIndex,
        const std::vector<Vec3>& vertices,
        const Vec3& normal);
    
    static bool isConvexVertex(
        const Vec3& prev, const Vec3& curr, const Vec3& next,
        const Vec3& normal);
    
    static bool pointInTriangle(
        const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c,
        const Vec3& normal);
    
    static Vec3 calculateFaceNormal(
        const std::vector<int>& faceIndices,
        const std::vector<Vec3>& vertices);
    
    static bool isFaceCoplanar(
        const std::vector<int>& faceIndices,
        const std::vector<Vec3>& vertices,
        float tolerance = 0.001f);
};

// OBJ file loader
class ObjLoader {
public:
    static bool load(const std::string& filename, Mesh& mesh);
    static void assignRandomColors(Mesh& mesh);
    static void assignGradientColors(Mesh& mesh, uint32_t color1, uint32_t color2);
};