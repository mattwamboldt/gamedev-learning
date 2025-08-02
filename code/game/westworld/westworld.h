#pragma once
#include "miner.h"
#include "minersWife.h"

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
    MinersWife* elsa;

    int32 groundTextureId;

    // Locations in the town
    Building buildings[NUM_LOCATIONS];

    char messageBox[128];
    bool messageBoxActive;
};

void WestworldUpdateAndRender(GameState* state, GameInput* input);
