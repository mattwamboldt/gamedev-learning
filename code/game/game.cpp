#include "game.h"

// TODO LIST
// - Font loading and text rendering
// - A list to select which thing to run
// - First thing should be AI Book, add an entry for each of those as we go

// TODO: All temp code, ditch it later
#define PI     3.14159265359
#define TWO_PI 6.28318530718

#include <math.h>

#define HexColor(name, r, g, b, a) \
	Color name = {(real32)r / 255.0f, (real32)g / 255.0f, (real32)b / 255.0f, (real32)a / 255.0f};

Color black = { 0, 0, 0, 1 };
Color white = { 1, 1, 1, 1 };
Color transparent = { 0, 0, 0, 0 };

HexColor(gold, 0xFF, 0xEB, 0x80, 0xFF)
HexColor(grey, 0x49, 0x43, 0x44, 0xFF)
HexColor(red, 0xAD, 0x38, 0x1D, 0xFF)
HexColor(blue, 0x29, 0x4D, 0xA9, 0xFF)
HexColor(green, 0x33, 0x9A, 0x06, 0xFF)

static IPlatform* gPlatform = 0;
static GameState* gState = 0;

inline Vector2 operator-(Vector2 a, Vector2 b)
{
    Vector2 result = {};
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline Vector2 operator*(Vector2 v, real32 s)
{
    Vector2 result = {};
    result.x = v.x * s;
    result.y = v.y * s;
    return result;
}

inline Vector2 operator*(real32 s, Vector2 v)
{
    return v * s;
}

void Oscillator::setFrequency(real64 desiredFrequency)
{
    frequency = desiredFrequency;
    increment = frequency * samplingRadians;
}

real64 Oscillator::nextSample()
{
    double value = sin(phase);
    phase += increment;
    if (phase >= TWO_PI)
    {
        phase -= TWO_PI;
    }

    if (phase < 0.0)
    {
        phase += TWO_PI;
    }

    return value;
}

void Oscillator::start()
{
    isPlaying = true;
}

void Oscillator::stop()
{
    isPlaying = false;
}

real32 lerp(real32 start, real32 end, real32 t)
{
    return start + (end - start) * t;
}

#define STBI_ASSERT Assert
#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

void ProcessPNG(Buffer tempFileBuffer, Texture* texture)
{
    uint8* data = stbi_load_from_memory((uint8*)tempFileBuffer.memory, tempFileBuffer.size, &texture->width, &texture->height, 0, 4);
    uint32 numBytes = texture->width * texture->height * sizeof(uint32);
    texture->bytes = (uint32*)malloc(numBytes);

    uint32* dest = texture->bytes;
    uint32* sourceRow = (uint32*)data + texture->width * (texture->height - 1);

    for (int y = 0; y < texture->height; ++y)
    {
        uint32* source = sourceRow;
        for (int x = 0; x < texture->width; ++x)
        {
            uint32 sc = *source++;
            uint32 sr = (sc & 0x000000FF);
            uint32 sg = (sc & 0x0000FF00) >> 8;
            uint32 sb = (sc & 0x00FF0000) >> 16;
            uint32 sa = (sc & 0xFF000000) >> 24;
            real32 a = sa / 255.0f;

            *dest++ = sa << 24 |
                (uint32)(sr * a - 0.5f) << 16 |
                (uint32)(sg * a - 0.5f) << 8 |
                (uint32)(sb * a - 0.5f);
        }

        sourceRow -= texture->width;
    }

    stbi_image_free(data);
    gPlatform->loadHardwareTexture(texture);
}

BitmapFont* GetFont(Assets* assets, int32 handle)
{
	return assets->fonts + handle;
}

uint32 AddAssetEntry(Assets* assets, FileType type, char* path, void* address)
{
    uint32 entryId = assets->numEntries++;
    AssetEntry* entry = assets->entries + entryId;
    entry->type = type;
    strcpy(entry->fileName, path);
    entry->address = address;
    entry->lastModified = gPlatform->getFileLastModified(path);
    return entryId;
}

Texture* GetTexture(Assets* assets, int32 handle)
{
    AssetEntry entry = assets->entries[handle];
    return (Texture*)entry.address;
}

Vector2 GetTextureSize(int32 textureId)
{
    Texture* texture = GetTexture(&gState->assets, textureId);
    return{ (real32)texture->width, (real32)texture->height };
}

uint32 LoadPNG(char* path)
{
    Buffer tempFileBuffer = gPlatform->readWholeFile(path);
    Assert(tempFileBuffer.memory);

    Texture* texture = gState->assets.textures + gState->assets.numTextures++;
    ProcessPNG(tempFileBuffer, texture);
    gPlatform->freeBuffer(tempFileBuffer);
    return AddAssetEntry(&gState->assets, FILETYPE_PNG, path, texture);
}

// TODO: Get kerning information. May not be efficient to do with stb, may need to be purely asset processor driven
// Maybe best to use this as a DEBUG only method
int32 LoadTTF(char* path, real32 size)
{
    Buffer fileContent = gPlatform->readWholeFile(path);
    Assert(fileContent.memory);

    const uint32 firstChar = ' ';
    const uint32 lastChar = '~';

    stbtt_fontinfo sourceFont;
    sourceFont.userdata = 0;
    if (!stbtt_InitFont(&sourceFont, (uint8*)fileContent.memory, 0))
    {
        return -1;
    }

    // TODO: Switch to scratch stack later
    int textureSize = 512;
    uint8* tempBitmap = (uint8*)malloc(textureSize * textureSize);
    memset(tempBitmap, 0, textureSize * textureSize); // background of 0 around pixels
    
    real32 scale = stbtt_ScaleForPixelHeight(&sourceFont, size);
    int32 ascent; // distance above baseline
    int32 descent; // distance below baseline (usually negative
    int32 leading; // gap between descent and ascent of next line
    stbtt_GetFontVMetrics(&sourceFont, &ascent, &descent, &leading);

    int32 fontId = gState->assets.numFonts++;
    BitmapFont* font = gState->assets.fonts + fontId;
    font->size = size;
    font->numGlyphs = lastChar - firstChar;
    font->baseline = ascent * scale;
    font->lineHeight = font->baseline + (-descent * scale) + (leading * scale);

    bool packSuccessful = true;

    {
        int x = 1;
        int y = 1;
        int bottom_y = 1;
        uint32 i = 0;
        uint32 numGlyphs = lastChar - firstChar;

        while (i < numGlyphs)
        {
            Glyph* glyph = font->glyphs + i;
            glyph->symbol = firstChar + i;

            int glyphIndex = stbtt_FindGlyphIndex(&sourceFont, glyph->symbol);

            int advance;
            stbtt_GetGlyphHMetrics(&sourceFont, glyphIndex, &advance, 0);
            glyph->xAdvance = scale * advance;

            int x0, y0, x1, y1;
            stbtt_GetGlyphBitmapBox(&sourceFont, glyphIndex, scale, scale, &x0, &y0, &x1, &y1);

            int width = x1 - x0;
            int height = y1 - y0;

            if (x + width + 1 >= textureSize)
            {
                // advance to next row
                y = bottom_y;
                x = 1;
            }

            if (y + height + 1 >= textureSize)
            {
                // check if it fits vertically AFTER potentially moving to next row
                packSuccessful = false;
                break;
            }

            Assert((x + width < textureSize) && (y + height < textureSize));

            stbtt_MakeGlyphBitmap(&sourceFont, tempBitmap + x + y * textureSize, width, height, textureSize, scale, scale, glyphIndex);

            glyph->sourceRect.x = (real32)x;
            glyph->sourceRect.y = (real32)y;
            glyph->sourceRect.width = (real32)width;
            glyph->sourceRect.height = (real32)height;
            glyph->xOffset = (real32)x0;
            glyph->yOffset = (real32)-y0;

            x = x + width + 1;
            if (y + height + 1 > bottom_y)
            {
                bottom_y = y + height + 1;
            }

            ++i;
        }
    }

    gPlatform->freeBuffer(fileContent);
    if (!packSuccessful) {
        free(tempBitmap);
        --gState->assets.numFonts;
        return -1;
    }

    font->texture.width = textureSize;
    font->texture.height = textureSize;
    font->texture.bytes = (uint32*)malloc(textureSize * textureSize * sizeof(uint32));

    uint8* source = tempBitmap;
    uint32* destRow = font->texture.bytes + (textureSize - 1) * textureSize;

    for (int y = 0; y < textureSize; ++y)
    {
        uint32* dest = destRow;
        for (int x = 0; x < textureSize; ++x)
        {
            uint8 alpha = *source++;
            *dest++ = (alpha << 24) | (alpha << 16) | (alpha << 8) | alpha;
        }

        destRow -= textureSize;
    }

    gPlatform->loadHardwareTexture(&font->texture);
    free(tempBitmap);
    return fontId;
}

real32 ToRadians(real32 degrees)
{
    return (degrees * (real32)(PI)) / 180.0f;
}

// TODO: figure out the invalid/not yet loaded case for these
// TODO: set this to handle invalid ids
void RenderTexture(int32 textureId, RectF dest, Vector2 pivot, real32 rotation, Vector2 shear, Color color)
{
    Sprite sprite = {};
    sprite.texture = GetTexture(&gState->assets, textureId);
    sprite.color = color;
    sprite.source = { 0, 0, (real32)sprite.texture->width, (real32)sprite.texture->height };

    Vector2 offset = { dest.x, dest.y };

    real32 radians = ToRadians(rotation + shear.x);
    sprite.xAxis = { cosf(radians), sinf(radians) };

    radians = ToRadians(rotation + 90.0f + shear.y);
    sprite.yAxis = { cosf(radians), sinf(radians) };

    Vector2 align = { pivot.x * dest.width, pivot.y * dest.height };
    sprite.origin = offset - align.x * sprite.xAxis - align.y * sprite.yAxis;

    sprite.xAxis = sprite.xAxis * dest.width;
    sprite.yAxis = sprite.yAxis * dest.height;

    // Stand in for world to view transform
    sprite.origin = sprite.origin + gState->camera.position;

    gPlatform->renderSprite(sprite);
}

void DrawRectangle(real32 x, real32 y, real32 width = 1.0f, real32 height = 1.0f, 
	Color fill = { 1, 1, 1, 1 }, Vector2 pivot = { 0.5f, 0.5f }, Color stroke = { 0, 0, 0, 0 }, real32 rotation = 0, Vector2 shear = { 0, 0 })
{
	Sprite sprite = {};
	sprite.color = fill;
	sprite.stroke = stroke;

	Vector2 offset = { x, y };

	real32 radians = ToRadians(rotation + shear.x);
	sprite.xAxis = { cosf(radians), sinf(radians) };

	radians = ToRadians(rotation + 90.0f + shear.y);
	sprite.yAxis = { cosf(radians), sinf(radians) };

	Vector2 align = { pivot.x * width, pivot.y * height };
	sprite.origin = offset - align.x * sprite.xAxis - align.y * sprite.yAxis;

	sprite.xAxis = sprite.xAxis * width;
	sprite.yAxis = sprite.yAxis * height;

	// Stand in for world to view transform
	sprite.origin = sprite.origin + gState->camera.position;

	gPlatform->renderSprite(sprite);
}

void RenderTexture(int32 textureId, RectF dest, Vector2 pivot = { 0.5f, 0.5f }, real32 rotation = 0.0f, Vector2 shear = { 0, 0 })
{
    RenderTexture(textureId, dest, pivot, rotation, shear, white);
}

Glyph GetGlyph(BitmapFont* font, uint32 character)
{
    Assert(font->numGlyphs > 0);

    // skip 0, it's the null glyph
    for (uint32 g = 1; g < font->numGlyphs; ++g)
    {
        if (font->glyphs[g].symbol == character)
        {
            return font->glyphs[g];
        }
    }

    return font->glyphs[0];
}

Vector2 MeasureText(int32 fontId, const char* text, real32 scale = 1.0f)
{
    BitmapFont* font = GetFont(&gState->assets, fontId);
    Vector2 dimensions = { 1, font->lineHeight * scale };

    real32 cursorX = 0;
    real32 cursorY = 0 - font->baseline * scale;

    // TODO: Handle UTF-8 conversion
    for (int i = 0; text[i] != 0; ++i)
    {
        uint32 id = text[i];
        if (id == '\r' || id == '\n')
        {
            if (text[i + 1] == '\n')
            {
                ++i;
            }

            cursorX = 0;
            cursorY -= font->lineHeight * scale;
            dimensions.y += font->lineHeight * scale;
            continue;
        }

        Glyph glyph = GetGlyph(font, id);
        cursorX += glyph.xAdvance * scale;
        if (cursorX > dimensions.x)
        {
            dimensions.x = cursorX;
        }
    }

    return dimensions;
}

void RenderText(int32 fontId, const char* text, real32 x, real32 y, Vector2 pivot = { 0.5f, 0.5f }, Color color = black, real32 scale = 1.0f, real32 rotation = 0, Vector2 shear = {0, 0})
{
    Vector2 textDimensions = MeasureText(fontId, text, scale);
    x -= textDimensions.x * pivot.x;
    y += textDimensions.y * (1 - pivot.y);

    BitmapFont* font = GetFont(&gState->assets, fontId);
    RectF dimensions = { x, y, 1, font->lineHeight * scale };

    real32 cursorX = x;
    real32 cursorY = y - font->baseline * scale;

    Sprite sprite = {};
    sprite.texture = &font->texture;
    sprite.color = color;
    //sprite.stroke = green;

    // TODO: Handle UTF-8 conversion
    for (int i = 0; text[i] != 0; ++i)
    {
        uint32 id = text[i];
        if (id == '\r' || id == '\n')
        {
            if (text[i + 1] == '\n')
            {
                ++i;
            }

            cursorX = (real32)x;
            cursorY -= font->lineHeight * scale;
            dimensions.height += font->lineHeight * scale;

            continue;
        }

        Glyph glyph = GetGlyph(font, id);
        sprite.source = glyph.sourceRect;

        // Whitespace should just move the cursor forward
        if (id != '\t' && id != ' ')
        {
            // TODO: if we want wibly wobbly text, position the origin
            // and alter the pivot accordingly, now that we have these metrics
            // real32 ascent = glyph.yOffset;
            // real32 descent = ascent - sprite.source.height;

            Vector2 position = {
                cursorX + (glyph.xOffset * scale) + (sprite.source.width * 0.5f * scale),
                cursorY + glyph.yOffset
            };

            real32 radians = ToRadians(rotation + shear.x);
            sprite.xAxis = { cosf(radians), sinf(radians) };

            radians = ToRadians(rotation + 90.0f + shear.y);
            sprite.yAxis = { cosf(radians), sinf(radians) };

            Vector2 align = { 0.5f * sprite.source.width, sprite.source.height };
            sprite.origin = position - align.x * sprite.xAxis - align.y * sprite.yAxis;

            sprite.xAxis = sprite.xAxis * sprite.source.width * scale;
            sprite.yAxis = sprite.yAxis * sprite.source.height * scale;

            gPlatform->renderSprite(sprite);
        }

        cursorX += (real32)glyph.xAdvance * scale;
        if (cursorX - x > dimensions.width)
        {
            dimensions.width = cursorX - x;
        }
    }
}

extern "C" __declspec(dllexport) GAME_UPDATE_AND_RENDER(gameUpdateAndRender)
{
    gPlatform = platform;
    GameState* state = gState = (GameState*)memory->permanentStorage;
    if (!memory->isInitialized)
    {
        state->sound.samplingRadians = TWO_PI / 48000.0f;
        state->sound.setFrequency(440); // Middle C I think?

        state->player.textureId = LoadPNG("character.png");
        state->monster.textureId = LoadPNG("monster.png");

        state->mainFontId = LoadTTF("fonts/Inconsolata-Regular.ttf", 32.0f);
        state->currentDemo = DEMO_SELECT;

        memory->isInitialized = true;
    }

    state->camera.screenSize = { renderer->screenWidth, renderer->screenHeight };
    state->camera.aspect = state->camera.screenSize.x / state->camera.screenSize.y;

    // Set up our UI camera to scale to 1080 high, orthographic, no depth
    real32 virtualHeight = 1080.0f;
    state->camera.screenScale = virtualHeight / state->camera.screenSize.y;
    state->camera.size = {
        state->camera.screenScale * state->camera.screenSize.x,
        virtualHeight
    };

    platform->setProjection(state->camera.size.x, state->camera.size.y);
    state->frame.center = state->camera.size.x * 0.5f;
    state->frame.middle = state->camera.size.y * 0.5f;
    state->frame.left = 0;
    state->frame.right = state->camera.size.x;
    state->frame.top = state->camera.size.y;
    state->frame.bottom = 0;
    state->frame.width = state->camera.size.x;
    state->frame.height = state->camera.size.y;

    state->screenColor.r = input->controller.leftStick.x;
    state->screenColor.g = input->controller.leftStick.y;

    state->screenColor.b = input->controller.rightStick.x;

    platform->clearScreen(state->screenColor);

    if (state->currentDemo == DEMO_SELECT)
    {
        if (input->controller.cross.wasPressed())
        {
            state->currentDemo = (Demo)state->selectedDemo;
        }

        RenderText(state->mainFontId, "Demos", state->frame.center, state->frame.top - 10, { 0.5f, 1 }, white);

        Vector2 listStart = {state->frame.left + 20, state->frame.top - 50};
        char* menuItems[] = {
            "West World"
        };

        Vector2 largestText = {};
        for (int i = 0; i < NUM_DEMOS; ++i)
        {
            Vector2 textSize = MeasureText(state->mainFontId, menuItems[i]);
            if (largestText.x < textSize.x) largestText.x = textSize.x;
            if (largestText.y < textSize.y) largestText.y = textSize.y;
        }

        largestText.x += 20;
        largestText.y += 20;

        for (int i = 0; i < NUM_DEMOS; ++i)
        {
            if (i == state->selectedDemo)
            {
                DrawRectangle(listStart.x, listStart.y, largestText.x, largestText.y, blue, {0, 1}, white);
            }

            RenderText(state->mainFontId, menuItems[i], listStart.x + 10, listStart.y - 10, { 0, 1 }, white);

            listStart.y += largestText.y + 10;
        }
    }
    else if (state->currentDemo == DEMO_WESTWORLD)
    {
        WestworldUpdateAndRender(state, input);
    }
}

#include <cstring>

extern "C" __declspec(dllexport) GAME_OUTPUT_SOUND(gameOutputSound)
{
    GameState* state = (GameState*)memory->permanentStorage;
    // memset cause this is dummy code
    memset(output->samples, 0, output->sampleCount * output->bytesPerSample);

    if (!memory->isInitialized || (!state->sound.isPlaying && !state->sound.wasPlaying))
    {
        return;
    }

    real32 gain = 0;
    real32 desiredGain = 0;

    if (state->sound.isPlaying)
    {
        desiredGain = 1.0;
    }

    if (state->sound.wasPlaying)
    {
        gain = 1.0;
    }

    for (uint32 i = 0; i < output->sampleCount; i += 2)
    {
        real32 sampleGain = lerp(gain, desiredGain, (real32)i / (real32)output->sampleCount);
        real64 sampleValue = state->sound.nextSample() * sampleGain * 32767;
        output->samples[i] = output->samples[i + 1] = (int16)sampleValue;
    }

    if (!state->sound.isPlaying)
    {
        state->sound.phase = 0;
    }

    state->sound.wasPlaying = state->sound.isPlaying;
}

#include "westworld/westworld.cpp"
