#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Math/Math.h>
#include <Windows.h>
#include <unordered_set>

namespace dx3d
{
    enum class KeyCode
    {
        // Letters
        A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G', H = 'H',
        I = 'I', J = 'J', K = 'K', L = 'L', M = 'M', N = 'N', O = 'O', P = 'P',
        Q = 'Q', R = 'R', S = 'S', T = 'T', U = 'U', V = 'V', W = 'W', X = 'X',
        Y = 'Y', Z = 'Z',

        // Numbers
        Key0 = '0', Key1 = '1', Key2 = '2', Key3 = '3', Key4 = '4',
        Key5 = '5', Key6 = '6', Key7 = '7', Key8 = '8', Key9 = '9',

        // Special keys
        Space = VK_SPACE,
        Enter = VK_RETURN,
        Escape = VK_ESCAPE,
        Tab = VK_TAB,
        Shift = VK_SHIFT,
        Control = VK_CONTROL,
        Alt = VK_MENU,

        // Arrow keys
        Left = VK_LEFT,
        Right = VK_RIGHT,
        Up = VK_UP,
        Down = VK_DOWN,

        // Function keys
        F1 = VK_F1, F2 = VK_F2, F3 = VK_F3, F4 = VK_F4,
        F5 = VK_F5, F6 = VK_F6, F7 = VK_F7, F8 = VK_F8,
        F9 = VK_F9, F10 = VK_F10, F11 = VK_F11, F12 = VK_F12
    };

    enum class MouseButton
    {
        Left = 0,
        Right = 1,
        Middle = 2
    };

    class Input
    {
    public:
        static Input& getInstance()
        {
            static Input instance;
            return instance;
        }

        // Keyboard methods
        bool isKeyPressed(KeyCode key) const;
        bool isKeyJustPressed(KeyCode key) const;
        bool isKeyJustReleased(KeyCode key) const;

        // Mouse methods
        bool isMouseButtonPressed(MouseButton button) const;
        bool isMouseButtonJustPressed(MouseButton button) const;
        bool isMouseButtonJustReleased(MouseButton button) const;

        // Mouse position
        i32 getMouseX() const { return m_mouseX; }
        i32 getMouseY() const { return m_mouseY; }
        i32 getMouseDeltaX() const { return m_mouseDeltaX; }
        i32 getMouseDeltaY() const { return m_mouseDeltaY; }

        // Mouse wheel
        i32 getMouseWheelDelta() const { return m_mouseWheelDelta; }

        // Update methods (called by Window)
        void onKeyDown(KeyCode key);
        void onKeyUp(KeyCode key);
        void onMouseButtonDown(MouseButton button);
        void onMouseButtonUp(MouseButton button);
        void onMouseMove(i32 x, i32 y);
        void onMouseWheel(i32 delta);

        // Called at the end of each frame to reset "just pressed/released" states
        void update();

        // Process Windows messages
        bool processWindowsMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    private:
        Input() = default;
        ~Input() = default;
        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;

    private:
        // Keyboard state
        std::unordered_set<KeyCode> m_keysPressed;
        std::unordered_set<KeyCode> m_keysJustPressed;
        std::unordered_set<KeyCode> m_keysJustReleased;

        // Mouse state
        bool m_mouseButtons[3] = { false, false, false };
        bool m_mouseButtonsJustPressed[3] = { false, false, false };
        bool m_mouseButtonsJustReleased[3] = { false, false, false };

        // Mouse position
        i32 m_mouseX = 0;
        i32 m_mouseY = 0;
        i32 m_lastMouseX = 0;
        i32 m_lastMouseY = 0;
        i32 m_mouseDeltaX = 0;
        i32 m_mouseDeltaY = 0;

        // Mouse wheel
        i32 m_mouseWheelDelta = 0;
    };
}