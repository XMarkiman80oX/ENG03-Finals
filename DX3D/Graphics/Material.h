#pragma once
#include <../Core/Forward.h>
#include <../Math/Math.h>
#include <../Graphics/Texture2D.h>
#include <memory>
#include <string>

namespace dx3d
{
    class Material
    {
    public:
        Material();
        Material(const std::string& name);
        ~Material() = default;

        // Texture properties
        void setDiffuseTexture(std::shared_ptr<Texture2D> texture) { m_diffuseTexture = texture; }
        std::shared_ptr<Texture2D> getDiffuseTexture() const { return m_diffuseTexture; }
        bool hasDiffuseTexture() const { return m_diffuseTexture != nullptr; }

        // Material colors
        void setDiffuseColor(const Vector4& color) { m_diffuseColor = color; }
        void setAmbientColor(const Vector4& color) { m_ambientColor = color; }
        void setSpecularColor(const Vector4& color) { m_specularColor = color; }
        void setEmissiveColor(const Vector4& color) { m_emissiveColor = color; }

        const Vector4& getDiffuseColor() const { return m_diffuseColor; }
        const Vector4& getAmbientColor() const { return m_ambientColor; }
        const Vector4& getSpecularColor() const { return m_specularColor; }
        const Vector4& getEmissiveColor() const { return m_emissiveColor; }

        // Material properties
        void setSpecularPower(float power) { m_specularPower = power; }
        void setOpacity(float opacity) { m_opacity = opacity; }

        float getSpecularPower() const { return m_specularPower; }
        float getOpacity() const { return m_opacity; }

        // Name
        void setName(const std::string& name) { m_name = name; }
        const std::string& getName() const { return m_name; }

    private:
        std::string m_name;

        // Textures
        std::shared_ptr<Texture2D> m_diffuseTexture;

        // Material colors
        Vector4 m_diffuseColor;
        Vector4 m_ambientColor;
        Vector4 m_specularColor;
        Vector4 m_emissiveColor;

        // Material properties
        float m_specularPower;
        float m_opacity;
    };
}