#pragma once

#include "game.h"

namespace Steering
{
    const int MAX_ENTITIES = 100;

    enum EntityType
    {
        ENTITY_GENERIC = 0,
        NUM_ENTITY_TYPES,
    };

    struct Entity
    {
        int32 id;
        EntityType type;
        Vector2 position;
        Vector2 scale;
        real64 boundingRadius;
        bool isTagged;

        // Movement data
        Vector2 velocity;

        // heading and side are normals that define the local coordinate space
        // Usually velocity and heading will be aligned and somewhat redundant
        Vector2 heading;
        Vector2 side;

        real32 mass;

        real32 maxSpeed;
        real32 maxForce;
        real64 maxTurnRate; // max rotation speed in radians
    };
    
    struct DemoState
    {
        bool isInitialized;

        // Instead of messing around with different algos for something that'll only come up in debug
        // I'm just gonna use a texture
        int32 circleTextureId;

        bool seekTarget;
        Vector2 targetPosition;

        uint16 numEntities;
        Entity enitities[MAX_ENTITIES];
    };

    void UpdateAndRender(GameState* state, GameInput* input);
}