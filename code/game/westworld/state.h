#pragma once

#include "telegram.h"

template <class Entity_Type>
class State
{
    public:
    virtual ~State(){};
    virtual void Enter(Entity_Type*) = 0;
    virtual void Execute(Entity_Type*) = 0;
    virtual void Exit(Entity_Type*) = 0;
    virtual bool OnMessage(Entity_Type*, const Telegram&) { return false; }
};
