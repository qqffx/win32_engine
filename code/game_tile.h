/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   ======================================================================== */


struct tile_map_position
{

    // NOTE: these are fixed point tile location the high bits are tile chunk
    // and the low bits is the tile in the chunk
    uint32 AbsTileX;
    uint32 AbsTileY;

    // NOTE: Coordinates inside tile, not whole map
    real32 TileRelX;
    real32 TileRelY;
};

struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;

    uint32 RelTileX;
    uint32 RelTileY;
};

struct tile_chunk
{
    uint32 *Tiles;
};

struct tile_map
{
    tile_chunk *TileChunks;
    uint32 TileChunkXDim;
    uint32 TileChunkYDim;

    real32 TileSideInMeters;
    int32 TileSideInPixels;
    real32 MetersToPixels;

    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;;
};


