#pragma once
#include "../platform.h"

enum MessageType
{
    MSG_HiHoneyImHome,
    MSG_StewReady,
};

inline const char* MessageTypeToString(int32 messageType)
{
    switch (messageType)
    {
        case MSG_HiHoneyImHome: return "HiHoneyImHome"; 
        case MSG_StewReady: return "StewReady";
        default: return "Not recognized!";
    }
}
