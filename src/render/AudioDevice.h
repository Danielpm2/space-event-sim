#pragma once

#include "core/ShipAudio.h"

#include <memory>

namespace render {

// Plays a ShipAudio on the default output device. If no device can be opened it
// stays silent and the app carries on.
class AudioDevice {
public:
    explicit AudioDevice(core::ShipAudio& source);
    ~AudioDevice();
    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    bool ok() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace render
