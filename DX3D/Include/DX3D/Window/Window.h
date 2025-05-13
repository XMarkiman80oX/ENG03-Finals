#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/Core/Common.h>

namespace dx3d
{
    class Window : public Base
    {
    public:
        explicit Window(const WindowDesc& desc);
        virtual ~Window() override;

        const Rect& getSize() const { return m_size; }

    protected:
        void* m_handle{};
        Rect m_size{};
    };
}