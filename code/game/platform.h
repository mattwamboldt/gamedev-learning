#pragma once

// This file contains definitions of types that are shared between
// the platform specific code and the generic game code

// TODO: Currently much of this is derivative of handmade hero. I prefer using some amount of classes for organization, unlike casey
// So I want to refactor a lot of this to my style/idioms once code reloading and the other platform lessons are in here and stable

// Common types, typedefs and macros that make life simpler
#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;

typedef float real32;
typedef double real64;

#ifndef FINAL
#define Assert(expression) if(!(expression)) { *(int *)0 = 0; }
#else
#define Assert(expression)
#endif

#define Kilobytes(value) ((value) * 1024)
#define Megabytes(value) (Kilobytes(value) * 1024)
#define Gigabytes(value) (Megabytes(value) * 1024)
#define Terabytes(value) (Gigabytes(value) * 1024)

struct Color
{
	real32 r;
	real32 g;
	real32 b;
	real32 a;
};

struct RectF
{
    real32 x;
    real32 y;
    real32 width;
    real32 height;
};

struct Vector2
{
    real32 x;
    real32 y;
};

inline Vector2 operator+(Vector2 a, Vector2 b)
{
    Vector2 result = {};
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

struct GameRenderer
{
    real32 screenWidth;
    real32 screenHeight;
};

struct Button
{
    bool isDown;
    bool wasDown;

    void setState(bool state)
    {
        wasDown = isDown;
        isDown = state;
    }

    bool wasPressed() { return isDown && !wasDown; }
    bool wasReleased() { return !isDown && wasDown; }
};

struct Controller
{
    enum Buttons
    {
        // DPAD
        UP = 0,
        DOWN,
        LEFT,
        RIGHT,

        // Face (using sony names as positions are unambiguous)
        CROSS,
        CIRCLE,
        SQUARE,
        TRIANGLE,

        LEFT_SHOULDER,
        RIGHT_SHOULDER,
        LEFT_THUMB,
        RIGHT_THUMB,

        // TODO: Need to figure out if these are ok names
        START,
        SELECT,

        NUM_BUTTONS
    };

    int32 deviceId;
    bool isConnected;

    // NOTE: unions are super cool
    union
    {
        Button buttons[NUM_BUTTONS];
        struct
        {
            Button up;
            Button down;
            Button left;
            Button right;

            Button cross;
            Button circle;
            Button square;
            Button triangle;

            Button leftShoulder;
            Button rightShoulder;
            Button leftThumb;
            Button rightThumb;

            Button start;
            Button select;
        };
    };

    Vector2 leftStick;
    Vector2 rightStick;

    real32 leftTrigger;
    real32 rightTrigger;
};

struct Mouse
{
    bool isConnected;
    real32 x;
    real32 y;

    Button left;
    Button right;
};

struct GameInput
{
    real32 timeDelta;

    Controller controller;
    Mouse mouse;
};

// TODO: Split this into a generic set of data thats shared with game, and platform specific info
struct GameAudio
{
    int32 samplesPerSecond;
    int32 bytesPerSample;
    int32 numChannels;
    uint32 bytesPerSecond;
    uint32 bytesPerFrame;
    uint32 bufferSize;
    uint32 padding;

    uint32 sampleIndex;
    uint32 sampleCount;
    int16* samples;
};

// TODO: Currently following the split handmade hero so I can get live code reloading basics up but will refactor once that's working
struct GameMemory
{
    // Whether the game has run its startup code. I might split this to a third game service function, depends on how reload goes
    bool isInitialized;

    // Used for core data that is required for the game to run
    void* permanentStorage;
    uint32 permanentStorageSize;

    // Assets, Scratch space, stuff that can be reloaded/regenerated if needed
    void* transientStorage;
    uint32 transientStorageSize;
};

struct Texture
{
    int32  width;
    int32  height;
    uint32 handle;
    uint32* bytes; // for now we keep the bytes around
};

struct Sprite
{
    Texture* texture;
    Color color;
    RectF source;

    // This effectively IS the transformation matrix
    Vector2 xAxis;
    Vector2 yAxis;
    Vector2 origin;

    Color stroke;
};

struct Buffer
{
    uint32 size;
    void* memory;
};

class IPlatform
{
public:
    virtual uint64 getFileLastModified(char*) = 0;
    virtual Buffer readWholeFile(char* path) = 0;
    virtual void writeWholeFile(char* path, void* memory, uint32 numBytes) = 0;
    virtual void freeBuffer(Buffer buffer) = 0;

    virtual void loadHardwareTexture(Texture* texture) = 0;
    virtual void freeHardwareTexture(uint32 handle) = 0;

    virtual void setProjection(real32 width, real32 height) = 0;
    virtual void renderSprite(Sprite sprite) = 0;
    virtual void clearScreen(Color color) = 0;
};

// These defines help avoid issues with changing the parameters of
// C based interfaces
#define GAME_UPDATE_AND_RENDER(name) void name(GameMemory* memory, GameInput* input, GameRenderer* renderer, IPlatform* platform)
typedef GAME_UPDATE_AND_RENDER(GameUpdateAndRenderFn);

#define GAME_OUTPUT_SOUND(name) void name(GameMemory* memory, GameAudio* output)
typedef GAME_OUTPUT_SOUND(GameOutputSoundFn);
