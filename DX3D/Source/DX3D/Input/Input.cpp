#include <DX3D/Input/Input.h>
#include <cstdio>

using namespace dx3d;

bool Input::isKeyPressed(KeyCode key) const
{
    return m_keysPressed.find(key) != m_keysPressed.end();
}

bool Input::isKeyJustPressed(KeyCode key) const
{
    return m_keysJustPressed.find(key) != m_keysJustPressed.end();
}

bool Input::isKeyJustReleased(KeyCode key) const
{
    return m_keysJustReleased.find(key) != m_keysJustReleased.end();
}

bool Input::isMouseButtonPressed(MouseButton button) const
{
    return m_mouseButtons[static_cast<int>(button)];
}

bool Input::isMouseButtonJustPressed(MouseButton button) const
{
    return m_mouseButtonsJustPressed[static_cast<int>(button)];
}

bool Input::isMouseButtonJustReleased(MouseButton button) const
{
    return m_mouseButtonsJustReleased[static_cast<int>(button)];
}

void Input::onKeyDown(KeyCode key)
{
    if (m_keysPressed.find(key) == m_keysPressed.end())
    {
        m_keysPressed.insert(key);
        m_keysJustPressed.insert(key);

        // Debug log key press
        char keyChar = static_cast<char>(key);
        if (keyChar >= 32 && keyChar <= 126)
        {
            printf("[Input] Key pressed: %c\n", keyChar);
        }
        else
        {
            printf("[Input] Key pressed: 0x%X\n", static_cast<int>(key));
        }
    }
}

void Input::onKeyUp(KeyCode key)
{
    if (m_keysPressed.find(key) != m_keysPressed.end())
    {
        m_keysPressed.erase(key);
        m_keysJustReleased.insert(key);
    }
}

void Input::onMouseButtonDown(MouseButton button)
{
    int index = static_cast<int>(button);
    if (!m_mouseButtons[index])
    {
        m_mouseButtons[index] = true;
        m_mouseButtonsJustPressed[index] = true;

        const char* buttonName = (button == MouseButton::Left) ? "Left" :
            (button == MouseButton::Right) ? "Right" : "Middle";
        printf("[Input] Mouse button pressed: %s\n", buttonName);
    }
}

void Input::onMouseButtonUp(MouseButton button)
{
    int index = static_cast<int>(button);
    if (m_mouseButtons[index])
    {
        m_mouseButtons[index] = false;
        m_mouseButtonsJustReleased[index] = true;
    }
}

void Input::onMouseMove(i32 x, i32 y)
{
    m_mouseDeltaX = x - m_mouseX;
    m_mouseDeltaY = y - m_mouseY;
    m_mouseX = x;
    m_mouseY = y;
}

void Input::onMouseWheel(i32 delta)
{
    m_mouseWheelDelta = delta;
}

bool Input::isCtrlPressed() const
{
    return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
}

bool Input::isShiftPressed() const
{
    return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
}

bool Input::isAltPressed() const
{
    return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
}

bool Input::isKeyJustPressedWithCtrl(KeyCode key) const
{
    return isKeyJustPressed(key) && isCtrlPressed();
}

bool Input::isKeyJustPressedWithShiftCtrl(KeyCode key) const
{
    return isKeyJustPressed(key) && isCtrlPressed() && isShiftPressed();
}

void Input::update()
{
    // Clear "just pressed" and "just released" states
    m_keysJustPressed.clear();
    m_keysJustReleased.clear();

    for (int i = 0; i < 3; ++i)
    {
        m_mouseButtonsJustPressed[i] = false;
        m_mouseButtonsJustReleased[i] = false;
    }

    // Reset mouse deltas
    m_mouseDeltaX = 0;
    m_mouseDeltaY = 0;
    m_mouseWheelDelta = 0;
}

bool Input::processWindowsMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        // Keyboard messages
    case WM_KEYDOWN:
    {
        if (!(lparam & 0x40000000)) // Check if it's not a repeat
        {
            onKeyDown(static_cast<KeyCode>(wparam));
        }
        return true;
    }

    case WM_KEYUP:
    {
        onKeyUp(static_cast<KeyCode>(wparam));
        return true;
    }

    // Mouse button messages
    case WM_LBUTTONDOWN:
    {
        onMouseButtonDown(MouseButton::Left);
        SetCapture(hwnd); 
        return true;
    }

    case WM_LBUTTONUP:
    {
        onMouseButtonUp(MouseButton::Left);
        ReleaseCapture();
        return true;
    }

    case WM_RBUTTONDOWN:
    {
        onMouseButtonDown(MouseButton::Right);
        SetCapture(hwnd);
        return true;
    }

    case WM_RBUTTONUP:
    {
        onMouseButtonUp(MouseButton::Right);
        ReleaseCapture();
        return true;
    }

    case WM_MBUTTONDOWN:
    {
        onMouseButtonDown(MouseButton::Middle);
        SetCapture(hwnd);
        return true;
    }

    case WM_MBUTTONUP:
    {
        onMouseButtonUp(MouseButton::Middle);
        ReleaseCapture();
        return true;
    }

    // Mouse movement
    case WM_MOUSEMOVE:
    {
        i32 x = static_cast<i32>(LOWORD(lparam));
        i32 y = static_cast<i32>(HIWORD(lparam));
        onMouseMove(x, y);
        return true;
    }

    // Mouse wheel
    case WM_MOUSEWHEEL:
    {
        i32 delta = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
        onMouseWheel(delta);
        return true;
    }
    }

    return false;
}