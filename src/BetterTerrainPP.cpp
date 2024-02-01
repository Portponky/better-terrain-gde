#include "BetterTerrainPP.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/tile_set_atlas_source.hpp>
#include <set>
#include <algorithm>

namespace
{

const char meta_name[] = "_better_terrain";
const char meta_version[] = "0.2";

enum TerrainType {
  MATCH_TILES,
  MATCH_VERTICES,
  CATEGORY,
  DECORATION,
  MAX,

  EMPTY = -1,
  NON_TERRAIN = -2,
  ERROR = -3
};

enum SymmetryType {
  NONE,
  MIRROR,
  FLIP,
  REFLECT,
  ROTATE_CLOCKWISE,
  ROTATE_COUNTER_CLOCKWISE,
  ROTATE_180,
  ROTATE_ALL,
  ALL
};

const int transform_flip_h = 0x1000;
const int transform_flip_v = 0x2000;
const int transform_transpose = 0x4000;

const std::vector<std::vector<int>> symmetry_mapping = {
  {0},
  {0, transform_flip_h},
  {0, transform_flip_v},
  {0, transform_flip_h, transform_flip_v, transform_flip_h | transform_flip_v},
  {0, transform_flip_h | transform_transpose},
  {0, transform_flip_v | transform_transpose},
  {0, transform_flip_h | transform_flip_v},
  {0, transform_flip_h | transform_transpose, transform_flip_h | transform_flip_v, transform_flip_v | transform_transpose},
  {
    0,
    transform_flip_h,
    transform_flip_v,
    transform_flip_h | transform_flip_v,
    transform_transpose,
    transform_flip_h | transform_transpose,
    transform_flip_v | transform_transpose,
    transform_flip_h | transform_flip_v | transform_transpose
  }
};

const std::vector<int> terrain_peering_square_tiles = {0, 3, 4, 7, 8, 11, 12, 15};
const std::vector<int> terrain_peering_isometric_tiles = {1, 2, 5, 6, 9, 10, 13, 14};
const std::vector<int> terrain_peering_horiztonal_tiles = {0, 2, 6, 8, 10, 14};
const std::vector<int> terrain_peering_vertical_tiles = {2, 4, 6, 10, 12, 14};

const int terrain_peering_hflip[] = {8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11};
const int terrain_peering_vflip[] = {0, 1, 14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3};
const int terrain_peering_transpose[] = {4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8, 9, 6, 7};

bool has_intersection(const std::vector<int>& bits, const godot::Array& check)
{
  for (int i : bits)
    if (check.has(i))
      return true;
  return false;
}

std::map<int, std::vector<int>> peering_bits_after_symmetry(const std::map<int, std::vector<int>>& peering, int flags)
{
  if (flags == 0)
    return peering;

  std::map<int, std::vector<int>> result;
  for (const auto& [k, v] : peering)
  {
    auto bit = k;
    if (flags & transform_transpose)
      bit = terrain_peering_transpose[bit];
    if (flags& transform_flip_h)
      bit = terrain_peering_hflip[bit];
    if (flags & transform_flip_v)
      bit = terrain_peering_vflip[bit];
    result[bit] = v;
  }
  return result;
}

std::vector<godot::Vector2i> neighboring_coords(godot::TileMap* tilemap, godot::Vector2i coord, const std::vector<int>& peering)
{
  std::vector<godot::Vector2i> result;
  result.reserve(peering.size());
  for (int p : peering)
  {
    result.push_back(tilemap->get_neighbor_cell(coord, static_cast<godot::TileSet::CellNeighbor>(p)));
  }
  return result;
}

std::vector<godot::Vector2i> associated_vertex_cells(godot::TileMap* tilemap, godot::Vector2i coord, godot::TileSet::CellNeighbor corner)
{
  godot::TileSet* tileset = tilemap->get_tileset().ptr();
  if (tileset->get_tile_shape() == godot::TileSet::TILE_SHAPE_SQUARE ||
      tileset->get_tile_shape() == godot::TileSet::TILE_SHAPE_ISOMETRIC)
    switch (corner)
    {
    case godot::TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
      return neighboring_coords(tilemap, coord, {0, 3, 4});
    case godot::TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
      return neighboring_coords(tilemap, coord, {4, 7, 8});
    case godot::TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
      return neighboring_coords(tilemap, coord, {8, 11, 12});
    case godot::TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
      return neighboring_coords(tilemap, coord, {12, 15, 0});
    case godot::TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
      return neighboring_coords(tilemap, coord, {14, 1, 2});
    case godot::TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
      return neighboring_coords(tilemap, coord, {2, 5, 6});
    case godot::TileSet::CELL_NEIGHBOR_LEFT_CORNER:
      return neighboring_coords(tilemap, coord, {6, 9, 10});
    case godot::TileSet::CELL_NEIGHBOR_TOP_CORNER:
      return neighboring_coords(tilemap, coord, {10, 13, 14});
    default:
      break;
    }

  if (tileset->get_tile_offset_axis() == godot::TileSet::TILE_OFFSET_AXIS_HORIZONTAL)
    switch (corner)
    {
    case godot::TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
      return neighboring_coords(tilemap, coord, {0, 2});
    case godot::TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
      return neighboring_coords(tilemap, coord, {2, 6});
    case godot::TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
      return neighboring_coords(tilemap, coord, {6, 8});
    case godot::TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
      return neighboring_coords(tilemap, coord, {8, 10});
    case godot::TileSet::CELL_NEIGHBOR_TOP_CORNER:
      return neighboring_coords(tilemap, coord, {10, 14});
    case godot::TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
      return neighboring_coords(tilemap, coord, {14, 0});
    default:
      break;
    }

  switch(corner)
  {
  case godot::TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
    return neighboring_coords(tilemap, coord, {14, 2});
  case godot::TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
    return neighboring_coords(tilemap, coord, {2, 4});
  case godot::TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
    return neighboring_coords(tilemap, coord, {4, 6});
  case godot::TileSet::CELL_NEIGHBOR_LEFT_CORNER:
    return neighboring_coords(tilemap, coord, {6, 10});
  case godot::TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
    return neighboring_coords(tilemap, coord, {10, 12});
  case godot::TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
    return neighboring_coords(tilemap, coord, {12, 14});
  default:
    break;
  }

  return {};
}

template<typename K, typename V>
V map_safe_get(const std::map<K, V>& m, const K& key, const V& def)
{
  auto it = m.find(key);
  return it == m.end() ? def : it->second;
}

}

BetterTerrainPP::Placement BetterTerrainPP::empty_placement{-1, godot::Vector2i(0, 0), -1, {}, 1.0};

void BetterTerrainPP::_bind_methods()
{
  godot::ClassDB::bind_method(godot::D_METHOD("init", "map"), &BetterTerrainPP::init);
  godot::ClassDB::bind_method(godot::D_METHOD("get_cell", "layer", "coord"), &BetterTerrainPP::get_cell);
  godot::ClassDB::bind_method(godot::D_METHOD("set_cells", "layer", "coords", "type"), &BetterTerrainPP::set_cells);
  godot::ClassDB::bind_method(godot::D_METHOD("set_cell", "layer", "coord", "type"), &BetterTerrainPP::set_cell);
  godot::ClassDB::bind_method(godot::D_METHOD("update_terrain_cells", "layer", "cells", "and_surrounding_cells"), &BetterTerrainPP::update_terrain_cells, DEFVAL(true));
  godot::ClassDB::bind_method(godot::D_METHOD("update_terrain_cell", "layer", "cell", "and_surrounding_cells"), &BetterTerrainPP::update_terrain_cell, DEFVAL(true));
  godot::ClassDB::bind_method(godot::D_METHOD("update_terrain_area", "layer", "area", "and_surrounding_cells"), &BetterTerrainPP::update_terrain_area, DEFVAL(true));
}

bool BetterTerrainPP::init(godot::TileMap* map)
{
  if (!map)
    return false;

  auto tileset = map->get_tileset();
  if (!tileset.is_valid())
    return false;

  m_tilemap = map;
  m_tileset = tileset.ptr();

  godot::Dictionary meta = tileset->get_meta(meta_name);
  if (godot::String(meta["version"]) != meta_version)
    return false;

  m_cache.clear();
  m_terrain_types.clear();

  std::map<int, std::vector<int>> types;
  godot::Array terrains = meta["terrains"];
  for (int i = 0; i < terrains.size(); ++i)
  {
    godot::Array terrain = terrains[i];
    m_terrain_types.push_back(static_cast<int>(terrain[2]));
    godot::Array categories = terrain[3];
    std::vector<int> bits = {i};
    for (int c = 0; c < categories.size(); ++c)
      bits.push_back(categories[c]);
    types[i] = std::move(bits);
    m_cache[i] = {};
  }

  types[-1] = {-1};
  m_cache[-1] = {Placement{-1, godot::Vector2i(0, 0), -1, {}, 1.0}};

  for (int s = 0; s < tileset->get_source_count(); ++s)
  {
    auto source_id = tileset->get_source_id(s);
    auto source = godot::Object::cast_to<godot::TileSetAtlasSource>(tileset->get_source(source_id).ptr());
    if (!source)
      continue;

    for (int c = 0; c < source->get_tiles_count(); ++c)
    {
      godot::Vector2i coord = source->get_tile_id(c);
      for (int a = 0; a < source->get_alternative_tiles_count(coord); ++a)
      {
        int alternate = source->get_alternative_tile_id(coord, a);
        godot::TileData* td = source->get_tile_data(coord, alternate);
        if (!td->has_meta(meta_name))
          continue;
        godot::Dictionary td_meta = td->get_meta(meta_name);
        int td_meta_type = td_meta["type"];
        if (td_meta_type < TerrainType::EMPTY || td_meta_type > terrains.size())
          continue;

        std::map<int, std::vector<int>> peering;
        godot::Array td_meta_keys = td_meta.keys();
        for (int k = 0; k < td_meta_keys.size(); ++k)
        {
          auto key = td_meta_keys[k];
          if (key.get_type() != godot::Variant::INT)
            continue;

          std::vector<int> targets;
          for (const auto& [type, bits] : types)
          {
            if (has_intersection(bits, td_meta[key]))
              targets.push_back(type);
          }

          peering[int(key)] = std::move(targets);
        }

        if (td_meta_type == TerrainType::DECORATION && peering.empty())
          continue;

        int symmetry = td_meta.get("symmetry", SymmetryType::NONE);
        for (const auto flags : symmetry_mapping[symmetry])
        {
          peering = peering_bits_after_symmetry(peering, flags);
          m_cache[td_meta_type].push_back(Placement{source_id, coord, alternate | flags, std::move(peering), td->get_probability()});
        }
      }
    }
  }

  m_rng.randomize();
  return true;
}

int BetterTerrainPP::get_cell(int layer, godot::Vector2i coord) const
{
  if (!m_tilemap || !m_tileset || layer < 0 || layer >= m_tilemap->get_layers_count())
    return TerrainType::ERROR;

  if (m_tilemap->get_cell_source_id(layer, coord) == -1)
    return TerrainType::EMPTY;

  godot::TileData* td = m_tilemap->get_cell_tile_data(layer, coord);
  if (!td)
    return TerrainType::EMPTY;

  if (!td->has_meta(meta_name))
    return TerrainType::NON_TERRAIN;

  godot::Dictionary td_meta = td->get_meta(meta_name);
  int td_meta_type = td_meta["type"];
  return td_meta_type;
}

bool BetterTerrainPP::set_cell(int layer, godot::Vector2i coord, int type)
{
  if (!m_tilemap || !m_tileset || layer < 0 || layer >= m_tilemap->get_layers_count() || type < TerrainType::EMPTY)
    return false;

  if (type == TerrainType::EMPTY)
  {
    m_tilemap->erase_cell(layer, coord);
    return true;
  }

  if (type >= m_terrain_types.size())
    return false;

  if (m_cache[type].empty())
    return false;

  const Placement& p = *(m_cache[type].begin());
  m_tilemap->set_cell(layer, coord, p.source_id, p.coord, p.alternative);
  return true;
}

bool BetterTerrainPP::set_cells(int layer, const godot::Array& coords, int type)
{
  if (!m_tilemap || !m_tileset || layer < 0 || layer >= m_tilemap->get_layers_count() || type < TerrainType::EMPTY)
    return false;

  if (type == TerrainType::EMPTY)
  {
    for (int c = 0; c < coords.size(); ++c)
      m_tilemap->erase_cell(layer, coords[c]);
    return true;
  }

  if (type >= m_terrain_types.size())
    return false;

  if (m_cache[type].empty())
    return false;

  const Placement& p = *(m_cache[type].begin());
  for (int c = 0; c < coords.size(); ++c)
    m_tilemap->set_cell(layer, coords[c], p.source_id, p.coord, p.alternative);
  return true;
}

void BetterTerrainPP::update_terrain_cells(int layer, const godot::Array& cells, bool and_surrounding_cells)
{
  if (!m_tilemap || !m_tileset || layer < 0 || layer >= m_tilemap->get_layers_count())
    return;

  std::vector<godot::Vector2i> coords;
  coords.reserve(cells.size());
  for (int c = 0; c < cells.size(); ++c)
    coords.push_back(cells[c]);

  if (and_surrounding_cells)
    coords = widen(coords);
  auto needed_cells = widen(coords);

  std::map<godot::Vector2i, int> types;
  for (const auto& c : needed_cells)
    types[c] = get_cell(layer, c);

  for (const auto& c : coords)
    update_tile_immediate(layer, c, types);
}

void BetterTerrainPP::update_terrain_cell(int layer, godot::Vector2i cell, bool and_surrounding_cells)
{
  godot::Array cells;
  cells.push_back(cell);
  update_terrain_cells(layer, cells, and_surrounding_cells);
}

void BetterTerrainPP::update_terrain_area(int layer, godot::Rect2i area, bool and_surrounding_cells)
{
  if (!m_tilemap || !m_tileset || layer < 0 || layer >= m_tilemap->get_layers_count())
    return;

  area = area.abs();

  std::vector<godot::Vector2i> edges;
  for (int x = area.position.x; x < area.position.x + area.size.x; ++x)
  {
    edges.push_back(godot::Vector2i(x, area.position.y));
    edges.push_back(godot::Vector2i(x, area.position.y + area.size.y - 1));
  }
  for (int y = area.position.y + 1; y < area.position.y + area.size.y - 1; ++y)
  {
    edges.push_back(godot::Vector2i(area.position.x, y));
    edges.push_back(godot::Vector2i(area.position.x + area.size.x - 1, y));
  }

  std::vector<godot::Vector2i> additional_cells;
  std::vector<godot::Vector2i> needed_cells = widen_with_exclusion(edges, area);

  if (and_surrounding_cells)
  {
    additional_cells = needed_cells;
    needed_cells = widen_with_exclusion(needed_cells, area);
  }

  std::map<godot::Vector2i, int> types;
  for (int y = area.position.y; y < area.position.y + area.size.y; ++y)
    for (int x = area.position.x; x < area.position.x + area.size.x; ++x)
    {
      godot::Vector2i coord(x, y);
      types[coord] = get_cell(layer, coord);
    }
  for (const auto& c : needed_cells)
    types[c] = get_cell(layer, c);

  for (int y = area.position.y; y < area.position.y + area.size.y; ++y)
    for (int x = area.position.x; x < area.position.x + area.size.x; ++x)
      update_tile_immediate(layer, godot::Vector2i(x, y), types);
  for (const auto& c : additional_cells)
    update_tile_immediate(layer, c, types);
}

std::vector<godot::Vector2i> BetterTerrainPP::widen(const std::vector<godot::Vector2i>& coords) const
{
  std::set<godot::Vector2i> result;
  for (const godot::Vector2i& c : coords)
  {
    result.insert(c);
    for (int p : get_terrain_peering_cells())
    {
      godot::Vector2i t = m_tilemap->get_neighbor_cell(c, static_cast<godot::TileSet::CellNeighbor>(p));
      result.insert(t);
    }
  }

  return {result.begin(), result.end()};
}

std::vector<godot::Vector2i> BetterTerrainPP::widen_with_exclusion(const std::vector<godot::Vector2i>& coords, const godot::Rect2i& exclusion) const
{
  std::set<godot::Vector2i> result;
  for (const godot::Vector2i& c : coords)
  {
    if (!exclusion.has_point(c))
      result.insert(c);

    for (int p : get_terrain_peering_cells())
    {
      godot::Vector2i t = m_tilemap->get_neighbor_cell(c, static_cast<godot::TileSet::CellNeighbor>(p));
      if (!exclusion.has_point(t))
        result.insert(t);
    }
  }

  return {result.begin(), result.end()};
}

const std::vector<int>& BetterTerrainPP::get_terrain_peering_cells() const
{
  if (m_tileset->get_tile_shape() == godot::TileSet::TILE_SHAPE_SQUARE)
    return terrain_peering_square_tiles;
  if (m_tileset->get_tile_shape() == godot::TileSet::TILE_SHAPE_ISOMETRIC)
    return terrain_peering_isometric_tiles;
  if (m_tileset->get_tile_offset_axis() == godot::TileSet::TILE_OFFSET_AXIS_VERTICAL)
    return terrain_peering_vertical_tiles;
  return terrain_peering_horiztonal_tiles;
}

void BetterTerrainPP::update_tile_immediate(int layer, godot::Vector2i coord, const std::map<godot::Vector2i, int>& types)
{
  int type = map_safe_get(types, coord, -1);
  if (type < TerrainType::EMPTY || type >= m_terrain_types.size())
    return;

  const Placement* placement {nullptr};
  if (m_terrain_types[type] == TerrainType::MATCH_TILES || m_terrain_types[type] == TerrainType::DECORATION)
    placement = update_tile_tiles(coord, types, m_terrain_types[type] == TerrainType::DECORATION);
  else if (m_terrain_types[type] == TerrainType::MATCH_VERTICES)
    placement = update_tile_vertices(coord, types);

  if (placement)
    m_tilemap->set_cell(layer, coord, placement->source_id, placement->coord, placement->alternative);
}

const BetterTerrainPP::Placement* BetterTerrainPP::update_tile_tiles(godot::Vector2i coord, const std::map<godot::Vector2i, int>& types, bool apply_empty_probability) const
{
  int type = map_safe_get(types, coord, -1);
  int best_score = -1000;
  std::vector<const Placement*> best;

  auto it = m_cache.find(type);
  if (it == m_cache.end())
    return nullptr; //wtf

  for (const auto& p : it->second)
  {
    int score = 0;
    for (const auto& [k, v] : p.peering)
    {
      godot::Vector2i neighbor = m_tilemap->get_neighbor_cell(coord, static_cast<godot::TileSet::CellNeighbor>(k));
      int target = map_safe_get(types, neighbor, -2);
      if (std::find(v.begin(), v.end(), target) != v.end())
        score += 3;
      else
        score -= 10;
    }

    if (score > best_score)
    {
      best_score = score;
      best = {&p};
    }
    else if (score == best_score)
      best.push_back(&p);
  }

  if (best.empty())
    return nullptr;

  return weighted_selection(best, apply_empty_probability);
}

const BetterTerrainPP::Placement* BetterTerrainPP::update_tile_vertices(godot::Vector2i coord, const std::map<godot::Vector2i, int>& types) const
{
  int type = map_safe_get(types, coord, -1);
  int best_score = -1000;
  std::vector<const Placement*> best;

  auto it = m_cache.find(type);
  if (it == m_cache.end())
    return nullptr; //wtf

  for (const auto& p : it->second)
  {
    int score = 0;
    for (const auto& [k, v] : p.peering)
    {
      int target = probe(coord, static_cast<godot::TileSet::CellNeighbor>(k), type, types);
      if (std::find(v.begin(), v.end(), target) != v.end())
        score += 3;
      else
        score -= 10;
    }

    if (score > best_score)
    {
      best_score = score;
      best = {&p};
    }
    else if (score == best_score)
      best.push_back(&p);
  }

  if (best.empty())
    return nullptr;

  return weighted_selection(best, false);
}

int BetterTerrainPP::probe(godot::Vector2i coord, int peering, int type, const std::map<godot::Vector2i, int>& types) const
{
  std::vector<godot::Vector2i> coords = associated_vertex_cells(m_tilemap, coord, static_cast<godot::TileSet::CellNeighbor>(peering));
  std::vector<int> targets;
  for (int c = 0; c < coords.size(); ++c)
    targets.push_back(map_safe_get(types, coords[c], -1));

  int first = targets[0];
  bool all_equal = true;
  for (int t = 1; t < targets.size() && all_equal; ++t)
    if (targets[t] != first)
      all_equal = false;

  if (all_equal)
    return first;

  targets.erase(std::remove(targets.begin(), targets.end(), type), targets.end());
  return *std::min_element(targets.begin(), targets.end());
}

const BetterTerrainPP::Placement* BetterTerrainPP::weighted_selection(const std::vector<const Placement*>& choices, bool apply_empty_probability) const
{
  if (choices.empty())
    return nullptr;

  if (apply_empty_probability)
  {
    double best = 0.0;
    for (const Placement* p : choices)
      best = std::max(best, p->probability);
    if (best < 1.0 && m_rng.randf() > best)
      return &empty_placement;
  }

  if (choices.size() == 1)
    return choices[0];

  double sum = 0.0;
  for (const Placement* p : choices)
    sum += p->probability;
  if (sum == 0.0)
    return choices[m_rng.randi() % choices.size()];

  double pick = m_rng.randf() * sum;
  for (const Placement* p : choices)
  {
    if (pick < p->probability)
      return p;
    pick -= p->probability;
  }
  return choices.back();
}
