/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   ======================================================================== */


inline tile_chunk *
GetTileChunk(tile_map * TileMap, uint32 TilechunkX, uint32 TilechunkY )
{
    tile_chunk *Tilechunk = 0;
    if((TilechunkX >= 0) && (TilechunkX < TileMap->TileChunkXDim) &&
       (TilechunkY >= 0) && (TilechunkY < TileMap->TileChunkYDim))
    {
        Tilechunk = &TileMap->TileChunks[TilechunkY*TileMap->TileChunkXDim + TilechunkX];
    }
    return(Tilechunk);
}


inline tile_chunk_position
GetTileChunkPositionFor(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position Result;

    Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
    Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
    Result.RelTileX = AbsTileX & TileMap->ChunkMask;
    Result.RelTileY = AbsTileY & TileMap->ChunkMask;

    return(Result);        
}


inline void
ReCanonicalizeizeCoord(tile_map *TileMap, uint32 *Tile, real32 *TileRel)
{
    int32 TileOffset = RoundReal32ToInt32((*TileRel)/TileMap->TileSideInMeters);

    //NOTE: world is toroidal (if u step off on end u will come back to the other) 

    *Tile += TileOffset;
    *TileRel -= TileOffset*TileMap->TileSideInMeters;

    Assert(*TileRel >= -0.5f*TileMap->TileSideInMeters);
    Assert(*TileRel <= 0.5f*TileMap->TileSideInMeters);

}

// NOTE: this is inline function so we don't need to pass out large struct via
// pointer
inline tile_map_position
ReCanonicalizePosition(tile_map *TileMap, tile_map_position Pos)
{
    tile_map_position Result = Pos;

    ReCanonicalizeizeCoord(TileMap, &Result.AbsTileX, &Result.TileRelX);
    ReCanonicalizeizeCoord(TileMap, &Result.AbsTileY, &Result.TileRelY);

    return(Result);
}


inline uint32
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY )
{
    tile_chunk_position ChunkPos = GetTileChunkPositionFor(TileMap, AbsTileX, AbsTileY);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY);

    int32 TileX = ChunkPos.RelTileX;
    int32 TileY = ChunkPos.RelTileY;
    uint32 Value = 0;
    if(TileChunk)
    {
    Value = TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX];
    }
    return(Value); 
}


inline void
SetTileValue(memory_area *Area, tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 TileValue)
{
    tile_chunk_position ChunkPos = GetTileChunkPositionFor(TileMap, AbsTileX, AbsTileY);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
    Assert(TileChunk);

    int32 TileX = ChunkPos.RelTileX;
    int32 TileY = ChunkPos.RelTileY;
    TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX] = TileValue;
}

internal bool32
IsTileEmpty(tile_map *TileMap, tile_chunk *Tilechunk, uint32 TestTileX, uint32 TestTileY)
{
    bool32 IsEmpty = false;

    if(Tilechunk)
    {
        uint32 TilechunkValue = GetTileValue(TileMap, TestTileX, TestTileY); 
        IsEmpty = (TilechunkValue == 0);
    }
    
    return(IsEmpty);
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position CanPos)
{
    bool32 IsEmpty = false;
    
    tile_chunk_position ChunkPos = GetTileChunkPositionFor(TileMap, CanPos.AbsTileX, CanPos.AbsTileY);
   
    //tile_chunk *Tilechunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
    //IsEmpty = IsTileEmpty(TileMap, Tilechunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
    uint32 TileChunkValue = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY);
    IsEmpty = (TileChunkValue == 1);

    return(IsEmpty);
}
