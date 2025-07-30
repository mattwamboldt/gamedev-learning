#pragma once
#include "miner.h"

/**
 * In this state the miner will walk to a goldmine and pick up a nugget of gold.
 * If the miner already has a nugget of gold he'll change state to
 * VisitBankAndDepositGold. If he gets thirsty he'll change state to QuenchThirst
*/
class EnterMineAndDigForNugget : public State<Miner>
{
public:
    static EnterMineAndDigForNugget* Instance();

    virtual void Enter(Miner* miner);
    virtual void Execute(Miner* miner);
    virtual void Exit(Miner* miner);

private:
    // Prevents accidental non singleton instantiation
    EnterMineAndDigForNugget(){}
    EnterMineAndDigForNugget(const EnterMineAndDigForNugget&);
    EnterMineAndDigForNugget& operator=(const EnterMineAndDigForNugget&);
};

/**
 * Entity will go to a bank and deposit any nuggets he is carrying. If the miner
 * is subsequently wealthy enough he'll walk home, otherwise he'll keep going to
 * get more gold
 */
class VisitBankAndDepositGold : public State<Miner>
{
public:
    static VisitBankAndDepositGold* Instance();

    virtual void Enter(Miner* miner);
    virtual void Execute(Miner* miner);
    virtual void Exit(Miner* miner);

private:
    // Prevents accidental non singleton instantiation
    VisitBankAndDepositGold(){}
    VisitBankAndDepositGold(const VisitBankAndDepositGold&);
    VisitBankAndDepositGold& operator=(const VisitBankAndDepositGold&);
};

/// @brief go home and sleep until his fatigue is decreased sufficiently
class GoHomeAndSleepTilRested : public State<Miner>
{
public:
    static GoHomeAndSleepTilRested* Instance();

    virtual void Enter(Miner* miner);
    virtual void Execute(Miner* miner);
    virtual void Exit(Miner* miner);

private:
    // Prevents accidental non singleton instantiation
    GoHomeAndSleepTilRested(){}
    GoHomeAndSleepTilRested(const GoHomeAndSleepTilRested&);
    GoHomeAndSleepTilRested& operator=(const GoHomeAndSleepTilRested&);
};

/// @brief Going to the bar to quench his thirst
class QuenchThirst : public State<Miner>
{
public:
    static QuenchThirst* Instance();

    virtual void Enter(Miner* miner);
    virtual void Execute(Miner* miner);
    virtual void Exit(Miner* miner);

private:
    // Prevents accidental non singleton instantiation
    QuenchThirst(){}
    QuenchThirst(const QuenchThirst&);
    QuenchThirst& operator=(const QuenchThirst&);
};
