#ifndef WP_ASSIMP_MATERIAL_IMPORT_DEFAULTS_HPP
#define WP_ASSIMP_MATERIAL_IMPORT_DEFAULTS_HPP

#include <algorithm>
#include <cmath>

namespace workphone::detail
{
    inline float materialUnitFactor( float value, float fallback )
    {
        return std::isfinite( value ) ? std::clamp( value, 0.0f, 1.0f ) : fallback;
    }

    inline bool recoverObjMaterialOpacity( float opacity, bool isObj, bool recoveryEnabled )
    {
        return isObj && recoveryEnabled && std::isfinite( opacity ) && opacity <= 0.0f;
    }
}

#endif
