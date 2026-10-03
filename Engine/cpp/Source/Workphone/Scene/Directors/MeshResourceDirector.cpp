#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/MeshResourceDirector.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, MeshResourceDirector, ResourceDirector );

    const String MeshResourceDirector::scaleStr = String( "scale" );
    const String MeshResourceDirector::constraintsStr = String( "constraints" );
    const String MeshResourceDirector::animationStr = String( "animation" );
    const String MeshResourceDirector::visibilityStr = String( "visibility" );
    const String MeshResourceDirector::camerasStr = String( "cameras" );
    const String MeshResourceDirector::swapZYStr = String( "swapZY" );
    const String MeshResourceDirector::rotate90DegreesXStr = String( "rotate90DegreesX" );
    const String MeshResourceDirector::lightsStr = String( "lights" );
    const String MeshResourceDirector::lightmapUVsStr = String( "lightmapUVs" );
    const String MeshResourceDirector::generateNormalsStr = String( "generateNormals" );
    const String MeshResourceDirector::generateSmoothNormalsStr = String( "generateSmoothNormals" );
    const String MeshResourceDirector::generateTangentsStr = String( "generateTangents" );
    const String MeshResourceDirector::genUVCoordsStr = String( "genUVCoords" );
    const String MeshResourceDirector::triangulateStr = String( "triangulate" );
    const String MeshResourceDirector::joinIdenticalVerticesStr = String( "joinIdenticalVertices" );
    const String MeshResourceDirector::hasSharedVertexDataStr = String( "hasSharedVertexData" );
    const String MeshResourceDirector::useMeshInstancingStr = String( "useMeshInstancing" );
    const String MeshResourceDirector::progressiveMeshOptionsStr = String( "progressiveMeshOptions" );

    MeshResourceDirector::MeshResourceDirector() = default;

    MeshResourceDirector::~MeshResourceDirector() = default;

    SmartPtr<Properties> MeshResourceDirector::getProperties() const
    {
        auto properties = ResourceDirector::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( scaleStr, m_scale );
        properties->setProperty( constraintsStr, m_constraints );
        properties->setProperty( animationStr, m_animation );
        properties->setProperty( visibilityStr, m_visibility );
        properties->setProperty( camerasStr, m_cameras );
        properties->setProperty( swapZYStr, m_swapZY );
        properties->setProperty( rotate90DegreesXStr, m_rotate90DegreesX );
        properties->setProperty( lightsStr, m_lights );
        properties->setProperty( lightmapUVsStr, m_lightmapUVs );
        properties->setProperty( generateNormalsStr, m_genNormals );
        properties->setProperty( generateSmoothNormalsStr, m_genSmoothNormals );
        properties->setProperty( generateTangentsStr, m_genTangents );
        properties->setProperty( genUVCoordsStr, m_genUVCoords );
        properties->setProperty( triangulateStr, m_triangulate );
        properties->setProperty( joinIdenticalVerticesStr, m_joinIdenticalVertices );
        properties->setProperty( hasSharedVertexDataStr, m_hasSharedVertexData );
        properties->setProperty( useMeshInstancingStr, m_useMeshInstancing );
        properties->addChild( m_progressiveMeshOptions.toProperties( progressiveMeshOptionsStr ) );

        return properties;
    }

    void MeshResourceDirector::setProperties( SmartPtr<Properties> properties )
    {
        ResourceDirector::setProperties( properties );
        if( !properties )
        {
            return;
        }

        auto scale = getScale();
        auto constraints = getConstraints();
        auto animation = getAnimation();
        auto visibility = getVisibility();
        auto cameras = getCameras();
        auto swapZY = getSwapZY();
        auto rotate90DegreesX = getRotate90DegreesX();
        auto lights = getLights();
        auto lightmapUVs = getLightmapUVs();
        auto genNormals = getGenNormals();
        auto genSmoothNormals = getGenSmoothNormals();
        auto genTangents = getGenTangents();
        auto genUVCoords = getGenUVCoords();
        auto triangulate = getTriangulate();
        auto joinIdenticalVertices = getJoinIdenticalVertices();
        auto sharedVertexData = hasSharedVertexData();
        auto useMeshInstancing = getUseMeshInstancing();

        properties->getPropertyValue( scaleStr, scale );
        properties->getPropertyValue( constraintsStr, constraints );
        properties->getPropertyValue( animationStr, animation );
        properties->getPropertyValue( visibilityStr, visibility );
        properties->getPropertyValue( camerasStr, cameras );
        properties->getPropertyValue( swapZYStr, swapZY );
        properties->getPropertyValue( rotate90DegreesXStr, rotate90DegreesX );
        properties->getPropertyValue( lightsStr, lights );
        properties->getPropertyValue( lightmapUVsStr, lightmapUVs );
        properties->getPropertyValue( generateNormalsStr, genNormals );
        properties->getPropertyValue( generateSmoothNormalsStr, genSmoothNormals );
        properties->getPropertyValue( generateTangentsStr, genTangents );
        properties->getPropertyValue( genUVCoordsStr, genUVCoords );
        properties->getPropertyValue( triangulateStr, triangulate );
        properties->getPropertyValue( joinIdenticalVerticesStr, joinIdenticalVertices );
        properties->getPropertyValue( hasSharedVertexDataStr, sharedVertexData );
        properties->getPropertyValue( useMeshInstancingStr, useMeshInstancing );

        const auto progressiveMeshProperties =
            properties->getChildrenByName( progressiveMeshOptionsStr );
        if( !progressiveMeshProperties.empty() )
        {
            m_progressiveMeshOptions.fromProperties( progressiveMeshProperties.front() );
        }

        setScale( scale );
        setConstraints( constraints );
        setAnimation( animation );
        setVisibility( visibility );
        setCameras( cameras );
        setSwapZY( swapZY );
        setRotate90DegreesX( rotate90DegreesX );
        setLights( lights );
        setLightmapUVs( lightmapUVs );
        setGenNormals( genNormals );
        setGenSmoothNormals( genSmoothNormals );
        setGenTangents( genTangents );
        setGenUVCoords( genUVCoords );
        setTriangulate( triangulate );
        setJoinIdenticalVertices( joinIdenticalVertices );
        setHasSharedVertexData( sharedVertexData );
        setUseMeshInstancing( useMeshInstancing );
    }

    void MeshResourceDirector::setLightmapUVs( bool lightmapUVs )
    {
        m_lightmapUVs = lightmapUVs;
    }

    bool MeshResourceDirector::getLightmapUVs() const
    {
        return m_lightmapUVs;
    }

    void MeshResourceDirector::setLights( bool lights )
    {
        m_lights = lights;
    }

    bool MeshResourceDirector::getLights() const
    {
        return m_lights;
    }

    void MeshResourceDirector::setSwapZY( bool swapZY )
    {
        m_swapZY = swapZY;
    }

    bool MeshResourceDirector::getSwapZY() const
    {
        return m_swapZY;
    }

    void MeshResourceDirector::setCameras( bool cameras )
    {
        m_cameras = cameras;
    }

    bool MeshResourceDirector::getCameras() const
    {
        return m_cameras;
    }

    void MeshResourceDirector::setVisibility( bool visibility )
    {
        m_visibility = visibility;
    }

    bool MeshResourceDirector::getVisibility() const
    {
        return m_visibility;
    }

    void MeshResourceDirector::setAnimation( bool animation )
    {
        m_animation = animation;
    }

    bool MeshResourceDirector::getAnimation() const
    {
        return m_animation;
    }

    void MeshResourceDirector::setConstraints( bool constraints )
    {
        m_constraints = constraints;
    }

    bool MeshResourceDirector::getConstraints() const
    {
        return m_constraints;
    }

    void MeshResourceDirector::setScale( f32 scale )
    {
        m_scale = scale;
    }

    f32 MeshResourceDirector::getScale() const
    {
        return m_scale;
    }

    void MeshResourceDirector::setRotate90DegreesX( bool rotate90DegreesX )
    {
        m_rotate90DegreesX = rotate90DegreesX;
    }

    bool MeshResourceDirector::getRotate90DegreesX() const
    {
        return m_rotate90DegreesX;
    }

    void MeshResourceDirector::setTriangulate( bool triangulate )
    {
        m_triangulate = triangulate;
    }

    bool MeshResourceDirector::getTriangulate() const
    {
        return m_triangulate;
    }

    void MeshResourceDirector::setGenUVCoords( bool genUVCoords )
    {
        m_genUVCoords = genUVCoords;
    }

    bool MeshResourceDirector::getGenUVCoords() const
    {
        return m_genUVCoords;
    }

    void MeshResourceDirector::setGenTangents( bool genTangents )
    {
        m_genTangents = genTangents;
    }

    bool MeshResourceDirector::getGenTangents() const
    {
        return m_genTangents;
    }

    void MeshResourceDirector::setGenSmoothNormals( bool genSmoothNormals )
    {
        m_genSmoothNormals = genSmoothNormals;
    }

    bool MeshResourceDirector::getGenSmoothNormals() const
    {
        return m_genSmoothNormals;
    }

    void MeshResourceDirector::setGenNormals( bool genNormals )
    {
        m_genNormals = genNormals;
    }

    bool MeshResourceDirector::getGenNormals() const
    {
        return m_genNormals;
    }

    void MeshResourceDirector::setJoinIdenticalVertices( bool joinIdenticalVertices )
    {
        m_joinIdenticalVertices = joinIdenticalVertices;
    }

    bool MeshResourceDirector::getJoinIdenticalVertices() const
    {
        return m_joinIdenticalVertices;
    }

    void MeshResourceDirector::setHasSharedVertexData( bool hasSharedVertexData )
    {
        m_hasSharedVertexData = hasSharedVertexData;
    }

    bool MeshResourceDirector::hasSharedVertexData() const
    {
        return m_hasSharedVertexData;
    }

    void MeshResourceDirector::setUseMeshInstancing( bool useMeshInstancing )
    {
        m_useMeshInstancing = useMeshInstancing;
    }

    bool MeshResourceDirector::getUseMeshInstancing() const
    {
        return m_useMeshInstancing;
    }

    const ProgressiveMeshOptions &MeshResourceDirector::getProgressiveMeshOptions() const
    {
        return m_progressiveMeshOptions;
    }

    void MeshResourceDirector::setProgressiveMeshOptions( const ProgressiveMeshOptions &options )
    {
        m_progressiveMeshOptions = options;
    }

}  // namespace workphone::scene
