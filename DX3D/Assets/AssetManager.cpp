#include "../Assets/AssetManager.h"
#include "../Assets/ModelLoader.h"
#include <thread>
#include <atomic>
#include <iostream>

using namespace dx3d;

std::shared_ptr<Model> AssetManager::loadModelSync(
    const std::string& filePath,
    const GraphicsResourceDesc& resourceDesc)
{
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_modelCache.find(filePath);
        if (it != m_modelCache.end())
        {
            return it->second;
        }
    }

    // Load model
    auto model = ModelLoader::LoadModel(filePath, resourceDesc);

    // Cache the model if loading was successful
    if (model)
    {
        cacheModel(filePath, model);
    }

    return model;
}

std::string AssetManager::loadModelAsync(
    const std::string& filePath,
    const GraphicsResourceDesc& resourceDesc)
{
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_modelCache.find(filePath);
        if (it != m_modelCache.end())
        {
            // Return a completed task ID for cached model
            std::string taskId = generateTaskId();
            LoadingTask task;
            task.progress = 100.0f;
            task.isComplete = true;
            task.hasError = false;
            task.filePath = filePath;
            // Create a completed future
            std::promise<std::shared_ptr<Model>> promise;
            promise.set_value(it->second);
            task.future = promise.get_future();

            {
                std::lock_guard<std::mutex> taskLock(m_tasksMutex);
                m_loadingTasks[taskId] = std::move(task);
            }

            return taskId;
        }
    }

    // Create task ID
    std::string taskId = generateTaskId();

    // Create progress tracking
    auto progressPtr = std::make_shared<std::atomic<float>>(0.0f);

    // Start async loading
    LoadingTask task;
    task.filePath = filePath;
    task.future = std::async(std::launch::async, loadModelWorker, filePath, resourceDesc, progressPtr);

    {
        std::lock_guard<std::mutex> lock(m_tasksMutex);
        m_loadingTasks[taskId] = std::move(task);
    }

    return taskId;
}

bool AssetManager::isLoadingComplete(const std::string& taskId)
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    auto it = m_loadingTasks.find(taskId);
    if (it != m_loadingTasks.end())
    {
        return it->second.isComplete;
    }
    return false;
}

float AssetManager::getLoadingProgress(const std::string& taskId)
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    auto it = m_loadingTasks.find(taskId);
    if (it != m_loadingTasks.end())
    {
        return it->second.progress;
    }
    return 0.0f;
}

std::shared_ptr<Model> AssetManager::getLoadedModel(const std::string& taskId)
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    auto it = m_loadingTasks.find(taskId);
    if (it != m_loadingTasks.end() && it->second.isComplete && !it->second.hasError)
    {
        return it->second.future.get();
    }
    return nullptr;
}

bool AssetManager::hasLoadingError(const std::string& taskId)
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    auto it = m_loadingTasks.find(taskId);
    if (it != m_loadingTasks.end())
    {
        return it->second.hasError;
    }
    return false;
}

std::string AssetManager::getLoadingError(const std::string& taskId)
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    auto it = m_loadingTasks.find(taskId);
    if (it != m_loadingTasks.end())
    {
        return it->second.errorMessage;
    }
    return "";
}

void AssetManager::cleanupTask(const std::string& taskId)
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    m_loadingTasks.erase(taskId);
}

void AssetManager::cleanupCompletedTasks()
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    auto it = m_loadingTasks.begin();
    while (it != m_loadingTasks.end())
    {
        if (it->second.isComplete)
        {
            it = m_loadingTasks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void AssetManager::cacheModel(const std::string& filePath, std::shared_ptr<Model> model)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_modelCache[filePath] = model;
}

std::shared_ptr<Model> AssetManager::getCachedModel(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_modelCache.find(filePath);
    return (it != m_modelCache.end()) ? it->second : nullptr;
}

bool AssetManager::isModelCached(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_modelCache.find(filePath) != m_modelCache.end();
}

void AssetManager::clearCache()
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_modelCache.clear();
}

void AssetManager::update()
{
    std::lock_guard<std::mutex> lock(m_tasksMutex);

    for (auto& pair : m_loadingTasks)
    {
        auto& task = pair.second;

        if (!task.isComplete)
        {
            // Check if future is ready
            if (task.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                try
                {
                    auto model = task.future.get();
                    if (model)
                    {
                        // Cache the loaded model
                        cacheModel(task.filePath, model);
                        task.progress = 100.0f;
                        task.isComplete = true;
                    }
                    else
                    {
                        task.hasError = true;
                        task.errorMessage = "Failed to load model";
                        task.isComplete = true;
                    }
                }
                catch (const std::exception& e)
                {
                    task.hasError = true;
                    task.errorMessage = e.what();
                    task.isComplete = true;
                }
            }
            else
            {
                // Update progress (simple increment for now)
                task.progress = std::min(90.0f, task.progress + 10.0f);
            }
        }
    }
}

std::string AssetManager::generateTaskId()
{
    return "task_" + std::to_string(m_taskCounter.fetch_add(1));
}

std::shared_ptr<Model> AssetManager::loadModelWorker(
    const std::string& filePath,
    GraphicsResourceDesc resourceDesc,
    std::shared_ptr<std::atomic<float>> progressPtr)
{
    try
    {
        progressPtr->store(10.0f);
        auto model = ModelLoader::LoadModel(filePath, resourceDesc);
        progressPtr->store(100.0f);
        return model;
    }
    catch (const std::exception& e)
    {
        // Log error and return nullptr
        return nullptr;
    }
}