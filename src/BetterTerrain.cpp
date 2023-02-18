#include "BetterTerrain.hpp"

#include <godot_cpp/core/class_db.hpp>

void BetterTerrain::_bind_methods()
{
  godot::ClassDB::bind_static_method("BetterTerrain", godot::D_METHOD("test_static", "a", "b"), &BetterTerrain::test_static);
}

int BetterTerrain::test_static(int a, int b)
{
  return a + 2 * b;
}


