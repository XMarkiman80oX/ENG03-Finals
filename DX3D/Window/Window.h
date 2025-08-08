#pragma once
#include <../Core/Core.h>
#include <../Core/Common.h>

namespace dx3d
{
    class Window : public Core
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