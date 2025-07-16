#include "DX3D/Graphics/Shaders/ModelVertexShader.h"  
#include <DX3D/Graphics/Vertex.h>
#include <DX3D/Graphics/Shaders/ModelShader.h>  

std::shared_ptr<dx3d::VertexShader> dx3d::createModelVertexShader(const GraphicsResourceDesc& desc)
{
    return std::make_shared<VertexShader>(desc, ModelShader::GetVertexShaderCode());
}