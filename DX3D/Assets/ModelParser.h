#pragma once
#include "../Graphics/Primitives/Model.h"
#include "../Graphics/GraphicsResource.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <sstream>

namespace dx3d
{
    class ModelParser
    {
    private:
        ModelParser() = delete;

        static bool parseOBJ(
            const std::string& path,
            std::shared_ptr<Model>& asset,
            const GraphicsResourceDesc& resDesc
        );

        static std::shared_ptr<Material> retrieveMaterial(
            const std::string& name,
            const std::string& file,
            const std::string& directory,
            const GraphicsResourceDesc& resDesc
        );

        static std::string findAssetPath(const std::string& path);
        static std::string extractDirectory(const std::string& path);
        static std::shared_ptr<Model> generateDefaultModel(const GraphicsResourceDesc& resDesc);

        static std::unordered_map<std::string, std::shared_ptr<Material>> m_materialRegistry;

    public:
        static std::shared_ptr<Model> LoadModel(
            const std::string& path,
            const GraphicsResourceDesc& resDesc
        );

        struct ProgressReport
        {
            float value = 0.0f;
            bool isDone = false;
            bool hasError = false;
            std::string message;
        };
    };
}