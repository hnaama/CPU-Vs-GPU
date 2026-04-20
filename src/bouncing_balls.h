#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include "pixelbuffer.h"
#include "utils.h"

// Single bouncing ball
struct BouncingBall {
    float x, y;           // 2D position
    float vx, vy;         // 2D velocity
    float radius;
    float mass;           // Mass for collision response
    uint32_t color;
    std::vector<std::pair<float, float>> trail;  // 2D Trail effect
    
    BouncingBall(float startX, float startY, float velX, float velY, float r, uint32_t c);
};

// Manager class for the bouncing balls simulation
class BouncingBallSystem {
private:
    std::vector<BouncingBall> balls;
    int boxMargin;
    int boxLeft, boxRight, boxTop, boxBottom;
    bool showTrail;
    int maxTrailLength;
    float maxSpeed;
    
    void handleWallCollisions(BouncingBall& ball);
    void handleBallCollisions();
    void updateTrail(BouncingBall& ball);
    
public:
    BouncingBallSystem(int margin = 50);
    
    // Initialize with default balls
    void initialize(int screenWidth, int screenHeight);
    
    // Reset the simulation
    void reset();
    
    // Update physics
    void update(float deltaTime, int screenWidth, int screenHeight);
    
    // Render to pixel buffer
    void render(PixelBuffer& buffer);
    
    // Add a new ball
    void addBall(int screenWidth, int screenHeight);
    
    // Toggle trail visibility
    void toggleTrail();
    
    // Getters
    bool isTrailEnabled() const { return showTrail; }
    size_t getBallCount() const { return balls.size(); }
    
    // Extract ball data for GPU rendering
    void getBallData(std::vector<Vec3>& positions, std::vector<float>& radii, std::vector<uint32_t>& colors) const {
        positions.clear();
        radii.clear();
        colors.clear();
        
        for (const auto& ball : balls) {
            positions.push_back(Vec3(ball.x, ball.y, 0.0f));
            radii.push_back(ball.radius);
            colors.push_back(ball.color);
        }
    }
};
