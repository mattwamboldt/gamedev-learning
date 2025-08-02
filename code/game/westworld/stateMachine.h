#pragma once
#include "state.h"

template <class Entity_Type>
class StateMachine
{
public:
    StateMachine(Entity_Type* owner) :
        owner(owner),
        currentState(0),
        desiredState(0),
        previousState(0),
        globalState(0),
        isPaused(false)
    {}

    virtual ~StateMachine(){}

    //use these methods to initialize the FSM
    void SetGlobalState(State<Entity_Type>* s) { globalState = s; }
    void SetCurrentState(State<Entity_Type>* s) { desiredState = currentState = s; }
    void SetPreviousState(State<Entity_Type>* s) { previousState = s; }

    State<Entity_Type>* CurrentState() const { return currentState; }
    State<Entity_Type>* GlobalState() const { return globalState; }
    State<Entity_Type>* PreviousState() const { return previousState; }

    bool isInState(const State<Entity_Type>& st) const
    {
        return typeid(*currentState) == typeid(st);
    }

    bool isInTransition()
    {
        return currentState != desiredState;
    }

    // The original wasn't made taking into account that transitions (exit, etc.)
    // may want to interrupt the state machine, becasue teh original was a console app
    // This is a cheap way to get this to work realtime.
    void SetPaused(bool value) { isPaused = value; }

    void RevertToPreviousState()
    {
        ChangeState(previousState);
    }

    void Update()
    {
        if (isPaused)
        {
            return;
        }

        // if a global state exists, call its execute method, else do nothing
        if (!isInTransition() && globalState)
        {
            // Make sure global state doesn't do any pause type stuff...
            globalState->Execute(owner);

            if (isPaused)
            {
                return;
            }
        }

        // same for the current state
        if (isInTransition())
        {
            if (currentState)
            {
                previousState = currentState;
                currentState->Exit(owner);
                currentState = 0;
            }

            if (!currentState && !isPaused)
            {
                currentState = desiredState;
                currentState->Enter(owner);
            }
        }
        else if (currentState)
        {
            currentState->Execute(owner);
        }
    }

    void ChangeState(State<Entity_Type>* newState)
    {
        if (isInTransition())
        {
            // TODO: failue?
            return;
        }

        desiredState = newState;

        if (!isPaused)
        {
            previousState = currentState;
            currentState->Exit(owner);
            currentState = 0;
        }

        if (!isPaused)
        {
            currentState = newState;
            currentState->Enter(owner);
        }
    }

private:
    Entity_Type* owner;

    State<Entity_Type>* currentState;
    State<Entity_Type>* desiredState;
    State<Entity_Type>* previousState;
    State<Entity_Type>* globalState;

    bool isPaused;
};
