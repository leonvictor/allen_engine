#pragma once

// TODO: This is engine-wide. Should be used by the main game loop
// TODO: having numstages this way could be a bit wonky
enum UpdateStage
{
    FrameStart,
    PrePhysics,
    Physics,
    PostPhysics,
    FrameEnd,
    NumStages
};