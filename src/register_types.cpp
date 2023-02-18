#include <gdextension_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "BetterTerrain.hpp"

void initialize_better_terrain(godot::ModuleInitializationLevel level)
{
  if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    return;
  
  godot::ClassDB::register_class<BetterTerrain>();
}

void uninitialize_better_terrain(godot::ModuleInitializationLevel level)
{
  if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    return;
}

extern "C"
{

GDExtensionBool GDE_EXPORT better_terrain_init(const GDExtensionInterface* p_interface, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* r_initialization)
{
  godot::GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);

  init_obj.register_initializer(initialize_better_terrain);
  init_obj.register_terminator(uninitialize_better_terrain);
  init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);

  return init_obj.init();
}

}

