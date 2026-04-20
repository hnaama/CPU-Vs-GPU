#include "obj_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <array>

Mesh::Mesh() : center(0, 0, 0), scale(1.0f) {}

void Mesh::calculateBounds() {
    if (vertices.empty()) return;
    
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
    
    center = Vec3(
        (minBound.x + maxBound.x) * 0.5f,
        (minBound.y + maxBound.y) * 0.5f,
        (minBound.z + maxBound.z) * 0.5f
    );
    
    float sizeX = maxBound.x - minBound.x;
    float sizeY = maxBound.y - minBound.y;
    float sizeZ = maxBound.z - minBound.z;
    scale = std::max({sizeX, sizeY, sizeZ});
}

void Mesh::centerAndNormalize() {
    calculateBounds();
    
    if (scale < 0.0001f) scale = 1.0f;
    
    // Center and scale all vertices to fit in a unit cube
    for (auto& v : vertices) {
        v.x = (v.x - center.x) / scale * 2.0f;
        v.y = (v.y - center.y) / scale * 2.0f;
        v.z = (v.z - center.z) / scale * 2.0f;
    }
    
    center = Vec3(0, 0, 0);
    scale = 1.0f;
}

Vec3 Mesh::calculateFaceNormal(
    const std::vector<int>& faceIndices,
    const std::vector<Vec3>& vertices) {
    
    if (faceIndices.size() < 3) return Vec3(0, 0, 1);
    
    // Use Newell's method for robust normal calculation (works for non-convex polygons)
    Vec3 normal(0, 0, 0);
    
    for (size_t i = 0; i < faceIndices.size(); i++) {
        const Vec3& current = vertices[faceIndices[i]];
        const Vec3& next = vertices[faceIndices[(i + 1) % faceIndices.size()]];
        
        normal.x += (current.y - next.y) * (current.z + next.z);
        normal.y += (current.z - next.z) * (current.x + next.x);
        normal.z += (current.x - next.x) * (current.y + next.y);
    }
    
    float len = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (len > 0.0001f) {
        normal.x /= len;
        normal.y /= len;
        normal.z /= len;
    }
    
    return normal;
}

bool Mesh::isFaceCoplanar(
    const std::vector<int>& faceIndices,
    const std::vector<Vec3>& vertices,
    float tolerance) {
    
    if (faceIndices.size() <= 3) return true;  // Triangles are always coplanar
    
    // Calculate the plane from first 3 vertices
    const Vec3& p0 = vertices[faceIndices[0]];
    const Vec3& p1 = vertices[faceIndices[1]];
    const Vec3& p2 = vertices[faceIndices[2]];
    
    Vec3 v1(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
    Vec3 v2(p2.x - p0.x, p2.y - p0.y, p2.z - p0.z);
    
    // Normal of the plane
    Vec3 normal(
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    );
    
    float len = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (len < 0.0001f) return true;  // Degenerate face
    
    normal.x /= len;
    normal.y /= len;
    normal.z /= len;
    
    // Check all other vertices against the plane
    for (size_t i = 3; i < faceIndices.size(); i++) {
        const Vec3& p = vertices[faceIndices[i]];
        Vec3 diff(p.x - p0.x, p.y - p0.y, p.z - p0.z);
        
        // Distance from point to plane
        float dist = std::abs(normal.x * diff.x + normal.y * diff.y + normal.z * diff.z);
        
        if (dist > tolerance) {
            return false;
        }
    }
    
    return true;
}

bool Mesh::isConvexVertex(
    const Vec3& prev, const Vec3& curr, const Vec3& next,
    const Vec3& normal) {
    
    // Calculate cross product of edges
    Vec3 edge1(curr.x - prev.x, curr.y - prev.y, curr.z - prev.z);
    Vec3 edge2(next.x - curr.x, next.y - curr.y, next.z - curr.z);
    
    Vec3 cross(
        edge1.y * edge2.z - edge1.z * edge2.y,
        edge1.z * edge2.x - edge1.x * edge2.z,
        edge1.x * edge2.y - edge1.y * edge2.x
    );
    
    // Dot product with face normal - positive means convex
    float dot = cross.x * normal.x + cross.y * normal.y + cross.z * normal.z;
    
    return dot >= 0;
}

bool Mesh::pointInTriangle(
    const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c,
    const Vec3& normal) {
    
    // Project onto 2D plane (use the two axes with largest normal component)
    int axis1, axis2;
    float absX = std::abs(normal.x);
    float absY = std::abs(normal.y);
    float absZ = std::abs(normal.z);
    
    if (absZ >= absX && absZ >= absY) {
        axis1 = 0; axis2 = 1;  // Project onto XY
    } else if (absY >= absX && absY >= absZ) {
        axis1 = 0; axis2 = 2;  // Project onto XZ
    } else {
        axis1 = 1; axis2 = 2;  // Project onto YZ
    }
    
    auto getCoord = [](const Vec3& v, int axis) -> float {
        if (axis == 0) return v.x;
        if (axis == 1) return v.y;
        return v.z;
    };
    
    float px = getCoord(p, axis1), py = getCoord(p, axis2);
    float ax = getCoord(a, axis1), ay = getCoord(a, axis2);
    float bx = getCoord(b, axis1), by = getCoord(b, axis2);
    float cx = getCoord(c, axis1), cy = getCoord(c, axis2);
    
    // Barycentric coordinate test
    float denom = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy);
    if (std::abs(denom) < 0.0001f) return false;
    
    float u = ((by - cy) * (px - cx) + (cx - bx) * (py - cy)) / denom;
    float v = ((cy - ay) * (px - cx) + (ax - cx) * (py - cy)) / denom;
    float w = 1.0f - u - v;
    
    // Point is inside if all barycentric coords are positive (with small epsilon for edges)
    const float eps = 0.0001f;
    return u >= -eps && v >= -eps && w >= -eps;
}

bool Mesh::isEar(
    const std::vector<int>& polygon,
    int earIndex,
    const std::vector<Vec3>& vertices,
    const Vec3& normal) {
    
    int n = polygon.size();
    if (n < 3) return false;
    
    int prevIdx = (earIndex - 1 + n) % n;
    int nextIdx = (earIndex + 1) % n;
    
    const Vec3& prev = vertices[polygon[prevIdx]];
    const Vec3& curr = vertices[polygon[earIndex]];
    const Vec3& next = vertices[polygon[nextIdx]];
    
    // Must be a convex vertex
    if (!isConvexVertex(prev, curr, next, normal)) {
        return false;
    }
    
    // Check that no other vertex is inside the triangle
    for (int i = 0; i < n; i++) {
        if (i == prevIdx || i == earIndex || i == nextIdx) continue;
        
        const Vec3& p = vertices[polygon[i]];
        if (pointInTriangle(p, prev, curr, next, normal)) {
            return false;
        }
    }
    
    return true;
}

std::vector<std::array<int, 3>> Mesh::earClipTriangulate(
    const std::vector<int>& faceIndices,
    const std::vector<Vec3>& vertices,
    const Vec3& normal) {
    
    std::vector<std::array<int, 3>> triangles;
    
    if (faceIndices.size() < 3) return triangles;
    if (faceIndices.size() == 3) {
        triangles.push_back({faceIndices[0], faceIndices[1], faceIndices[2]});
        return triangles;
    }
    
    // Create a working copy of the polygon indices
    std::vector<int> polygon = faceIndices;
    
    int maxIterations = polygon.size() * polygon.size();  // Safety limit
    int iterations = 0;
    
    while (polygon.size() > 3 && iterations < maxIterations) {
        iterations++;
        bool earFound = false;
        
        for (size_t i = 0; i < polygon.size(); i++) {
            if (isEar(polygon, i, vertices, normal)) {
                int n = polygon.size();
                int prevIdx = (i - 1 + n) % n;
                int nextIdx = (i + 1) % n;
                
                // Add the ear triangle
                triangles.push_back({
                    polygon[prevIdx],
                    polygon[i],
                    polygon[nextIdx]
                });
                
                // Remove the ear vertex
                polygon.erase(polygon.begin() + i);
                earFound = true;
                break;
            }
        }
        
        // If no ear found, we have a degenerate polygon - fall back to fan triangulation
        if (!earFound) {
            std::cerr << "Warning: Ear clipping failed, falling back to fan triangulation\n";
            for (size_t i = 1; i < polygon.size() - 1; i++) {
                triangles.push_back({polygon[0], polygon[i], polygon[i + 1]});
            }
            break;
        }
    }
    
    // Add the last triangle
    if (polygon.size() == 3) {
        triangles.push_back({polygon[0], polygon[1], polygon[2]});
    }
    
    return triangles;
}

std::vector<std::array<int, 3>> Mesh::triangulateFace(
    const std::vector<int>& faceIndices,
    const std::vector<Vec3>& vertices) {
    
    std::vector<std::array<int, 3>> triangles;
    
    if (faceIndices.size() < 3) return triangles;
    
    // For triangles, just return as-is
    if (faceIndices.size() == 3) {
        triangles.push_back({faceIndices[0], faceIndices[1], faceIndices[2]});
        return triangles;
    }
    
    // Calculate the face normal
    Vec3 normal = calculateFaceNormal(faceIndices, vertices);
    
    // Check if face is coplanar
    bool coplanar = isFaceCoplanar(faceIndices, vertices, 0.01f);
    
    if (!coplanar) {
        // For non-coplanar faces, use centroid-based triangulation
        // This creates triangles from each edge to the centroid
        Vec3 centroid(0, 0, 0);
        for (int idx : faceIndices) {
            centroid.x += vertices[idx].x;
            centroid.y += vertices[idx].y;
            centroid.z += vertices[idx].z;
        }
        centroid.x /= faceIndices.size();
        centroid.y /= faceIndices.size();
        centroid.z /= faceIndices.size();
        
        // We can't add a new vertex easily, so fall back to fan from first vertex
        // but with smaller sub-polygons if the face is very non-coplanar
        std::cerr << "Warning: Non-coplanar face detected, using fan triangulation\n";
        for (size_t i = 1; i < faceIndices.size() - 1; i++) {
            triangles.push_back({faceIndices[0], faceIndices[i], faceIndices[i + 1]});
        }
        return triangles;
    }
    
    // Use ear clipping for coplanar polygons (handles concave correctly)
    return earClipTriangulate(faceIndices, vertices, normal);
}

std::vector<Triangle3D> Mesh::toTriangles() const {
    std::vector<Triangle3D> triangles;
    
    for (size_t i = 0; i < faces.size(); i++) {
        const auto& face = faces[i];
        uint32_t color = (i < faceColors.size()) ? faceColors[i] : 0xFFCCCCCC;
        
        // Use proper triangulation for polygons
        auto faceTriangles = triangulateFace(face, vertices);
        
        for (const auto& tri : faceTriangles) {
            int idx0 = tri[0];
            int idx1 = tri[1];
            int idx2 = tri[2];
            
            if (idx0 >= 0 && idx0 < (int)vertices.size() &&
                idx1 >= 0 && idx1 < (int)vertices.size() &&
                idx2 >= 0 && idx2 < (int)vertices.size()) {
                
                triangles.push_back(Triangle3D(
                    vertices[idx0],
                    vertices[idx1],
                    vertices[idx2],
                    color, color, color
                ));
            }
        }
    }
    
    return triangles;
}

bool ObjLoader::load(const std::string& filename, Mesh& mesh) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open OBJ file: " << filename << "\n";
        return false;
    }
    
    std::cout << "Loading OBJ file: " << filename << "\n";
    
    mesh.vertices.clear();
    mesh.normals.clear();
    mesh.faces.clear();
    mesh.faceNormals.clear();
    mesh.faceColors.clear();
    
    std::string line;
    int vertexCount = 0;
    int faceCount = 0;
    
    while (std::getline(file, line)) {
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            // Vertex position
            float x, y, z;
            if (iss >> x >> y >> z) {
                mesh.vertices.push_back(Vec3(x, y, z));
                vertexCount++;
            }
        }
        else if (prefix == "vn") {
            // Vertex normal
            float x, y, z;
            if (iss >> x >> y >> z) {
                mesh.normals.push_back(Vec3(x, y, z).normalize());
            }
        }
        else if (prefix == "f") {
            // Face
            std::vector<int> faceVertices;
            std::vector<int> faceNorms;
            std::string vertexData;
            
            while (iss >> vertexData) {
                // Parse vertex index (format: v, v/vt, v/vt/vn, or v//vn)
                std::replace(vertexData.begin(), vertexData.end(), '/', ' ');
                std::istringstream viss(vertexData);
                
                int vIdx = 0, vtIdx = 0, vnIdx = 0;
                viss >> vIdx;
                
                // Check for texture coordinate
                if (viss.peek() != EOF) {
                    viss >> vtIdx;
                }
                // Check for normal index
                if (viss.peek() != EOF) {
                    viss >> vnIdx;
                }
                
                // OBJ indices are 1-based, convert to 0-based
                if (vIdx != 0) {
                    faceVertices.push_back(vIdx > 0 ? vIdx - 1 : (int)mesh.vertices.size() + vIdx);
                }
                if (vnIdx != 0) {
                    faceNorms.push_back(vnIdx > 0 ? vnIdx - 1 : (int)mesh.normals.size() + vnIdx);
                }
            }
            
            if (faceVertices.size() >= 3) {
                mesh.faces.push_back(faceVertices);
                mesh.faceNormals.push_back(faceNorms);
                faceCount++;
            }
        }
    }
    
    file.close();
    
    std::cout << "Loaded " << vertexCount << " vertices, " << faceCount << " faces\n";
    
    // Center and normalize the mesh
    mesh.centerAndNormalize();
    
    // Assign colors to faces
    assignRandomColors(mesh);
    
    return true;
}

void ObjLoader::assignRandomColors(Mesh& mesh) {
    mesh.faceColors.clear();
    mesh.faceColors.reserve(mesh.faces.size());
    
    for (size_t i = 0; i < mesh.faces.size(); i++) {
        // Generate pleasing random colors
        float hue = (float)i / mesh.faces.size() * 360.0f + randomFloat(0, 30);
        uint32_t color = hsvToRgb(fmod(hue, 360.0f), 0.7f, 0.9f);
        mesh.faceColors.push_back(color);
    }
}

void ObjLoader::assignGradientColors(Mesh& mesh, uint32_t color1, uint32_t color2) {
    mesh.faceColors.clear();
    mesh.faceColors.reserve(mesh.faces.size());
    
    for (size_t i = 0; i < mesh.faces.size(); i++) {
        float t = (float)i / (mesh.faces.size() - 1);
        uint32_t color = blendColors(color1, color2, t);
        mesh.faceColors.push_back(color);
    }
}