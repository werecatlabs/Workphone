#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <array>

namespace workphone
{

    struct MeshImposterTriangle
    {
        std::array<Vector3F, 3> positions;
        ColourF colour;
    };

    struct MeshImposterView
    {
        Vector3F right, up, towardCamera;
        Vector2F minimum, maximum;
    };

    struct WPCore_API MeshImposterAtlas : public SourceBudgetObject
    {
        MeshImposterAtlas() = default;

        MeshImposterAtlas( const MeshImposterAtlas &other ) :
            width( other.width ),
            height( other.height ),
            tileWidth( other.tileWidth ),
            rgba( makeSourceBudgetSnapshot( other.rgba ) ),
            views( other.views )
        {
        }

        MeshImposterAtlas &operator=( const MeshImposterAtlas &other )
        {
            if( this != &other )
            {
                MeshImposterAtlas candidate( other );
                swap( candidate );
            }
            return *this;
        }

        MeshImposterAtlas( MeshImposterAtlas && ) noexcept = default;
        MeshImposterAtlas &operator=( MeshImposterAtlas && ) noexcept = default;

        void swap( MeshImposterAtlas &other ) noexcept
        {
            using std::swap;
            swap( width, other.width );
            swap( height, other.height );
            swap( tileWidth, other.tileWidth );
            rgba.swap( other.rgba );
            views.swap( other.views );
        }

        u32 width = 0, height = 0, tileWidth = 0;
        Array<u8> rgba{ Allocator<u8>( sourceBudget ) };
        std::array<MeshImposterView, 3> views;
    };

    /// CPU orthographic rasterization of mesh triangles into two side views and a top view.
    /// Output is straight-alpha sRGB RGBA, suitable for depth-writing cutout materials.
    WPCore_API MeshImposterAtlas generateMeshImposters( const Array<MeshImposterTriangle> &triangles,
                                                        const Vector3F &boundsMin,
                                                        const Vector3F &boundsMax, u32 tileWidth = 128,
                                                        u32 tileHeight = 256 );
}  // namespace workphone
