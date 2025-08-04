#include <DX3D/Graphics/ResourceManager.h>
#include <filesystem>
#include <algorithm>

using namespace dx3d;

// Static member definition
const std::vector<std::string> ResourceManager::s_texturePaths = {
    "DX3D/Assets/Textures/",
    "GDENG03-Engine/DX3D/Assets/Textures/",
    "../DX3D/Assets/Textures/",
    "../../DX3D/Assets/Textures/",
    "Assets/Textures/",
    "Textures/"
};

void ResourceManager::initialize(const GraphicsResourceDesc& resourceDesc)
{
    std::lock_guard<std::mutex> lock(m_textureMutex);

    if (m_initialized)
    {
        return;
    }

    m_resourceDesc = std::make_unique<GraphicsResourceDesc>(resourceDesc); 
    m_initialized = true;
}

void ResourceManager::shutdown()
{
    std::lock_guard<std::mutex> lock(m_textureMutex);

    //DX3DLogInfo(("ResourceManager shutdown - clearing " + std::to_string(m_textureCache.size()) + " cached textures").c_str());

    m_textureCache.clear();
    m_initialized = false;
}

std::shared_ptr<Texture2D> ResourceManager::loadTexture(const std::string& fileName)
{
    if (!m_initialized || !m_resourceDesc) // Add null check
    {
        return nullptr;
    }

    if (!m_initialized)
    {
        //DX3DLogError("ResourceManager not initialized - cannot load texture");
        return nullptr;
    }

    if (fileName.empty())
    {
        //DX3DLogError("Cannot load texture with empty filename");
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_textureMutex);

    // Check if texture is already cached
    auto it = m_textureCache.find(fileName);
    if (it != m_textureCache.end())
    {
        //DX3DLogInfo(("Using cached texture: " + fileName).c_str());
        return it->second;
    }

    // Find the texture file
    std::string fullPath = findTexturePath(fileName);
    if (fullPath.empty())
    {
        //DX3DLogError(("Texture file not found: " + fileName).c_str());
        return nullptr;
    }

    try
    {
        auto texture = std::make_shared<Texture2D>(fullPath, *m_resourceDesc); // Add dereference
        m_textureCache[fileName] = texture;
        return texture;
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }
}

std::shared_ptr<Texture2D> ResourceManager::getTexture(const std::string& fileName) const
{
    if (!m_initialized)
        return nullptr;

    std::lock_guard<std::mutex> lock(m_textureMutex);

    auto it = m_textureCache.find(fileName);
    return (it != m_textureCache.end()) ? it->second : nullptr;
}

bool ResourceManager::isTextureLoaded(const std::string& fileName) const
{
    if (!m_initialized)
        return false;

    std::lock_guard<std::mutex> lock(m_textureMutex);
    return m_textureCache.find(fileName) != m_textureCache.end();
}

std::shared_ptr<Material> ResourceManager::createMaterial(const std::string& name)
{
    std::string materialName = name.empty() ? "Material_" + std::to_string(rand()) : name;
    return std::make_shared<Material>(materialName);
}

void ResourceManager::clearTextureCache()
{
    std::lock_guard<std::mutex> lock(m_textureMutex);

    //DX3DLogInfo(("Clearing texture cache - removing " + std::to_string(m_textureCache.size()) + " textures").c_str());
    m_textureCache.clear();
}

void ResourceManager::removeTexture(const std::string& fileName)
{
    std::lock_guard<std::mutex> lock(m_textureMutex);

    auto it = m_textureCache.find(fileName);
    if (it != m_textureCache.end())
    {
        m_textureCache.erase(it);
        //DX3DLogInfo(("Removed texture from cache: " + fileName).c_str());
    }
}

std::vector<std::string> ResourceManager::getLoadedTextureNames() const
{
    std::lock_guard<std::mutex> lock(m_textureMutex);

    std::vector<std::string> names;
    names.reserve(m_textureCache.size());

    for (const auto& pair : m_textureCache)
    {
        names.push_back(pair.first);
    }

    return names;
}

std::string ResourceManager::findTexturePath(const std::string& fileName) const
{
    // Try each possible path
    for (const auto& basePath : s_texturePaths)
    {
        std::string fullPath = basePath + fileName;

        if (std::filesystem::exists(fullPath))
        {
            return fullPath;
        }
    }

    // Also try the filename as-is (in case it's already a full path)
    if (std::filesystem::exists(fileName))
    {
        return fileName;
    }

    // Try common image extensions if no extension provided
    if (fileName.find('.') == std::string::npos)
    {
        std::vector<std::string> extensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds" };

        for (const auto& basePath : s_texturePaths)
        {
            for (const auto& ext : extensions)
            {
                std::string fullPath = basePath + fileName + ext;
                if (std::filesystem::exists(fullPath))
                {
                    return fullPath;
                }
            }
        }
    }

    return ""; // File not found
}