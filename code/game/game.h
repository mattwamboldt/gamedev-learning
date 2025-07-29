#pragma once

#include "platform.h"

// Global types that are used by the game that the platform doesn't need to know about

struct Oscillator
{
    bool isPlaying;
    bool wasPlaying;

    real64 phase;
    real64 increment;
    real64 frequency;
    real64 samplingRadians;

    void setFrequency(real64 desiredFrequency);

    real64 nextSample();

    void start();
    void stop();
};

enum FileType
{
    FILETYPE_HTX,
    FILETYPE_BMP,
    FILETYPE_PNG,
    FILETYPE_WAV,
    FILETYPE_TTF
};

// Mainly used for asset reloading
struct AssetEntry
{
    uint64 lastModified;
    char fileName[256];
    FileType type;

    void* address;
    uint32 size;
};

#define MAX_GLYPHS 2048
struct Glyph
{
    uint32 symbol;
    RectF sourceRect;
    real32 xOffset;
    real32 yOffset;
    real32 xAdvance;
};

struct BitmapFont
{
    real32 size;
    real32 lineHeight;
	real32 baseline;
    uint32 numGlyphs;
    Glyph glyphs[MAX_GLYPHS];
	Texture texture;
};

// TODO: Some of these assets arrays should potentially be dynamic, at least in size
// since different levels have different requirements for them, perhaps we have multiple
// levels of asset cache for permanent game wide assets vs level specific/mode specific
// This may let us get away with stack allocation at runtime, though it has issues with hot reload in development
// TODO: Add a mapping of pathname, lastmodified to this for hotreload
struct Assets
{
    uint32 numFonts;
    BitmapFont fonts[10];

    //uint32 numSoundSources;
    //SoundSource soundSources[30];

    uint32 numTextures;
    Texture textures[200];

    uint32 numEntries;
    AssetEntry entries[1000];
};

struct Camera
{
    // used for projecting and unprojecting mouse coordinates
    Vector2 screenSize;
    real32 screenScale;
    Vector2 size;
    Vector2 position;
    real32 aspect;
};

struct UIFrame
{
    real32 center;
    real32 middle;
    real32 left;
    real32 right;
    real32 top;
    real32 bottom;
    real32 width;
    real32 height;
};

enum Demo
{
    DEMO_WESTWORLD = 0,
    NUM_DEMOS,

    DEMO_SELECT = -1,
};

#include "westworld/westworld.h"

struct GameState
{
    Oscillator sound;
    Color screenColor;

    Assets assets;

    UIFrame frame;
    Camera camera;

    Demo currentDemo;
    int selectedDemo;

    int32 mainFontId;

    // If I add more demos this is probably best done as a stack push that can
    // can be popped when we leave this demos lifetime
    WestworldState westworld;
};
