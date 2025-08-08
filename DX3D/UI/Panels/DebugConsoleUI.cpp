#include <../UI/Panels/DebugConsoleUI.h>
#include <../Core/EventLog.h>
#include <imgui.h>

using namespace dx3d;

DebugConsoleUI::DebugConsoleUI(EventLog& logger)
    : m_loggerInstance(logger)
{
}

void DebugConsoleUI::render()
{
    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;
    float leftPanelWidth = windowWidth / 4.0f;
    float topOffset = 170;
    float spacing = 20.0f;
    float panelHeight = (windowHeight - topOffset - spacing) / 2.0f;
    float inspectorEndY = topOffset + panelHeight;

    ImGui::SetNextWindowPos(ImVec2(0, inspectorEndY + spacing), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, panelHeight), ImGuiCond_FirstUseEver);

    ImGui::Begin("Debug Console", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImVec2 currentPos = ImGui::GetWindowPos();
    if (currentPos.x != 0.0f)
    {
        ImGui::SetWindowPos(ImVec2(0.0f, currentPos.y));
    }


    auto logEntries = m_loggerInstance.fetchRecentLogs(100);

    if (ImGui::Button("Clear Logs"))
    {
        m_loggerInstance.purgeLogs();
    }

    ImGui::SameLine();
    ImGui::Text("Log Entries: %zu", logEntries.size());

    ImGui::Separator();

    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& entry : logEntries)
    {
        ImVec4 color;
        const char* levelText;

        switch (entry.severity)
        {
        case LogEntry::Level::Error:
            color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
            levelText = "[ERROR]";
            break;
        case LogEntry::Level::Warning:
            color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
            levelText = "[WARN]";
            break;
        case LogEntry::Level::Info:
            color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
            levelText = "[INFO]";
            break;
        default:
            color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            levelText = "[LOG]";
            break;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%s %s %s", entry.time.c_str(), levelText, entry.text.c_str());
        ImGui::PopStyleColor();
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();

    ImGui::End();
}