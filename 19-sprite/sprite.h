#pragma once

class GameSprite
{
    enum class Direction
    {
        left = 0,
        right,
        up,
        down
    };

public:
    explicit GameSprite(const std::string& filename,
        std::shared_ptr<magma::CommandBuffer> cmdBuffer);
    void setPosition(int32_t x_, int32_t y_) noexcept { x = x_; y = y_; }
    void scale(float factor) noexcept;
    void moveLeft(uint32_t step) noexcept;
    void moveRight(uint32_t step) noexcept;
    void draw(std::shared_ptr<magma::CommandBuffer> cmdBuffer,
        std::shared_ptr<magma::Image> dstImage);

private:
    std::shared_ptr<magma::aux::Sprite> sprite;
    int32_t x = 0, y = 0;
    Direction direction = Direction::right;
    float scaling = 1.f;
    VkExtent2D extent = {0, 0};
};
