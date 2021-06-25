#pragma once

// TODO: This is engine-wide. Should be used by the main game loop
// TODO: having numstages this way could be a bit wonky
/// @brief Update stages in a single frame. Systems are queued and executed within those stages.
enum UpdateStage
{
    FrameStart,
    PrePhysics,
    Physics,
    PostPhysics,
    FrameEnd,
    NumStages
};