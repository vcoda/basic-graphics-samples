/*
Copyright (C) 2018 Victor Coda.
This is a rewritten particle system from Kevin Harris.
I used C++11, STL and Mersenne Twister pseudo random number generator.
Particle vertices are copied into vertex buffer.
Math and logic of original code is preserved.
*/
//-----------------------------------------------------------------------------
//           Name: particlesystem.cpp
//         Author: Kevin Harris
//  Last Modified: 02/01/05
//    Description: Implementation file for the CParticleSystem Class
//-----------------------------------------------------------------------------
#include <ctime>
#include "particlesystem.h"

ParticleSystem::ParticleSystem()
{
    rng.seed(static_cast<std::mt19937::result_type>(clock()));
}

void ParticleSystem::setCollisionPlane(const rapid::float3& planeNormal, const rapid::float3& point,
    float bounceFactor /* 1 */, CollisionResult collisionResult /* CollisionResult::BOUNCE */)
{
    Plane plane;
    plane.normal = planeNormal;
    plane.point = point;
    plane.bounceFactor = bounceFactor;
    plane.collisionResult = collisionResult;
    planes.push_back(plane);
}

void ParticleSystem::initialize(std::shared_ptr<magma::Device> device)
{
    vertexBuffer = std::make_shared<magma::VertexBuffer>(device, nullptr, maxParticles * sizeof(ParticleVertex), maxParticles);
    drawParams = std::make_shared<magma::IndirectBuffer>(device);
}

void ParticleSystem::update(float dt)
{
    currentTime += dt;

    for (auto it = activeList.begin(); it != activeList.end();)
    {
        Particle& particle = *it;
        // Calculate new position
        float timePassed  = currentTime - particle.startTime;
        if (timePassed >= lifeCycle)
        {   // Move particle to free list
            freeList.splice(freeList.end(), activeList, it);
            it = activeList.end();
        }
        else
        {   // Update velocity with respect to Gravity (Constant Accelaration)
            particle.velocity += gravity * dt;
            // Update velocity with respect to Wind (Accelaration based on difference of vectors)
            if (airResistence)
                particle.velocity += (wind - particle.velocity) * dt;
            // Finally, update position with respect to velocity
            const rapid::vector3 oldPosition = particle.position;
            particle.position += particle.velocity * dt;
            // Checking the particle against each plane that was set up
            for (auto& plane : planes)
            {
                ClassifyPoint result = classifyPoint(particle.position, plane);
                if (result == ClassifyPoint::BACK)
                {
                    if (plane.collisionResult == CollisionResult::BOUNCE)
                    {
                        particle.position = oldPosition;
                        const float Kr = plane.bounceFactor;
                        const rapid::vector3 vn = (plane.normal | particle.velocity) * plane.normal;
                        const rapid::vector3 vt = particle.velocity - vn;
                        const rapid::vector3 vp = vt - Kr * vn;
                        particle.velocity = vp;
                    }
                    else if (plane.collisionResult == CollisionResult::RECYCLE)
                    {
                        particle.startTime -= lifeCycle;
                    }
                    else if (plane.collisionResult == CollisionResult::STICK)
                    {
                        particle.position = oldPosition;
                        particle.velocity.zero();
                    }
                }
            }
            ++it;
        }
    }

    if (currentTime - lastUpdate > releaseInterval)
    {   // Reset update timing...
        lastUpdate = currentTime;
        // Emit new particles at specified flow rate...
        for (uint32_t i = 0; i < numToRelease; ++i)
        {   // Do we have any free particles to put back to work?
            if (!freeList.empty())
            {   // If so, hand over the first free one to be reused.
                activeList.splice(activeList.end(), freeList, std::prev(freeList.end()));
            }
            else if (activeList.size() < maxParticles)
            {   // There are no free particles to recycle...
                // We'll have to create a new one from scratch!
                activeList.push_back(Particle());
            }
            if (activeList.size() < maxParticles)
            {   // Set the attributes for our new particle...
                Particle& particle = activeList.back();
                particle.velocity = velocity;
                if (velocityScale != 0.f)
                {
                    const rapid::vector3 randomVec = randomVector();
                    particle.velocity += randomVec * velocityScale;
                }
                particle.startTime = currentTime;
                particle.position = position;
                particle.color = randomColor();
            }
        }
    }

    // Update vertex buffer
    if (!activeList.empty())
    {
        if (ParticleVertex *pv = vertexBuffer->getMemory()->map<ParticleVertex>(0, VK_WHOLE_SIZE))
        {
            for (const auto& particle : activeList)
            {
                particle.position.store(&pv->position);
                pv->color = particle.color;
                ++pv;
            }
            vertexBuffer->getMemory()->unmap();
        }
        drawParams->writeDrawCommand(static_cast<uint32_t>(activeList.size()), 0);
    }
}

void ParticleSystem::reset()
{
    freeList.splice(freeList.end(), activeList);
}

void ParticleSystem::draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer)
{
    cmdBuffer->bindVertexBuffer(0, vertexBuffer);
    cmdBuffer->drawIndirect(drawParams, 0, 1, 0);
}

float ParticleSystem::randomScalar(float min, float max)
{
    float f = rng()/(float)rng.max();
    return min + (max - min) * f;
}

rapid::float3 ParticleSystem::randomVector()
{
    rapid::float3 v;
    v.z = randomScalar(-1.0f, 1.0f);
    float radius = sqrtf(1.f - v.z * v.z); // Get radius of this circle
    float t = randomScalar(-rapid::constants::pi, rapid::constants::pi); // Pick a random point on a circle
    v.x = (float)cosf(t) * radius; // Compute matching X and Y for our Z
    v.y = (float)sinf(t) * radius;
    return v;
}

rapid::float3 ParticleSystem::randomColor()
{
    rapid::float3 color;
    color.x = rng()/(float)rng.max();
    color.y = rng()/(float)rng.max();
    color.z = rng()/(float)rng.max();
    return color;
}

inline ClassifyPoint classifyPoint(const rapid::vector3& point, const ParticleSystem::Plane& plane)
{
    const rapid::vector3 direction = plane.point - point;
    const float dp = direction | plane.normal;
    if (dp < -0.001f)
        return ClassifyPoint::FRONT;
    if (dp > 0.001f)
        return ClassifyPoint::BACK;
    return ClassifyPoint::ONPLANE;
}
