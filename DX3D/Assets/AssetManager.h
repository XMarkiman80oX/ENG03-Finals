#pragma once
#include "../Graphics/Primitives/Model.h"
#include "../Graphics/GraphicsResource.h"
#include <memory>
#include <future>
#include <string>
#include <unordered_map>
#include <mutex>

namespace dx3d
{
    class AssetManager
    {
    public:
        struct AsyncTask
        {
            std::future<std::shared_ptr<Model>> futureResult;
            float percentComplete = 0.0f;
            bool isFinished = false;
            bool hasFailed = false;
            std::string errorDetails;
            std::string resourcePath;
        };

    private:
        AssetManager() = default;
        ~AssetManager() = default;
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;

        std::string createUniqueTaskId();

        static std::shared_ptr<Model> modelLoadingWorker(
            const std::string& path,
            GraphicsResourceDesc resDesc,
            std::shared_ptr<std::atomic<float>> progress
        );

    private:
        std::mutex m_resourceMutex;
        std::mutex m_taskRegistryMutex;
        std::unordered_map<std::string, std::shared_ptr<Model>> m_assetRegistry;
        std::unordered_map<std::string, AsyncTask> m_asyncTaskRegistry;
        std::atomic<uint32_t> m_taskIdCounter{ 0 };

        static AssetManager& getInstance()
        {
            static AssetManager instance;
            return instance;
        }

        void update();

        std::shared_ptr<Model> loadModelSync(
            const std::string& path,
            const GraphicsResourceDesc& resDesc
        );

        std::string loadModelAsync(
            const std::string& path,
            const GraphicsResourceDesc& resDesc
        );

        float getLoadingProgress(const std::string& id);
        bool isLoadingComplete(const std::string& id);
        std::string getLoadingError(const std::string& id);
        bool hasLoadingError(const std::string& id);
        std::shared_ptr<Model> getLoadedModel(const std::string& id);

        void cleanupTask(const std::string& id);
        void cleanupCompletedTasks();

        void cacheModel(const std::string& path, std::shared_ptr<Model> asset);
        std::shared_ptr<Model> getCachedModel(const std::string& path);
        void clearCache();
        bool isModelCached(const std::string& path);
    };
}