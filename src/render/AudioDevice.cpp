#include "render/AudioDevice.h"

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <cstdio>

namespace render {

struct AudioDevice::Impl {
    ma_device device{};
    bool ok = false;
};

namespace {

void dataCallback(ma_device* device, void* output, const void*, ma_uint32 frames) {
    static_cast<core::ShipAudio*>(device->pUserData)->render(static_cast<float*>(output), static_cast<int>(frames));
}

} // namespace

AudioDevice::AudioDevice(core::ShipAudio& source) : m_impl(std::make_unique<Impl>()) {
    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate = 48000; // matches ShipAudio's default; the device resamples if needed
    cfg.dataCallback = dataCallback;
    cfg.pUserData = &source;

    if (ma_device_init(nullptr, &cfg, &m_impl->device) != MA_SUCCESS) {
        std::fprintf(stderr, "[Audio] no output device; running silent\n");
        return;
    }
    if (ma_device_start(&m_impl->device) != MA_SUCCESS) {
        std::fprintf(stderr, "[Audio] cannot start the output device; running silent\n");
        ma_device_uninit(&m_impl->device);
        return;
    }
    m_impl->ok = true;
}

AudioDevice::~AudioDevice() {
    if (m_impl && m_impl->ok)
        ma_device_uninit(&m_impl->device);
}

bool AudioDevice::ok() const { return m_impl && m_impl->ok; }

} // namespace render
