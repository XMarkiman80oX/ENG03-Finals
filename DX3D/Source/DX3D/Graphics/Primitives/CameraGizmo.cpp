#include <DX3D/Graphics/Primitives/CameraGizmo.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Graphics/Vertex.h>
#include <vector>

using namespace dx3d;

// Helper function to create the vertices for a single arrow
void CreateArrowVertices(std::vector<Vertex>& vertices, const Vector3& direction, const Vector4& color)
{
    const float shaftLength = 0.8f;
    const float shaftRadius = 0.02f;
    const float headLength = 0.2f;
    const float headRadius = 0.08f;
    const ui32 segments = 12;

    Vector3 shaftEnd = direction * shaftLength;
    Vector3 headBase = shaftEnd;
    Vector3 headTip = direction * (shaftLength + headLength);

    // Create orthonormal basis vectors for the arrow's orientation
    Vector3 up = (abs(direction.y) < 0.9f) ? Vector3(0, 1, 0) : Vector3(1, 0, 0);
    Vector3 right;
    right.x = up.y * direction.z - up.z * direction.y;
    right.y = up.z * direction.x - up.x * direction.z;
    right.z = up.x * direction.y - up.y * direction.x;

    up.x = direction.y * right.z - direction.z * right.y;
    up.y = direction.z * right.x - direction.x * right.z;
    up.z = direction.x * right.y - direction.y * right.x;


    // Shaft Vertices
    for (ui32 i = 0; i <= segments; ++i)
    {
        float angle = i * 2.0f * 3.14159265f / segments;
        Vector3 offset = (right * cos(angle) + up * sin(angle)) * shaftRadius;

        // Corrected push_back calls
        vertices.push_back({ {offset.x, offset.y, offset.z}, {color.x, color.y, color.z, color.w} });
        Vector3 shaftTipPosition = shaftEnd + offset;
        vertices.push_back({ {shaftTipPosition.x, shaftTipPosition.y, shaftTipPosition.z}, {color.x, color.y, color.z, color.w} });
    }

    // Head Vertices (Cone)
    vertices.push_back({ {headBase.x, headBase.y, headBase.z}, {color.x, color.y, color.z, color.w} }); // Center of base
    for (ui32 i = 0; i <= segments; ++i)
    {
        float angle = i * 2.0f * 3.14159265f / segments;
        Vector3 offset = (right * cos(angle) + up * sin(angle)) * headRadius;

        // Corrected push_back call
        Vector3 headBasePosition = headBase + offset;
        vertices.push_back({ {headBasePosition.x, headBasePosition.y, headBasePosition.z}, {color.x, color.y, color.z, color.w} });
    }
    // Corrected push_back call
    vertices.push_back({ {headTip.x, headTip.y, headTip.z}, {color.x, color.y, color.z, color.w} }); // Tip of cone
}


std::shared_ptr<VertexBuffer> CameraGizmo::CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc)
{
    std::vector<Vertex> vertices;

    // X-Axis Arrow (Red)
    CreateArrowVertices(vertices, Vector3(1, 0, 0), Vector4(1, 0, 0, 1));
    // Y-Axis Arrow (Green)
    CreateArrowVertices(vertices, Vector3(0, 1, 0), Vector4(0, 1, 0, 1));
    // Z-Axis Arrow (Blue)
    CreateArrowVertices(vertices, Vector3(0, 0, 1), Vector4(0, 0, 1, 1));

    return std::make_shared<VertexBuffer>(
        vertices.data(),
        sizeof(Vertex),
        static_cast<ui32>(vertices.size()),
        resourceDesc
    );
}
// Helper function to create the indices for a single arrow
void CreateArrowIndices(std::vector<ui32>& indices, ui32 baseVertexOffset)
{
    const ui32 segments = 12;
    ui32 shaftVertexCount = (segments + 1) * 2;

    // Shaft Indices
    for (ui32 i = 0; i < segments; ++i)
    {
        ui32 currentBase = baseVertexOffset + i * 2;
        ui32 nextBase = baseVertexOffset + (i + 1) * 2;
        indices.push_back(currentBase);
        indices.push_back(nextBase + 1);
        indices.push_back(currentBase + 1);
        indices.push_back(currentBase);
        indices.push_back(nextBase);
        indices.push_back(nextBase + 1);
    }

    // Head Indices (Cone)
    ui32 headBaseCenter = baseVertexOffset + shaftVertexCount;
    ui32 headTip = headBaseCenter + segments + 2;
    for (ui32 i = 0; i < segments; ++i)
    {
        ui32 current = headBaseCenter + 1 + i;
        ui32 next = headBaseCenter + 1 + i + 1;
        // Base
        indices.push_back(headBaseCenter);
        indices.push_back(next);
        indices.push_back(current);
        // Side
        indices.push_back(current);
        indices.push_back(next);
        indices.push_back(headTip);
    }
}
std::shared_ptr<IndexBuffer> CameraGizmo::CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc)
{
    std::vector<ui32> indices;
    const ui32 verticesPerArrow = (12 + 1) * 2 + (12 + 1) + 2;

    // X-Axis Arrow
    CreateArrowIndices(indices, 0);
    // Y-Axis Arrow
    CreateArrowIndices(indices, verticesPerArrow);
    // Z-Axis Arrow
    CreateArrowIndices(indices, verticesPerArrow * 2);

    return std::make_shared<IndexBuffer>(
        indices.data(),
        static_cast<ui32>(indices.size()),
        resourceDesc
    );
}