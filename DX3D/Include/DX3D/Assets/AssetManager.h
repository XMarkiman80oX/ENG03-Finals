#pragma once
#include <DX3D/Graphics/Primitives/Model.h>
#include <DX3D/Graphics/GraphicsResource.h>
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
        struct LoadingTask
        {
            std::future<std::shared_ptr<Model>> future;
            float progress = 0.0f;
            bool isComplete = false;
            bool hasError = false;
            std::string errorMessage;
            std::string filePath;
        };

        static AssetManager& getInstance()
        {
            static AssetManager instance;
            return instance;
        }

        // Synchronous loading
        std::shared_ptr<Model> loadModelSync(
            const std::string& filePath,
            const GraphicsResourceDesc& resourceDesc
        );

        // Asynchronous loading
        std::string loadModelAsync(
            const std::string& filePath,
            const GraphicsResourceDesc& resourceDesc
        );

        // Check loading progress
        bool isLoadingComplete(const std::string& taskId);
        float getLoadingProgress(const std::string& taskId);
        std::shared_ptr<Model> getLoadedModel(const std::string& taskId);
        bool hasLoadingError(const std::string& taskId);
        std::string getLoadingError(const std::string& taskId);

        // Cleanup completed tasks
        void cleanupTask(const std::string& taskId);
        void cleanupCompletedTasks();

        // Model caching
        void cacheModel(const std::string& filePath, std::shared_ptr<Model> model);
        std::shared_ptr<Model> getCachedModel(const std::string& filePath);
        bool isModelCached(const std::string& filePath);
        void clearCache();

        // Update method (call from main thread)
        void update();

    private:
        AssetManager() = default;
        ~AssetManager() = default;
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;

        // Generate unique task ID
        std::string generateTaskId();

        // Async loading worker
        static std::shared_ptr<Model> loadModelWorker(
            const std::string& filePath,
            GraphicsResourceDesc resourceDesc,
            std::shared_ptr<std::atomic<float>> progressPtr
        );

    private:
        std::unordered_map<std::string, LoadingTask> m_loadingTasks;
        std::unordered_map<std::string, std::shared_ptr<Model>> m_modelCache;
        std::mutex m_tasksMutex;
        std::mutex m_cacheMutex;
        std::atomic<uint32_t> m_taskCounter{ 0 };
    };
}