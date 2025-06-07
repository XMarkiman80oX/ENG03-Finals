#pragma once
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Math/Math.h>
#include <memory>

namespace dx3d
{
    class Cube
    {
    public:
        struct Transform
        {
            Vector3 position{ 0.0f, 0.0f, 0.0f };
            Vector3 rotation{ 0.0f, 0.0f, 0.0f }; // Euler angles in radians
            Vector3 scale{ 1.0f, 1.0f, 1.0f };

            Matrix4x4 getWorldMatrix() const;
        };

    public:
        static std::shared_ptr<VertexBuffer> CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc);
        static std::shared_ptr<IndexBuffer> CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc);

        // Get the number of indices for a cube
        static ui32 GetIndexCount() { return 36; } // 6 faces * 2 triangles * 3 vertices

        // Create a cube instance with transform
        Cube();
        Cube(const Vector3& position, const Vector3& rotation = Vector3(0, 0, 0), const Vector3& scale = Vector3(1, 1, 1));

        // Transform operations
        void setPosition(const Vector3& position) { m_transform.position = position; }
        void setRotation(const Vector3& rotation) { m_transform.rotation = rotation; }
        void setScale(const Vector3& scale) { m_transform.scale = scale; }

        const Vector3& getPosition() const { return m_transform.position; }
        const Vector3& getRotation() const { return m_transform.rotation; }
        const Vector3& getScale() const { return m_transform.scale; }

        Matrix4x4 getWorldMatrix() const { return m_transform.getWorldMatrix(); }

        // Animation helpers
        void rotate(const Vector3& deltaRotation);
        void translate(const Vector3& deltaPosition);

    private:
        Transform m_transform;
    };
}