#ifndef __Lightmap_H__
#define __Lightmap_H__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/ILightmap.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace cimg_library
{
    template <typename T>
    struct CImg;
};

namespace workphone
{
    using CImgPtr = SharedPtr<cimg_library::CImg<unsigned char>>;

    class WPGraphics_API Lightmap : public render::ILightmap
    {
    public:
        Lightmap();

        ~Lightmap() override;

        void initialize( SmartPtr<scene::IGameActor> entity, SmartPtr<ISubMesh> subMesh,
                         const String &lightMapName, const Array<SmartPtr<scene::IGameActor>> &entities,
                         int texSize, bool autoCalculateSize );

        void loadResource( IResource *resource );

        String getName();

        static void resetCounter();

        struct SortCoordsByDistance
        {
            bool operator()( std::pair<int, int> &left, std::pair<int, int> &right );
        };

        WP_CLASS_REGISTER_DECL;

    protected:
        void saveImage( SmartPtr<render::ITexture> textureToSave, String filename );

        int createDDS( const char *fname, bool alpha );

        void lightTriangle( const Vector3F &p1, const Vector3F &p2, const Vector3F &p3,
                            const Vector3F &n1, const Vector3F &n2, const Vector3F &n3,
                            const Vector2F &t1, const Vector2F &t2, const Vector2F &t3 );

        u8 getLightIntensity( const Vector3F &position, const Vector3F &normal );

        bool calculateLightMap();

        void assignMaterial();

        void createTexture();

        void fillInvalidPixels();

        void buildSearchPattern();

        /// Convert between texture coordinates given as reals and pixel coordinates given as integers
        int getPixelCoordinate( f32 textureCoord );

        f32 getTextureCoordinate( int pixelCoord );

        /// Calculate coordinates of P in terms of P1, P2 and P3
        /// P = x*P1 + y*P2 + z*P3
        /// If any of P.x, P.y or P.z are negative then P is outside of the triangle
        Vector3F getBarycentricCoordinates( const Vector2F &p1, const Vector2F &p2, const Vector2F &p3,
                                            const Vector2F &p );

        /// Get the surface area of a triangle
        f32 getTriangleArea( const Vector3F &p1, const Vector3F &p2, const Vector3F &p3 );

        SmartPtr<scene::IGameActor> m_entity;

        SmartPtr<ISubMesh> m_subMesh;
        SmartPtr<render::ITexture> m_texture;
        CImgPtr m_lightMap;

        s32 m_texSize;
        s32 m_coordSet;

        f32 m_pixelsPerUnit;

        bool m_debugLightmaps;

        String m_lightMapName;

        Array<SmartPtr<scene::IGameActor>> m_entities;
        Array<std::pair<s32, s32>> m_searchPattern;

        static s32 m_lightMapCounter;
    };
}  // namespace workphone

#endif
