// Unit tests for the GL-free simulation core. No framework: each CHECK counts a failure.
#include "core/ApproachController.h"
#include "core/BlackHoleSim.h"
#include "core/MagnetarSim.h"
#include "core/MergerSim.h"
#include "core/OrbitCamera.h"
#include "core/PulsarSim.h"
#include "core/SupernovaSim.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                      \
    do {                                                                                 \
        if (!(cond)) {                                                                   \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                \
            ++g_failures;                                                                \
        }                                                                                \
    } while (0)

#define CHECK_NEAR(a, b, tol)                                                            \
    do {                                                                                 \
        const double va = (a), vb = (b);                                                 \
        if (!(std::fabs(va - vb) <= (tol))) {                                            \
            std::printf("  FAIL %s:%d: %s = %g, expected %g +/- %g\n", __FILE__, __LINE__, #a, va, vb, \
                        static_cast<double>(tol));                                       \
            ++g_failures;                                                                \
        }                                                                                \
    } while (0)

void blackHoleMatchesKerr() {
    core::BlackHoleSim bh;
    bh.mass = 1.f;

    bh.spin = 0.f; // Schwarzschild
    CHECK_NEAR(bh.horizonRadius(), 2.0, 1e-4);
    CHECK_NEAR(bh.iscoRadius(), 6.0, 1e-3);
    CHECK_NEAR(bh.schwarzschildRadius(), 2.0, 1e-6);

    bh.spin = 0.998f; // near-extremal: horizon -> 1 M, prograde ISCO -> 1.24 M
    CHECK_NEAR(bh.horizonRadius(), 1.0632, 1e-3);
    CHECK_NEAR(bh.iscoRadius(), 1.237, 0.01);

    bh.mass = 2.f; // lengths scale with M
    bh.spin = 0.f;
    CHECK_NEAR(bh.horizonRadius(), 4.0, 1e-4);
    CHECK_NEAR(bh.iscoRadius(), 12.0, 2e-3);

    bh.mass = 1.f;
    float prevIsco = 1e9f;
    for (float a = 0.f; a <= 0.99f; a += 0.05f) {
        bh.spin = a;
        CHECK(bh.iscoRadius() > bh.horizonRadius()); // the disk never reaches the hole
        CHECK(bh.iscoRadius() < prevIsco);           // prograde ISCO shrinks with spin
        prevIsco = bh.iscoRadius();
    }
}

void cameraOrbitAndZoom() {
    core::OrbitCamera cam;
    cam.target = glm::vec3(1.f, 2.f, 3.f);
    cam.distance = 10.f;
    CHECK_NEAR(glm::length(cam.position() - cam.target), 10.0, 1e-4);
    CHECK_NEAR(glm::dot(cam.forward(), glm::normalize(cam.target - cam.position())), 1.0, 1e-5);
    CHECK_NEAR(glm::dot(cam.right(), cam.forward()), 0.0, 1e-5);
    CHECK_NEAR(glm::dot(cam.up(), cam.forward()), 0.0, 1e-5);

    cam.orbit(0.f, 100.f); // pitch stops short of the pole
    CHECK(cam.pitch < glm::half_pi<float>());
    cam.orbit(0.f, -200.f);
    CHECK(cam.pitch > -glm::half_pi<float>());

    cam.zoom(1000.f);
    CHECK_NEAR(cam.distance, cam.maxDistance, 1e-5);
    cam.zoom(1e-6f);
    CHECK_NEAR(cam.distance, cam.minDistance, 1e-5);
}

void pulsarLighthouse() {
    core::PulsarSim p;
    const glm::vec3 m = p.magneticAxis();
    CHECK_NEAR(glm::length(m), 1.0, 1e-5);
    CHECK_NEAR(m.y, std::cos(p.tiltDeg * glm::pi<float>() / 180.f), 1e-5); // tilt from the spin axis

    CHECK_NEAR(p.pulseIntensity(m), 1.0, 1e-5);  // looking down the north beam
    CHECK_NEAR(p.pulseIntensity(-m), 1.0, 1e-5); // and the south beam
    const glm::vec3 side = glm::normalize(glm::cross(m, glm::vec3(0.f, 0.f, 1.f)));
    CHECK(p.pulseIntensity(side) < 1e-3f);       // 90 degrees off-axis sees nothing

    const float before = p.phase();
    p.update(0.1);
    CHECK_NEAR(p.phase() - before, 2.f * glm::pi<float>() * p.spinRate * 0.1f, 1e-4);

    for (int i = 0; i < 2000; ++i)
        p.update(0.01);
    CHECK(!p.particles().empty());
    CHECK(p.particles().size() <= core::PulsarSim::kMaxParticles);
}

void supernovaTimeline() {
    core::SupernovaSim sn;
    CHECK(!sn.exploded());
    CHECK(sn.shockRadius() == 0.f);
    CHECK(sn.progenitorRadius() > 0.f);

    float prev = 0.f;
    bool exploded = false;
    for (int i = 0; i < 1600; ++i) { // 16 s
        sn.update(0.01);
        if (sn.exploded()) {
            exploded = true;
            CHECK(sn.progenitorRadius() == 0.f);
            CHECK(sn.shockRadius() >= prev); // the shock never shrinks
            prev = sn.shockRadius();
        }
    }
    CHECK(exploded);
    CHECK(prev > 1.f);

    // R ~ sqrt(E)
    core::SupernovaSim a, b;
    b.energy = 4.f;
    for (int i = 0; i < 800; ++i) {
        a.update(0.01);
        b.update(0.01);
    }
    CHECK_NEAR(b.shockRadius() / a.shockRadius(), 2.0, 1e-3);

    // The cycle loops instead of running off the end.
    core::SupernovaSim loop;
    for (int i = 0; i < 4000; ++i) // 40 s > kCycleTime
        loop.update(0.01);
    CHECK(loop.time() < core::SupernovaSim::kCycleTime);
}

void mergerInspiralAndLoop() {
    core::MergerSim m;
    float prevSep = m.separation();
    double mergeTime = -1.0, t = 0.0;
    bool looped = false;
    for (int i = 0; i < 6000 && !looped; ++i) { // up to 60 s
        m.update(0.01);
        t += 0.01;
        if (!m.merged() && mergeTime < 0.0) {
            CHECK(m.separation() <= prevSep + 1e-6f); // gravitational waves only shrink the orbit
            prevSep = m.separation();
        }
        if (m.merged() && mergeTime < 0.0)
            mergeTime = t;
        if (mergeTime > 0.0 && !m.merged())
            looped = true;
    }
    CHECK(mergeTime > 8.0 && mergeTime < 25.0);
    CHECK(looped);

    // Heavier stars merge sooner.
    core::MergerSim heavy;
    heavy.mass1 = heavy.mass2 = 2.5f;
    double heavyTime = -1.0;
    t = 0.0;
    for (int i = 0; i < 6000 && heavyTime < 0.0; ++i) {
        heavy.update(0.01);
        t += 0.01;
        if (heavy.merged())
            heavyTime = t;
    }
    CHECK(heavyTime > 0.0 && heavyTime < mergeTime);
}

void magnetarFieldAndFlare() {
    core::MagnetarSim mag;
    // A dipole line r = L sin^2(theta) starts and ends on the star's surface.
    for (const auto& line : mag.fieldLines()) {
        CHECK(line.size() >= 2);
        CHECK_NEAR(glm::length(line.front()), core::MagnetarSim::kStarRadius, 1e-3);
        CHECK_NEAR(glm::length(line.back()), core::MagnetarSim::kStarRadius, 1e-3);
        for (const auto& p : line)
            CHECK(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z));
    }

    float maxTwist = 0.f, twistBeforeFlare = 0.f;
    bool flared = false;
    for (int i = 0; i < 800 && !flared; ++i) { // first flare is due after half an interval
        const float before = mag.twist();
        mag.update(0.01);
        maxTwist = std::max(maxTwist, mag.twist());
        if (mag.flash() > 0.5f) {
            flared = true;
            twistBeforeFlare = before;
        }
    }
    CHECK(flared);
    CHECK(mag.twist() < twistBeforeFlare); // the quake relaxes the field
}

void paramsStartInsideTheirRange() {
    std::vector<std::unique_ptr<core::Simulation>> sims;
    sims.push_back(std::make_unique<core::BlackHoleSim>());
    sims.push_back(std::make_unique<core::PulsarSim>());
    sims.push_back(std::make_unique<core::SupernovaSim>());
    sims.push_back(std::make_unique<core::MergerSim>());
    sims.push_back(std::make_unique<core::MagnetarSim>());
    for (auto& s : sims) {
        CHECK(!s->name().empty());
        for (const auto& p : s->params()) {
            CHECK(p.min < p.max);
            CHECK(*p.value >= p.min && *p.value <= p.max);
        }
    }
}

void cameraRollStaysOrthonormal() {
    core::OrbitCamera cam;
    cam.roll = 0.7f;
    CHECK_NEAR(glm::length(cam.right()), 1.0, 1e-5);
    CHECK_NEAR(glm::length(cam.up()), 1.0, 1e-5);
    CHECK_NEAR(glm::dot(cam.right(), cam.up()), 0.0, 1e-5);
    CHECK_NEAR(glm::dot(cam.up(), cam.forward()), 0.0, 1e-5);

    const core::CameraPose p{1.f, 0.2f, 55.f, 0.1f, 70.f, glm::vec3(1.f, 2.f, 3.f)};
    cam.setPose(p);
    CHECK_NEAR(cam.distance, 55.0, 1e-6); // scripted poses ignore the zoom limits
    CHECK_NEAR(cam.pose().fovY, 70.0, 1e-6);

    core::CameraPose fp;
    fp.firstPerson = true;
    fp.eye = glm::vec3(3.f, 4.f, 0.f);
    fp.heading = glm::vec3(0.f, 0.f, -2.f); // need not be normalised
    fp.roll = 0.3f;
    cam.setPose(fp);
    CHECK_NEAR(cam.distance, 5.0, 1e-6); // still |eye - target|, which renderers rely on
    CHECK_NEAR(glm::length(cam.forward()), 1.0, 1e-6);
    CHECK_NEAR(glm::dot(cam.right(), cam.up()), 0.0, 1e-5);
    CHECK_NEAR(glm::dot(cam.up(), cam.forward()), 0.0, 1e-5);
    CHECK_NEAR(glm::length(cam.position() - fp.eye), 0.0, 1e-6);
}

void approachFlythrough() {
    core::SupernovaSim sn;
    core::BlackHoleSim bh;
    core::PulsarSim pulsar;
    CHECK(sn.supportsApproach());
    CHECK(bh.supportsApproach());
    CHECK(!pulsar.supportsApproach());

    CHECK(bh.approachTimeDilation(1e6f) > 0.999f);
    CHECK(bh.approachTimeDilation(bh.schwarzschildRadius()) < 1e-3f);

    // Starts far, passes the event at the lateral offset, and leaves on the other side.
    using core::ApproachController;
    const glm::vec2 off(3.f, -2.f);
    float prevZ = 1e9f;
    for (float u = 0.f; u <= 1.f; u += 0.05f) {
        const glm::vec3 p = ApproachController::flightPosition(100.f, off, u);
        CHECK(p.z <= prevZ);
        CHECK_NEAR(p.x, 3.0, 1e-6);
        CHECK_NEAR(p.y, -2.0, 1e-6);
        prevZ = p.z;
    }
    CHECK_NEAR(ApproachController::flightPosition(100.f, off, 0.f).z, 100.0, 1e-4);
    CHECK_NEAR(ApproachController::flightPosition(100.f, off, 0.5f).z, 0.0, 1e-4);
    CHECK_NEAR(ApproachController::flightPosition(100.f, off, 1.f).z, -100.0, 1e-4);

    // The closest pass equals the chosen offset, and a survivable pass never blacks out.
    ApproachController ctl;
    const core::CameraPose before = bh.camera.pose();
    ctl.setAim({0.5f, 0.f});
    ctl.start(bh);
    CHECK(ctl.active());
    float closest = 1e9f;
    for (int i = 0; i < 3000 && !ctl.finished(); ++i) {
        ctl.update(bh, 0.05);
        closest = std::min(closest, glm::length(bh.camera.position()));
        CHECK(ctl.proximity() >= 0.f && ctl.proximity() <= 1.f);
    }
    CHECK(ctl.finished() && !ctl.dead());
    CHECK_NEAR(closest, 0.5 * bh.approachSpec().gridExtent, 0.3);
    CHECK_NEAR(ctl.deathFade(), 0.0, 1e-4);
    CHECK(bh.camera.position().x > 0.f); // aimed right of the event

    // The pilot looks at the event; looking elsewhere is a glance that fades.
    ctl.setProgress(0.3f);
    ctl.update(bh, 0.0);
    CHECK(glm::dot(bh.camera.forward(), glm::normalize(-bh.camera.position())) > 0.999f);
    CHECK_NEAR(glm::length(ctl.velocity()), 0.0, 1e-6); // paused: no motion streaks

    ctl.stop(bh);
    CHECK(!ctl.active());
    CHECK(!bh.camera.firstPerson);
    CHECK_NEAR(bh.camera.distance, before.distance, 1e-6); // the user's camera comes back

    // A direct hit is fatal: the screen goes black and the flight stops.
    for (core::Simulation* sim : {static_cast<core::Simulation*>(&bh), static_cast<core::Simulation*>(&sn)}) {
        ApproachController hit;
        hit.setAim({0.f, 0.f});
        hit.start(*sim);
        for (int i = 0; i < 3000 && !hit.finished(); ++i)
            hit.update(*sim, 0.05);
        CHECK(hit.dead());
        CHECK_NEAR(hit.deathFade(), 1.0, 1e-4);
        const float stopped = hit.progress();
        hit.update(*sim, 1.0);
        CHECK_NEAR(hit.progress(), stopped, 1e-6);
    }

    // Grid shapes follow the sim's state.
    CHECK(bh.approachLethalRadius() > bh.horizonRadius());
    bool sawHorizon = false;
    for (const auto& s : bh.approachShapes())
        sawHorizon |= !s.bar && std::fabs(s.radius - bh.horizonRadius()) < 1e-5f;
    CHECK(sawHorizon);

    core::SupernovaSim weak, strong;
    strong.energy = 4.f;
    CHECK(!weak.approachShapes().empty());
    CHECK_NEAR(strong.approachShapes()[0].radius / weak.approachShapes()[0].radius, 2.0, 1e-3);
}

struct Test {
    const char* name;
    void (*fn)();
};

} // namespace

int main() {
    const Test tests[] = {
        {"black hole matches Kerr", blackHoleMatchesKerr},
        {"camera orbit and zoom", cameraOrbitAndZoom},
        {"camera roll stays orthonormal", cameraRollStaysOrthonormal},
        {"approach flythrough", approachFlythrough},
        {"pulsar lighthouse", pulsarLighthouse},
        {"supernova timeline", supernovaTimeline},
        {"merger inspiral and loop", mergerInspiralAndLoop},
        {"magnetar field and flare", magnetarFieldAndFlare},
        {"params start inside their range", paramsStartInsideTheirRange},
    };
    for (const auto& t : tests) {
        const int before = g_failures;
        t.fn();
        std::printf("%s %s\n", g_failures == before ? "[ ok ]" : "[FAIL]", t.name);
    }
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
