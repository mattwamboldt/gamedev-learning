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

private:
    StateMachine<MinersWife>* stateMachine;
    real32 stateElapsed;
    real32 stateDuration;

    int8 currentChore;
};
