#ifndef GraphicsObjectHelper_h__
#define GraphicsObjectHelper_h__

#include "WPLuabind/WPLuabindPrerequisites.hpp"
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>

namespace workphone
{
    class GraphicsObjectHelper
    {
    public:
        static void _setMaterialName( render::IGraphicsObject *obj, const char *materialName );
        static void _setMaterialNameStr( render::IGraphicsObject *obj, String materialName );
        static void _setMaterialNameIndx( render::IGraphicsObject *obj, const char *materialName,
                                          lua_Integer idx );

        static void _setVisibilityFlags( render::IGraphicsObject *object, lua_Integer flag );

        static lua_Integer _getVisibilityFlags( render::IGraphicsObject *object );
    };
} // namespace workphone

#endif // GraphicsObjectHelper_h__
