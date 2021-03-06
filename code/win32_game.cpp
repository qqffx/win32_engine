// 2018 January

// NOTE: THis is a platform dependent win32 code with game logics injections,
// if u want to port this game on other platform u should rewrite only
// the platform code
#include "game.h"

#include <windows.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "win32_game.h"
//NOTE: this is the flag that stop the programm
global_variable bool32 GlobalRunning;
//NOTE: global struct that contains all what we need to draw stuff
global_variable win32_offscreen_buffer GlobalBackBuffer;
//NOTE: global ptr to the system given sound buffer
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
//NOTE: this is for timing purposes throughout the whole program
global_variable int64 GlobalCounterFrequency;
// NOTE:
global_variable bool32 GlobalPause;

//-----------------------------------------Utility-------------------------------
internal void
CatStrings(size_t SourceACount, char *SourceA,
                size_t SourceBCount, char *SourceB,
                size_t DestCount, char *Dest)
{
    for(int Index = 0; Index < SourceACount; Index++)
    {
                *Dest++ = *SourceA++;
    }    
    for(int Index = 0; Index < SourceBCount; Index++)
    {
        *Dest++ = *SourceB++;
    }
    *Dest++ = 0;        
}

internal void
Win32GetEXEFilename(win32_state *State)
{
    DWORD SizeOfPathname = GetModuleFileNameA(0, State->EXEFilename,
                                              sizeof(State->EXEFilename));
    State->OnePastLastEXEFilenameSlash = State->EXEFilename;
    for(char *Scan = State->EXEFilename; *Scan; Scan++)
    {
        if(*Scan == '\\')
        {
            State->OnePastLastEXEFilenameSlash = Scan + 1;
        }
    }
}

internal int
StringLength(char *String)
{
    int Count = 0;
    while(*String++)
        Count++;
    return(Count);    
}

internal void
Win32BuildEXEPathFilename(win32_state *State, char *Filename,
                          int DestCount, char *Dest)
{
    CatStrings(State->OnePastLastEXEFilenameSlash - State->EXEFilename,
               State->EXEFilename,
               StringLength(Filename), Filename,
               DestCount, Dest);
}
//-------------------------------------------------------------------------------

//-------------------------Audio Debug Visualisation-----------------------------
internal void
Win32DebugDrawVertical(win32_offscreen_buffer *BackBuffer,
                       int  X, int Top, int Bottom, uint32 Color)
{
    if(Top < 0)
    {
        Top = 0;
    }
    if(Bottom >= BackBuffer->Height)
    {
        Bottom = BackBuffer->Height; // NOTE: Y < Bottom in next for loop
    }
    if(X >= 0 && X < BackBuffer->Width)
    {
    
        uint8 *Pixel = ((uint8 *)BackBuffer->Memory +
                        X*BackBuffer->BytesPerPixel +
                        Top*BackBuffer->Pitch);
    
        for(int Y = Top; Y < Bottom; Y++)
        {
            *(uint32 *)Pixel = Color ;
            Pixel += BackBuffer->Pitch;
        }
    }
}

internal void
Win32DrawSoundBufferMarker(win32_offscreen_buffer *BackBuffer,
                           win32_sound_output *SoundOutput, DWORD Value,
                           int PadX, uint32 Color, int Top, int Bottom, real32 C)
{
    real32 XReal32PC = (C * (real32)Value);
    int X = PadX + (int)XReal32PC;
    Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
}


internal void
Win32DebugSyncDisplay(win32_offscreen_buffer *BackBuffer,
                      int MarkerCount,
                      win32_debug_time_marker *Markers,
                      int CurrentMarkerIndex,
                      win32_sound_output *SoundOutput,
                      real32 TargetSecondsPerFrame)
{
    int SecondaryBufferSize = SoundOutput->SecondaryBufferSize;
    int Width = BackBuffer->Width;

    int PadX = 16;
    int PadY = 16;
    int LineHeight = 64;
    
    real32 C = (real32)( (Width - PadX*2) / (real32)SecondaryBufferSize);
    
    for(int MarkerIndex = 0; MarkerIndex < MarkerCount;
        MarkerIndex++)
    {
        
        win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
        Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);

        DWORD PlayColor = 0xFFFFFFFF;
        DWORD WriteColor = 0xFFFF0000;
        DWORD ExpectedFlipColor = 0x00000000;

        int Bottom = PadY + LineHeight;
        int Top = PadY;
        
        if(MarkerIndex == CurrentMarkerIndex)
        {
            Top += Bottom;
            Bottom += Bottom;
            PlayColor = 0xFFFFFFFF;
            WriteColor = 0xFFFF0000;
            int FirstTop = Top;

            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput,
                                       ThisMarker->OutputPlayCursor,
                                       PadX, PlayColor, Top, Bottom, C);
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput,
                                       ThisMarker->OutputWriteCursor,
                                       PadX, WriteColor, Top, Bottom, C);
            

            Top += PadY + LineHeight;
            Bottom += PadY + LineHeight;
            
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput,
                                       ThisMarker->OutputLocation,
                                       PadX, PlayColor, Top, Bottom, C);
            // NOTE: There is summation here for suitability
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput,
                                       ThisMarker->OutputByteCount +
                                       ThisMarker->OutputLocation,
                                       PadX, WriteColor, Top, Bottom, C);

            Top += PadY + LineHeight;
            Bottom += PadY + LineHeight;

            /* Win32DrawSoundBufferMarker(BackBuffer, SoundOutput,
               ThisMarker->ExpectedFlipPlayCursor - 1920/2,
               PadX, ExpectedFlipColor, FirstTop, Bottom, C);
            
               Win32DrawSoundBufferMarker(BackBuffer, SoundOutput,
               ThisMarker->ExpectedFlipPlayCursor + 1920/2,
               PadX, ExpectedFlipColor, FirstTop, Bottom, C);*/
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput,
                                       ThisMarker->ExpectedFlipPlayCursor,
                                       PadX, ExpectedFlipColor, FirstTop, Bottom, C);
            
        }

        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput,
                                   ThisMarker->FlipPlayCursor,
                                   PadX, PlayColor, Top, Bottom, C);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput,
                                   ThisMarker->FlipWriteCursor,
                                   PadX, WriteColor, Top, Bottom, C);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput,
                                   ThisMarker->FlipPlayCursor + 1920,
                                   PadX, 0x0000FFFF, Top, Bottom, C);
    }
    
}
//-------------------------------------------------------------------------------

//--------------------DEBUG FILE IO----------------------------------------------
DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}


DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};
    
    HANDLE FileHandle =  CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0,
                                     OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize; 
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0)
                   && (FileSize32 == BytesRead))
                {
                    // NOTE: Read Succesfully
                    Result.ContentsSize = FileSize32;
                }
                else
                {
                    // TODO: Logging
                    DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
                    Result = {};
                }
                
            }
            else
            {
                //TODO: Logging
            }
        }
        else
        {
            // TODO: Logging
        }
        CloseHandle(FileHandle);
            
    }
    else
    {
        // TODO: Logging
    }
    
    return(Result);
}


DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 Result = false;
    HANDLE FileHandle =  CreateFileA(Filename, GENERIC_WRITE, 0, 0,
                                     CREATE_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            // NOTE: Read Succesfully
            Result = (BytesWritten == MemorySize);
        }
        else
        {
            // TODO: Logging
        }
        CloseHandle(FileHandle);
    }
    else
    {
        // TODO: Logging Handle error
    }

    return(Result);
}
//-------------------------------------------------------------------------------

// -------------loading windows functions by myself------------------------------

// fof navigation facilitation
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(  \
        DWORD dwUserIndex,XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(          \
        DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)

// create new type of function that will be used as ptr to function of this type
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

// dummy functions
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}

// getting ptr to the function of declared type
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

// actually snippet for code integrity
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void)
{
    // TODO: log what version is in use
    HMODULE XInputLibrary =  LoadLibraryA("Xinput1_4.dll");
    if(!XInputLibrary)
    {
        // TODO: log what version
        XInputLibrary = LoadLibraryA("Xinput1_3.dll");
    }
    if(XInputLibrary)
    {
        // NOTE: don't forget that this is XInputGetState_ in debugmode
        XInputGetState = (x_input_get_state *)GetProcAddress(
            XInputLibrary, "XInputGetState");
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}
        
        XInputSetState = (x_input_set_state *)GetProcAddress(
            XInputLibrary,"XInputSetState");
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;};
    }
    else
    {
        // TODO: Diagnostic
    }
}

// we will be using this call just once so we don't need any dummy functions
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(                  \
        LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32InitDSound (HWND Windows,
                               int32 SamplesPerSecond, int32 BufferSize)
{
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if(DSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)
            GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        
        LPDIRECTSOUND DirectSound;
        // NOTE: SUCCEEDED is another WS macro for error cheking
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            // Struct that describe sound format
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;                    
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.cbSize = 0 ;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample)/8;
            WaveFormat.nAvgBytesPerSec = SamplesPerSecond * WaveFormat.nBlockAlign;
            
            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Windows, DSSCL_PRIORITY)))
            {
                // Initialize and set other fields to zero
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                                                  
                // TODO: DSBCAPS_GLOBALFOCUS?                                   
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription,
                                                            &PrimaryBuffer, 0)))
                {
                    HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat); 
                    if(SUCCEEDED(Error))
                    {
                        OutputDebugStringA("PrimaryBuffer: format was set.\n");
                    }
                    else
                    {
                        // TODO: Diagnostic
                    }
                }
                else
                {
                    // TODO: Diagnostic
                }
                
            }
            else
            {
                // TODO: Diagnostic
            }

            // TODO: DSBCAPS_PRIMARYBUFFER?
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
       

            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription,
                                                           &GlobalSecondaryBuffer, 0); 
            if(SUCCEEDED(Error))
            {
                OutputDebugStringA("SecondaryBuffer: Created successfully.\n");
            }
            else
            {
                // TODO: Diagnostic
            }
        
        }
        else
        {
            // TODO: Diagnostic
        }
    }
    else
    {
        // TODO: Diagnostic
    }

}

// ------------------------------------------------------------------------------

//----------------------Dynamically loading code---------------------------------
inline FILETIME
Win32GetLastWriteTime(char* Filename)
{
    FILETIME LastWriteTime = {};
    WIN32_FILE_ATTRIBUTE_DATA Data;
    //NOTE: GetFileExInfoStandart some predefined shit in windows.h I suppose
    if(GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;        
    }
    return(LastWriteTime);
}

// NOTE: there game code loadings dynamically from .dll
internal win32_game_code
Win32LoadGameCode(char *SourceDLLName, char *TempDLLName)
{
    win32_game_code Result = {};
    
    Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

    CopyFileA(SourceDLLName, TempDLLName, FALSE);
    Result.GameCodeDLL =  LoadLibraryA(TempDLLName);
    if(Result.GameCodeDLL)
    {
        Result.UpdateAndRender = (game_update_and_render *)
            GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
        
        Result.GetSoundSamples = (game_get_sound_samples *)
            GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");

        Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
    }
    if(!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
        Result.GetSoundSamples = 0;
    }
    return(Result);
}

internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
    if(GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }
    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
    GameCode->GetSoundSamples = 0;
}
//-------------------------------------------------------------------------------

//------------------Screen output------------------------------------------------
internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT ClientRect;
    
    // NOTE: get the size of area of window that available for painting
    GetClientRect(Window, &ClientRect);    
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}


// DIB - device independent bitmap
internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer,
                                    int Width, int Height)
{
    
    // TODO: bulletproof this maybe free first get after is not the
    // best idea, and get first and free after if get fails

    // we want free our memory before allocate new one

    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE) ;
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    int BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;
 
    // biHeight is negative for puprpose to have bitmap that top-down,
    // not bottom-up
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    // other field already 0 bcoz of global_variable(static)
    
    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize,
                                  MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Width*BytesPerPixel;
}


// TODO: check the actual perfomance in the real build
internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                                         HDC DeviceContext,
                                         int WindowWidth, int WindowHeight)
{
    int OffsetX = 10;
    int OffsetY = 10;
    
    PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
    PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
    PatBlt(DeviceContext, 0, Buffer->Height+OffsetY, WindowWidth, WindowHeight, BLACKNESS);
    PatBlt(DeviceContext, Buffer->Width+OffsetX, 0, WindowWidth, WindowHeight, BLACKNESS);

    // Display our bitmap in window and stretch it if it needed
    StretchDIBits(DeviceContext,
                  OffsetX, OffsetY, Buffer->Width, Buffer->Height,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}
//-------------------------------------------------------------------------------

//--------------------Audio output-----------------------------------------------
internal void Win32ClearBuffer(win32_sound_output *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;

    HRESULT HERROR = GlobalSecondaryBuffer->Lock(0,
                                                 SoundOutput->SecondaryBufferSize,
                                                 &Region1, &Region1Size,
                                                 &Region2, &Region2Size,
                                                 0);
    // NOTE: Lock Check
    if(SUCCEEDED(HERROR))
    {
        uint8  *DestSample = (uint8 *)Region1;
        for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++)
        {
            *DestSample++ = 0;
        }
        
        DestSample = (uint8 *)Region2;
        for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++)
        {
            *DestSample++ = 0;
        }
        
        GlobalSecondaryBuffer->Unlock(
            Region1, Region1Size,
            Region2, Region2Size);
    }
}


internal void Win32FillSoundBuffer(
    win32_sound_output *SoundOutput,
    DWORD ByteToLock, DWORD BytesToWrite,
    game_sound_output_buffer *SoundBuffer)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;

    // NOTE:
    // Buffer is a interleaved int16 chunks where each chunk is one
    // |sample| for the one channel and the [block] is one sample for
    // each channel:
    //         [ |int16| |int16| ] [ |int16| |int16| ] ...
    // Buffer: [ |LEFT | |RIGHT| ] [ |LEFT | |RIGHT| ] ...
    //

    // LOCK
    HRESULT HERROR = GlobalSecondaryBuffer->Lock(ByteToLock,
                                                 BytesToWrite,
                                                 &Region1, &Region1Size,
                                                 &Region2, &Region2Size,
                                                 0);
    // NOTE: Lock Check
    if(SUCCEEDED(HERROR))
    {
        // TODO: Assert that RegionSize's is valid
        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        int16 *DestSample = (int16 *)Region1;
        int16 *SourceSample = SoundBuffer->Samples;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount;
             ++SampleIndex)
        {
            *DestSample++ = *SourceSample++; //LEFT
            *DestSample++ = *SourceSample++; //RIGHT
            SoundOutput->RunningSampleIndex++;
        }
                        
        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        DestSample = (int16 *)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount;
             ++SampleIndex)
        {
            *DestSample++ = *SourceSample++; //LEFT
            *DestSample++ = *SourceSample++; //RIGHT
            SoundOutput->RunningSampleIndex++;
        }
        //UNLOCK
        GlobalSecondaryBuffer->Unlock(
            Region1, Region1Size,
            Region2, Region2Size);
    }
}
//-------------------------------------------------------------------------------

//-------------------------Game Input Looping------------------------------------
internal void
Win32GetInputFileLocation(win32_state *Win32State, bool32 InputStream,  int SlotIndex, int DestCount,
                          char *Dest)
{
    Assert(SlotIndex <= ArrayCount(Win32State->ReplayBuffers));
    char Temp[64];
    wsprintf(Temp, "recordedInput_%d_%s.gi", SlotIndex , InputStream ? "input" : "state");
    Win32BuildEXEPathFilename(Win32State, Temp, DestCount, Dest);
}

internal win32_replay_buffer *
Win32GetReplayBuffer(win32_state *Win32State, int unsigned Index)
{
    Assert(Index < ArrayCount(Win32State->ReplayBuffers));
    win32_replay_buffer *Result = &Win32State->ReplayBuffers[Index];
    return(Result);
}

internal void
Win32BeginRecordingInput(win32_state *Win32State, int InputRecordingIndex)
{
    win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(Win32State,
                                                             InputRecordingIndex);
    if(ReplayBuffer->MemoryBlock)
    {
        Win32State->InputRecordingIndex = InputRecordingIndex;
        char Filename[WIN32_STATE_FILENAME_COUNT];
        // NOTE: (InputRecordingIndex - 1) ->  bcoz we starting numeration from 1
        // and using InputRecordingIndex as flag 
        Win32GetInputFileLocation(Win32State, true, (InputRecordingIndex - 1),
                                  sizeof(Filename), Filename);
        
        Win32State->RecordingHandle = CreateFileA(
                    Filename, GENERIC_WRITE,
                    0, 0, CREATE_ALWAYS, 0, 0);
#if 0
        LARGE_INTEGER FilePosition;
        FilePosition.QuadPart = Win32State->TotalSize; 
        SetFilePointerEx(Win32State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif
        CopyMemory(ReplayBuffer->MemoryBlock, Win32State->GameMemoryBlock,
                   Win32State->TotalSize);
    }
}

internal void
Win32EndRecordingInput(win32_state *Win32State)
{
    CloseHandle(Win32State->RecordingHandle);
    Win32State->InputRecordingIndex = 0;
}

internal void
Win32BeginInputPlayBack(win32_state *Win32State, int InputPlayingIndex)
{
    
    win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(Win32State,
                                                             InputPlayingIndex);
    if(ReplayBuffer->MemoryBlock)
    {
        Win32State->InputPlayingIndex = InputPlayingIndex;
        
        char Filename[WIN32_STATE_FILENAME_COUNT];
        Win32GetInputFileLocation(Win32State, true, (InputPlayingIndex - 1),
                                  sizeof(Filename), Filename);
        
        Win32State->PlayBackHandle = CreateFileA(
            Filename, GENERIC_READ,
            0, 0, OPEN_EXISTING, 0, 0);
#if 0
        LARGE_INTEGER FilePosition;
        FilePosition.QuadPart = Win32State->TotalSize; 
        SetFilePointerEx(Win32State->PlayBackHandle, FilePosition, 0, FILE_BEGIN);
#endif        
        CopyMemory(Win32State->GameMemoryBlock, ReplayBuffer->MemoryBlock,
                   Win32State->TotalSize);
    }
}

internal void
Win32EndInputPlayBack(win32_state *Win32State)
{
    CloseHandle(Win32State->PlayBackHandle);
    Win32State->InputPlayingIndex = 0;
}

internal void
Win32RecordInput(win32_state *Win32State, game_input *NewInput)
{
    DWORD BytesWritten;
    WriteFile(Win32State->RecordingHandle, NewInput,
              sizeof(*NewInput), &BytesWritten, 0);
}

internal void
Win32PlayBackInput(win32_state *Win32State, game_input *NewInput)
{
    DWORD BytesRead = 0;
    if(ReadFile(Win32State->PlayBackHandle, NewInput,
                sizeof(*NewInput), &BytesRead, 0))
    {
        if(BytesRead == 0)
        {
            // NOTE: we hit the end of the stream or we catch an error
            // anyway go start from the beginning
            int PlayingIndex = Win32State->InputPlayingIndex;
            Win32EndInputPlayBack(Win32State);
            Win32BeginInputPlayBack(Win32State, PlayingIndex);
            ReadFile(Win32State->PlayBackHandle, NewInput,
                     sizeof(*NewInput), &BytesRead, 0);
        }
    }

}
//-------------------------------------------------------------------------------

//-------------------------Input Processing--------------------------------------
internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
                                game_button_state *OldState,
                                game_button_state *NewState,
                                DWORD ButtonBit)
{
    // WARNING: Maybe += not =
    NewState->HalfTransitionCount =
        (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
    NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
}

internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void
Win32ProcessPendingMessages(win32_state *Win32State, game_controller_input *KeyboardController, game_input *Input)
{
    MSG Message;                    
    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        // NOTE: workaround keyboard messages in main loop instead of handling
        // them in winCallback
        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            }break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYUP:
            case WM_KEYDOWN:
            {
                uint32 VKcode = (uint32)Message.wParam;
                bool32 WasDown = (((Message.lParam >> 30) & 1) != 0);
                bool32 IsDown = (((Message.lParam >> 31) & 1) == 0);
                if(WasDown != IsDown)
                {
                    if(VKcode == 'W')
                    {
                        Win32ProcessKeyboardMessage(
                            &KeyboardController->MoveUp,
                            IsDown);
                    }
                    else if(VKcode == 'A')
                    {
                        Win32ProcessKeyboardMessage(
                            &KeyboardController->MoveLeft,
                            IsDown);
                    }
                    else if(VKcode == 'S')
                    {
                        Win32ProcessKeyboardMessage(
                            &KeyboardController->MoveDown,
                            IsDown);
                    }
                    else if(VKcode == 'D')
                    {
                        Win32ProcessKeyboardMessage(
                            &KeyboardController->MoveRight,
                            IsDown);
                    }
                    else if(VKcode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(
                            &KeyboardController->LeftShoulder,
                            IsDown);
                    }
                    else if(VKcode == 'E')
                    {
                        Win32ProcessKeyboardMessage(
                            &KeyboardController->RightShoulder,
                            IsDown);
                    }
                    else if(VKcode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(
                            &KeyboardController->ActionUp,
                            IsDown);
                    }
                    else if(VKcode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(
                            &KeyboardController->ActionDown,
                            IsDown);
                    }
                    else if(VKcode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(
                            &KeyboardController->ActionLeft,
                            IsDown);
                    }
                    else if(VKcode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(
                            &KeyboardController->ActionRight,
                            IsDown);
                    }
                    else if(VKcode == VK_ESCAPE)
                    {
                        GlobalRunning = false;
                    }
                    else if(VKcode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(
                            &KeyboardController->SpaceBar,
                            IsDown);
                    }
#if INTERNAL_MODE
                    else if(VKcode == 'P')
                    {
                        if(IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }
                    else if(VKcode == 'L')
                    {
                        if(IsDown)
                        {
                            if(Win32State->InputPlayingIndex == 0)
                            {
                                if(Win32State->InputRecordingIndex == 0)
                                {
                                    Win32BeginRecordingInput(Win32State, 1);
                                }
                                else
                                {
                                    Win32EndRecordingInput(Win32State);
                                    Win32BeginInputPlayBack(Win32State, 1);
                                }
                            }
                            else
                            {
                                Win32EndInputPlayBack(Win32State);
                                
                            }
                            
                            
                        }
                    }
#endif
                }
                bool32 AltKeyWasDown = (((Message.lParam >> 29) & 1) != 0);
                if (AltKeyWasDown && VKcode == VK_F4)
                {
                    // NOTE: This is a really necessary feachure
                    GlobalRunning = false;
                }
            } break;
            default:
            {
                //Translates virtual key messages to character messages and
                //put them into thread message queue
                TranslateMessage(&Message);
                //send message to window procedure
                DispatchMessageA(&Message);
                                
            }break;
        }
    }
}
//-------------------------------------------------------------------------------

//---------------------Timing----------------------------------------------------

inline LARGE_INTEGER Win32GetWallClockValue() 
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return(Result);
                
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = (real32)(End.QuadPart - Start.QuadPart) /
        (real32)GlobalCounterFrequency;
    return(Result);
}

//-------------------------------------------------------------------------------

//Windows Mandatory Callbacl for processing pending messages from our window and
//OS to us
internal LRESULT CALLBACK
Win32MainWindowCallback(HWND   Window,
                        UINT   Message,
                        WPARAM wParam,
                        LPARAM lParam)
{
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_SIZE:
        {
            OutputDebugStringA("WM_SIZE\n");
        } break;
        
        case WM_DESTROY:
        {
            GlobalRunning = false;
            OutputDebugStringA("WM_DESTROY\n");
        } break;

        case WM_CLOSE:
        {
            GlobalRunning = false;
            OutputDebugStringA("WM_CLOSE\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYUP:
        case WM_KEYDOWN:
        {

            Assert(!"KeyEvents passed through processing in main loop");
        }break;

        case WM_ACTIVATEAPP:
        {
            if(wParam == TRUE)
            {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA);
            }
            else
            {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 64, LWA_ALPHA);
            }
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            OutputDebugStringA("WM_PAINT\n");
            // take a struct that contain information for painting in
            // client area of owned window
            HDC DeviceContext = BeginPaint(Window, &Paint);
                
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                                       Dimension.Width, Dimension.Height);

            // close window to paint
            EndPaint(Window, &Paint);
        } break;
        
        default:
        {
//              OutputDebugStringA("default_case\n");
//              Process the window messages we don't want to process manually
            Result = DefWindowProcA(Window, Message, wParam, lParam);
        } break;
    }
    
    return(Result);
}


// ENTRY POINT
int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    LARGE_INTEGER CounterFrequencyResult;
    QueryPerformanceFrequency(&CounterFrequencyResult);
    GlobalCounterFrequency = CounterFrequencyResult.QuadPart;
    
    // NOTE: Struct that cointan service information about the game
    win32_state Win32State = {};
    
#if INTERNAL_MODE

    Win32GetEXEFilename(&Win32State);
    
    char SourceGameCodeDLLFullPath[WIN32_STATE_FILENAME_COUNT];
    Win32BuildEXEPathFilename(&Win32State, "game.dll",
                              sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);
    
    char TempGameCodeDLLFullPath[MAX_PATH];
    Win32BuildEXEPathFilename(&Win32State, "game_temp.dll",
                              sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);
    
#endif
    
    // NOTE: in Milliseconds, Defined what resolution scheduler will be have
    // so that our sleep can be more accurate and we will not skip the fucking frame
    UINT DesiredSchedulerGranularityMs = 1; 
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerGranularityMs) == TIMERR_NOERROR);
    
    Win32LoadXInput();
    // allocate unique device context for each windows from this class
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);
    
    // mb do not really matter nowadays
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    // pointer that takes ptrToFunc on CALLBACK FUNCTION
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = hInstance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "TheGame_WindowClass";

    if(RegisterClass(&WindowClass))
    {
#if WIN32_TRANSPARENT_MODE
        HWND Window =
            CreateWindowEx(
                WS_EX_TOPMOST|WS_EX_LAYERED,
                WindowClass.lpszClassName,
                "Epylepsy_simulator",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                hInstance,
                0);
#else
        HWND Window =
            CreateWindowEx(
                0, 
                WindowClass.lpszClassName,
                "Epylepsy_simulator",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                hInstance,
                0);
#endif
        if(Window)
        {
            
            SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 128, LWA_ALPHA);

            //IMPORTANT: we are enforcing FPS of the game

            int MonitorRefreshHz = 60;
            HDC RefreshDC = GetDC(Window);
            int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if(Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            
            real32 GameUpdateHz = ( MonitorRefreshHz / 2.0f);
            real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

            win32_sound_output SoundOutput = {};
            SoundOutput.SampleFreq = 48000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.BytesPerSample = sizeof(int16)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SampleFreq*SoundOutput.BytesPerSample;
            // TODO: test it with precise value of jitter
            SoundOutput.SafetyBytes =
                (int)(((real32)SoundOutput.SampleFreq * (real32)SoundOutput.BytesPerSample) /
                      GameUpdateHz) / 3;
            //SoundOutput.SafetyBytes += 6400;

            Win32InitDSound(Window, SoundOutput.SampleFreq, SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);



            // NOTE: Running is a global variable that flags that signalize
            // that we want our window to be open
            GlobalRunning = true;
#if 0
            // NOTE: Audio granularity check DEVMACHINE: 480 samples 1920 bytes
            while(GlobalRunning)
            {
                
                DWORD PlayCursor;
                DWORD WriteCursor;
                GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

                
                char AudioDebugBuffer[256];
                _snprintf_s(AudioDebugBuffer, sizeof(AudioDebugBuffer),
                            "PC: %u | WC: %u \n",
                            PlayCursor, WriteCursor);
                OutputDebugStringA(AudioDebugBuffer);
            }
#endif            

            // TODO: pool with bitmap virtualAlloc
            // WARNING: MemAlloc be aware
            int16 * Samples = (int16 *)VirtualAlloc(0,
                                                    SoundOutput.SecondaryBufferSize,
                                                    MEM_RESERVE | MEM_COMMIT,
                                                    PAGE_READWRITE);

            // NOTE: Memory allocation (!!!!)  be aware my friend


            
#if INTERNAL_MODE
            LPVOID BaseAdress = (LPVOID)Gigabytes(1024*2);
#else
            LPVOID BaseAdress = 0;
#endif
            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes(1);
            GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
            
            Win32State.TotalSize = GameMemory.PermanentStorageSize +
                GameMemory.TransientStorageSize;
            // NOTE: we save pointer to our data block in Win32State
            Win32State.GameMemoryBlock = VirtualAlloc(
                BaseAdress, (size_t)Win32State.TotalSize,
                MEM_RESERVE | MEM_COMMIT,
                PAGE_READWRITE);
            
            GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
            GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage
                                           + GameMemory.PermanentStorageSize);

            for(int ReplayIndex = 0;
                ReplayIndex < ArrayCount(Win32State.ReplayBuffers); ReplayIndex++)
            {
                win32_replay_buffer *ReplayBuffer = &Win32State.ReplayBuffers[ReplayIndex];
                
                Win32GetInputFileLocation(&Win32State, false, ReplayIndex,
                                          sizeof(ReplayBuffer->Filename), ReplayBuffer->Filename);
                
                ReplayBuffer->FileHandle =  CreateFileA(
                    ReplayBuffer->Filename, GENERIC_WRITE | GENERIC_READ,
                    0, 0, CREATE_ALWAYS, 0, 0);

                // NOTE: exclusively for facilitating exctraction high and low part
                // for FileMapping
                LARGE_INTEGER MaxSize;
                MaxSize.QuadPart = Win32State.TotalSize;
                ReplayBuffer->MemoryMap = CreateFileMappingA(
                    ReplayBuffer->FileHandle, 0, PAGE_READWRITE,
                    MaxSize.HighPart,
                    MaxSize.LowPart,
                    0);
                DWORD Error = GetLastError();
                // NOTE: file-memory-map file is a file that will be updating
                // coherent with memory so they will correspond,
                // and it will do memory allocation implicitly
                ReplayBuffer->MemoryBlock = MapViewOfFile(
                    ReplayBuffer->MemoryMap, FILE_MAP_ALL_ACCESS, 0, 0,
                    Win32State.TotalSize);
                if(ReplayBuffer->MemoryBlock)
                {
                }
                else
                {
                    // TODO: change this to log isn't good when people can't run this
                    // machines with low memory
                }
            }

            

            if(Samples &&
               GameMemory.PermanentStorage &&
               GameMemory.TransientStorage)
            {
                // NOTE: If we are here we will never run out of memory
                // if after this point someone will fuck up memory alignation
                // or something, it will be OS not me

                bool32 SoundIsValid = false;
                DWORD AudioLatenceInBytes = 0;
                real32 AudioLatencyInSeconds = 0;

                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];
                // NOTE: setting the elementary period of time for updating
                NewInput->dtOverFrame = TargetSecondsPerFrame;
                OldInput->dtOverFrame = TargetSecondsPerFrame;

                int DebugTimeMarkersIndex = 0;
                win32_debug_time_marker DebugTimeMarkers[30] = {};
            
                // NOTE: Performance counter
                LARGE_INTEGER LastCounter = Win32GetWallClockValue();
                LARGE_INTEGER FlipWallClock = Win32GetWallClockValue();

                
                // NOTE: dynamically load game code
                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                         TempGameCodeDLLFullPath);
                uint32 LoadCounter = 0;
                
                uint64 LastCycleCount = __rdtsc();
                while(GlobalRunning)
                {
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                 TempGameCodeDLLFullPath);
                        LoadCounter = 0;
                    }
                    
                    game_controller_input *OldKeyboardController =
                        GetController(OldInput, 0);
                    game_controller_input *NewKeyboardController =
                        GetController(NewInput, 0);
                    
                    // TODO: Zeroing macro
                    *NewKeyboardController = {};
                    
                    NewKeyboardController->IsConnected = true;
                    
                    for(int ButtonIndex = 0;
                        ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                        ButtonIndex++)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }
                
                    Win32ProcessPendingMessages(&Win32State, NewKeyboardController, NewInput);
                    
                    if(!GlobalPause)
                    {
                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);
                        
                        NewInput->MouseX = MouseP.x;
                        NewInput->MouseY = MouseP.y;
                        NewInput->MouseZ = 0;
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0],
                                                    GetKeyState(VK_LBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1],
                                                    GetKeyState(VK_RBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2],
                                                    GetKeyState(VK_MBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3],
                                                    GetKeyState(VK_XBUTTON1) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4],
                                                    GetKeyState(VK_XBUTTON2) & (1 << 15));
                                                  
                        //TODO: mb we should poll controller more frequently
                        DWORD MaxControllerCount = XUSER_MAX_COUNT;
                        if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
                        {
                            MaxControllerCount = ArrayCount(NewInput->Controllers) - 1;
                        }

                        for(DWORD ControllerIndex = 0;
                            ControllerIndex < MaxControllerCount;
                            ++ControllerIndex)
                        {
                            DWORD OurControllerIndex = ControllerIndex + 1;
                            game_controller_input *OldController =
                                GetController(OldInput, OurControllerIndex);
                    
                            game_controller_input *NewController =
                                GetController(NewInput, OurControllerIndex);
                        
                            XINPUT_STATE ControllerState;
                            if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                            {
                                // NOTE: This controller exist and gave us his state

                                XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;


                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->ActionUp,
                                                                &NewController->ActionUp,
                                                                XINPUT_GAMEPAD_DPAD_UP);
                        
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->ActionDown,
                                                                &NewController->ActionDown,
                                                                XINPUT_GAMEPAD_DPAD_DOWN);
                        
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->ActionLeft,
                                                                &NewController->ActionLeft,
                                                                XINPUT_GAMEPAD_DPAD_LEFT);
                        
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->ActionRight,
                                                                &NewController->ActionRight,
                                                                XINPUT_GAMEPAD_DPAD_RIGHT);
                        
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->Start,
                                                                &NewController->Start,
                                                                XINPUT_GAMEPAD_START);
                        
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->Back,
                                                                &NewController->Back,
                                                                XINPUT_GAMEPAD_BACK);
                        
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->LeftThumb,
                                                                &NewController->LeftThumb,
                                                                XINPUT_GAMEPAD_LEFT_THUMB);
                        
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->RightThumb,
                                                                &NewController->RightThumb,
                                                                XINPUT_GAMEPAD_RIGHT_THUMB);
                        
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->LeftShoulder,
                                                                &NewController->LeftShoulder,
                                                                XINPUT_GAMEPAD_LEFT_SHOULDER);
                        
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->RightShoulder,
                                                                &NewController->RightShoulder,
                                                                XINPUT_GAMEPAD_RIGHT_SHOULDER);

                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->A,
                                                                &NewController->A,
                                                                XINPUT_GAMEPAD_A);

                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->B,
                                                                &NewController->B,
                                                                XINPUT_GAMEPAD_B);
                        
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->X,
                                                                &NewController->X,
                                                                XINPUT_GAMEPAD_X);
                        
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->Y,
                                                                &NewController->Y,
                                                                XINPUT_GAMEPAD_Y);

                                // Sticks XY position 
                                int16 LStickX = Pad->sThumbLX;
                                int16 LStickY = Pad->sThumbLX;
                                int16 RStickX = Pad->sThumbRX;
                                int16 RStickY = Pad->sThumbRX;
                        
                            }
                            else
                            {
                                // NOTE: This controller doesn't exist probably
                                // plugged out
                            }
                        }


                        // NOTE: Direct Sound output test
                    

                        // TODO: Tighten up sound logic so that we know whrere we should
                        // be writing to and can anticipate the time spent in the game update

                        // NOTE: Sound Buffer for reading data

                        thread_context Thread = {};                    
                
                        // NOTE: Video Buffer for drawing data
                        game_offscreen_buffer Buffer;
                        Buffer.Memory = GlobalBackBuffer.Memory;
                        Buffer.Width = GlobalBackBuffer.Width;
                        Buffer.Height = GlobalBackBuffer.Height;
                        Buffer.Pitch = GlobalBackBuffer.Pitch;
                        Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
                        
                        // NOTE: Game input looping 
                        if(Win32State.InputRecordingIndex)
                        {
                            Win32RecordInput(&Win32State, NewInput);
                        }
                        if(Win32State.InputPlayingIndex)
                        {
                            Win32PlayBackInput(&Win32State, NewInput);
                        }

                        if(Game.UpdateAndRender)
                        {
                            // NOTE: there is where we update our game state
                            Game.UpdateAndRender(&Thread, &GameMemory, &Buffer, NewInput);
                        }
                        DWORD WriteCursor = 0;
                        DWORD PlayCursor = 0;


                        LARGE_INTEGER AudioWallClock = Win32GetWallClockValue();
                        real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);
                        if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor,
                                                                     &WriteCursor) == DS_OK)
                        {
                            if(!SoundIsValid)
                            {
                                // NOTE: this is first time when we start our game
                                // we write where write cursor initially is
                                SoundOutput.RunningSampleIndex = WriteCursor /
                                    SoundOutput.BytesPerSample;
                                SoundIsValid = true;
                            }
                        
                            DWORD ByteToLock = ((SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) %
                                                SoundOutput.SecondaryBufferSize);

                            DWORD ExpectedSoundBytesPerFrame = 
                                (DWORD)(((real32)SoundOutput.SampleFreq * (real32)SoundOutput.BytesPerSample) /
                                        GameUpdateHz);
                        
                            // NOTE: Actually this is predicted location of PlayCursor
                            // in next frame boundaries
                            

                            real32 SecondsLeftUntilFlip =
                                (TargetSecondsPerFrame - FromBeginToAudioSeconds);
                            DWORD ExpectedBytesUntilFlip =
                                (DWORD)((SecondsLeftUntilFlip/TargetSecondsPerFrame)
                                        * (real32)ExpectedSoundBytesPerFrame);
                            
                            DWORD ExpectedFrameBoundaryByte =
                                PlayCursor + ExpectedBytesUntilFlip;
                            

                            /*
                              NOTE: this should neutralize our WriteCursor position
                              jitter by always assuming worst case scenario -->
                              --> WriteCursor + SafetyBytes;
                          
                              Where:
                              SafetyBytes  -> Jitter/2,
                              Jitter -> 1920 Bytes on DevMachine;
                            */
                            DWORD SafeWriteCursor = WriteCursor; 
                            if(SafeWriteCursor < PlayCursor)
                            {
                                SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            Assert(SafeWriteCursor >= PlayCursor);
                            SafeWriteCursor += SoundOutput.SafetyBytes;
                        
                            // NOTE: there are 2 modes in that we can operate low
                            // latency AudioCard and high latency
                            bool32 AudioCardIsNonLatent =
                                (SafeWriteCursor < ExpectedFrameBoundaryByte);
                        
                            DWORD TargetCursor;
                            if(AudioCardIsNonLatent)
                            {
                                TargetCursor =
                                    (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
                            }
                            else
                            {
                                TargetCursor =
                                    (WriteCursor + ExpectedSoundBytesPerFrame +
                                     SoundOutput.SafetyBytes);
                            }
                        
                            // NOTE: we wrap out TargetCursor in SceondaryBufferSize
                            // circular buffer
                            TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;

                            DWORD BytesToWrite = 0;
                            if(TargetCursor < ByteToLock)
                            {
                                BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
                                BytesToWrite += TargetCursor;
                            }
                            else
                            {
                                BytesToWrite = TargetCursor - ByteToLock;
                            }
                        
                            game_sound_output_buffer SoundBuffer = {};
                            SoundBuffer.SampleFreq = 48000;
                            SoundBuffer.SampleCount = BytesToWrite/SoundOutput.BytesPerSample;
                            SoundBuffer.Samples = Samples;
                            if(Game.GetSoundSamples)
                            {
                                Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
                            }
                            // TODO: we should write our samples from game code into
                            // sound buffer right here
                    
#if INTERNAL_MODE
                            // GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

                            win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkersIndex];
                            Marker->OutputPlayCursor = PlayCursor;
                            Marker->OutputWriteCursor = WriteCursor;
                            Marker->OutputLocation = ByteToLock;
                            Marker->OutputByteCount = BytesToWrite;
                            Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;
                        
                            DWORD UnwrappedWriteCursor = WriteCursor;
                            if(UnwrappedWriteCursor <  PlayCursor)
                            {
                                // NOTE: We basically compute where would be WriteCursor
                                // if it wouldn't be wrapperd as circular buffer
                                UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            AudioLatenceInBytes = UnwrappedWriteCursor - PlayCursor;
                        
                            AudioLatencyInSeconds =
                                (((real32)AudioLatenceInBytes / (real32)SoundOutput.BytesPerSample) /
                                 (real32)SoundOutput.SampleFreq);
                        
                            char AudioDebugBuffer[256];
                            _snprintf_s(AudioDebugBuffer, sizeof(AudioDebugBuffer),
                                        "BTL: %u TC: %u BTW: %u - PC: %u WC: %u DELTA: %u/%f \n \n",
                                        ByteToLock,
                                        TargetCursor, BytesToWrite, PlayCursor,
                                        WriteCursor, AudioLatenceInBytes,
                                        AudioLatencyInSeconds);

                            OutputDebugStringA(AudioDebugBuffer);
#endif
                            Win32FillSoundBuffer(&SoundOutput,
                                                 ByteToLock, BytesToWrite, &SoundBuffer);
                        }
                        else
                        {
                            SoundIsValid = false;
                        }

                        LARGE_INTEGER WorkCounter = Win32GetWallClockValue();
                        real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                        //WARNING: Dont ever think about to trust this part of code
                        // this shit will fucking stab you in the back asap
                        real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                        if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {

                            if(SleepIsGranular)
                            {
                                DWORD SleepMs;
                                SleepMs = (DWORD)(1000.0f *
                                                        (TargetSecondsPerFrame - SecondsElapsedForFrame));
                                if(SleepMs > 0)
                                {
                                    Sleep(SleepMs);
                                }
                            }
                            
                            real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(
                                LastCounter, Win32GetWallClockValue());


                            if(TestSecondsElapsedForFrame > TargetSecondsPerFrame)
                            {
                                Assert(TestSecondsElapsedForFrame > TargetSecondsPerFrame);
                            }
                        
                            while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedForFrame = Win32GetSecondsElapsed(
                                    LastCounter,
                                    Win32GetWallClockValue());
                            }
                        }
                        else
                        {
                            // WARNING: shit, we miss a frame, god almighty save us
                            // TODO: log this for ancestors
                        }

                        // NOTE: update the clocks
                        LARGE_INTEGER EndCounter = Win32GetWallClockValue();
                        real32 mSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
                        LastCounter = EndCounter;

                        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
// NOTE: This is debug only code                    
#if 0 
                        Win32DebugSyncDisplay(&GlobalBackBuffer,
                                              ArrayCount(DebugTimeMarkers),
                                              DebugTimeMarkers, DebugTimeMarkersIndex - 1,
                                              &SoundOutput,
                                              TargetSecondsPerFrame);
#endif

                       
                        HDC DeviceContext = GetDC(Window);
                        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                                                   Dimension.Width, Dimension.Height);
                        ReleaseDC(Window, DeviceContext);


                        FlipWallClock = Win32GetWallClockValue();
// NOTE: This is debug only code
#if INTERNAL_MODE
                        {
                            DWORD PlayCursor;
                            DWORD WriteCursor;
                            if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                            {
                                Assert(DebugTimeMarkersIndex < ArrayCount(DebugTimeMarkers));
                                win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkersIndex];

                                Marker->FlipWallClock = FlipWallClock;
                                Marker->FlipPlayCursor = PlayCursor;
                                Marker->FlipWriteCursor = WriteCursor;
                            }
                        }
#endif
                        // NOTE: update the cycles counter
                        uint64 EndCycleCount = __rdtsc();
                        uint64 CycleElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;

                        real32 FPS = 0.0;// ((real32)GlobalCounterFrequency/(real32)CounterElapsed);
                        real32 MegaCyclesPerFrame = (real32)CycleElapsed/(1000.0f*1000.0f);
                    
                        char FPSBuffer[256];
                        _snprintf_s(FPSBuffer, sizeof(FPSBuffer), "%f Ms/F %f FPS %f MegaCycles/F\n",
                                    mSPerFrame, FPS, MegaCyclesPerFrame );

                        OutputDebugStringA(FPSBuffer);

                        // NOTE: Controller swap
                        game_input *Temp = NewInput;
                        NewInput  = OldInput;
                        OldInput = Temp;
#if INTERNAL_MODE
                        DebugTimeMarkersIndex++;
                        DebugTimeMarkersIndex = DebugTimeMarkersIndex % ArrayCount(DebugTimeMarkers);
#endif
                    }
                }
            }
            else
            {
                // TODO: Logging errors with MemAlloc
            }
        
        }
        else
        {
            //logging procedure
        }
    }
    else
    {
        //logging procedure 
    }
    
    return(0);
} 

