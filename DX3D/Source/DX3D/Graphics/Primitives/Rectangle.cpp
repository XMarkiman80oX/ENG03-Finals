#include <DX3D/Graphics/Primitives/Rectangle.h>

std::shared_ptr<dx3d::VertexBuffer> dx3d::Rectangle::Create(const GraphicsResourceDesc& resourceDesc)
{
    Vertex rectangleVertices[] = {
        // First triangle
        { {-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },   
        { {0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },    
        { {-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },  

        // Second triangle
        { {0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },    
        { {0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },   
        { {-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }   
    };

    return std::make_shared<VertexBuffer>(
        rectangleVertices,
        sizeof(Vertex),
        6, 
        resourceDesc
    );
}