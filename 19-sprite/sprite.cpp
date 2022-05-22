#include <fstream>
#include "magma/magma.h"
#include "sprite.h"
#include "../framework/utilities.h"

GameSprite::GameSprite(const std::string& filename, std::shared_ptr<magma::CommandBuffer> cmdBuffer)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
        throw std::runtime_error("failed to open file \"" + filename + "\"");
    const std::streamoff size = file.tellg();
    file.seekg(0, std::ios::beg);
    gliml::context ctx;
    VkDeviceSize baseMipOffset = 0;
    std::shared_ptr<magma::SrcTransferBuffer> buffer = std::make_shared<magma::SrcTransferBuffer>(cmdBuffer->getDevice(), size);
    magma::helpers::mapScoped<uint8_t>(buffer,
        [&](uint8_t *data)
        {   // Read data to buffer
            file.read(reinterpret_cast<char *>(data), size);
            file.close();
            ctx.enable_dxt(true);
            ctx.enable_bgra(true);
            if (!ctx.load(data, static_cast<unsigned>(size)))
                throw std::runtime_error("failed to load DDS texture");
            // Skip DDS header
            baseMipOffset = reinterpret_cast<const uint8_t *>(ctx.image_data(0, 0)) - data;
        });
    // Setup texture data description
    const VkFormat format = utilities::getBlockCompressedFormat(ctx);
    extent.width = (uint32_t)ctx.image_width(0, 0);
    extent.height = (uint32_t)ctx.image_height(0, 0);
    // Upload bitmap data
    sprite = std::make_shared<magma::aux::Sprite>(cmdBuffer, format, extent, buffer, baseMipOffset);
    x = sprite->getPosition().x;
    y = sprite->getPosition().y;
    // Flush
    std::shared_ptr<magma::Queue> queue(cmdBuffer->getDevice()->getQueue(VK_QUEUE_GRAPHICS_BIT, 0));
    std::shared_ptr<magma::Fence> fence(cmdBuffer->getFence());
    if (!queue->submit(cmdBuffer, 0, nullptr, nullptr, fence))
        MAGMA_THROW("failed to submit command buffer to graphics queue");
    if (!fence->wait())
        MAGMA_THROW("failed to wait fence");
}

void GameSprite::scale(float factor) noexcept
{
    if (scaling + factor > 0.f)
        scaling += factor;
    float width = roundf(extent.width * scaling);
    float height = roundf(extent.height * scaling);
    sprite->setWidth((uint32_t)std::max(4.f, width));
    sprite->setHeight((uint32_t)std::max(4.f, height));
}

void GameSprite::moveLeft(uint32_t step) noexcept
{
    if (direction != Direction::left)
    {
        sprite->flipHorizontal();
        direction = Direction::left;
    }
    x -= step;
}

void GameSprite::moveRight(uint32_t step) noexcept
{
    if (direction != Direction::right)
    {
        sprite->flipHorizontal();
        direction = Direction::right;
    }
    x += step;
}

void GameSprite::draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer, std::shared_ptr<magma::Image> dstImage)
{
    sprite->setPosition(x, y);
    sprite->blit(cmdBuffer, dstImage, VK_FILTER_NEAREST);//sprite->isScaled() ? VK_FILTER_LINEAR : VK_FILTER_NEAREST);
}