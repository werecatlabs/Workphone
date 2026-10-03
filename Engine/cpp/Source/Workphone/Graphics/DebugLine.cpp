#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/DebugLine.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, DebugLine, IDebugLine );
    u32 DebugLine::m_nameExt = 0;

    DebugLine::DebugLine() = default;

    DebugLine::~DebugLine() = default;

    void DebugLine::load( SmartPtr<ISharedObject> data )
    {
        // Setup Ogre manual object and scene resources
        // This will be called when the debug line is added to the rendering system
    }

    void DebugLine::unload( SmartPtr<ISharedObject> data )
    {
        // Cleanup Ogre resources and scene nodes
        // This will be called when the debug line is removed from the rendering system
    }

    void DebugLine::update()
    {
        // Update lifetime counter
        if( m_maxLifeTime > 0.0 )
        {
            //m_lifeTime += getUpdateFrequency();
            //if( m_lifeTime >= m_maxLifeTime )
            //{
            //    setVisible( false );
            //}
        }

        // Rebuild geometry if dirty flag is set
        if( isDirty() )
        {
            // Rebuild Ogre manual object geometry
            setDirty( false );
        }
    }

    Vector3<real_Num> DebugLine::getVector() const
    {
        return m_vector;
    }

    void DebugLine::setVector( const Vector3<real_Num> &vector )
    {
        m_vector = vector;
        setDirty( true );
    }

    Vector3<real_Num> DebugLine::getPosition() const
    {
        return m_position;
    }

    void DebugLine::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;
        setDirty( true );
    }

    f64 DebugLine::getLifeTime() const
    {
        return m_lifeTime;
    }

    void DebugLine::setLifeTime( f64 lifeTime )
    {
        m_lifeTime = lifeTime;
    }

    f64 DebugLine::getMaxLifeTime() const
    {
        return m_maxLifeTime;
    }

    void DebugLine::setMaxLifeTime( f64 maxLifeTime )
    {
        m_maxLifeTime = maxLifeTime;
    }

    bool DebugLine::isVisible() const
    {
        return m_isVisible.load();
    }

    void DebugLine::setVisible( bool visible )
    {
        m_isVisible.store( visible );
    }

    String DebugLine::getMaterialName() const
    {
        return m_materialName;
    }

    void DebugLine::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
        setDirty( true );
    }

    u32 DebugLine::getColour() const
    {
        return m_colour.load();
    }

    void DebugLine::setColour( u32 colour )
    {
        m_colour.store( colour );
        setDirty( true );
    }

    bool DebugLine::isDirty() const
    {
        return m_isDirty.load();
    }

    void DebugLine::setDirty( bool dirty )
    {
        m_isDirty.store( dirty );
    }

    void DebugLine::lock()
    {
        // Implement mutual exclusion lock for thread-safe access
    }

    bool DebugLine::try_lock()
    {
        // Attempt to acquire lock without blocking
        return true;
    }

    void DebugLine::unlock()
    {
        // Release the lock
    }

}  // namespace workphone::render
