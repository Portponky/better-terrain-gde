#pragma once

#include <godot_cpp/variant/variant.hpp>
#include <vector>

template<typename... STUFF>
godot::Array create_array(const STUFF&... stuff)
{
  godot::Array arr;
  (arr.push_back(stuff) , ...);
  return arr;
}

godot::Dictionary create_dictionary(const std::vector<std::pair<const char*, godot::Variant>>& entries)
{
  godot::Dictionary dict;
  for (const auto& [k, v] : entries)
    dict[k] = v;
  return dict;
}
