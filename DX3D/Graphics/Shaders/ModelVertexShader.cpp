#include "../Graphics/Shaders/ModelVertexShader.h"  
#include <../Graphics/Vertex.h>
#include <../Graphics/Shaders/ModelShader.h>  

std::shared_ptr<dx3d::VertexShader> dx3d::createModelVertexShader(const GraphicsResourceDesc& desc)
{
    return std::make_shared<VertexShader>(desc, ModelShader::GetVertexShaderCode());
}