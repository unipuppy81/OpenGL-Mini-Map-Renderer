#pragma once
#include "Types.hpp"
#include <string>

class GeoJsonLoader
{
public:
    static MapData load(const std::string& path);
};