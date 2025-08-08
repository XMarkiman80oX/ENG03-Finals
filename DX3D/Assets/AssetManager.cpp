#include "../Assets/AssetManager.h"
#include "../Assets/ModelParser.h"
#include <thread>
#include <atomic>
#include <iostream>
#include <algorithm>

using namespace dx3d;

void AssetManager::update()
{
    std::lock_guard<std::mutex> guard(m_taskRegistryMutex);

    for (auto& item : m_asyncTaskRegistry)
    {
        auto& currentTask = item.second;

        if (!currentTask.isFinished)
        {
            if (currentTask.futureResult.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                try
                {
                    auto asset = currentTask.futureResult.get();
                    if (asset)
                    {
                        cacheModel(currentTask.resourcePath, asset);
                        currentTask.percentComplete = 100.0f;
                        currentTask.isFinished = true;
                    }
                    else
                    {
                        currentTask.hasFailed = true;
                        currentTask.isFinished = true;
                        currentTask.errorDetails = "Model loading failed.";
                    }
                }
                catch (const std::exception& ex)
                {
                    currentTask.hasFailed = true;
                    currentTask.errorDetails = ex.what();
                    currentTask.isFinished = true;
                }
            }
            else
            {
                currentTask.percentComplete = std::min(90.0f, currentTask.percentComplete + 10.0f);
            }
        }
    }
}

std::shared_ptr<Model> AssetManager::loadModelSync(
    const std::string& path,
    const GraphicsResourceDesc& resDesc)
{
    {
        std::lock_guard<std::mutex> guard(m_resourceMutex);
        auto iter = m_assetRegistry.find(path);
        if (iter != m_assetRegistry.end())
        {
            return iter->second;
        }
    }

    auto asset = ModelParser::LoadModel(path, resDesc);

    if (asset)
    {
        cacheModel(path, asset);
    }

    return asset;
}

std::string AssetManager::loadModelAsync(
    const std::string& path,
    const GraphicsResourceDesc& resDesc)
{
    {
        std::lock_guard<std::mutex> guard(m_resourceMutex);
        auto iter = m_assetRegistry.find(path);
        if (iter != m_assetRegistry.end())
        {
            std::string id = createUniqueTaskId();
            AsyncTask completedTask;
            completedTask.percentComplete = 100.0f;
            completedTask.isFinished = true;
            completedTask.hasFailed = false;
            completedTask.resourcePath = path;

            std::promise<std::shared_ptr<Model>> p;
            p.set_value(iter->second);
            completedTask.futureResult = p.get_future();

            {
                std::lock_guard<std::mutex> taskGuard(m_taskRegistryMutex);
                m_asyncTaskRegistry[id] = std::move(completedTask);
            }

            return id;
        }
    }

    std::string id = createUniqueTaskId();
    auto progress = std::make_shared<std::atomic<float>>(0.0f);

    AsyncTask newTask;
    newTask.resourcePath = path;
    newTask.futureResult = std::async(std::launch::async, modelLoadingWorker, path, resDesc, progress);

    {
        std::lock_guard<std::mutex> guard(m_taskRegistryMutex);
        m_asyncTaskRegistry[id] = std::move(newTask);
    }

    return id;
}

float AssetManager::getLoadingProgress(const std::string& id)
{
    std::lock_guard<std::mutex> guard(m_taskRegistryMutex);
    auto iter = m_asyncTaskRegistry.find(id);
    if (iter != m_asyncTaskRegistry.end())
    {
        return iter->second.percentComplete;
    }
    return 0.0f;
}


bool AssetManager::isLoadingComplete(const std::string& id)
{
    std::lock_guard<std::mutex> guard(m_taskRegistryMutex);
    auto iter = m_asyncTaskRegistry.find(id);
    if (iter != m_asyncTaskRegistry.end())
    {
        return iter->second.isFinished;
    }
    return false;
}

std::string AssetManager::getLoadingError(const std::string& id)
{
    std::lock_guard<std::mutex> guard(m_taskRegistryMutex);
    auto iter = m_asyncTaskRegistry.find(id);
    if (iter != m_asyncTaskRegistry.end())
    {
        return iter->second.errorDetails;
    }
    return "";
}

bool AssetManager::hasLoadingError(const std::string& id)
{
    std::lock_guard<std::mutex> guard(m_taskRegistryMutex);
    auto iter = m_asyncTaskRegistry.find(id);
    if (iter != m_asyncTaskRegistry.end())
    {
        return iter->second.hasFailed;
    }
    return false;
}

std::shared_ptr<Model> AssetManager::getLoadedModel(const std::string& id)
{
    std::lock_guard<std::mutex> guard(m_taskRegistryMutex);
    auto iter = m_asyncTaskRegistry.find(id);
    if (iter != m_asyncTaskRegistry.end() && iter->second.isFinished && !iter->second.hasFailed)
    {
        return iter->second.futureResult.get();
    }
    return nullptr;
}


void AssetManager::cleanupTask(const std::string& id)
{
    std::lock_guard<std::mutex> guard(m_taskRegistryMutex);
    m_asyncTaskRegistry.erase(id);
}

void AssetManager::cleanupCompletedTasks()
{
    std::lock_guard<std::mutex> guard(m_taskRegistryMutex);
    auto iter = m_asyncTaskRegistry.begin();
    while (iter != m_asyncTaskRegistry.end())
    {
        if (iter->second.isFinished)
        {
            iter = m_asyncTaskRegistry.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

void AssetManager::cacheModel(const std::string& path, std::shared_ptr<Model> asset)
{
    std::lock_guard<std::mutex> guard(m_resourceMutex);
    m_assetRegistry[path] = asset;
}

std::shared_ptr<Model> AssetManager::getCachedModel(const std::string& path)
{
    std::lock_guard<std::mutex> guard(m_resourceMutex);
    auto iter = m_assetRegistry.find(path);
    return (iter != m_assetRegistry.end()) ? iter->second : nullptr;
}

void AssetManager::clearCache()
{
    std::lock_guard<std::mutex> guard(m_resourceMutex);
    m_assetRegistry.clear();
}

bool AssetManager::isModelCached(const std::string& path)
{
    std::lock_guard<std::mutex> guard(m_resourceMutex);
    return m_assetRegistry.find(path) != m_assetRegistry.end();
}

std::string AssetManager::createUniqueTaskId()
{
    return "async_task_" + std::to_string(m_taskIdCounter.fetch_add(1));
}

std::shared_ptr<Model> AssetManager::modelLoadingWorker(
    const std::string& path,
    GraphicsResourceDesc resDesc,
    std::shared_ptr<std::atomic<float>> progress)
{
    try
    {
        progress->store(10.0f);
        auto asset = ModelParser::LoadModel(path, resDesc);
        progress->store(100.0f);
        return asset;
    }
    catch (const std::exception& ex)
    {
        return nullptr;
    }
}