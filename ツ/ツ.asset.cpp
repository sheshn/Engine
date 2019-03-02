#include "ツ.asset.h"

struct Assets
{
    Memory_Arena* memory_arena;
};

internal Assets assets;

void init_asset_system(Memory_Arena* memory_arena)
{
    assets.memory_arena = memory_arena;
}