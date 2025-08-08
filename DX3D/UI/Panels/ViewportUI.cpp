#include <../UI/Panels/ViewportUI.h>
#include <../Game/ViewportManager.h>
#include <../Graphics/RenderTexture.h>
#include <imgui.h>

using namespace dx3d;

ViewportUI::ViewportUI(ViewportManager& viewportManager)
    : m_viewportManager(viewportManager)
{
}

void ViewportUI::renderGameView()
{
    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;
    float leftPanelWidth = windowWidth / 4.0f;
    float topBarHeight = 70; // Height of the menu and scene controls
    float viewportWidth = (windowWidth - leftPanelWidth) / 2.0f; // Divide remaining width by 2

    renderViewport(ViewportType::Game, "Game View",
        Vector2(leftPanelWidth, topBarHeight),
        Vector2(viewportWidth, windowHeight - topBarHeight));
}

void ViewportUI::renderSceneView()
{
    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;
    float leftPanelWidth = windowWidth / 4.0f;
    float topBarHeight = 70; // Height of the menu and scene controls
    float viewportWidth = (windowWidth - leftPanelWidth) / 2.0f; // Divide remaining width by 2
    float gameViewX_End = leftPanelWidth + viewportWidth;

    renderViewport(ViewportType::Scene, "Scene View",
        Vector2(gameViewX_End, topBarHeight),
        Vector2(viewportWidth, windowHeight - topBarHeight));
}

void ViewportUI::renderViewport(ViewportType type, const char* title, const Vector2& position, const Vector2& size)
{
    ImGui::SetNextWindowPos(ImVec2(position.x, position.y));
    ImGui::SetNextWindowSize(ImVec2(size.x, size.y));

    // Add the ImGuiWindowFlags_NoBringToFrontOnFocus flag here
    ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (viewportSize.x > 0 && viewportSize.y > 0)
    {
        m_viewportManager.resize(type, static_cast<ui32>(viewportSize.x), static_cast<ui32>(viewportSize.y));
        auto& viewport = m_viewportManager.getViewport(type);

        ImVec2 imagePos = ImGui::GetCursorScreenPos();
        ImGui::Image((void*)viewport.renderTexture->getShaderResourceView(), viewportSize);

        bool isImageHovered = ImGui::IsItemHovered();
        ImVec2 mousePos = ImGui::GetMousePos();
        float localX = mousePos.x - imagePos.x;
        float localY = mousePos.y - imagePos.y;

        bool isMouseInImageBounds = (localX >= 0 && localY >= 0 &&
            localX < viewportSize.x && localY < viewportSize.y);

        m_viewportManager.updateViewportStates(type,
            isImageHovered && isMouseInImageBounds,
            ImGui::IsWindowFocused(),
            localX, localY);
    }
    ImGui::End();
}