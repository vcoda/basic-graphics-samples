#include "../framework/vulkanApp.h"
#include "sprite.h"

class SpriteApp : public VulkanApp
{
    std::shared_ptr<GameSprite> sprite;

public:
    SpriteApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("19 - Sprite"), 512, 512)
    {
        initialize();
        sprite = std::make_shared<GameSprite>("mario/stand.dds", cmdImageCopy);
        sprite->setPosition(20, 300);
        sprite->scale(2.f);
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
    }

    virtual void render(uint32_t bufferIndex) override
    {
        submitCommandBuffer(bufferIndex);
    }

    virtual void onKeyDown(char key, int repeat, uint32_t flags) override
    {
        switch (key)
        {
        case AppKey::Left:
            sprite->moveLeft(10);
            break;
        case AppKey::Right:
            sprite->moveRight(10);
            break;
        case AppKey::Up:
            //sprite->flipVertical();
            break;
        case AppKey::Down:
            //sprite->flipVertical();
            break;
        case AppKey::PgUp:
            sprite->scale(0.1f);
            break;
        case AppKey::PgDn:
            sprite->scale(-0.1f);
            break;
        }
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
        VulkanApp::onKeyDown(key, repeat, flags);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            constexpr magma::ClearColor clearColor(0.35f, 0.53f, 0.7f, 1.f);
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clear::white});
            cmdBuffer->endRenderPass();
            std::shared_ptr<magma::Image> backBuffer = framebuffers[index]->getAttachments().front()->getImage();
            sprite->draw(cmdBuffer, std::move(backBuffer));
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<SpriteApp>(new SpriteApp(entry));
}
