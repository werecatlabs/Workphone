#ifndef _UVLookup_H
#define _UVLookup_H

#include <FBMesh/FBMeshPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include "Workphone/Core/Grid2.hpp"
#include "Workphone/Core/StringTypes.hpp"
#include "Workphone/Math/Vector2.hpp"

namespace workphone
{

    class UVLookup
    {
    public:
        UVLookup();
        UVLookup( const Vector2F &dimensions );
        ~UVLookup();

        void load( const String &filePath );
        void save( const String &filePath );

        Vector2F getUV( const Vector2F &position ) const;

        void add( const Vector2F &position, const Vector2F &uv );
        bool remove( const Vector2F &position );

        void setDimensions( const Vector2F &dimensions );
        Vector2F getDimensions() const;

    protected:
        Grid2 m_grid;

        using UVLookupMap = Array<Vector2F>;
        UVLookupMap m_uvLookupMap;
    };

}  // namespace workphone

#endif
