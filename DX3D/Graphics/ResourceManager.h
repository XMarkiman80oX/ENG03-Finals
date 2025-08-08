#pragma once
#include <../Graphics/Texture2D.h>
#include <../Graphics/Material.h>
#include <../Graphics/GraphicsResource.h>
#include <../Core/Logger.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>

namespace dx3d
{
    class ResourceManager
    {
    public:
        static ResourceManager& getInstance()
        {
            static ResourceManager instance;
            return instance;
        }

        // Initialize the resource manager with graphics resource description
        void initialize(const GraphicsResourceDesc& resourceDesc);

        // Shutdown and clear all cached resources
        void shutdown();

        // Texture loading methods
        std::shared_ptr<Texture2D> loadTexture(const std::string& fileName);
        std::shared_ptr<Texture2D> getTexture(const std::string& fileName) const;
        bool isTextureLoaded(const std::string& fileName) const;

        // Material creation methods
        std::shared_ptr<Material> createMaterial(const std::string& name = "");

        // Cache management
        void clearTextureCache();
        void removeTexture(const std::string& fileName);

        // Statistics and debugging
        size_t getTextureCount() const { return m_textureCache.size(); }
        std::vector<std::string> getLoadedTextureNames() const;

        // Check if initialized
        bool isInitialized() const { return m_initialized; }

    private:
        ResourceManager() = default;
        ~ResourceManager() = default;
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        // Try multiple paths to find texture files
        std::string findTexturePath(const std::string& fileName) const;

    private:
        std::unique_ptr<GraphicsResourceDesc> m_resourceDesc;
        std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_textureCache;
        mutable std::mutex m_textureMutex;
        bool m_initialized = false;

        // Common texture search paths
        static const std::vector<std::string> s_texturePaths;
    };
}