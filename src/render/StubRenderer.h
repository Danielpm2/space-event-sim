#pragma once

#include "render/FullscreenTriangle.h"
#include "render/Shader.h"
#include "render/SimRenderer.h"

namespace render {

class StubRenderer : public SimRenderer {
public:
    StubRenderer();
    void draw(const core::Simulation& sim, int width, int height) override;

private:
    Shader m_shader;
    FullscreenTriangle m_triangle;
};

} // namespace render
