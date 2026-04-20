#include "bouncing_balls.h"
#include <iostream>

// BouncingBall constructor
BouncingBall::BouncingBall(float startX, float startY, float velX, float velY, float r, uint32_t c)
    : x(startX), y(startY), vx(velX), vy(velY), radius(r), mass(r * r), color(c) {}

// BouncingBallSystem constructor
BouncingBallSystem::BouncingBallSystem(int margin)
    : boxMargin(margin), boxLeft(0), boxRight(0), boxTop(0), boxBottom(0),
      showTrail(true), maxTrailLength(30), maxSpeed(500.0f) {}

void BouncingBallSystem::initialize(int screenWidth, int screenHeight) {
    balls.clear();
    
    // Update box boundaries
    boxLeft = boxMargin;
    boxRight = screenWidth - boxMargin;
    boxTop = boxMargin;
    boxBottom = screenHeight - boxMargin;
    
    // Add initial balls
    balls.push_back(BouncingBall(screenWidth / 2.0f, screenHeight / 2.0f, 400.0f, 300.0f, 30.0f, 0xFFFF6B6B));
    balls.push_back(BouncingBall(screenWidth / 3.0f, screenHeight / 3.0f, -200.0f, 150.0f, 20.0f, 0xFF6BFF6B));
    balls.push_back(BouncingBall(screenWidth / 4.0f, screenHeight / 4.0f, 100.0f, -250.0f, 25.0f, 0xFF6B6BFF));
}

void BouncingBallSystem::reset() {
    balls.clear();
}

void BouncingBallSystem::handleWallCollisions(BouncingBall& ball) {
    if (ball.x - ball.radius < boxLeft) {
        ball.x = boxLeft + ball.radius;
        ball.vx *= -1;
        ball.color = randomColor();
    }
    if (ball.x + ball.radius > boxRight) {
        ball.x = boxRight - ball.radius;
        ball.vx *= -1;
        ball.color = randomColor();
    }
    if (ball.y - ball.radius < boxTop) {
        ball.y = boxTop + ball.radius;
        ball.vy *= -1;
        ball.color = randomColor();
    }
    if (ball.y + ball.radius > boxBottom) {
        ball.y = boxBottom - ball.radius;
        ball.vy *= -1;
        ball.color = randomColor();
    }
}

void BouncingBallSystem::handleBallCollisions() {
    for (size_t i = 0; i < balls.size(); i++) {
        for (size_t j = i + 1; j < balls.size(); j++) {
            BouncingBall& ball1 = balls[i];
            BouncingBall& ball2 = balls[j];
            
            float dx = ball2.x - ball1.x;
            float dy = ball2.y - ball1.y;
            float distance = sqrt(dx * dx + dy * dy);
            float minDist = ball1.radius + ball2.radius;
            
            if (distance < minDist && distance > 0.0001f) {
                // Resolve overlap
                float overlap = 0.5f * (distance - minDist);
                ball1.x -= overlap * (ball1.x - ball2.x) / distance;
                ball1.y -= overlap * (ball1.y - ball2.y) / distance;
                ball2.x += overlap * (ball1.x - ball2.x) / distance;
                ball2.y += overlap * (ball1.y - ball2.y) / distance;
                
                // Calculate new velocities using elastic collision
                float nx = (ball2.x - ball1.x) / distance;
                float ny = (ball2.y - ball1.y) / distance;
                float tx = -ny;
                float ty = nx;
                
                float dpTan1 = ball1.vx * tx + ball1.vy * ty;
                float dpTan2 = ball2.vx * tx + ball2.vy * ty;
                
                float dpNorm1 = ball1.vx * nx + ball1.vy * ny;
                float dpNorm2 = ball2.vx * nx + ball2.vy * ny;
                
                float m1 = (dpNorm1 * (ball1.mass - ball2.mass) + 2.0f * ball2.mass * dpNorm2) / (ball1.mass + ball2.mass);
                float m2 = (dpNorm2 * (ball2.mass - ball1.mass) + 2.0f * ball1.mass * dpNorm1) / (ball1.mass + ball2.mass);
                
                ball1.vx = tx * dpTan1 + nx * m1;
                ball1.vy = ty * dpTan1 + ny * m1;
                ball2.vx = tx * dpTan2 + nx * m2;
                ball2.vy = ty * dpTan2 + ny * m2;
                
                // Cap speed to prevent runaway acceleration
                float speed1 = sqrt(ball1.vx * ball1.vx + ball1.vy * ball1.vy);
                float speed2 = sqrt(ball2.vx * ball2.vx + ball2.vy * ball2.vy);
                
                if (speed1 > maxSpeed) {
                    ball1.vx = (ball1.vx / speed1) * maxSpeed;
                    ball1.vy = (ball1.vy / speed1) * maxSpeed;
                }
                if (speed2 > maxSpeed) {
                    ball2.vx = (ball2.vx / speed2) * maxSpeed;
                    ball2.vy = (ball2.vy / speed2) * maxSpeed;
                }
            }
        }
    }
}

void BouncingBallSystem::updateTrail(BouncingBall& ball) {
    if (showTrail) {
        ball.trail.push_back({ball.x, ball.y});
        if ((int)ball.trail.size() > maxTrailLength) {
            ball.trail.erase(ball.trail.begin());
        }
    }
}

void BouncingBallSystem::update(float deltaTime, int screenWidth, int screenHeight) {
    // Update box boundaries in case screen size changed
    boxLeft = boxMargin;
    boxRight = screenWidth - boxMargin;
    boxTop = boxMargin;
    boxBottom = screenHeight - boxMargin;
    
    // Initialize if empty
    if (balls.empty()) {
        initialize(screenWidth, screenHeight);
    }
    
    // Update each ball
    for (auto& ball : balls) {
        // Update position
        ball.x += ball.vx * deltaTime;
        ball.y += ball.vy * deltaTime;
        
        // Handle wall collisions
        handleWallCollisions(ball);
        
        // Update trail
        updateTrail(ball);
    }
    
    // Handle ball-to-ball collisions
    handleBallCollisions();
}

void BouncingBallSystem::render(PixelBuffer& buffer) {
    // Draw the box
    buffer.drawRectangle(boxLeft, boxTop, 
                         boxRight - boxLeft, boxBottom - boxTop, 
                         0xFF60A0FF);  // Light blue box outline
    
    // Draw balls and their trails
    for (const auto& ball : balls) {
        // Draw trail
        if (showTrail && ball.trail.size() > 1) {
            for (size_t i = 1; i < ball.trail.size(); i++) {
                float alpha = (float)i / ball.trail.size();
                uint32_t trailColor = 0xFF000000 | 
                    (((int)(((ball.color >> 16) & 0xFF) * alpha)) << 16) |
                    (((int)(((ball.color >> 8) & 0xFF) * alpha)) << 8) |
                    ((int)((ball.color & 0xFF) * alpha));
                
                buffer.drawLine(
                    (int)ball.trail[i-1].first, (int)ball.trail[i-1].second,
                    (int)ball.trail[i].first, (int)ball.trail[i].second,
                    trailColor
                );
            }
        }
        
        // Draw filled ball
        buffer.fillCircle((int)ball.x, (int)ball.y, (int)ball.radius, ball.color);
    }
}

void BouncingBallSystem::addBall(int screenWidth, int screenHeight) {
    float newRadius = randomFloat(15.0f, 40.0f);
    float newX = randomFloat(boxMargin + newRadius, screenWidth - boxMargin - newRadius);
    float newY = randomFloat(boxMargin + newRadius, screenHeight - boxMargin - newRadius);
    
    // Fixed speed, random direction
    float speed = 300.0f;
    float angle = randomFloat(0, 2.0f * M_PI);
    float newVx = speed * cos(angle);
    float newVy = speed * sin(angle);
    
    uint32_t newColor = randomColor();
    balls.push_back(BouncingBall(newX, newY, newVx, newVy, newRadius, newColor));
    
    std::cout << "Added ball! Total: " << balls.size() << "\n" << std::flush;
}

void BouncingBallSystem::toggleTrail() {
    showTrail = !showTrail;
    if (!showTrail) {
        for (auto& ball : balls) {
            ball.trail.clear();
        }
    }
    std::cout << "Trail: " << (showTrail ? "ON" : "OFF") << "\n" << std::flush;
}
