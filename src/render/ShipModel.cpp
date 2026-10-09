#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "render/ShipModel.h"

#include "render/GlCheck.h"
#include "render/Shader.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <future>
#include <map>
#include <string>
#include <vector>

namespace render {

namespace {

constexpr int kMaxTextureSize = 2048; // the file ships 4096^2 maps; half size is plenty for a cockpit

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 color;
};

struct TexRef {
    int image = -1;
    bool srgb = false;
};

enum class AlphaMode { Opaque, Mask, Blend };

struct CpuMaterial {
    glm::vec4 baseColor{1.f};
    float metallic = 1.f;
    float roughness = 1.f;
    glm::vec3 emissive{0.f}; // factor * strength
    float alphaCutoff = 0.5f;
    AlphaMode alpha = AlphaMode::Opaque;
    TexRef base, metalRough, normal, emissiveTex;
};

struct CpuPrim {
    uint32_t first = 0;
    uint32_t count = 0;
    int material = 0;
};

struct CpuImage {
    int w = 0, h = 0;
    std::vector<unsigned char> rgba;
};

struct CpuModel {
    std::vector<Vertex> verts;
    std::vector<uint32_t> indices;
    std::vector<CpuPrim> prims;
    std::vector<CpuMaterial> materials;
    std::vector<CpuImage> images;
    std::string error;
};

CpuImage decodeImage(const unsigned char* data, size_t size) {
    CpuImage img;
    int ch = 0;
    unsigned char* px = stbi_load_from_memory(data, static_cast<int>(size), &img.w, &img.h, &ch, 4);
    if (!px)
        return img;
    img.rgba.assign(px, px + static_cast<size_t>(img.w) * img.h * 4);
    stbi_image_free(px);

    // 2x2 box filter until it fits.
    while (img.w > kMaxTextureSize || img.h > kMaxTextureSize) {
        const int nw = std::max(1, img.w / 2), nh = std::max(1, img.h / 2);
        std::vector<unsigned char> out(static_cast<size_t>(nw) * nh * 4);
        for (int y = 0; y < nh; ++y)
            for (int x = 0; x < nw; ++x)
                for (int c = 0; c < 4; ++c) {
                    const int x0 = std::min(2 * x, img.w - 1), x1 = std::min(2 * x + 1, img.w - 1);
                    const int y0 = std::min(2 * y, img.h - 1), y1 = std::min(2 * y + 1, img.h - 1);
                    const int sum = img.rgba[(static_cast<size_t>(y0) * img.w + x0) * 4 + c] +
                                    img.rgba[(static_cast<size_t>(y0) * img.w + x1) * 4 + c] +
                                    img.rgba[(static_cast<size_t>(y1) * img.w + x0) * 4 + c] +
                                    img.rgba[(static_cast<size_t>(y1) * img.w + x1) * 4 + c];
                    out[(static_cast<size_t>(y) * nw + x) * 4 + c] = static_cast<unsigned char>((sum + 2) / 4);
                }
        img.rgba = std::move(out);
        img.w = nw;
        img.h = nh;
    }
    return img;
}

TexRef texRef(const cgltf_data* data, const cgltf_texture_view& view, bool srgb) {
    TexRef r;
    if (view.texture && view.texture->image) {
        r.image = static_cast<int>(view.texture->image - data->images);
        r.srgb = srgb;
    }
    return r;
}

CpuModel loadGlb(const std::filesystem::path& file) {
    CpuModel m;
    const std::string path = file.string();
    cgltf_options options{};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        m.error = "cannot parse " + path;
        return m;
    }
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        cgltf_free(data);
        m.error = "cannot load buffers of " + path;
        return m;
    }

    // Images decode in parallel; they dominate the load time.
    std::vector<std::future<CpuImage>> decoding;
    for (size_t i = 0; i < data->images_count; ++i) {
        const cgltf_buffer_view* bv = data->images[i].buffer_view;
        if (!bv) {
            decoding.emplace_back(std::async(std::launch::deferred, [] { return CpuImage{}; }));
            continue;
        }
        const unsigned char* bytes = static_cast<const unsigned char*>(bv->buffer->data) + bv->offset;
        decoding.emplace_back(std::async(std::launch::async, decodeImage, bytes, static_cast<size_t>(bv->size)));
    }

    for (size_t i = 0; i < data->materials_count; ++i) {
        const cgltf_material& src = data->materials[i];
        CpuMaterial mat;
        if (src.has_pbr_metallic_roughness) {
            const auto& pbr = src.pbr_metallic_roughness;
            mat.baseColor = glm::make_vec4(pbr.base_color_factor);
            mat.metallic = pbr.metallic_factor;
            mat.roughness = pbr.roughness_factor;
            mat.base = texRef(data, pbr.base_color_texture, true);
            mat.metalRough = texRef(data, pbr.metallic_roughness_texture, false);
        }
        mat.normal = texRef(data, src.normal_texture, false);
        mat.emissiveTex = texRef(data, src.emissive_texture, true);
        const float strength = src.has_emissive_strength ? src.emissive_strength.emissive_strength : 1.f;
        mat.emissive = glm::make_vec3(src.emissive_factor) * strength;
        mat.alphaCutoff = src.alpha_cutoff;
        // The file marks the hull BLEND to carry a cut-out opacity map; sorting 500k triangles is
        // pointless, so a texture alpha on the opaque hull is treated as a mask instead.
        mat.alpha = src.alpha_mode == cgltf_alpha_mode_opaque ? AlphaMode::Opaque : AlphaMode::Blend;
        m.materials.push_back(mat);
    }
    m.materials.push_back(CpuMaterial{}); // fallback for primitives without a material

    for (size_t n = 0; n < data->nodes_count; ++n) {
        const cgltf_node* node = &data->nodes[n];
        if (!node->mesh)
            continue;
        float wm[16];
        cgltf_node_transform_world(node, wm);
        const glm::mat4 world = glm::make_mat4(wm);
        const glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(world));

        for (size_t p = 0; p < node->mesh->primitives_count; ++p) {
            const cgltf_primitive& prim = node->mesh->primitives[p];
            if (prim.type != cgltf_primitive_type_triangles)
                continue;
            const cgltf_accessor *pos = nullptr, *nrm = nullptr, *uv = nullptr, *col = nullptr;
            for (size_t a = 0; a < prim.attributes_count; ++a) {
                const cgltf_attribute& at = prim.attributes[a];
                if (at.index != 0)
                    continue;
                if (at.type == cgltf_attribute_type_position) pos = at.data;
                if (at.type == cgltf_attribute_type_normal) nrm = at.data;
                if (at.type == cgltf_attribute_type_texcoord) uv = at.data;
                if (at.type == cgltf_attribute_type_color) col = at.data;
            }
            if (!pos)
                continue;

            const size_t base = m.verts.size();
            const size_t count = pos->count;
            std::vector<float> buf(count * 4);
            m.verts.resize(base + count);
            cgltf_accessor_unpack_floats(pos, buf.data(), count * 3);
            for (size_t v = 0; v < count; ++v)
                m.verts[base + v].pos = glm::vec3(world * glm::vec4(buf[3 * v], buf[3 * v + 1], buf[3 * v + 2], 1.f));
            for (size_t v = 0; v < count; ++v)
                m.verts[base + v].normal = glm::vec3(0.f, 1.f, 0.f);
            if (nrm) {
                cgltf_accessor_unpack_floats(nrm, buf.data(), count * 3);
                for (size_t v = 0; v < count; ++v)
                    m.verts[base + v].normal =
                        glm::normalize(normalMat * glm::vec3(buf[3 * v], buf[3 * v + 1], buf[3 * v + 2]));
            }
            for (size_t v = 0; v < count; ++v)
                m.verts[base + v].uv = glm::vec2(0.f);
            if (uv) {
                cgltf_accessor_unpack_floats(uv, buf.data(), count * 2);
                for (size_t v = 0; v < count; ++v)
                    m.verts[base + v].uv = glm::vec2(buf[2 * v], buf[2 * v + 1]);
            }
            for (size_t v = 0; v < count; ++v)
                m.verts[base + v].color = glm::vec4(1.f);
            if (col) {
                const size_t comps = cgltf_num_components(col->type);
                cgltf_accessor_unpack_floats(col, buf.data(), count * comps);
                for (size_t v = 0; v < count; ++v)
                    m.verts[base + v].color = glm::vec4(buf[comps * v], buf[comps * v + 1], buf[comps * v + 2],
                                                        comps == 4 ? buf[comps * v + 3] : 1.f);
            }

            CpuPrim out;
            out.first = static_cast<uint32_t>(m.indices.size());
            if (prim.indices) {
                for (size_t i = 0; i < prim.indices->count; ++i)
                    m.indices.push_back(static_cast<uint32_t>(base + cgltf_accessor_read_index(prim.indices, i)));
            } else {
                for (size_t i = 0; i < count; ++i)
                    m.indices.push_back(static_cast<uint32_t>(base + i));
            }
            out.count = static_cast<uint32_t>(m.indices.size()) - out.first;
            out.material = prim.material ? static_cast<int>(prim.material - data->materials)
                                         : static_cast<int>(data->materials_count);
            m.prims.push_back(out);
        }
    }
    // The decoders read straight from the file's buffers, so wait for them before freeing it.
    for (auto& f : decoding)
        m.images.push_back(f.get());
    cgltf_free(data);

    if (m.prims.empty())
        m.error = "no triangle meshes in " + path;
    return m;
}

} // namespace

struct ShipModel::Impl {
    std::future<CpuModel> loading;
    bool started = false;
    bool uploaded = false;
    bool failed = false;

    std::unique_ptr<Shader> shader;
    GLuint vao = 0, vbo = 0, ebo = 0;
    std::vector<CpuPrim> prims;
    std::vector<CpuMaterial> materials;
    std::map<int, GLuint> textures; // key = image * 2 + srgb

    ~Impl() {
        if (loading.valid())
            loading.wait();
        for (auto& [key, tex] : textures)
            glDeleteTextures(1, &tex);
        if (ebo) glDeleteBuffers(1, &ebo);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
    }

    GLuint texture(const std::vector<CpuImage>& images, const TexRef& ref) {
        if (ref.image < 0 || ref.image >= static_cast<int>(images.size()) || images[ref.image].rgba.empty())
            return 0;
        const int key = ref.image * 2 + (ref.srgb ? 1 : 0);
        auto it = textures.find(key);
        if (it != textures.end())
            return it->second;
        const CpuImage& img = images[ref.image];
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, ref.srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, img.w, img.h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, img.rgba.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        textures[key] = tex;
        return tex;
    }

    std::vector<std::array<GLuint, 4>> bound; // per material: base, metal-rough, normal, emissive

    void upload(CpuModel& cpu) {
        shader = std::make_unique<Shader>("ship.vert.glsl", "ship.frag.glsl");

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpu.verts.size() * sizeof(Vertex)), cpu.verts.data(),
                     GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpu.indices.size() * sizeof(uint32_t)),
                     cpu.indices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, pos)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
        glBindVertexArray(0);

        for (const CpuMaterial& mat : cpu.materials)
            bound.push_back({texture(cpu.images, mat.base), texture(cpu.images, mat.metalRough),
                             texture(cpu.images, mat.normal), texture(cpu.images, mat.emissiveTex)});
        prims = std::move(cpu.prims);
        materials = std::move(cpu.materials);
        uploaded = true;
        checkGl("ShipModel upload");
    }
};

ShipModel::ShipModel() : m_impl(std::make_unique<Impl>()) {}
ShipModel::~ShipModel() = default;

void ShipModel::startLoading(const std::filesystem::path& file) {
    if (m_impl->started)
        return;
    m_impl->started = true;
    if (!std::filesystem::exists(file)) {
        std::fprintf(stderr, "[Ship] %s not found; using the built-in cockpit\n", file.string().c_str());
        m_impl->failed = true;
        return;
    }
    m_impl->loading = std::async(std::launch::async, loadGlb, file);
}

bool ShipModel::started() const { return m_impl->started; }
bool ShipModel::failed() const { return m_impl->failed; }

bool ShipModel::poll() {
    Impl& s = *m_impl;
    if (s.uploaded)
        return true;
    if (s.failed || !s.loading.valid())
        return false;
    if (s.loading.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return false;
    CpuModel cpu = s.loading.get();
    if (!cpu.error.empty()) {
        std::fprintf(stderr, "[Ship] %s; using the built-in cockpit\n", cpu.error.c_str());
        s.failed = true;
        return false;
    }
    s.upload(cpu);
    return true;
}

bool ShipModel::waitUntilReady() {
    if (m_impl->loading.valid())
        m_impl->loading.wait();
    return poll();
}

void ShipModel::draw(const glm::mat4& proj, const glm::mat4& modelView, const ShipLighting& light) {
    Impl& s = *m_impl;
    if (!s.uploaded)
        return;

    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    s.shader->use();
    s.shader->set("uProj", proj);
    s.shader->set("uModelView", modelView);
    s.shader->set("uNormalMat", glm::inverseTranspose(glm::mat3(modelView)));
    s.shader->set("uEventDir", light.eventDir);
    s.shader->set("uEventColor", light.eventColor);
    s.shader->set("uCabinDir", light.cabinDir);
    s.shader->set("uCabinColor", light.cabinColor);
    s.shader->set("uAmbient", light.ambient);
    s.shader->set("uEmissiveScale", light.emissiveScale);
    s.shader->set("uTime", light.time);
    for (int i = 0; i < 4; ++i) {
        static const char* names[4] = {"uBase", "uMetalRough", "uNormalMap", "uEmissive"};
        s.shader->set(names[i], i);
    }

    glBindVertexArray(s.vao);
    for (int pass = 0; pass < 2; ++pass) {
        if (pass == 1) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
        }
        for (const CpuPrim& prim : s.prims) {
            const CpuMaterial& mat = s.materials[prim.material];
            const bool blended = mat.alpha == AlphaMode::Blend && mat.emissiveTex.image < 0 && mat.emissive == glm::vec3(0.f);
            if ((pass == 1) != blended)
                continue;
            const auto& tex = s.bound[prim.material];
            for (int unit = 0; unit < 4; ++unit) {
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, tex[unit]);
            }
            s.shader->set("uHasBase", tex[0] ? 1 : 0);
            s.shader->set("uHasMetalRough", tex[1] ? 1 : 0);
            s.shader->set("uHasNormal", tex[2] ? 1 : 0);
            s.shader->set("uHasEmissive", tex[3] ? 1 : 0);
            s.shader->set("uBaseFactor", mat.baseColor);
            s.shader->set("uMetallic", mat.metallic);
            s.shader->set("uRoughness", mat.roughness);
            s.shader->set("uEmissiveFactor", mat.emissive);
            s.shader->set("uAlphaMode", blended ? 2 : (mat.alpha == AlphaMode::Opaque ? 0 : 1));
            s.shader->set("uAlphaCutoff", mat.alphaCutoff);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(prim.count), GL_UNSIGNED_INT,
                           reinterpret_cast<void*>(static_cast<uintptr_t>(prim.first) * sizeof(uint32_t)));
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
}

} // namespace render
