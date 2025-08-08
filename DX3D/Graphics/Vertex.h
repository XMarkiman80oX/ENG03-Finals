#pragma once
#include <../Core/Forward.h>
#include <../Math/Math.h>

namespace dx3d
{
    struct Vertex
    {
        Vector3 position;
        Vector4 color;
        Vector3 normal;
        Vector2 texCoord;

        Vertex() : position(0, 0, 0), color(1, 1, 1, 1), normal(0, 0, 1), texCoord(0, 0) {}

        Vertex(const Vector3& pos, const Vector4& col)
            : position(pos), color(col), normal(0, 0, 1), texCoord(0, 0) {
        }

        Vertex(const Vector3& pos, const Vector4& col, const Vector3& norm, const Vector2& tex)
            : position(pos), color(col), normal(norm), texCoord(tex) {
        }
    };
}