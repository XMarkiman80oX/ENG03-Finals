#include <DX3D/Game/Game.h>
#include <DX3D/Input/Input.h>
#include <Windows.h>

//void dx3d::Game::run()
//{
//    MSG msg{};
//    while (m_isRunning)
//    {
//        // Process all Windows messages
//        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
//        {
//            if (msg.message == WM_QUIT)
//            {
//                m_isRunning = false;
//                break;
//            }
//
//            TranslateMessage(&msg);
//            DispatchMessage(&msg);
//        }
//
//        // If game is still running, render a frame
//        if (m_isRunning)
//        {
//            render();
//        }
//    }
//}