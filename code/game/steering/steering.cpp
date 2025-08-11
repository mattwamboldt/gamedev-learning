#include "steering.h"

namespace Steering
{
    static DemoState* gSteeringState = 0;

    Vector2 Seek(Entity* entity, Vector2 targetPosition)
    {
        Vector2 desiredVelocity = targetPosition - entity->position;
        desiredVelocity.Normalize();
        desiredVelocity *= entity->maxSpeed;
        return desiredVelocity - entity->velocity;
    }

    Vector2 Flee(Entity* entity, Vector2 targetPosition)
    {
        real32 panicDistanceSquared = 100.f * 100.0f;
        if (entity->position.DistanceSquared(targetPosition) > panicDistanceSquared)
        {
            return { 0, 0 };
        }

        Vector2 desiredVelocity = entity->position - targetPosition;
        desiredVelocity.Normalize();
        desiredVelocity *= entity->maxSpeed;
        return desiredVelocity - entity->velocity;
    }

    Vector2 Arrive(Entity* entity, Vector2 targetPosition)
    {
        Vector2 desiredVelocity = targetPosition - entity->position;
        real32 distance = desiredVelocity.Length();

        if (distance <= 0.0)
        {
            return { 0, 0 };
        }

        if (distance < 100)
        {
            real32 desiredSpeed = (distance / 100.0f) * entity->maxSpeed;
            desiredVelocity.SetMagnitude(desiredSpeed);
        }
        else
        {
            desiredVelocity.SetMagnitude(entity->maxSpeed);
        }

        return desiredVelocity - entity->velocity;
    }

    Vector2 CalculateSteeringForce(Entity* entity)
    {
        if (gSteeringState->seekTarget)
        {
            return Arrive(entity, gSteeringState->targetPosition);
        }

        return {0, 0};
    }

    void UpdateAndRender(GameState* state, GameInput* input)
    {
        gSteeringState = &state->steering;
        if (!gSteeringState->isInitialized)
        {
            Entity* newEntity = gSteeringState->enitities + gSteeringState->numEntities;
            newEntity->id = gSteeringState->numEntities++;
            newEntity->position = { state->frame.center, state->frame.middle };
            newEntity->heading = { 1.0f, 0.0f };
            newEntity->maxSpeed = 4;
            newEntity->maxForce = 0.1f;
            newEntity->mass = 1.0f;

            gSteeringState->circleTextureId = LoadPNG("circle-outline.png");

            gSteeringState->isInitialized = true;
        }

        if (input->mouse.left.wasPressed())
        {
            gSteeringState->targetPosition = { input->mouse.x, input->mouse.y };
            gSteeringState->seekTarget = true;
        }

        // Update loop
        for (uint16 i = 0; i < gSteeringState->numEntities; ++i)
        {
            Entity* entity = gSteeringState->enitities + i;

            Vector2 steeringForce = CalculateSteeringForce(entity);
            Vector2 acceleration = steeringForce / entity->mass;
            acceleration.Truncate(entity->maxForce);
            entity->velocity += acceleration;
            entity->velocity.Truncate(entity->maxSpeed);

            entity->position += entity->velocity;
            if (entity->position.x >= state->frame.width)
            {
                entity->position.x -= state->frame.width;
            }
            else if (entity->position.x < 0)
            {
                entity->position.x += state->frame.width;
            }

            if (entity->position.y >= state->frame.height)
            {
                entity->position.y -= state->frame.height;
            }
            else if (entity->position.y < 0)
            {
                entity->position.y += state->frame.height;
            }

            if (entity->velocity.LengthSquared() > 0.0001f)
            {
                entity->heading = entity->velocity.ToNormal();
                entity->side = entity->heading.Perp();
            }
        }

        // Rendering loop
        if (gSteeringState->seekTarget)
        {
            DrawRectangle(gSteeringState->targetPosition.x, gSteeringState->targetPosition.y, 4, 4, white);
        }

        DrawRectangle(input->mouse.x, input->mouse.y, 4, 4, green);

        for (uint16 i = 0; i < gSteeringState->numEntities; ++i)
        {
            Entity* entity = gSteeringState->enitities + i;
            Vector2 points[] = {
                { -10, -6 },
                { 10, 0.0 },
                { -10, 6 },
            };

            for (int p = 0; p < 3; ++p)
            {
                points[p].x += entity->position.x;
                points[p].y += entity->position.y;
            }

            gPlatform->renderLines(points, 3, white);
        }
    }
}
