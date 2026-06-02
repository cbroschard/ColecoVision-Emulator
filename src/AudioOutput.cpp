// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <stdexcept>
#include "AudioOutput.h"

AudioOutput::AudioOutput() :
    psg(nullptr),
    stream(nullptr),
    initialized(false)
{
    desired.freq = SAMPLE_RATE;
    desired.format = SDL_AUDIO_S16;
    desired.channels = CHANNELS;

    if (!SDL_Init(SDL_INIT_AUDIO))
    {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }
}

AudioOutput::~AudioOutput()
{
    stopAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool AudioOutput::playAudio()
{
    if (initialized)
    {
        resumeAudio();
        return true;
    }

    desired.freq = SAMPLE_RATE;
    desired.format = SDL_AUDIO_S16;
    desired.channels = CHANNELS;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                       &desired,
                                       nullptr,
                                       nullptr);

    if (!stream)
    {
        SDL_Log("SDL_OpenAudioDeviceStream failed: %s", SDL_GetError());
        return false;
    }

    initialized = true;

    // SDL3 starts paused. Resume the device associated with this stream.
    if (!SDL_ResumeAudioStreamDevice(stream))
    {
        SDL_Log("SDL_ResumeAudioStreamDevice failed: %s", SDL_GetError());
        stopAudio();
        return false;
    }

    return true;
}

void AudioOutput::pauseAudio()
{
    if (stream)
        SDL_PauseAudioStreamDevice(stream);
}

void AudioOutput::resumeAudio()
{
    if (stream)
        SDL_ResumeAudioStreamDevice(stream);
}

void AudioOutput::stopAudio()
{
    if (stream)
    {
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }

    initialized = false;
}

void AudioOutput::queueSamples()
{
    if (!stream)
        return;

    // Avoid building up too much latency.
    const int queuedBytes = SDL_GetAudioStreamQueued(stream);
    const int maxQueuedBytes = BUFFER_SIZE * CHANNELS * static_cast<int>(sizeof(int16_t)) * 4;

    if (queuedBytes >= maxQueuedBytes)
        return;

    std::vector<int16_t> buffer;
    buffer.resize(BUFFER_SIZE * CHANNELS);

    for (int i = 0; i < BUFFER_SIZE; ++i)
    {
        float s = 0.0f;

        if (psg)
            s = psg->sample();

        s *= MASTER_VOLUME;
        s = std::clamp(s, -1.0f, 1.0f);

        const int16_t out = static_cast<int16_t>(s * 32767.0f);

        // Mono PSG duplicated to stereo.
        buffer[i * 2 + 0] = out;
        buffer[i * 2 + 1] = out;
    }

    if (!SDL_PutAudioStreamData(stream,
                                buffer.data(),
                                static_cast<int>(buffer.size() * sizeof(int16_t))))
    {
        SDL_Log("SDL_PutAudioStreamData failed: %s", SDL_GetError());
    }
}
