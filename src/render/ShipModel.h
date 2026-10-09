#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <filesystem>
#include <memory>

namespace render {

// Lighting for the ship interior, all in view space.
struct ShipLighting {
    glm::vec3 eventDir{0.f, 0.f, -1.f};  // unit vector toward the event
    glm::vec3 eventColor{0.f};           // radiance of the event light
    glm::vec3 cabinDir{0.f, 1.f, 0.f};   // unit vector toward the cabin light
    glm::vec3 cabinColor{0.f};
    glm::vec3 ambient{0.f};
    float emissiveScale = 0.25f;         // the file's emissive strengths are tuned for a daylight viewer
    float time = 0.f;
};

// A glTF/GLB ship interior. Decoding happens on a worker thread; the GPU upload
// happens in poll() on the main thread, so the app never stalls while it loads.
class ShipModel {
public:
    ShipModel();
    ~ShipModel();
    ShipModel(const ShipModel&) = delete;
    ShipModel& operator=(const ShipModel&) = delete;

    // No-op after the first call.
    void startLoading(const std::filesystem::path& file);
    // Uploads finished data. Returns true once the model can be drawn.
    bool poll();
    // Blocks until loading finishes (dev screenshots), then uploads.
    bool waitUntilReady();
    bool started() const;
    bool failed() const;

    // Draws into the current framebuffer with a fresh depth range. `modelView` maps
    // the file's coordinates (after its node transforms) into view space.
    void draw(const glm::mat4& proj, const glm::mat4& modelView, const ShipLighting& light);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace render
