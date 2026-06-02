// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <stdexcept>
#include "VideoOutput.h"

VideoOutput::VideoOutput() :
    window(nullptr),
    renderer(nullptr),
    texture(nullptr)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error(
            std::string("SDL Video Init Failed: ") + SDL_GetError()
        );
    }

    window = SDL_CreateWindow(
        "ColecoVision Emulator",
        SCREEN_WIDTH * SCALE,
        SCREEN_HEIGHT * SCALE,
        0
    );

    if (!window)
    {
        throw std::runtime_error(
            std::string("SDL_CreateWindow failed: ") + SDL_GetError()
        );
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    renderer = SDL_CreateRenderer(window, nullptr);

    if (!renderer)
    {
        throw std::runtime_error(
            std::string("SDL_CreateRenderer failed: ") + SDL_GetError()
        );
    }

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    if (!texture)
    {
        throw std::runtime_error(
            std::string("SDL_CreateTexture failed: ") + SDL_GetError()
        );
    }

    framebuffer.fill(0xFF000000);
}

VideoOutput::~VideoOutput()
{
    if (texture)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }

    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void VideoOutput::clear()
{
    framebuffer.fill(0xFF000000);

    if (!renderer)
        return;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void VideoOutput::present()
{
    if (!renderer || !texture)
        return;

    SDL_UpdateTexture(
        texture,
        nullptr,
        framebuffer.data(),
        SCREEN_WIDTH * sizeof(uint32_t)
    );

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_FRect dest;
    dest.x = 0.0f;
    dest.y = 0.0f;
    dest.w = static_cast<float>(SCREEN_WIDTH * SCALE);
    dest.h = static_cast<float>(SCREEN_HEIGHT * SCALE);

    SDL_RenderTexture(renderer, texture, nullptr, &dest);

    SDL_RenderPresent(renderer);
}

void VideoOutput::setPixel(int x, int y, uint8_t colorIndex)
{
    if (x < 0 || x >= 256 || y < 0 || y >= 192)
        return;

    static constexpr uint32_t palette[16] =
    {
        0xFF000000, // 0 transparent/black for now
        0xFF000000, // 1 black
        0xFF21C842, // 2 medium green
        0xFF5EDC78, // 3 light green
        0xFF5455ED, // 4 dark blue
        0xFF7D76FC, // 5 light blue
        0xFFD4524D, // 6 dark red
        0xFF42EBF5, // 7 cyan
        0xFFFC5554, // 8 medium red
        0xFFFF7978, // 9 light red
        0xFFD4C154, // A dark yellow
        0xFFE6CE80, // B light yellow
        0xFF21B03B, // C dark green
        0xFFC95BBA, // D magenta
        0xFFCCCCCC, // E gray
        0xFFFFFFFF  // F white
    };

    framebuffer[y * 256 + x] = palette[colorIndex & 0x0F];
}
