#pragma once

namespace dx3d
{
    enum class SceneState
    {
        Edit,   
        Play,   
        Pause   
    };

    struct SceneStateInfo
    {
        SceneState currentState = SceneState::Edit;
        SceneState previousState = SceneState::Edit;
        bool canFrameStep = false;
        bool frameStepRequested = false;
    };
}