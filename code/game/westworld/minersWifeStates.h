#pragma once
#include "minersWife.h"

class WifesGlobalState : public State<MinersWife>
{
public:
    static WifesGlobalState* Instance();

    virtual void Enter(MinersWife* wife){}
    virtual void Execute(MinersWife* wife);
    virtual void Exit(MinersWife* wife){}

private:
    //copy ctor and assignment should be private
    WifesGlobalState(){}
    WifesGlobalState(const WifesGlobalState&);
    WifesGlobalState& operator=(const WifesGlobalState&);
};

class DoHouseWork : public State<MinersWife>
{
public:
    static DoHouseWork* Instance();

    virtual void Enter(MinersWife* wife){}
    virtual void Execute(MinersWife* wife);
    virtual void Exit(MinersWife* wife){}

private:
    //copy ctor and assignment should be private
    DoHouseWork(){}
    DoHouseWork(const DoHouseWork&);
    DoHouseWork& operator=(const DoHouseWork&);
};

class VisitBathroom : public State<MinersWife>
{
public:
    static VisitBathroom* Instance();

    virtual void Enter(MinersWife* wife);
    virtual void Execute(MinersWife* wife);
    virtual void Exit(MinersWife* wife);

private:
    //copy ctor and assignment should be private
    VisitBathroom(){}
    VisitBathroom(const VisitBathroom&);
    VisitBathroom& operator=(const VisitBathroom&);
};
