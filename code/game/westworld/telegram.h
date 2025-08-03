#pragma once

#include "../platform.h"

struct Telegram
{
    int32 sender;
    int32 receiver;
    int32 messageType; // see "MessageTypes.h"
    real64 dispatchTime;
    void* extraInfo;

    Telegram() :
        dispatchTime(-1),
        sender(-1),
        receiver(-1),
        messageType(-1),
        extraInfo(0)
    {}

    Telegram(real64 time, int32 sender, int32 receiver, int32 messageType, void* info = 0) :
        dispatchTime(time),
        sender(sender),
        receiver(receiver),
        messageType(messageType),
        extraInfo(info)
    {}
};
