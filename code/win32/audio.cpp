#include "../game/platform.h"

// TODO: Starting with DirectSound cause thats the code I have, switch to more modern api (XAudio, CoreAudio?)
#include <dsound.h>

static bool soundIsValid = false;

static LPDIRECTSOUNDBUFFER directSoundOutputBuffer;
typedef HRESULT WINAPI DirectSoundCreateFn(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);

bool initDirectSound(HWND window, const GameAudio& audioFormat)
{
    // We use the version of direct sound that's installed on the system, rather than link directly, more compatibility
    HMODULE directSoundLib = LoadLibraryA("dsound.dll");
    if (!directSoundLib)
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to load dll\n");
        return false;
    }

    DirectSoundCreateFn* directSoundCreate = (DirectSoundCreateFn*)GetProcAddress(directSoundLib, "DirectSoundCreate");
    LPDIRECTSOUND directSound = 0;
    if (!directSoundCreate || !SUCCEEDED(directSoundCreate(0, &directSound, 0)))
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to create Primary Device object\n");
        return false;
    }

    // TODO: There are examples around of how to do device enumeration, which would just involve calling a bunch of this same code with
    // the guid of the selected device in our dsoundcreate function. Maybe look into that as a future feature

    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = (WORD)audioFormat.numChannels;
    waveFormat.wBitsPerSample = (WORD)(audioFormat.bytesPerSample * 8);
    waveFormat.nSamplesPerSec = audioFormat.samplesPerSecond;
    waveFormat.nBlockAlign = (WORD)(audioFormat.numChannels * audioFormat.bytesPerSample);
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

    // These chnages to the primary buffer shouldn't prevent playback as far as I understand.
    // They just set priority and prevent resampling that would cause performace issues if formats didn't match
    if (!SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to set cooperative level\n");
        return false;
    }

    DSBUFFERDESC bufferDesc = {};
    bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    bufferDesc.dwSize = sizeof(DSBUFFERDESC);

    LPDIRECTSOUNDBUFFER primaryBuffer;
    if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, 0)))
    {
        if (!SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
        {
            OutputDebugStringA("[Audio::DirectSound] Failed to set primary buffer format\n");
            return false;
        }
    }
    else
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to create primary buffer\n");
        return false;
    }

    DSBUFFERDESC secondaryBufferDesc = {};
    secondaryBufferDesc.dwSize = sizeof(DSBUFFERDESC);
    secondaryBufferDesc.dwBufferBytes = audioFormat.bufferSize;
    secondaryBufferDesc.lpwfxFormat = &waveFormat;

    if (!SUCCEEDED(directSound->CreateSoundBuffer(&secondaryBufferDesc, &directSoundOutputBuffer, 0)))
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to create secondary buffer\n");
        return false;
    }

    // We need to clear the audio buffer so we don't get any weird sounds until we want sounds
    void* region1;
    DWORD region1Size;
    void* region2;
    DWORD region2Size;

    if (SUCCEEDED(directSoundOutputBuffer->Lock(0, audioFormat.bufferSize,
        &region1, &region1Size, &region2, &region2Size, 0)))
    {
        uint8* currentSample = (uint8*)region1;
        for (DWORD i = 0; i < region1Size; ++i)
        {
            *currentSample++ = 0;
        }

        // Realistically we're asking for the whole buffer so the first loop will catch everything
        // This is just here for converage/safety's sake
        currentSample = (uint8*)region2;
        for (DWORD i = 0; i < region2Size; ++i)
        {
            *currentSample++ = 0;
        }

        directSoundOutputBuffer->Unlock(region1, region1Size, region2, region2Size);
    }

    // This starts the background process and has to be called so we can stream into it later
    directSoundOutputBuffer->Play(0, 0, DSBPLAY_LOOPING);
    return true;
}

void fillDirectSoundBuffer(GameAudio* output, DWORD startPosition, DWORD size)
{
    void* region1;
    DWORD region1Size;
    void* region2;
    DWORD region2Size;

    if (SUCCEEDED(directSoundOutputBuffer->Lock(startPosition, size,
        &region1, &region1Size, &region2, &region2Size, 0)))
    {
        int16* sourceSample = output->samples;
        int16* currentSample = (int16*)region1;
        DWORD region1SampleCount = region1Size / (output->bytesPerSample * output->numChannels);
        for (DWORD i = 0; i < region1SampleCount; ++i)
        {
            *(currentSample++) = *(sourceSample++); //LEFT channel
            *(currentSample++) = *(sourceSample++); //RIGHT channel
            ++(output->sampleIndex);
        }

        currentSample = (int16*)region2;
        DWORD region2SampleCount = region2Size / (output->bytesPerSample * output->numChannels);
        for (DWORD i = 0; i < region2SampleCount; ++i)
        {
            *(currentSample++) = *(sourceSample++); //LEFT channel
            *(currentSample++) = *(sourceSample++); //RIGHT channel
            ++(output->sampleIndex);
        }

        directSoundOutputBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

void updateAudio(GameMemory* gameMemory, GameAudio* output, GameOutputSoundFn* getGameSamples)
{
    DWORD playCursor = 0;
    DWORD writeCursor = 0;
    if (directSoundOutputBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
    {
        if (!soundIsValid)
        {
            output->sampleIndex = writeCursor / (output->bytesPerSample * output->numChannels);
            soundIsValid = true;
        }

        DWORD writeEndByte = writeCursor + output->bytesPerFrame + output->padding;
        writeEndByte %= output->bufferSize;

        DWORD bytesToWrite = 0;
        DWORD byteToLock = (output->sampleIndex * output->bytesPerSample * output->numChannels) % output->bufferSize;
        if (byteToLock > writeEndByte)
        {
            bytesToWrite = (output->bufferSize - byteToLock) + writeEndByte;
        }
        else
        {
            bytesToWrite = writeEndByte - byteToLock;
        }

        // Our current "Game Audio"
        if (getGameSamples)
        {
            output->sampleCount = bytesToWrite / output->bytesPerSample;
            getGameSamples(gameMemory, output);
        }

        fillDirectSoundBuffer(output, byteToLock, bytesToWrite);
    }
    else
    {
        soundIsValid = false;
    }
}
