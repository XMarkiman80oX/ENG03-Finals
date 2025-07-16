#pragma once
#include <DX3D/Core/Core.h>

namespace dx3d
{
    struct Vertex
    {
        float position[3];
        float color[4];
        float normal[3];      
        float texCoord[2];   

        Vertex() : position{ 0,0,0 }, color{ 1,1,1,1 }, normal{ 0,0,1 }, texCoord{ 0,0 } {}

        Vertex(const float pos[3], const float col[4])
            : position{ pos[0], pos[1], pos[2] }
            , color{ col[0], col[1], col[2], col[3] }
            , normal{ 0,0,1 }
            , texCoord{ 0,0 } {
        }

        Vertex(const float pos[3], const float col[4], const float norm[3], const float tex[2])
            : position{ pos[0], pos[1], pos[2] }
            , color{ col[0], col[1], col[2], col[3] }
            , normal{ norm[0], norm[1], norm[2] }
            , texCoord{ tex[0], tex[1] } {
        }
    };
}