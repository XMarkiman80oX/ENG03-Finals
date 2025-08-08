#pragma once
#include "../Graphics/Material.h"
#include <memory>
#include <string>

namespace dx3d
{
    struct MaterialComponent
    {
        std::shared_ptr<Material> material;
        std::string textureFileName;
        bool hasTexture = false;
    };
}