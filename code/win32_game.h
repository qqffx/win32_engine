#if !defined(WIN32_GAME_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   ======================================================================== */

struct win32_offscreen_buffer
{
    // NOTE: there was BytesPerPixel field and maybe it should go back at somepoint
    // pixels are always 32bit wide and should be arranged like this |xx|BB|GG|RR|
    // in memory bcoz of the LITTLE ENDIAN   
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

struct win32_sound_output
{
    // NOTE: Sound test
    DWORD SampleFreq;
    DWORD RunningSampleIndex;
    int BytesPerSample;
    DWORD SecondaryBufferSize;
    real32 tSine;
    int LatencySampleCount;
    DWORD SafetyBytes;
};

struct win32_debug_time_marker
{
    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
    LARGE_INTEGER FlipWallClock;
    DWORD ExpectedFlipPlayCursor;

    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;
    DWORD OutputLocation;
    DWORD OutputByteCount;    
};

struct win32_game_code
{
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;
    // IMPORTANT: Either of the callbacks can be 0 if dynamic linking not working
    // properly!! Check before 
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;
    // NOTE: if this is false both of the above functions is not loaded
    bool32 IsValid;
};

struct win32_recorded_input
{
    int InputCount;
    game_input *InputStream;
};

#define WIN32_STATE_FILENAME_COUNT MAX_PATH
struct win32_replay_buffer
{
    HANDLE FileHandle;
    HANDLE MemoryMap;
    char Filename[WIN32_STATE_FILENAME_COUNT];
    void *MemoryBlock;
};

struct win32_state
{
    uint64 TotalSize;
    void *GameMemoryBlock;
    win32_replay_buffer ReplayBuffers[2];
    
    HANDLE RecordingHandle;
    int InputRecordingIndex;

    HANDLE PlayBackHandle;
    int InputPlayingIndex;

    char EXEFilename[WIN32_STATE_FILENAME_COUNT];
    char *OnePastLastEXEFilenameSlash;
    
};

#define WIN32_GAME_H
#endif
