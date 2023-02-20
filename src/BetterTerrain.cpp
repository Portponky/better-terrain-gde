#include "BetterTerrain.hpp"
#include "Helpers.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace
{

struct Constants
{
  godot::StringName TERRAIN_META{"_better_terrain"};
  godot::String TERRAIN_SYSTEM_VERSION{"0.1"};
};

Constants c()
{
  static Constants cc;
  return cc;
}

godot::Dictionary _get_terrain_meta(godot::TileSet* ts)
{
  if (ts->has_meta(c().TERRAIN_META))
   return ts->get_meta(c().TERRAIN_META);

  return create_dictionary({{"terrains", godot::Array()}, {"version", c().TERRAIN_SYSTEM_VERSION}});
}

void _set_terrain_meta(godot::TileSet* ts, const godot::Dictionary& meta)
{
  ts->set_meta(c().TERRAIN_META, meta);
}

godot::Dictionary _get_tile_meta(godot::TileData* td)
{
  if (td->has_meta(c().TERRAIN_META))
    return td->get_meta(c().TERRAIN_META);

  return create_dictionary({{"type", -1}});
}

void _set_tile_meta(godot::TileData* td, godot::Dictionary meta)
{
  td->set_meta(c().TERRAIN_META, meta);
}

}

void BetterTerrain::_bind_methods()
{
  BIND_ENUM_CONSTANT(MATCH_TILES);
  BIND_ENUM_CONSTANT(MATCH_VERTICES);
  BIND_ENUM_CONSTANT(CATEGORY);
  BIND_ENUM_CONSTANT(MAX);

  godot::ClassDB::bind_static_method("BetterTerrain", godot::D_METHOD("get_terrain_categories", "ts"), &BetterTerrain::get_terrain_categories);
  godot::ClassDB::bind_static_method("BetterTerrain", godot::D_METHOD("add_terrain", "ts", "name", "color", "type", "categories"), &BetterTerrain::add_terrain, DEFVAL(godot::Array()));
  godot::ClassDB::bind_static_method("BetterTerrain", godot::D_METHOD("remove_terrain", "ts", "index"), &BetterTerrain::remove_terrain);
  godot::ClassDB::bind_static_method("BetterTerrain", godot::D_METHOD("terrain_count", "ts"), &BetterTerrain::terrain_count);
  godot::ClassDB::bind_static_method("BetterTerrain", godot::D_METHOD("set_terrain", "ts", "index", "name", "color", "type", "categories"), &BetterTerrain::set_terrain, DEFVAL(godot::Array()));
  godot::ClassDB::bind_static_method("BetterTerrain", godot::D_METHOD("get_terrain", "ts", "index"), &BetterTerrain::get_terrain);
  godot::ClassDB::bind_static_method("BetterTerrain", godot::D_METHOD("swap_terrains", "index1", "index2"), &BetterTerrain::swap_terrains);
}

godot::Array BetterTerrain::get_terrain_categories(godot::TileSet* ts)
{
  godot::Array result;
  if (!ts)
    return result;

  godot::Dictionary ts_meta = _get_terrain_meta(ts);
  godot::Array terrains = ts_meta["terrains"];
  for (int64_t id = 0; id < terrains.size(); ++id)
  {
    godot::Array t = terrains[id];
    if (t[2] == godot::Variant(BetterTerrain::CATEGORY))
      result.push_back(create_dictionary({{"name", t[0]}, {"color", t[1]}, {"id", id}}));
  }

  return result;
}

bool BetterTerrain::add_terrain(godot::TileSet* ts, godot::String name, godot::Color color, int type, godot::Array categories)
{
  if (!ts || name.is_empty() || type < 0 || type >= BetterTerrain::MAX)
    return false;

  godot::Dictionary ts_meta = _get_terrain_meta(ts);
  godot::Array terrains = ts_meta["terrains"];

  // check categories
  if (type == BetterTerrain::CATEGORY && !categories.is_empty())
    return false;
  for (int64_t i = 0; i < categories.size(); ++i)
  {
    int c = categories[i];
    if (c < 0 || c >= terrains.size())
      return false;
    godot::Array test = terrains[c];
    if (test[2] != godot::Variant(BetterTerrain::CATEGORY))
      return false;
  }

  terrains.push_back(create_array(name, color, type, categories));
  _set_terrain_meta(ts, ts_meta);
  //_purge_cache(ts)
  return true;
}

bool BetterTerrain::remove_terrain(godot::Ref<godot::TileSet> ts, int index)
{
  return false;
}

int BetterTerrain::terrain_count(godot::TileSet* ts)
{
  if (!ts)
    return 0;

  godot::Dictionary ts_meta = _get_terrain_meta(ts);
  godot::Array terrains = ts_meta["terrains"];

  return terrains.size();
}

godot::Dictionary BetterTerrain::get_terrain(godot::TileSet* ts, int index)
{
  if (!ts || index < 0)
    return create_dictionary({{"valid", false}});

  godot::Dictionary ts_meta = _get_terrain_meta(ts);
  godot::Array terrains = ts_meta["terrains"];
  if (index >= terrains.size())
    return create_dictionary({{"valid", false}});

  godot::Array terrain = terrains[index];
  return create_dictionary({{"name", terrain[0]}, {"color", terrain[1]}, {"type", terrain[2]}, {"categories", terrain[3]}, {"valid", true}});
}

bool BetterTerrain::set_terrain(godot::Ref<godot::TileSet> ts, int index, godot::String name, godot::Color color, int type, godot::Array categories)
{
  return false;
}

bool BetterTerrain::swap_terrains(godot::Ref<godot::TileSet> ts, int index1, int index2)
{
  return false;
}
