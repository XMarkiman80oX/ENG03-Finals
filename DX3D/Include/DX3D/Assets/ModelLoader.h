#pragma once
#include <DX3D/Graphics/Primitives/Model.h>
#include <DX3D/Graphics/GraphicsResource.h>
#include <string>
#include <memory>
#include <unordered_map>

namespace dx3d
{
    class ModelLoader
    {
    public:
        // Static loading methods
        static std::shared_ptr<Model> LoadModel(
            const std::string& filePath,
            const GraphicsResourceDesc& resourceDesc
        );

        // Async loading support (for future implementation)
        struct LoadingProgress
        {
            float percentage = 0.0f;
            bool isComplete = false;
            bool hasError = false;
            std::string errorMessage;
        };

    private:
        // Private constructor - static class only
        ModelLoader() = delete;

        // Helper methods
        static bool loadOBJ(
            const std::string& filePath,
            std::shared_ptr<Model>& model,
            const GraphicsResourceDesc& resourceDesc
        );

        static std::shared_ptr<Material> loadMaterial(
            const std::string& materialName,
            const std::string& materialFile,
            const std::string& baseDirectory,
            const GraphicsResourceDesc& resourceDesc
        );

        static std::string getDirectory(const std::string& filePath);
        static std::string getAssetPath(const std::string& relativePath);
        static std::shared_ptr<Model> createDefaultModel(const GraphicsResourceDesc& resourceDesc);

        // Material cache to avoid loading the same material multiple times
        static std::unordered_map<std::string, std::shared_ptr<Material>> s_materialCache;
    };
}