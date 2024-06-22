/*
Copyright (C) 2018 Victor Coda.
This is a rewritten particle system from Kevin Harris.
I used C++11, STL and Mersenne Twister pseudo random number generator.
Particle vertices are copied into vertex buffer.
Math and logic of original code is preserved.
*/
//-----------------------------------------------------------------------------
//           Name: particlesystem.h
//         Author: Kevin Harris
//  Last Modified: 02/01/05
//    Description: Implementation file for the CParticleSystem Class
//-----------------------------------------------------------------------------
#pragma once
#include <memory>
#include <list>
#include <random>
#include "../third-party/magma/magma.h"
#include "../third-party/rapid/rapid.h"
#include "../framework/platform.h"

enum class ClassifyPoint
{
    Front, Back, InPlane
};

enum class CollisionResult
{
    Bounce, Stick, Recycle
};

class ParticleSystem : public AlignAs<16>
{
public:
    struct ParticleVertex
    {
        rapid::float3 position;
        rapid::float3 color;
    };

    struct Particle
    {
        rapid::vector3 position;
        rapid::vector3 velocity;
        rapid::float3 color;
        float startTime;
    };

    struct Plane
    {
        rapid::vector3 normal;
        rapid::vector3 point;
        float bounceFactor;
        CollisionResult collisionResult;
    };

    struct Constants
    {
        float width;
        float height;
        float h;
        float pointSize;
    };

public:
    ParticleSystem();
    void setResolution(uint32_t width, uint32_t height) noexcept;
    void setFieldOfView(float fov) noexcept;
    void setPointSize(float pointSize) noexcept;
    void setMaxParticles(uint32_t maxParticles) { this->maxParticles = maxParticles; }
    void setNumToRelease(uint32_t numToRelease) { this->numToRelease = numToRelease; }
    void setReleaseInterval(float releaseInterval) { this->releaseInterval = releaseInterval; }
    void setLifeCycle(float lifeCycle) { this->lifeCycle = lifeCycle; }
    void setPosition(const rapid::float3& position) { this->position = rapid::vector3(position); }
    void setVelocity(const rapid::float3& velocity) { this->velocity = rapid::vector3(velocity); }
    void setGravity(const rapid::float3& gravity) { this->gravity = rapid::vector3(gravity); }
    void setWind(const rapid::float3& wind) { this->wind = rapid::vector3(wind); }
    void setAirResistence(bool airResistence) { this->airResistence = airResistence; }
    void setVelocityScale(float scale) { this->velocityScale = scale; }
    void setCollisionPlane(const rapid::float3& planeNormal, const rapid::float3& point,
        float bounceFactor = 1.f, CollisionResult collisionResult = CollisionResult::Bounce);
    void initialize(std::shared_ptr<magma::Device> device);
    void update(float dt);
    void reset(void);
    void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer,
        std::shared_ptr<magma::Pipeline> pipeline) noexcept;

private:
    rapid::float3 randomVector();
    rapid::float3 randomColor();

    std::mt19937 rng;
    std::uniform_real_distribution<float> rgbDistribution;
    std::uniform_real_distribution<float> normalDistribution;
    std::uniform_real_distribution<float> discDistribution;
    std::list<Particle> activeList, freeList;
    std::list<Plane> planes;

    Constants constants = {};
    float currentTime = 0.f;
    float lastUpdate = 0.f;

    uint32_t maxParticles = 1;
    uint32_t numToRelease = 1;
    float releaseInterval = 1.f;
    float lifeCycle = 1.f;

    rapid::vector3 position, velocity, gravity, wind;
    bool airResistence = true;
    float velocityScale = 1.f;

    std::shared_ptr<magma::DynamicVertexBuffer> vertexBuffer;
    std::shared_ptr<magma::DrawIndirectBuffer> drawParams;
};

ClassifyPoint classifyPoint(const rapid::vector3& point, const ParticleSystem::Plane& plane);

MAGMA_SPECIALIZE_VERTEX_ATTRIBUTE(rapid::float3, VK_FORMAT_R32G32B32_SFLOAT);
