#pragma once

#ifdef WIN32
#include <windows.h>
#endif

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/tile_map_layer.hpp>
#include <godot_cpp/classes/tile_set.hpp>
#include <godot_cpp/classes/tile_data.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>

#include <map>
#include <vector>

class BetterTerrainPP : public godot::Object
{
  GDCLASS(BetterTerrainPP, Object);

  struct Placement
  {
    int source_id;
    godot::Vector2i coord;
    int alternative;
    std::map<int, std::vector<int>> peering;
    double probability;
  };

  static Placement empty_placement;

  godot::TileMapLayer* m_tilemap_layer{nullptr};
  godot::TileSet* m_tileset{nullptr};
  std::map<int, std::vector<Placement>> m_cache;
  std::vector<int> m_terrain_types;

  bool m_fixed_random_seed{false};
  mutable godot::RandomNumberGenerator m_rng;

protected:
  static void _bind_methods();

public:
  bool init(godot::TileMapLayer* layer, bool fixed_random_seed = false);

  int get_cell(godot::TileMapLayer* layer, godot::Vector2i coord) const;
  bool set_cell(godot::TileMapLayer* layer, godot::Vector2i coord, int type);
  bool set_cells(godot::TileMapLayer* layer, const godot::Array& coords, int type);

  void update_terrain_cells(godot::TileMapLayer* layer, const godot::Array& cells, bool and_surrounding_cells = true);
  void update_terrain_cell(godot::TileMapLayer* layer, godot::Vector2i cell, bool and_surrounding_cells = true);
  void update_terrain_area(godot::TileMapLayer* layer, godot::Rect2i area, bool and_surrounding_cells = true);

private:
  std::vector<godot::Vector2i> widen(godot::TileMapLayer* layer, const std::vector<godot::Vector2i>& coords) const;
  std::vector<godot::Vector2i> widen_with_exclusion(godot::TileMapLayer* layer, const std::vector<godot::Vector2i>& coords, const godot::Rect2i& exclusion) const;
  const std::vector<int>& get_terrain_peering_cells() const;
  void update_tile_immediate(godot::TileMapLayer* layer, godot::Vector2i coord, const std::map<godot::Vector2i, int>& types);
  const Placement* update_tile_tiles(godot::TileMapLayer* layer, godot::Vector2i coord, const std::map<godot::Vector2i, int>& types, bool apply_empty_probability) const;
  const Placement* update_tile_vertices(godot::TileMapLayer* layer, godot::Vector2i coord, const std::map<godot::Vector2i, int>& types) const;
  int probe(godot::TileMapLayer* layer, godot::Vector2i coord, int peering, int type, const std::map<godot::Vector2i, int>& types) const;
  const Placement* weighted_selection(const std::vector<const Placement*>& choices, const godot::Vector2i& coord, bool apply_empty_probability) const;
};

