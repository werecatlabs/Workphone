#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace workphone::procedural
{
    // CPU layout shared by city rendering, road queries and race routes. No engine
    // singleton is required, so tools can preview and validate the same layout.
    struct OpenCityLayout
    {
        struct Point { float x, z; };
        struct Lot { float x, z, width, depth, height; bool park; };
        int blocks;
        float spacing, extent;
        std::vector<Lot> lots;
        std::vector<Point> route;

        static OpenCityLayout generate(std::uint32_t seed, int blocks = 8, int route = 0)
        {
            OpenCityLayout city;
            city.blocks = std::clamp(blocks / 2 * 2, 6, 10);
            city.spacing = 88.f + float(seed % 17);
            city.extent = city.blocks * city.spacing * .5f;
            auto random = [&seed]() {
                seed = seed * 1664525u + 1013904223u;
                return float(seed >> 8) / 16777216.f;
            };
            for(int x = 0; x < city.blocks; ++x)
                for(int z = 0; z < city.blocks; ++z)
                    for(int a = 0; a < 2; ++a)
                        for(int b = 0; b < 2; ++b)
                        {
                            const float px = -city.extent + (x + .25f + a * .5f) * city.spacing;
                            const float pz = -city.extent + (z + .25f + b * .5f) * city.spacing;
                            const float density = 1 - std::max(std::abs(px), std::abs(pz)) / city.extent;
                            city.lots.push_back({px, pz, 18 + random() * 6, 18 + random() * 6,
                                8 + random() * (12 + density * 55), random() < .12f});
                        }
            const float radius = (city.blocks / 2 - 2 + std::clamp(route, 0, 2)) * city.spacing;
            // Start at the origin facing -Z, matching the existing vehicle reset.
            const Point corners[] = {{0, 0}, {0, -radius}, {radius, -radius},
                {radius, radius}, {0, radius}, {0, 0}};
            for(int i = 0; i < 5; ++i)
            {
                const auto a = corners[i], b = corners[i + 1];
                const int steps = int(std::ceil(std::hypot(b.x - a.x, b.z - a.z) / 2));
                for(int j = 0; j < steps; ++j)
                {
                    const float t = float(j) / steps;
                    city.route.push_back({a.x + (b.x - a.x) * t, a.z + (b.z - a.z) * t});
                }
            }
            return city;
        }

        float roadDistance(float x, float z) const
        {
            if(std::abs(x) > extent + 6 || std::abs(z) > extent + 6)
                return 1000;
            return std::min(std::abs(x - std::round(x / spacing) * spacing),
                            std::abs(z - std::round(z / spacing) * spacing));
        }
    };
}

