#include "minerStates.h"
#include "minersWifeStates.h"
#include "westworld.h"

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

int BaseGameEntity::nextValidId = 0;
static WestworldState* gWestworld = 0;

enum Entities
{
    ENTITY_MINER_BOB,
    ENTITY_ELSA,
};

const char* GetNameOfEntity(int32 id)
{
    switch(id)
    {
        case ENTITY_MINER_BOB:
            return "Miner Bob";

        case ENTITY_ELSA:
            return "Elsa";

        default:
            return "UNKNOWN!";
    }
}

void ShowMessageBox(int32 entityId, char* format, ...)
{
    const char* src = GetNameOfEntity(entityId);
    char* dest = gWestworld->messageBox;
    while (*src) *dest++ = *src++;
    *dest++ = ':';
    *dest++ = ' ';

    va_list args;
    va_start(args, format);
    vsprintf(dest, format, args);
    va_end(args);

    gWestworld->messageBoxActive = true;
    gWestworld->miner->GetFSM()->SetPaused(true);
    gWestworld->elsa->GetFSM()->SetPaused(true);
}

void BaseGameEntity::SetID(int32 value)
{
    Assert(value >= nextValidId)
    id = value;
    nextValidId = id + 1;
}

Miner::Miner(int32 id) : BaseGameEntity(id),
    location(SHACK),
    destination(SHACK),
    textureId(0),
    numGoldCarried(0),
    numGoldInBank(0),
    thirst(0),
    fatigue(0)
{
    stateMachine = new StateMachine<Miner>(this);
    stateMachine->SetCurrentState(GoHomeAndSleepTilRested::Instance());
}

void Miner::Update(real32 elapsed)
{
    if (gWestworld->messageBoxActive)
    {
        // This will let us update anims but not state
        return;
    }

    if (location != destination)
    {
        Vector2 startPostion = gWestworld->buildings[location].position;
        Vector2 endPosition = gWestworld->buildings[destination].position;
        travelTime += elapsed;
        if (travelTime >= timeToDestination)
        {
            position = endPosition;
            location = destination;
        }
        else
        {
            position.x = lerp(startPostion.x, endPosition.x, travelTime / timeToDestination);
            position.y = lerp(startPostion.y, endPosition.y, travelTime / timeToDestination);
        }
    }
    else
    {
        if (!stateMachine->isInTransition())
        {
            ++thirst;
        }

        stateMachine->Update();
    }
}

void Miner::ChangeLocation(const TownLocation loc)
{
    destination = loc;
    travelTime = 0.0f;
    timeToDestination = 2.0f;
}

void Miner::AddToGoldCarried(const int val)
{
    numGoldCarried += val;

    if (numGoldCarried < 0) numGoldCarried = 0;
}

void Miner::AddToWealth(const int val)
{
    numGoldInBank += val;

    if (numGoldInBank < 0) numGoldInBank = 0;
}

bool Miner::Thirsty()const
{
    return thirst >= ThirstLevel;
}

bool Miner::Fatigued() const
{
    return fatigue > TirednessThreshold;
}

EnterMineAndDigForNugget* EnterMineAndDigForNugget::Instance()
{
    static EnterMineAndDigForNugget instance;
    return &instance;
}

void EnterMineAndDigForNugget::Enter(Miner* pMiner)
{
    if (pMiner->Location() != GOLDMINE)
    {
        ShowMessageBox(pMiner->Id(), "Walkin' to the goldmine");
        pMiner->ChangeLocation(GOLDMINE);
    }
}

void EnterMineAndDigForNugget::Execute(Miner* pMiner)
{  
    //the miner digs for gold until he is carrying in excess of MaxNuggets. 
    //If he gets thirsty during his digging he packs up work for a while and 
    //changes state to go to the saloon for a whiskey.
    pMiner->AddToGoldCarried(1);
    pMiner->IncreaseFatigue();

    ShowMessageBox(pMiner->Id(), "Pickin' up a nugget");

    //if enough gold mined, go and put it in the bank
    if (pMiner->PocketsFull())
    {
        pMiner->GetFSM()->ChangeState(VisitBankAndDepositGold::Instance());
    }

    if (pMiner->Thirsty())
    {
        pMiner->GetFSM()->ChangeState(QuenchThirst::Instance());
    }
}

void EnterMineAndDigForNugget::Exit(Miner* pMiner)
{
    ShowMessageBox(pMiner->Id(), "Ah'm leavin' the goldmine with mah pockets full o' sweet gold");
}

VisitBankAndDepositGold* VisitBankAndDepositGold::Instance()
{
    static VisitBankAndDepositGold instance;
    return &instance;
}

void VisitBankAndDepositGold::Enter(Miner* pMiner)
{  
    // on entry the miner makes sure he is located at the bank
    if (pMiner->Location() != BANK)
    {
        ShowMessageBox(pMiner->Id(), "Goin' to the bank. Yes siree");

        pMiner->ChangeLocation(BANK);
    }
}

void VisitBankAndDepositGold::Execute(Miner* pMiner)
{
    //deposit the gold
    pMiner->AddToWealth(pMiner->GoldCarried());
    pMiner->SetGoldCarried(0);

    //wealthy enough to have a well earned rest?
    if (pMiner->Wealth() >= ComfortLevel)
    {
        ShowMessageBox(pMiner->Id(), "WooHoo! Rich enough for now. Back home to mah li'lle lady.");
        pMiner->GetFSM()->ChangeState(GoHomeAndSleepTilRested::Instance());      
    }
    //otherwise get more gold
    else 
    {
        ShowMessageBox(pMiner->Id(), "Depositing gold. Total savings now: %d.", pMiner->Wealth());
        pMiner->GetFSM()->ChangeState(EnterMineAndDigForNugget::Instance());
    }
}

void VisitBankAndDepositGold::Exit(Miner* pMiner)
{
    ShowMessageBox(pMiner->Id(), "Leavin' the bank.");
}

GoHomeAndSleepTilRested* GoHomeAndSleepTilRested::Instance()
{
    static GoHomeAndSleepTilRested instance;
    return &instance;
}

void GoHomeAndSleepTilRested::Enter(Miner* pMiner)
{
    if (pMiner->Location() != SHACK)
    {
        ShowMessageBox(pMiner->Id(), "Walkin' home.");
        pMiner->ChangeLocation(SHACK); 
    }
}

void GoHomeAndSleepTilRested::Execute(Miner* pMiner)
{ 
    //if miner is not fatigued start to dig for nuggets again.
    if (!pMiner->Fatigued())
    {
        ShowMessageBox(pMiner->Id(), "What a God darn fantastic nap! Time to find more gold.");
        pMiner->GetFSM()->ChangeState(EnterMineAndDigForNugget::Instance());
    }
    else 
    {
        pMiner->DecreaseFatigue();
        ShowMessageBox(pMiner->Id(), "ZZZZ...");
    } 
}

void GoHomeAndSleepTilRested::Exit(Miner* pMiner)
{
    ShowMessageBox(pMiner->Id(), "Leaving the house.");
}

QuenchThirst* QuenchThirst::Instance()
{
    static QuenchThirst instance;
    return &instance;
}

void QuenchThirst::Enter(Miner* pMiner)
{
    if (pMiner->Location() != SALOON)
    {    
        pMiner->ChangeLocation(SALOON);
        ShowMessageBox(pMiner->Id(), "Boy, ah sure is thusty! Walking to the saloon.");
    }
}

void QuenchThirst::Execute(Miner* pMiner)
{
    if (pMiner->Thirsty())
    {
        pMiner->BuyAndDrinkAWhiskey();
        ShowMessageBox(pMiner->Id(), "That's mighty fine sippin liquer.");
    }
    else
    {
        pMiner->GetFSM()->ChangeState(EnterMineAndDigForNugget::Instance());
    }
}

void QuenchThirst::Exit(Miner* pMiner)
{
    ShowMessageBox(pMiner->Id(), "Leaving the saloon, feelin' good!");
}

MinersWife::MinersWife(int32 id) : BaseGameEntity(id),
    stateElapsed(0),
    stateDuration(2.0f)
{
    stateMachine = new StateMachine<MinersWife>(this);
    stateMachine->SetCurrentState(DoHouseWork::Instance());
    stateMachine->SetGlobalState(WifesGlobalState::Instance());
}

void MinersWife::Update(real32 elapsed)
{
    if (stateMachine->isInTransition())
    {
        stateMachine->Update();
    }
    else
    {
        stateElapsed += elapsed;
        if (stateElapsed > stateDuration)
        {
            stateMachine->Update();
            stateElapsed -= stateDuration;
        }
    }
}

WifesGlobalState* WifesGlobalState::Instance()
{
  static WifesGlobalState instance;
  return &instance;
}

void WifesGlobalState::Execute(MinersWife* wife)
{
    //1 in 10 chance of needing the bathroom
    if (rand() % 10 == 0)
    {
        wife->GetFSM()->ChangeState(VisitBathroom::Instance());
    }
}

DoHouseWork* DoHouseWork::Instance()
{
    static DoHouseWork instance;
    return &instance;
}

void DoHouseWork::Execute(MinersWife* wife)
{
    switch(rand() % 3)
    {
    case 0:
        ShowMessageBox(wife->Id(), "Moppin' the floor");
        break;

    case 1:
        ShowMessageBox(wife->Id(), "Washin' the dishes");
        break;

    case 2:
        ShowMessageBox(wife->Id(), "Makin' the bed");
        break;
    }
}

VisitBathroom* VisitBathroom::Instance()
{
    static VisitBathroom instance;
    return &instance;
}

void VisitBathroom::Enter(MinersWife* wife)
{
    ShowMessageBox(wife->Id(), "Walkin' to the can. Need to powda mah pretty li'lle nose"); 
}

void VisitBathroom::Execute(MinersWife* wife)
{
    ShowMessageBox(wife->Id(), "Ahhhhhh! Sweet relief!");
    wife->GetFSM()->RevertToPreviousState();
}

void VisitBathroom::Exit(MinersWife* wife)
{
    ShowMessageBox(wife->Id(), "Leavin' the Jon");
}

void WestworldUpdateAndRender(GameState *state, GameInput* input)
{
    gWestworld = &state->westworld;
    if (!gWestworld->isInitialized)
    {
        gWestworld->messageBox[0] = 0;
        gWestworld->miner = new Miner(ENTITY_MINER_BOB);
        gWestworld->miner->textureId = LoadPNG("westworld/prospector/idle0.png");
        
        gWestworld->elsa = new MinersWife(ENTITY_ELSA);

        gWestworld->miner->Update(0);
        gWestworld->elsa->Update(0);
        gWestworld->groundTextureId = LoadPNG("westworld/ground.png");

        // SHACK
        gWestworld->buildings[SHACK].location = SHACK;
        gWestworld->buildings[SHACK].position = {state->frame.width * 0.25f, state->frame.height * 0.75f};
        gWestworld->buildings[SHACK].textureId = LoadPNG("westworld/shack.png");

        // GOLDMINE
        gWestworld->buildings[GOLDMINE].location = GOLDMINE;
        gWestworld->buildings[GOLDMINE].position = {state->frame.width * 0.75f, state->frame.height * 0.75f};
        gWestworld->buildings[GOLDMINE].textureId = LoadPNG("westworld/goldmine.png");

        // BANK
        gWestworld->buildings[BANK].location = BANK;
        gWestworld->buildings[BANK].position = {state->frame.width * 0.25f, state->frame.height * 0.25f};
        gWestworld->buildings[BANK].textureId = LoadPNG("westworld/bank.png");

        // SALOON
        gWestworld->buildings[SALOON].location = SALOON;
        gWestworld->buildings[SALOON].position = { state->frame.center, state->frame.middle};
        gWestworld->buildings[SALOON].textureId = LoadPNG("westworld/saloon.png");

        // ENTRANCE
        gWestworld->buildings[ENTRANCE].location = ENTRANCE;
        gWestworld->buildings[ENTRANCE].position = { state->frame.center, 150};
        gWestworld->buildings[ENTRANCE].textureId = LoadPNG("westworld/entrance.png");

        // He starts at home in his shack
        gWestworld->miner->position = gWestworld->buildings[SHACK].position;

        gWestworld->isInitialized = true;
    }

    if (input->controller.cross.wasPressed())
    {
        gWestworld->messageBoxActive = false;
        gWestworld->miner->GetFSM()->SetPaused(false);
        gWestworld->elsa->GetFSM()->SetPaused(false);
    }

    gWestworld->miner->Update(input->timeDelta);
    gWestworld->elsa->Update(input->timeDelta);

    RectF screenRect = {
        0, 0,
        state->frame.width, state->frame.height
    };

    RenderTexture(gWestworld->groundTextureId, screenRect, { 0, 0 });

    for (int i = 0; i < NUM_LOCATIONS; ++i)
    {
        Building building = gWestworld->buildings[i];
        Vector2 size = GetTextureSize(building.textureId);
        RectF dest = {
            building.position.x, building.position.y,
            size.x, size.y
        };

        RenderTexture(building.textureId, dest, { 0.5f, 0 });
    }

    // Render Miner
    Vector2 minerSize = GetTextureSize(gWestworld->miner->textureId);
    RectF minerDest = {
        gWestworld->miner->position.x, gWestworld->miner->position.y,
        minerSize.x, minerSize.y
    };

    RenderTexture(gWestworld->miner->textureId, minerDest, {0.5, 0});

    {
        char buffer[64];
        sprintf(buffer, "Gold: %d / %d", gWestworld->miner->GoldCarried(), MaxNuggets);
        Vector2 messageSize = MeasureText(state->mainFontId, buffer);
        Vector2 goldPosition = {
            gWestworld->miner->position.x,
            gWestworld->miner->position.y - 5
        };

        DrawRectangle(goldPosition.x, goldPosition.y, messageSize.x + 20, messageSize.y + 20, blue, {0.5, 1}, white);
        RenderText(state->mainFontId, buffer, goldPosition.x, goldPosition.y - 10, {0.5, 1}, white);
    }

    // Render some meters
    {
        real32 width = gWestworld->miner->Thirst() / (real32)ThirstLevel;
        if (width > 1) width = 1;
        
        real32 barX = state->frame.right - 10 - 100;
        DrawRectangle(barX, state->frame.middle, 100, 20, black, {0, 0.5});
        DrawRectangle(barX, state->frame.middle, 100 * width, 20, blue, {0, 0.5});

        char buffer[64];
        sprintf(buffer, "Thirst: %d / %d", gWestworld->miner->Thirst(), ThirstLevel);
        Vector2 messageSize = MeasureText(state->mainFontId, buffer);
        RenderText(state->mainFontId, buffer, barX - 10, state->frame.middle, {1, 0.5}, black);
    }

    {
        char buffer[64];
        sprintf(buffer, "Fatigue: %d / %d", gWestworld->miner->Fatigue(), TirednessThreshold);
        Vector2 messageSize = MeasureText(state->mainFontId, buffer);

        real32 width = gWestworld->miner->Fatigue() / (real32)TirednessThreshold;
        if (width > 1) width = 1;

        real32 barX = state->frame.right - 10 - 100;
        real32 barY = state->frame.middle - 10 - messageSize.y;
        DrawRectangle(barX, barY, 100, 20, black, {0, 0.5});
        DrawRectangle(barX, barY, 100 * width, 20, red, {0, 0.5});

        RenderText(state->mainFontId, buffer, barX - 10, barY, {1, 0.5}, black);
    }

    if (gWestworld->messageBoxActive)
    {
        Vector2 messageSize = MeasureText(state->mainFontId, gWestworld->messageBox);
        DrawRectangle(20, 20, state->frame.width - 40, messageSize.y + 20, blue, {0, 0}, white);
        RenderText(state->mainFontId, gWestworld->messageBox, 30, 30, {0, 0}, white);
    }
}
