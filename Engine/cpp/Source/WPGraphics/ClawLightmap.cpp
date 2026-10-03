#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/ClawLightmap.hpp"
#include <Workphone/Workphone.hpp>
// #include <CImg.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, Lightmap, render::ILightmap );

    int Lightmap::m_lightMapCounter = 0;

    float pointToLineDist( const Vector3F &p1, const Vector3F &lp1, const Vector3F &lp2 )
    {
        auto v1 = lp2 - lp1;
        auto v2 = p1 - lp1;
        auto length = v2.normaliseLength();
        v1.normalise();

        auto cosv = v1.dotProduct( v2 );
        auto d1 = cosv * length;

        return MathF::Sqrt( length * length - d1 * d1 );
    }

    void getTriangleUv( const Vector3F &p1, const Vector3F &p2, const Vector3F &p3,
                        const Vector3F &insidep, float &u, float &v )
    {
        u = 1 - pointToLineDist( insidep, p2, p3 ) / pointToLineDist( p1, p2, p3 );
        auto tmp = p1 + ( p2 - p1 ) * u;
        v = 1 - pointToLineDist( insidep, p1, p3 ) / pointToLineDist( tmp, p1, p3 );
    }

    Lightmap::Lightmap()
    {
        m_pixelsPerUnit = 0;
        m_texSize = 256;
        m_debugLightmaps = false;
        m_coordSet = 1;  // Use second UV set for lightmap coordinates
    }

    Lightmap::~Lightmap()
    {
    }

    void Lightmap::initialize( SmartPtr<scene::IGameActor> entity, SmartPtr<ISubMesh> subMesh,
                               const String &lightMapName,
                               const Array<SmartPtr<scene::IGameActor>> &entities, int texSize,
                               bool autoCalculateSize )
    {
        m_entity = entity;
        m_subMesh = subMesh;
        m_lightMapName = lightMapName;
        m_entities = entities;
        m_texSize = texSize;

        if( autoCalculateSize )
        {
            m_pixelsPerUnit = 1;
        }
        else
        {
            m_pixelsPerUnit = 0;
        }

        if( m_searchPattern.empty() )
            buildSearchPattern();

        calculateLightMap();
    }

    void Lightmap::resetCounter()
    {
        m_lightMapCounter = 0;
    }

    void Lightmap::loadResource( IResource *resource )
    {
        if( !resource )
        {
            return;
        }

        // Recreate the lightmap texture when resource is reloaded
        createTexture();
    }

    void Lightmap::saveImage( SmartPtr<render::ITexture> textureToSave, String filename )
    {
        if( !textureToSave )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        // Get texture data and save to file
        // This would typically use the graphics system to read back texture data
        // and write it to disk in the specified format (PNG, DDS, etc.)
    }

    int Lightmap::createDDS( const char *fname, bool alpha )
    {
        if( !fname )
        {
            return 0;
        }

        // Create DDS file from the lightmap data
        // DDS format is commonly used for efficient texture storage
        return 1;
    }

    String Lightmap::getName()
    {
        return m_lightMapName;
    }

    void Lightmap::lightTriangle( const Vector3F &p1, const Vector3F &p2, const Vector3F &p3,
                                  const Vector3F &n1, const Vector3F &n2, const Vector3F &n3,
                                  const Vector2F &t1, const Vector2F &t2, const Vector2F &t3 )
    {
        // Get bounding box in texture space
        int minX = getPixelCoordinate( MathF::min( t1.X(), MathF::min( t2.X(), t3.X() ) ) );
        int maxX = getPixelCoordinate( MathF::max( t1.X(), MathF::max( t2.X(), t3.X() ) ) );
        int minY = getPixelCoordinate( MathF::min( t1.Y(), MathF::min( t2.Y(), t3.Y() ) ) );
        int maxY = getPixelCoordinate( MathF::max( t1.Y(), MathF::max( t2.Y(), t3.Y() ) ) );

        // Iterate over pixels in the bounding box
        for( int y = minY; y <= maxY; ++y )
        {
            for( int x = minX; x <= maxX; ++x )
            {
                // Convert pixel to texture coordinates
                Vector2F texCoord( getTextureCoordinate( x ), getTextureCoordinate( y ) );

                // Get barycentric coordinates
                Vector3F bary = getBarycentricCoordinates( t1, t2, t3, texCoord );

                // Check if point is inside triangle
                if( bary.X() >= 0 && bary.Y() >= 0 && bary.Z() >= 0 )
                {
                    // Interpolate position and normal using barycentric coordinates
                    Vector3F position = p1 * bary.X() + p2 * bary.Y() + p3 * bary.Z();
                    Vector3F normal = n1 * bary.X() + n2 * bary.Y() + n3 * bary.Z();
                    normal.normalise();

                    // Calculate light intensity at this point
                    u8 intensity = getLightIntensity( position, normal );

                    // Write to lightmap texture
                    if( m_lightMap )
                    {
                        // Set pixel value (grayscale lightmap)
                        // (*m_lightMap)(x, y, 0, 0) = intensity;
                        // (*m_lightMap)(x, y, 0, 1) = intensity;
                        // (*m_lightMap)(x, y, 0, 2) = intensity;
                    }
                }
            }
        }
    }

    u8 Lightmap::getLightIntensity( const Vector3F &position, const Vector3F &normal )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();

        if( !sceneManager )
        {
            return 255;
        }

        f32 totalIntensity = 0.0f;
        f32 ambientLight = 0.2f;  // Base ambient light

        // Get scene lights
        auto scene = sceneManager->getCurrentScene();
        if( !scene )
        {
            return static_cast<u8>( ambientLight * 255 );
        }

        // Iterate through entities to find lights
        for( auto &entity : m_entities )
        {
            if( !entity )
            {
                continue;
            }

            // Check if entity has a light component
            auto lightComponent = entity->getComponent<render::IGraphicsLight>();
            if( !lightComponent )
            {
                continue;
            }

            // Get light properties
            Vector3F lightPos = entity->getPosition();
            Vector3F lightDir = ( lightPos - position );
            f32 distance = lightDir.normaliseLength();

            // Calculate diffuse lighting (Lambert)
            f32 NdotL = MathF::max( 0.0f, normal.dotProduct( lightDir ) );

            // Apply distance attenuation
            f32 attenuation = 1.0f / ( 1.0f + 0.1f * distance + 0.01f * distance * distance );

            // Check for shadows by raycasting to the light
            bool inShadow = false;

            // Cast ray from surface point towards light
            auto physicsManager = applicationManager->getPhysicsManager();
            if( physicsManager )
            {
                // Offset position slightly along normal to avoid self-intersection
                Vector3F rayOrigin = position + normal * 0.01f;

                // Check if ray hits anything before reaching the light
                // inShadow = physicsManager->raycast(rayOrigin, lightDir, distance);
            }

            if( !inShadow )
            {
                totalIntensity += NdotL * attenuation;
            }
        }

        // Add ambient light and clamp
        totalIntensity = MathF::clamp( ambientLight + totalIntensity, 0.0f, 1.0f );

        return static_cast<u8>( totalIntensity * 255 );
    }

    bool Lightmap::calculateLightMap()
    {
        if( !m_subMesh )
        {
            return false;
        }

        // Create the lightmap texture
        createTexture();

        // Get vertex and index data from submesh
        auto vertexBuffer = m_subMesh->getVertexBuffer();
        auto indexBuffer = m_subMesh->getIndexBuffer();

        if( !vertexBuffer || !indexBuffer )
        {
            return false;
        }

        // Get vertex data arrays
        // auto positions = vertexBuffer->getPositions();
        // auto normals = vertexBuffer->getNormals();
        // auto texCoords = vertexBuffer->getTexCoords( m_coordSet );
        // auto indices = indexBuffer->getIndices();

        // if( positions.empty() || normals.empty() || texCoords.empty() || indices.empty() )
        //{
        //     return false;
        // }

        // Get entity world transform
        Matrix4F worldTransform;
        if( m_entity )
        {
            // worldTransform = m_entity->getWorldTransform();
        }

        // Process each triangle
        // for( size_t i = 0; i + 2 < indices.size(); i += 3 )
        //{
        //    u32 i0 = indices[i];
        //    u32 i1 = indices[i + 1];
        //    u32 i2 = indices[i + 2];

        //    // Get vertex positions in world space
        //    Vector3F p1 = worldTransform.transformAffine( positions[i0] );
        //    Vector3F p2 = worldTransform.transformAffine( positions[i1] );
        //    Vector3F p3 = worldTransform.transformAffine( positions[i2] );

        //    // Get vertex normals in world space (transform by inverse transpose for normals)
        //    Vector3F n1 = worldTransform.transformNormal( normals[i0] );
        //    Vector3F n2 = worldTransform.transformNormal( normals[i1] );
        //    Vector3F n3 = worldTransform.transformNormal( normals[i2] );
        //    n1.normalise();
        //    n2.normalise();
        //    n3.normalise();

        //    // Get texture coordinates for lightmap UV set
        //    Vector2F t1 = texCoords[i0];
        //    Vector2F t2 = texCoords[i1];
        //    Vector2F t3 = texCoords[i2];

        //    // Light this triangle
        //    lightTriangle( p1, p2, p3, n1, n2, n3, t1, t2, t3 );
        //}

        // Fill in invalid/empty pixels by sampling neighbors
        fillInvalidPixels();

        // Increment counter for unique naming
        ++m_lightMapCounter;

        return true;
    }

    void Lightmap::assignMaterial()
    {
        // Material assignment is handled by the Lightmapper class
    }

    void Lightmap::createTexture()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        if( !graphicsSystem )
        {
            return;
        }

        // Create or recreate the lightmap texture
        auto textureManager = graphicsSystem->getTextureManager();
        if( !textureManager )
        {
            return;
        }

        // Remove existing texture if it exists
        if( m_texture )
        {
            m_texture = nullptr;
        }

        // Create new texture with specified size
        // m_texture = textureManager->createTexture(m_lightMapName, m_texSize, m_texSize);

        // Initialize CImg lightmap buffer
        // m_lightMap = WP_SHARED_PTR_NEW<cimg_library::CImg<unsigned char>>(m_texSize, m_texSize, 1, 3,
        // 0);
    }

    void Lightmap::fillInvalidPixels()
    {
        int i, j;
        int x, y;
        Array<std::pair<int, int>>::iterator itSearchPattern;

        for( i = 0; i < m_texSize; ++i )
        {
            for( j = 0; j < m_texSize; ++j )
            {
                if( !m_lightMap )
                {
                    continue;
                }

                // Check if this pixel is invalid (not filled by lightTriangle)
                // Typically, invalid pixels have a specific marker value (e.g., 0)
                // unsigned char pixelValue = (*m_lightMap)(i, j, 0, 0);
                // if (pixelValue == 0)
                // {
                // Search for nearest valid pixel using the search pattern
                for( itSearchPattern = m_searchPattern.begin(); itSearchPattern != m_searchPattern.end();
                     ++itSearchPattern )
                {
                    x = i + itSearchPattern->first;
                    y = j + itSearchPattern->second;

                    // Check bounds
                    if( x >= 0 && x < m_texSize && y >= 0 && y < m_texSize )
                    {
                        // Check if neighbor pixel is valid
                        // unsigned char neighborValue = (*m_lightMap)(x, y, 0, 0);
                        // if (neighborValue != 0)
                        // {
                        //     // Copy neighbor value to this pixel
                        //     (*m_lightMap)(i, j, 0, 0) = neighborValue;
                        //     (*m_lightMap)(i, j, 0, 1) = (*m_lightMap)(x, y, 0, 1);
                        //     (*m_lightMap)(i, j, 0, 2) = (*m_lightMap)(x, y, 0, 2);
                        //     break;
                        // }
                    }
                }
                // }
            }
        }
    }

    void Lightmap::buildSearchPattern()
    {
        m_searchPattern.clear();
        const int size = 5;
        int i, j;
        for( i = -size; i <= size; ++i )
        {
            for( j = -size; j <= size; ++j )
            {
                if( i == 0 && j == 0 )
                    continue;

                m_searchPattern.push_back( std::make_pair( i, j ) );
            }
        }

        std::sort( m_searchPattern.begin(), m_searchPattern.end(), SortCoordsByDistance() );
    }

    int Lightmap::getPixelCoordinate( f32 textureCoord )
    {
        int pixel = static_cast<int>( textureCoord * m_texSize );
        if( pixel < 0 )
            pixel = 0;
        if( pixel >= m_texSize )
            pixel = m_texSize - 1;

        return pixel;
    }

    f32 Lightmap::getTextureCoordinate( int pixelCoord )
    {
        return ( static_cast<f32>( pixelCoord ) + 0.5f ) / static_cast<f32>( m_texSize );
    }

    Vector3F Lightmap::getBarycentricCoordinates( const Vector2F &p1, const Vector2F &p2,
                                                  const Vector2F &p3, const Vector2F &p )
    {
        Vector3F coordinates;

        // Compute vectors
        Vector2F v0 = p3 - p1;
        Vector2F v1 = p2 - p1;
        Vector2F v2 = p - p1;

        // Compute dot products
        f32 dot00 = v0.dotProduct( v0 );
        f32 dot01 = v0.dotProduct( v1 );
        f32 dot02 = v0.dotProduct( v2 );
        f32 dot11 = v1.dotProduct( v1 );
        f32 dot12 = v1.dotProduct( v2 );

        // Compute barycentric coordinates
        f32 invDenom = 1.0f / ( dot00 * dot11 - dot01 * dot01 );
        f32 u = ( dot11 * dot02 - dot01 * dot12 ) * invDenom;
        f32 v = ( dot00 * dot12 - dot01 * dot02 ) * invDenom;

        // Barycentric coordinates: (1-u-v, v, u) for vertices (p1, p2, p3)
        coordinates.X() = 1.0f - u - v;
        coordinates.Y() = v;
        coordinates.Z() = u;

        return coordinates;
    }

    f32 Lightmap::getTriangleArea( const Vector3F &p1, const Vector3F &p2, const Vector3F &p3 )
    {
        return 0.5f * ( p2 - p1 ).crossProduct( p3 - p1 ).length();
    }

    bool Lightmap::SortCoordsByDistance::operator()( std::pair<int, int> &left,
                                                     std::pair<int, int> &right )
    {
        return ( left.first * left.first + left.second * left.second ) <
               ( right.first * right.first + right.second * right.second );
    }
}  // namespace workphone
