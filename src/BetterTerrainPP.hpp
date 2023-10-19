#pragma once

#ifdef WIN32
#include <windows.h>
#endif

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/tile_map.hpp>
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

  godot::TileMap* m_tilemap{nullptr};
  godot::TileSet* m_tileset{nullptr};
  std::map<int, std::vector<Placement>> m_cache;
  std::vector<int> m_terrain_types;

  mutable godot::RandomNumberGenerator m_rng;

protected:
  static void _bind_methods();

public:
  bool init(godot::TileMap* map);

  int get_cell(int layer, godot::Vector2i coord) const;
  bool set_cell(int layer, godot::Vector2i coord, int type);
  bool set_cells(int layer, const godot::Array& coords, int type);

  void update_terrain_cells(int layer, const godot::Array& cells, bool and_surrounding_cells = true);
  void update_terrain_cell(int layer, godot::Vector2i cell, bool and_surrounding_cells = true);
  void update_terrain_area(int layer, godot::Rect2i area, bool and_surrounding_cells = true);

private:
  std::vector<godot::Vector2i> widen(const std::vector<godot::Vector2i>& coords) const;
  std::vector<godot::Vector2i> widen_with_exclusion(const std::vector<godot::Vector2i>& coords, const godot::Rect2i& exclusion) const;
  const std::vector<int>& get_terrain_peering_cells() const;
  void update_tile_immediate(int layer, godot::Vector2i coord, const std::map<godot::Vector2i, int>& types);
  const Placement* update_tile_tiles(godot::Vector2i coord, const std::map<godot::Vector2i, int>& types, bool apply_empty_probability) const;
  const Placement* update_tile_vertices(godot::Vector2i coord, const std::map<godot::Vector2i, int>& types) const;
  int probe(godot::Vector2i coord, int peering, int type, const std::map<godot::Vector2i, int>& types) const;
  const Placement* weighted_selection(const std::vector<const Placement*>& choices, bool apply_empty_probability) const;
};

