#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsCubemap.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsCubemap,
                               SharedGraphicsObject<IGraphicsCubemap> );

    GraphicsCubemap::GraphicsCubemap()
    {
    }

    GraphicsCubemap::~GraphicsCubemap()
    {
        // Destructor implementation
    }

    String GraphicsCubemap::getTextureName() const
    {
        return m_textureName;
    }

    void GraphicsCubemap::setTextureName( const String &textureName )
    {
        m_textureName = textureName;
    }

    SmartPtr<IGraphicsScene> GraphicsCubemap::getSceneManager() const
    {
        return m_sceneManager;
    }

    void GraphicsCubemap::setSceneManager( SmartPtr<IGraphicsScene> smgr )
    {
        m_sceneManager = smgr;
    }

    u32 GraphicsCubemap::getVisibilityMask() const
    {
        return m_visibilityMask;
    }

    void GraphicsCubemap::setVisibilityMask( u32 visibilityMask )
    {
        m_visibilityMask = visibilityMask;
    }

    u32 GraphicsCubemap::getExclusionMask() const
    {
        return m_exclusionMask;
    }

    void GraphicsCubemap::setExclusionMask( u32 exclusionMask )
    {
        m_exclusionMask = exclusionMask;
    }

    Vector3<real_Num> GraphicsCubemap::getPosition() const
    {
        return m_position;
    }

    void GraphicsCubemap::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;
    }

    bool GraphicsCubemap::getEnable() const
    {
        return m_enable;
    }

    void GraphicsCubemap::setEnable( bool enable )
    {
        m_enable = enable;
    }

    u32 GraphicsCubemap::getUpdateInterval() const
    {
        return m_updateInterval;
    }

    void GraphicsCubemap::setUpdateInterval( u32 milliseconds )
    {
        m_updateInterval = milliseconds;
    }

    void GraphicsCubemap::addExcludedObject( SmartPtr<IGraphicsObject> object )
    {
        if( object )
        {
            m_excludedObjects.push_back( object );
        }
    }

    Array<SmartPtr<IGraphicsObject>> GraphicsCubemap::getExcludedObjects() const
    {
        return m_excludedObjects;
    }
}  // namespace workphone::render
