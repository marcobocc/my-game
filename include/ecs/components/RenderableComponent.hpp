#pragma once
#include "core/Material.hpp"
#include "core/Mesh.hpp"

struct RenderableComponent {
    Mesh* mesh;
    Material* material;
};
