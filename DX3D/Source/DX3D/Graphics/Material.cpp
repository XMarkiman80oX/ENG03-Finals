#include <DX3D/Graphics/Material.h>

using namespace dx3d;

Material::Material()
    : m_name("DefaultMaterial")
    , m_diffuseTexture(nullptr)
    , m_diffuseColor(1.0f, 1.0f, 1.0f, 1.0f)    // White
    , m_ambientColor(0.2f, 0.2f, 0.2f, 1.0f)    // Dark gray
    , m_specularColor(1.0f, 1.0f, 1.0f, 1.0f)   // White
    , m_emissiveColor(0.0f, 0.0f, 0.0f, 1.0f)   // Black (no emission)
    , m_specularPower(32.0f)
    , m_opacity(1.0f)
{
}

Material::Material(const std::string& name)
    : m_name(name)
    , m_diffuseTexture(nullptr)
    , m_diffuseColor(1.0f, 1.0f, 1.0f, 1.0f)
    , m_ambientColor(0.2f, 0.2f, 0.2f, 1.0f)
    , m_specularColor(1.0f, 1.0f, 1.0f, 1.0f)
    , m_emissiveColor(0.0f, 0.0f, 0.0f, 1.0f)
    , m_specularPower(32.0f)
    , m_opacity(1.0f)
{
}