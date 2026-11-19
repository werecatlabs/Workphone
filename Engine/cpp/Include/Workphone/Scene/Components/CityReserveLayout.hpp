#pragma once

        namespace cityReserveLandscape
        {
            struct Site
            {
                Vector3F centre, along, outward;
                float halfLength = 0, halfDepth = 0;
                u64 key = 0;
                bool corner = false;
            };

            inline bool contains( const Site &site, Vector3F point, float radius = 0 )
            {
                const auto x = site.halfLength * .88f - radius, z = site.halfDepth * .88f - radius;
                if( x <= 0 || z <= 0 ) return false;
                const auto d = point - site.centre;
                const auto u = d.dotProduct( site.along ) / x, v = d.dotProduct( site.outward ) / z;
                return u * u + v * v <= 1.f;
            }
        }
