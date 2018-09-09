#include "game.h"
#include "game_tile.cpp"
#include "game_random.h"


internal void
GameSoundOutput(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SampleFreq/ToneHz;
    
    int16 *SampleOut = SoundBuffer->Samples;    
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount;
         ++SampleIndex)
    {
#if 0
        real32 SineValue = sinf(GameState->tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
        int16 SampleValue = 0;
#endif
        *SampleOut++ = SampleValue; //LEFT
        *SampleOut++ = SampleValue; //RIGHT
#if 0
        GameState->tSine  += 2.0f*Pi32*1.0f / (real32)WavePeriod;
        if(GameState->tSine > 2.0f*Pi32)
        {
            GameState->tSine -= 2.0f*Pi32;
        }
#endif
    } 
}

/* NOTE: Convention:

   Pixel Grid:
   0 1 2 3 4 5
   _ _ _ _ _ _       
0 |_|_|_|_|_|_|   When we talking about fill from (MinX; MinY) to (MaxX; MaxY)
1 |_|_|_|_|_|_|   we mind that rectangle will be filled:
2 |_|x|x|x|x|_|   [1,2] -> (5,6) i.e 
3 |_|x|x|x|x|_|   inclusively for MinValues and exclusively for MaxValues
4 |_|x|x|x|x|_|   
5 |_|x|x|x|x|_|   
6 |_|_|_|_|_|_|   
7 |_|_|_|_|_|_|

*/

internal void
DrawRectangle(game_offscreen_buffer *Buffer,
              real32 RealMinX, real32 RealMinY,
              real32 RealMaxX, real32 RealMaxY,
              real32 R, real32 G, real32 B)
{
    int32  MinX = RoundReal32ToInt32(RealMinX);
    int32  MinY = RoundReal32ToInt32(RealMinY);
    int32  MaxX = RoundReal32ToInt32(RealMaxX);
    int32  MaxY = RoundReal32ToInt32(RealMaxY);

    uint32 Color = ((RoundReal32ToUInt32(255.0f * R) << 16) |
                    (RoundReal32ToUInt32(255.0f * G) << 8) | 
                    (RoundReal32ToUInt32(255.0f * B)));

    if(MinX < 0)
    {
        MinX = 0;
    }

    if(MinY < 0)
    {
        MinY = 0;
    }

    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }
    uint8 *EndOfBuffer = (uint8 *)Buffer->Memory +
        (Buffer->Height * Buffer->Width) * Buffer->BytesPerPixel;
    
    //uint32 Color = 0xFFFFFFFF;
    
    uint8 *Row = ((uint8 *)Buffer->Memory +
                  MinX*Buffer->BytesPerPixel +
                  MinY*Buffer->Pitch);
    for(int Y = MinY; Y < MaxY; Y++)
    {
        uint32 *Pixel = (uint32 *)Row; 
        for(int X = MinX; X < MaxX; X++)
        {
            *Pixel++ = Color;
        }
        Row += Buffer->Pitch;
    }
}


internal void
InitializeMemoryArea(memory_area *Area, memory_index Size, uint8 *Base)
{
    Area->Size = Size;
    Area->Base = Base;
    Area->Used = 0;
}

#define PushStruct(Area, Type) (Type *)PushSize_(Area, sizeof(Type))
#define PushArray(Area, Count, Type) (Type *)PushSize_(Area,(Count)* sizeof(Type))

void *
PushSize_(memory_area *Area, memory_index Size)
{
    Assert((Area->Used + Size) <= Area->Size);
    void *Result = Area->Base + Area->Used;
    Area->Used += Size;
    return(Result);
}


// NOTE: look at the macro in game.h
// NOTE: extern "C" means that we don't want any name mangling in map file
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized)
    {

        GameState->PlayerPosition.AbsTileX = 1;
        GameState->PlayerPosition.AbsTileY = 3;
        GameState->PlayerPosition.TileRelX = 0.0f;
        GameState->PlayerPosition.TileRelY = 0.0f;

        InitializeMemoryArea(&GameState->WorldArea,
                             Memory->PermanentStorageSize - sizeof(game_state),
                             (uint8 *)Memory->PermanentStorage + sizeof(game_state));
        
        GameState->World = PushStruct(&GameState->WorldArea, world);
        world *World = GameState->World;

        World->TileMap = PushStruct(&GameState->WorldArea, tile_map);

        tile_map *TileMap = World->TileMap;
        
        TileMap->TileSideInPixels = 5;
        TileMap->TileSideInMeters = 1.4f;
        TileMap->MetersToPixels = TileMap->TileSideInPixels/TileMap->TileSideInMeters;

        //NOTE: this is set for using in virtual tile memory system 16x16
        //(X->8bit, Y->8bit)
        TileMap->ChunkShift = 4;
        TileMap->ChunkMask = (0x1 << TileMap->ChunkShift) - 1;
        TileMap->ChunkDim = (1 << TileMap->ChunkShift);

        TileMap->TileChunkXDim = 64;
        TileMap->TileChunkYDim = 64;
        
        TileMap->TileChunks = PushArray(&GameState->WorldArea,
                                        TileMap->TileChunkXDim*TileMap->TileChunkYDim, tile_chunk);
        
        for(uint32 Y = 0; Y < TileMap->TileChunkYDim; ++Y)
        {
            for(uint32 X = 0; X < TileMap->TileChunkXDim; ++X)
            {
                TileMap->TileChunks[Y * TileMap->TileChunkXDim + X].Tiles =
                    PushArray(&GameState->WorldArea, TileMap->ChunkDim * TileMap->ChunkDim, uint32);
            }
        }

        uint32 TilesPerWidth = 17;
        uint32 TilesPerHeight = 9;

        
        
        uint32 RandomNumberIndex = 0;
        uint32 ScreenY = 0;
        uint32 ScreenX = 0;
        for(uint32 ScreenIndex = 0; ScreenIndex < 100; ++ScreenIndex)
        {
            for(uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
            {
                for(uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
                {
                    uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
                    uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;

                    uint32 TileValue = 1;
                    if(TileY == 0 || TileY == TilesPerHeight - 1)
                    {
                        if(TileX == TilesPerWidth/2)
                        {
                            TileValue = 1;
                        }
                        else
                        {
                            TileValue = 2;
                        }
                    }

                    if(TileX == 0 || TileX == TilesPerWidth - 1)
                    {
                        if(TileY == TilesPerHeight/2)
                        {
                            TileValue = 1;
                        }
                        else
                        {
                            TileValue = 2;
                        }
                    }
                            
                    SetTileValue(&GameState->WorldArea,World->TileMap, AbsTileX, AbsTileY,
                                 TileValue);
                }
            }

            Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
            uint32 RandomChoice =  RandomNumberTable[RandomNumberIndex++] % 2;
            if(RandomChoice == 0)
            {
                ScreenX += 1;
            }
            else
            {
                ScreenY += 1;
            }
            
        }
        Memory->IsInitialized = true;
    }

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;

    real32 PlayerR = 0.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 1.0f;

    real32 PlayerWidth = 0.75f*TileMap->TileSideInMeters;
    real32 PlayerHeight = (real32)TileMap->TileSideInMeters;

    real32 ScreenCenterY = 0.5f*Buffer->Height;
    real32 ScreenCenterX = 0.5f*Buffer->Width;
    
    for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers);
        ControllerIndex++)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if(Controller->IsAnalog)
        {
        }
        else
        {
            // TODO: bug with movement suppose culprit platform layer or bit
            // rounding
            real32 PlayerDeltaX = 0.0f;
            real32 PlayerDeltaY = 0.0f;
            if(Controller->MoveUp.EndedDown)
            {
                PlayerDeltaY += 1.0f;
            }
            if(Controller->MoveDown.EndedDown)
            {
                PlayerDeltaY -= 1.0f;
            }
            if(Controller->MoveLeft.EndedDown)
            {
                PlayerDeltaX -= 1.0f;
            }
            if(Controller->MoveRight.EndedDown)
            {
                PlayerDeltaX += 1.0f;
            }

            real32 PlayerSpeed = 2.0f;
            if(Controller->ActionUp.EndedDown)
            {
                PlayerSpeed = 50.0f;
            }

            PlayerDeltaX *= PlayerSpeed;
            PlayerDeltaY *= PlayerSpeed;

            // TODO: fix the diagonal movement (implement vectors)
            tile_map_position NewPlayerPosition = GameState->PlayerPosition;
            NewPlayerPosition.TileRelX += Input->dtOverFrame*PlayerDeltaX;
            NewPlayerPosition.TileRelY += Input->dtOverFrame*PlayerDeltaY;
            NewPlayerPosition = ReCanonicalizePosition(TileMap, NewPlayerPosition);
            
            tile_map_position PlayerLeft = NewPlayerPosition;
            PlayerLeft.TileRelX -= 0.5f*PlayerWidth;
            PlayerLeft = ReCanonicalizePosition(TileMap, PlayerLeft);

            tile_map_position PlayerRight = NewPlayerPosition;
            PlayerRight.TileRelX += 0.5f*PlayerWidth;
            PlayerRight = ReCanonicalizePosition(TileMap, PlayerRight);

            if(IsTileMapPointEmpty(TileMap,NewPlayerPosition)&&
               IsTileMapPointEmpty(TileMap,PlayerRight)&&
               IsTileMapPointEmpty(TileMap,PlayerLeft))
            {
                GameState->PlayerPosition = NewPlayerPosition;
            }

        }
    }
    DrawRectangle(Buffer, 0.0f, 0.0f,
                  (real32)Buffer->Width ,(real32)Buffer->Height,
                  1.0f, 0.0f , 1.0f);



    for(int32 RelRow = -100 ; RelRow < 100; RelRow++)
    {
        for(int32 RelColumn = -200; RelColumn < 200; RelColumn++)
        {

            uint32 Column = RelColumn + GameState->PlayerPosition.AbsTileX;
            uint32 Row = RelRow + GameState->PlayerPosition.AbsTileY;

            uint32 TileID = GetTileValue(TileMap, Column, Row);

            if(TileID > 0)
            {
                real32 Gray = 0.5f;
                if (TileID == 2)
                {
                    Gray = 1.0f;
                }

                if((Column == GameState->PlayerPosition.AbsTileX) &&
                   (Row == GameState->PlayerPosition.AbsTileY))
                {
                    Gray = 0.0f;
                }
            
                real32 CenterX = ScreenCenterX + ((real32)RelColumn) * TileMap->TileSideInPixels
                    - GameState->PlayerPosition.TileRelX*TileMap->MetersToPixels;
                real32 CenterY = ScreenCenterY - ((real32)RelRow)  * TileMap->TileSideInPixels
                    + GameState->PlayerPosition.TileRelY*TileMap->MetersToPixels;

                real32 MinX = CenterX - 0.5f*TileMap->TileSideInPixels;
                real32 MinY = CenterY - 0.5f*TileMap->TileSideInPixels;
                real32 MaxX = MinX + TileMap->TileSideInPixels;
                real32 MaxY = MinY + TileMap->TileSideInPixels;
            
                DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
            }
        }
    }

    real32 PlayerLeftDot = ScreenCenterX - 0.5f*PlayerWidth*TileMap->MetersToPixels;
    real32 PlayerTopDot = ScreenCenterY - PlayerHeight*TileMap->MetersToPixels;

    DrawRectangle(Buffer, PlayerLeftDot, PlayerTopDot,
                  (PlayerLeftDot+PlayerWidth*TileMap->MetersToPixels),
                  (PlayerTopDot+PlayerHeight*TileMap->MetersToPixels),
                  PlayerR, PlayerG, PlayerB);

    real32 PlayerGroundDotX = ScreenCenterX;
    real32 PlayerGroundDotY = ScreenCenterY; 
    
    DrawRectangle(Buffer, PlayerGroundDotX-3, PlayerGroundDotY-3,
                  (PlayerGroundDotX+3), (PlayerGroundDotY+3),
                  1, 0, 0);
        
}


// NOTE: look at the macro in game.h
extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameSoundOutput(GameState, SoundBuffer, 45);    
}




//---------------------obsolete code---------------------------------------------

/*
internal void
DEBUGRenderWeirdGradient(game_offscreen_buffer *Buffer ,int Xoffset, int Yoffset)
{
    // TODO: there is a place where we should test what will be better
    // passing by pointer or value, so we gonna see what the optimizer
    // really does in this case
#if 1
    uint8 *Row = (uint8 *)Buffer->Memory;
    // The carret that allow as to switch between the rows
    
    for(int Y = 0; Y < Buffer->Height; Y++)
    {
        uint32 *Pixel = (uint32 *)Row;
        for( int X = 0; X < Buffer->Width; X++)
        {
            //                                         0    1    2    3 
            // Structure of pixel in Windows          |xx| |RR| |GG| |BB|
            // of pixel in machine memory:            |BB| |GG| |RR| |xx|
            // all this shit bcoz of the LITTLE ENDIAN architecture
            uint8 BB = (uint8)(X + Xoffset);
            uint8 GG = (uint8)(Y + Yoffset);
            uint8 RR = 0;// (uint8)(X + Y + Xoffset - Yoffset);
            uint8 xx = 0;
            
            // memory:    BB GG RR xx
            // register:  xx RR GG BB
            *Pixel++ = (xx | (RR<<16) |( GG<<16) | (BB));
        }
        Row += Buffer->Pitch;
    }
#endif
}
*/


