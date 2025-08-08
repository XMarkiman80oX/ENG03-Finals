#pragma once

namespace dx3d
{
    class EventLog;

    class DebugConsoleUI
    {
    public:
        explicit DebugConsoleUI(EventLog& logger);

        void render();

    private:
        EventLog& m_loggerInstance;
    };
}