#pragma once

#include "baseGameEntity.h"
#include "stateMachine.h"

class MinersWife : public BaseGameEntity
{
public:
    MinersWife(int32 id);
    ~MinersWife() { delete stateMachine; }

    void Update(real32 elapsed);

    StateMachine<MinersWife>* GetFSM() { return stateMachine; }

    virtual bool HandleMessage(const Telegram& msg);

    bool Cooking() { return isCooking; }
    void SetCooking(bool val) { isCooking = val; }

private:
    StateMachine<MinersWife>* stateMachine;
    real32 stateElapsed;
    real32 stateDuration;

    bool isCooking;
};
