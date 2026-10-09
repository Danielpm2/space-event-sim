#pragma once

#include "render/ApproachFx.h"
#include "render/AudioDevice.h"
#include "render/CockpitRenderer.h"
#include "render/ShipModel.h"
#include "render/FullscreenTriangle.h"
#include "render/ImGuiLayer.h"
#include "render/PostProcess.h"
#include "render/Shader.h"
#include "render/SimRenderer.h"
#include "render/Window.h"

#include "core/ApproachController.h"
#include "core/PostFxSettings.h"
#include "core/ShipAudio.h"
#include "core/Simulation.h"

#include <memory>
#include <string>

namespace render {

// Owns the window and drives the menu <-> simulation state machine.
class App {
public:
    App();
    int run();
    core::PostFxSettings& postFx() { return m_fx; }
    void launchSimulation(size_t index);
    // Dev aid: fast-forward the running simulation by `seconds`.
    void warpSimulation(double seconds);
    // Dev aid: skip all ImGui drawing so screenshots show only the scene.
    void hideUi() { m_noUi = true; }
    // Dev aid: write `frames` PPMs named <prefix>NNNN.ppm at a fixed 30 fps timestep, then exit.
    void recordAndExit(std::string prefix, int frames) {
        m_recordPrefix = std::move(prefix);
        m_recordFrames = frames;
    }
    // Dev aid: begin the cinematic approach at progress `u` (0..1) in the running simulation.
    void startApproach(float u);
    // Dev aid: choose the flyby course on the aim grid, each axis -1..1 (x right, y up).
    void setApproachAim(float x, float y) { m_approach.setAim({x, y}); }
    // Dev aid: never open an audio device.
    void disableAudio() { m_audioDisabled = true; }
    // Dev aid: draw the ship model from a free camera in the file's coordinates (yaw 0 looks down -Z).
    void setShipPreview(glm::vec3 eye, float yawDeg, float pitchDeg) {
        m_previewOn = true;
        m_previewEye = eye;
        m_previewYaw = yawDeg;
        m_previewPitch = pitchDeg;
    }
    // Dev aid: turn the camera around the target at `radPerSec`.
    void setAutoOrbit(float radPerSec) { m_autoOrbit = radPerSec; }
    // Dev aid: after `frames` frames write a PPM of the framebuffer and exit.
    void captureAndExit(std::string path, int frames) {
        m_capturePath = std::move(path);
        m_captureFrames = frames;
    }

private:
    void backToMenu();
    void handleCameraInput(double dt);
    void drawSimulationUi();
    void drawHelp();
    void drawOverlays();
    void drawFps();
    void drawApproachHud();
    void drawDeathOverlay();
    void drawAimGrid();
    void updateAudio();
    float flightSpeed01() const;
    void toggleApproach();
    void restartApproach();
    bool keyPressedOnce(int key);

    Window m_window;
    ImGuiLayer m_imgui;
    Shader m_menuBg;
    FullscreenTriangle m_triangle;
    PostProcess m_post;
    ApproachFx m_approachFx;
    CockpitRenderer m_cockpit;
    ShipModel m_ship;
    bool m_previewOn = false;
    glm::vec3 m_previewEye{0.f};
    float m_previewYaw = 0.f, m_previewPitch = 0.f;
    bool m_cockpitOn = true;
    core::ApproachController m_approach;
    core::ShipAudio m_shipAudio; // declared before m_audio: the device must stop first
    std::unique_ptr<AudioDevice> m_audio;
    bool m_audioOn = true;
    bool m_audioDisabled = false;
    float m_audioVolume = 0.7f;
    core::PostFxSettings m_fx;

    std::unique_ptr<core::Simulation> m_sim;
    std::unique_ptr<SimRenderer> m_renderer;
    core::OrbitCamera m_defaultCamera;
    std::string m_error;
    std::string m_capturePath;
    int m_captureFrames = 0;
    std::string m_recordPrefix;
    int m_recordFrames = 0;
    int m_recordIndex = 0;
    float m_autoOrbit = 0.f;
    bool m_dragging = false;
    bool m_wasMouseDown = false;
    size_t m_activeIndex = 0;
    bool m_paused = false;
    bool m_uiVisible = true;
    bool m_noUi = false;
    bool m_showHelp = false;
    float m_fade = 0.f; // 1 = fully black, eases to 0 after a screen change
    bool m_keyDown[512] = {};
};

} // namespace render
