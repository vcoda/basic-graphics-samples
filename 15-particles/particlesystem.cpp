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

ParticleSystem::ParticleSystem():
    rgbDistribution{0.f, 1.f},
    normalDistribution{-1.f, 1.f},
    discDistribution{-rapid::constants::pi, rapid::constants::pi}
{
    const auto seed = std::random_device()();
    rng.seed(seed);
}

void ParticleSystem::setResolution(uint32_t width, int32_t height) noexcept
{
    constants.width = static_cast<float>(width);
    constants.height = static_cast<float>(std::abs(height));
    constants.negateViewport = (height < 0);
}

void ParticleSystem::setFieldOfView(float fov) noexcept
{
    constants.h = constants.height / (2.f * tanf(fov / 2.f)); // Scale with distance
}

void ParticleSystem::setPointSize(float pointSize) noexcept
{
    constants.pointSize = pointSize;
}

void ParticleSystem::setCollisionPlane(const rapid::float3& planeNormal, const rapid::float3& point,
    float bounceFactor /* 1 */, CollisionResult collisionResult /* CollisionResult::Bounce */)
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
    const bool stagedPool = device->getFeatures()->supportsDeviceLocalHostVisibleMemory();
    vertexBuffer = std::make_shared<magma::DynamicVertexBuffer>(device, maxParticles * sizeof(ParticleVertex), stagedPool);
    drawParams = std::make_shared<magma::DrawIndirectBuffer>(device, 1);
    drawParams->writeDrawCommand(0, 0); // Submit stub draw call to command buffer
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
            for (auto const& plane: planes)
            {
                ClassifyPoint result = classifyPoint(particle.position, plane);
                if (ClassifyPoint::Back == result)
                {
                    switch (plane.collisionResult)
                    {
                    case CollisionResult::Bounce:
                        particle.position = oldPosition;
                        {
                            const float Kr = plane.bounceFactor;
                            const rapid::vector3 vn = (plane.normal | particle.velocity) * plane.normal;
                            const rapid::vector3 vt = particle.velocity - vn;
                            const rapid::vector3 vp = vt - Kr * vn;
                            particle.velocity = vp;
                        }
                        break;
                    case CollisionResult::Recycle:
                        particle.startTime -= lifeCycle;
                        break;
                    case CollisionResult::Stick:
                        particle.position = oldPosition;
                        particle.velocity.zero();
                        break;
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
        magma::helpers::mapScoped<ParticleVertex>(vertexBuffer,
            [this](auto *pv)
            {
                for (auto const& particle: activeList)
                {
                    particle.position.store(&pv->position);
                    pv->color = particle.color;
                    ++pv;
                }
            });
        drawParams->reset();
        drawParams->writeDrawCommand(magma::core::countof(activeList), 0);
    }
}

void ParticleSystem::reset()
{
    freeList.splice(freeList.end(), activeList);
}

void ParticleSystem::draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer, std::shared_ptr<magma::Pipeline> pipeline) noexcept
{
    cmdBuffer->pushConstantBlock(pipeline->getLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, constants);
    cmdBuffer->bindPipeline(pipeline);
    cmdBuffer->bindVertexBuffer(0, vertexBuffer);
    cmdBuffer->drawIndirect(drawParams);
}

rapid::float3 ParticleSystem::randomVector()
{
    rapid::float3 v;
    v.z = normalDistribution(rng);
    float radius = sqrtf(1.f - v.z * v.z); // Get radius of this circle
    float t = discDistribution(rng); // Pick a random point on a circle
    v.x = (float)cosf(t) * radius; // Compute matching X and Y for our Z
    v.y = (float)sinf(t) * radius;
    return v;
}

rapid::float3 ParticleSystem::randomColor()
{
    rapid::float3 color;
    color.x = rgbDistribution(rng);
    color.y = rgbDistribution(rng);
    color.z = rgbDistribution(rng);
    return color;
}

inline ClassifyPoint classifyPoint(const rapid::vector3& point, const ParticleSystem::Plane& plane)
{
    const rapid::vector3 direction = plane.point - point;
    const float dotProduct = direction | plane.normal;
    if (dotProduct < -0.001f)
        return ClassifyPoint::Front;
    if (dotProduct > 0.001f)
        return ClassifyPoint::Back;
    return ClassifyPoint::InPlane;
}
