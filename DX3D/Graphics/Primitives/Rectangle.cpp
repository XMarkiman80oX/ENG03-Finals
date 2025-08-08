#include <../Graphics/Primitives/Rectangle.h>

std::shared_ptr<dx3d::VertexBuffer> dx3d::Rectangle::Create(const GraphicsResourceDesc& resourceDesc)
{
    return CreateAt(resourceDesc, 0.0f, 0.0f, 1.0f, 1.0f);
}

std::shared_ptr<dx3d::VertexBuffer> dx3d::Rectangle::CreateAt(
    const GraphicsResourceDesc& resourceDesc,
    float centerX, float centerY,
    float width, float height)
{
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

   /* Vertex leftRectangleVertices[] = {
    { {-0.8f, 0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },   
    { {-0.4f, 0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },   
    { {-0.8f, -0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },  
    { {-0.4f, -0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }  
    };

    
    Vertex centerRectangleVertices[] = {
        { {-0.2f, 0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },   
        { {0.2f, 0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },   
        { {-0.2f, -0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }, 
        { {0.2f, -0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }   
    };

    
    Vertex rightRectangleVertices[] = {
        { {0.4f, 0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },    
        { {0.8f, 0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },   
        { {0.4f, -0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },   
        { {0.8f, -0.4f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }   
    };*/
        
    Vertex rectangleVertices[] = {
        { {centerX - halfWidth, centerY + halfHeight, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },   
        { {centerX + halfWidth, centerY + halfHeight, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },  
        { {centerX - halfWidth, centerY - halfHeight, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} },  
        { {centerX + halfWidth, centerY - halfHeight, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }   
    };

    return std::make_shared<VertexBuffer>(
        rectangleVertices,
        sizeof(Vertex),
        4,  
        resourceDesc
    );
}