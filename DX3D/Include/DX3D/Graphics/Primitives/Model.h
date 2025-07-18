#pragma once
#include <DX3D/Graphics/Primitives/AGameObject.h>
#include <DX3D/Graphics/Mesh.h>
#include <memory>
#include <vector>
#include <string>

namespace dx3d
{
    class Model : public AGameObject
    {
    public:
        Model();
        Model(const Vector3& position, const Vector3& rotation = Vector3(0, 0, 0), const Vector3& scale = Vector3(1, 1, 1));
        virtual ~Model() = default;

        // Mesh management
        void addMesh(std::shared_ptr<Mesh> mesh);
        void removeMesh(size_t index);
        void clearMeshes();

        std::shared_ptr<Mesh> getMesh(size_t index) const;
        size_t getMeshCount() const { return m_meshes.size(); }
        const std::vector<std::shared_ptr<Mesh>>& getMeshes() const { return m_meshes; }

        // Model properties
        void setFilePath(const std::string& filePath) { m_filePath = filePath; }
        const std::string& getFilePath() const { return m_filePath; }

        void setName(const std::string& name) { m_name = name; }
        const std::string& getName() const { return m_name; }

        // Check if model is ready for rendering
        bool isReadyForRendering() const;

        // Override virtual methods from base class
        virtual void update(float deltaTime) override;

        // Static factory method for loading models
        static std::shared_ptr<Model> LoadFromFile(
            const std::string& filePath,
            const GraphicsResourceDesc& resourceDesc
        );

    private:
        std::string m_name;
        std::string m_filePath;
        std::vector<std::shared_ptr<Mesh>> m_meshes;

    protected:
        virtual CollisionShapeType getCollisionShapeType() const override;
    };

    
}