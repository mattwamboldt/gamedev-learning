#pragma once
#include "miner.h"

struct GameState;

struct Building
{
    TownLocation location;
    Vector2 position;
    int32 textureId;
};

struct WestworldState
{
    bool isInitialized;
    Miner* miner;

    // Locations in the town
    int32 groundTextureId;

    Building buildings[NUM_LOCATIONS];

    char messageBox[1024];
    bool messageBoxActive;
};

void WestworldUpdateAndRender(GameState* state, GameInput* input);
