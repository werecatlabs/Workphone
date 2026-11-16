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
        /// Sprite-sheet tile origin in pixels and inner UV rectangle (top-left origin).
        u32 tileX = 0, tileY = 0;
        Vector2F uvMinimum, uvMaximum;
        /// Front support plane in capture space: maximum dot(source bounds, towardCamera).
        /// Legacy crossed-card captures retain their original centre plane (zero).
        f32 maximumDepth = 0;
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

    struct MeshImposterSpriteSheetOptions
    {
        u32 tileWidth = 128, tileHeight = 128;
        u32 azimuthViews = 16, elevationRings = 3, columns = 7;
        u32 gutter = 2, supersampling = 2;
        u64 maximumRasterSamples = 128ull * 1024 * 1024;
    };

    /** A direction-indexed hemisphere atlas. Legacy crossed-card atlases retain
     * their original fixed-view API and exact allocation behavior. */
    struct WPCore_API MeshImposterSpriteSheet : public SourceBudgetObject
    {
        MeshImposterSpriteSheet() = default;
        MeshImposterSpriteSheet( const MeshImposterSpriteSheet &other ) :
            width( other.width ), height( other.height ), tileWidth( other.tileWidth ),
            tileHeight( other.tileHeight ), columns( other.columns ), rows( other.rows ),
            gutter( other.gutter ), rasterSamples( other.rasterSamples ),
            rgba( makeSourceBudgetSnapshot( other.rgba ) ),
            views( makeSourceBudgetSnapshot( other.views ) )
        {
        }
        MeshImposterSpriteSheet &operator=( const MeshImposterSpriteSheet &other )
        {
            if( this != &other )
            {
                MeshImposterSpriteSheet candidate( other );
                swap( candidate );
            }
            return *this;
        }
        MeshImposterSpriteSheet( MeshImposterSpriteSheet && ) noexcept = default;
        MeshImposterSpriteSheet &operator=( MeshImposterSpriteSheet && ) noexcept = default;
        void swap( MeshImposterSpriteSheet &other ) noexcept
        {
            using std::swap;
            swap( width, other.width ); swap( height, other.height );
            swap( tileWidth, other.tileWidth ); swap( tileHeight, other.tileHeight );
            swap( columns, other.columns ); swap( rows, other.rows );
            swap( gutter, other.gutter ); swap( rasterSamples, other.rasterSamples );
            rgba.swap( other.rgba ); views.swap( other.views );
        }
        u32 width = 0, height = 0, tileWidth = 0, tileHeight = 0;
        u32 columns = 0, rows = 0, gutter = 0;
        u64 rasterSamples = 0;
        Array<u8> rgba{ Allocator<u8>( sourceBudget ) };
        Array<MeshImposterView> views{ Allocator<MeshImposterView>( sourceBudget ) };
    };

    /** Bakes evenly spaced azimuths at elevation rings [0,90), followed by zenith.
     * +Z is the first azimuth; increasing azimuth turns toward +X. Output is
     * straight-alpha sRGB with transparent per-tile gutters. Invalid inputs throw
     * invalid_argument; bounded raster-work exhaustion throws runtime_error. */
    WPCore_API MeshImposterSpriteSheet bakeMeshImposterSpriteSheet(
        const Array<MeshImposterTriangle> &triangles, const Vector3F &boundsMin,
        const Vector3F &boundsMax, const MeshImposterSpriteSheetOptions &options = {} );

    /** Selects a frame from object-to-camera direction. Hysteresis is the minimum
     * cosine-score improvement needed to replace a valid previous frame. Invalid
     * or zero directions preserve a valid previous frame, otherwise return -1. */
    WPCore_API s32 selectMeshImposterView( const Array<MeshImposterView> &views,
        const Vector3F &towardCamera, s32 previous = -1, f32 hysteresis = .01f );
}  // namespace workphone
