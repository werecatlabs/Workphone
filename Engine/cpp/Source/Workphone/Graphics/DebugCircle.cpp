#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/DebugCircle.hpp>

namespace workphone::render
{
    DebugCircle::DebugCircle() = default;

    DebugCircle::~DebugCircle() = default;

    void DebugCircle::load( SmartPtr<ISharedObject> data )
    {
        // Setup Ogre manual object and scene resources
        // This will be called when the debug circle is added to the rendering system
    }

    void DebugCircle::unload( SmartPtr<ISharedObject> data )
    {
        // Cleanup Ogre resources and scene nodes
        // This will be called when the debug circle is removed from the rendering system
    }

    void DebugCircle::update()
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
            // Rebuild Ogre manual object geometry with circle segments
            setDirty( false );
        }
    }

    f64 DebugCircle::getLifeTime() const
    {
        return m_lifeTime;
    }

    void DebugCircle::setLifeTime( f64 lifeTime )
    {
        m_lifeTime = lifeTime;
    }

    f64 DebugCircle::getMaxLifeTime() const
    {
        return m_maxLifeTime;
    }

    void DebugCircle::setMaxLifeTime( f64 maxLifeTime )
    {
        m_maxLifeTime = maxLifeTime;
    }

    bool DebugCircle::isVisible() const
    {
        return m_isVisible;
    }

    void DebugCircle::setVisible( bool visible )
    {
        m_isVisible = visible;
    }

    void DebugCircle::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;
        setDirty( true );
    }

    void DebugCircle::setRadius( real_Num radius )
    {
        m_radius = radius;
        setDirty( true );
    }

    void DebugCircle::setColor( u32 color )
    {
        m_colour = color;
        setDirty( true );
    }

    Vector3<real_Num> DebugCircle::getPosition() const
    {
        return m_position;
    }

    real_Num DebugCircle::getRadius() const
    {
        return m_radius;
    }

    u32 DebugCircle::getColor() const
    {
        return m_colour;
    }

    String DebugCircle::getMaterialName() const
    {
        return m_materialName;
    }

    void DebugCircle::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
        setDirty( true );
    }

    bool DebugCircle::isDirty() const
    {
        return m_dirty;
    }

    void DebugCircle::setDirty( bool dirty )
    {
        m_dirty = dirty;
    }

    Quaternion<real_Num> DebugCircle::getOrientation() const
    {
        return m_orientation;
    }

    void DebugCircle::setOrientation( const Quaternion<real_Num> &orientation )
    {
        m_orientation = orientation;
        setDirty( true );
    }

}  // namespace workphone::render
