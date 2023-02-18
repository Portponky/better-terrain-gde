#pragma once

#ifdef WIN32
#include <windows.h>
#endif

#include <godot_cpp/classes/node.hpp>

class BetterTerrain : public godot::Node
{
  GDCLASS(BetterTerrain, Node);

protected:
  static void _bind_methods();

public:
  static int test_static(int a, int b);
};

