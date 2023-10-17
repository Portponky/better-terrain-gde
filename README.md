# better-terrain-gde

Implementation of Better Terrain plugin algorithm using Godot's GDExtension.

Implements `get_cell`, `set_cell(s)`, `update_terrain_area` and `update_terrain_cell(s)`, so it can provide fast terrain matching. It is approximately 15x faster than the built-in terrain system, and about 8-10x faster than the Better Terrain plugin. Note that these values are very roughly calculated on my budget laptop. Your mileage may vary.

No support provided; only use this if you know what you're doing.
