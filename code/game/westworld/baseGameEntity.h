#pragma once
#include "../platform.h"
#include "telegram.h"

// page 52 of Programming Game AI. Would not do this this way
class BaseGameEntity
{
    public:
        BaseGameEntity(int32 id)
        {
            SetID(id);
        }

        virtual ~BaseGameEntity(){}

        int32 Id() const { return id; }

        // All entities must implement an update function
        virtual void Update(real32 elapsed) = 0;

        virtual bool HandleMessage(const Telegram& msg) = 0;

    private:
        // every entity has a unique identifying number
        int32 id;

        // This is the next valid ID Each time a base entity is instantiated
        // this value is updated
        static int32 nextValidId;

        // this is called within the constructor to make sure teh ID is set
        // correctly. It verifies the value is great than or equal to the next
        // valid ID
        void SetID(int32 val);
};
