#pragma once
#include "baseGameEntity.h"
#include "stateMachine.h"
#include <math.h>

enum TownLocation
{
    SHACK = 0,
    GOLDMINE,
    BANK,
    SALOON,
    ENTRANCE,

    NUM_LOCATIONS
};

//the amount of gold a miner must have before he feels comfortable
const int ComfortLevel  = 5;

//the amount of nuggets a miner can carry
const int MaxNuggets = 3;

//above this value a miner is thirsty
const int ThirstLevel = 5;

//above this value a miner is sleepy
const int TirednessThreshold = 5;

class Miner : public BaseGameEntity
{
public:
    Miner(int32 id);
    ~Miner(){ delete stateMachine; }

    void Update(real32 elapsed);
    StateMachine<Miner>* GetFSM() { return stateMachine; }

    virtual bool HandleMessage(const Telegram& msg);

    // NOTE: I wouldn't normally bother with getters and setters when they do nothing like this
    // NOTE: I also wouldn't bother with const correctness as it serves no practical benefit I find
    TownLocation Location() const { return location; }
    void ChangeLocation(const TownLocation loc);

    int GoldCarried() const {return numGoldCarried;}
    void SetGoldCarried(const int val) { numGoldCarried = val; }
    void AddToGoldCarried(const int val);
    bool PocketsFull() const { return numGoldCarried >= MaxNuggets; }

    int32 Fatigue() const { return (int32)ceil(fatigue); }
    bool Fatigued() const;
    void DecreaseFatigue(real32 amount);
    void IncreaseFatigue(){ ++fatigue; }

    int Wealth() const { return numGoldInBank; }
    void SetWealth(const int val) { numGoldInBank = val; }
    void AddToWealth(const int val);

    int32 Thirst() const { return thirst; } 
    bool Thirsty() const; 
    void BuyAndDrinkAWhiskey()
    {
        thirst = 0;
        numGoldInBank -= 2;
    }

    // New fields for the Matt part
    Vector2 position;
    TownLocation destination;

    real32 travelTime;
    real32 timeToDestination;

    uint32 textureId;

private:
    StateMachine<Miner>* stateMachine;
    real32 stateElapsed;
    real32 stateDuration;

    // Miners current location
    TownLocation location;

    // Amount of nuggets in pockets
    int32 numGoldCarried;

    // Amount of gold in the bank
    int32 numGoldInBank;

    // high value = more thirsty
    int32 thirst;

    // Higher value = more tired
    real32 fatigue;
};
