#pragma once

#include "core/Simulation.h"

namespace core {

// Temporary placeholder simulation used to exercise the menu/registry.
class StubSim : public Simulation {
public:
    StubSim(std::string name, float hue) : hue(hue), m_name(std::move(name)) {}

    void update(double dt) override { m_time += dt * speed; }
    std::vector<Param> params() override {
        return {{"Speed", &speed, 0.f, 5.f}, {"Hue", &hue, 0.f, 1.f}};
    }
    std::string name() const override { return m_name; }

    double time() const { return m_time; }

    float speed = 1.f;
    float hue;

private:
    std::string m_name;
    double m_time = 0.0;
};

} // namespace core
