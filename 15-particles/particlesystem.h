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
    FRONT,
    BACK,
    ONPLANE
};

enum class CollisionResult
{
    BOUNCE,
    STICK,
    RECYCLE
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

public:
    ParticleSystem();

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
        float bounceFactor = 1.f, CollisionResult collisionResult = CollisionResult::BOUNCE);

    void initialize(std::shared_ptr<magma::Device> device);
    void update(float dt);
    void reset(void);
    void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer);

private:
    float randomScalar(float min, float max);
    rapid::float3 randomVector();
    rapid::float3 randomColor();

private:
    std::list<Particle> activeList, freeList;
    std::list<Plane> planes;
    std::mt19937 rng;
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
    std::shared_ptr<magma::IndirectBuffer> drawParams;
};

ClassifyPoint classifyPoint(const rapid::vector3& point, const ParticleSystem::Plane& plane);

namespace magma
{
    namespace specialization
    {
        template<> struct VertexAttribute<rapid::float3> :
            public AttributeFormat<VK_FORMAT_R32G32B32_SFLOAT> {};
    }
}
