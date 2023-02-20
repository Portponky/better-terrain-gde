#pragma once

#ifdef WIN32
#include <windows.h>
#endif

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/tile_set.hpp>
#include <godot_cpp/classes/tile_data.hpp>

class BetterTerrain : public godot::Node
{
  GDCLASS(BetterTerrain, Node);

protected:
  static void _bind_methods();

public:
  enum TerrainType
  {
    MATCH_TILES,
    MATCH_VERTICES,
    CATEGORY,
    MAX
  };

  static godot::Array get_terrain_categories(godot::TileSet* ts);
  static bool add_terrain(godot::TileSet* ts, godot::String name, godot::Color color, int type, godot::Array categories = {});
  static bool remove_terrain(godot::Ref<godot::TileSet> ts, int index);
  static int terrain_count(godot::TileSet* ts);
  static godot::Dictionary get_terrain(godot::TileSet* ts, int index);
  static bool set_terrain(godot::Ref<godot::TileSet> ts, int index, godot::String name, godot::Color color, int type, godot::Array categories = {});
  static bool swap_terrains(godot::Ref<godot::TileSet> ts, int index1, int index2);
};

VARIANT_ENUM_CAST(BetterTerrain::TerrainType);
