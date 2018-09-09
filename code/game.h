#if !defined(GAME_H) //Include guard

/*
   NOTE:
   SLOW_MODE:
   = 1 allow to use assertions and other stuff that will slow code
   = 0 all slow code disabled
   INTERNAL_MODE:
   = 0 build designated for public release
   = 1 build designated for debug purposes only
   
*/

#define internal static
#define local_persist static
#define global_variable static


#define Pi32 3.14159265359f
#include <stdint.h>
#include <math.h>

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

//platform independent sizes of variables
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t memory_index; 

typedef  float real32;
typedef  double real64;

#if SLOW_MODE
#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

inline uint32
SafeTruncateUInt64(uint64 Value)
{
    // TODO: Define maximum values
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return Result;

}

struct thread_context
{
    int PlaceHolder;
};

#if INTERNAL_MODE
struct debug_read_file_result
{
    void *Contents;
    uint32 ContentsSize;
};


#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name( \
        thread_context *Thread, \
        char *Filename)

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, \
                                                        void *Memory)

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context *Thread, \
                                                           char *Filename, \
                                                           uint32 MemorySize, \
                                                           void *Memory)

typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))
#define Kilobytes(Value) (1024LL*(Value))
#define Megabytes(Value) (1024LL*Kilobytes(Value))
#define Gigabytes(Value) (1024LL*Megabytes(Value))

// FOUR THINGS - timing, controller/keyboard , bitmap buffer, sound buffer§é
struct game_offscreen_buffer
{
// NOTE: there was BytesPerPixel field and maybe it should go back at somepoint
// pixels are always 32bit wide and should be arranged like this |xx|BB|GG|RR|
// in memory bcoz of the LITTLE ENDIAN   
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct game_sound_output_buffer
{
    int SampleFreq;
    int SampleCount;
    int16* Samples;    
};


struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown ;
};

struct game_controller_input
{
    bool32 IsAnalog;
    bool32 IsConnected;
    
    real32 EndX;
    real32 EndY;

    real32 StartX;
    real32 StartY;

    real32 MinX;
    real32 MinY;

    real32 MaxX;
    real32 MaxY;
    union
    {
        game_button_state Buttons[11];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;
            
            game_button_state SpaceBar;
            
            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;
            
            game_button_state RightShoulder;
            game_button_state LeftShoulder;
            game_button_state Start;
            game_button_state Back;
            game_button_state LeftThumb;
            game_button_state RightThumb;
            
            game_button_state A;
            game_button_state B;
            game_button_state X;
            game_button_state Y;
        };
    };
};




struct game_input
{
    game_button_state MouseButtons[5];
    int32 MouseX;
    int32 MouseY;
    int32 MouseZ;

    
    game_controller_input Controllers[5];
    // TODO: Time that elapsed since last time checking
    
    real32 dtOverFrame;
};

inline game_controller_input *GetController(game_input *Input,
                                            int ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return(Result);
}

struct game_memory
{
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void *PermanentStorage; // NOTE: REQUIRED to be set to ZERO at a startup time
    
    uint64 TransientStorageSize;
    void *TransientStorage; // NOTE: REQUIRED to be set to ZERO at a startup time

    
    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
};




////////////////////////////////////////////////////////////////////////////////

// NOTE: Function prototype for GameUpdateAndRender
#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, \
                                               game_offscreen_buffer *Buffer, \
                                               game_input *Input)
// NOTE: Function prototype for GameGetSoundSamples
#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread,  \
                                               game_memory *Memory,     \
                                               game_sound_output_buffer *SoundBuffer)

typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#include "_intrinsics.h"
#include "game_tile.h"

struct memory_area
{
    memory_index Size;
    uint8 *Base;
    memory_index Used; 
    
};

struct world
{
    tile_map *TileMap;
};

struct game_state
{
    memory_area WorldArea;
    world *World;
    tile_map_position PlayerPosition;
};

#define GAME_H //include guard
#endif
