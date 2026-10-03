#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Animation.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Animation, SubComponent );

    const String Animation::nameStr = String( "name" );
    const String Animation::lengthStr = String( "length" );
    const String Animation::loopingStr = String( "looping" );
    const String Animation::speedStr = String( "speed" );

    Animation::Animation() = default;

    Animation::~Animation() = default;

    void Animation::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            SubComponent::load( data );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Animation::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );
                SubComponent::unload( data );
                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> Animation::getProperties() const
    {
        auto properties = SubComponent::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( nameStr, m_name );
        properties->setProperty( lengthStr, m_length );
        properties->setProperty( loopingStr, m_looping );
        properties->setProperty( speedStr, m_speed );

        return properties;
    }

    void Animation::setProperties( SmartPtr<Properties> properties )
    {
        SubComponent::setProperties( properties );
        if( !properties )
        {
            return;
        }

        properties->getPropertyValue( nameStr, m_name );
        properties->getPropertyValue( lengthStr, m_length );
        properties->getPropertyValue( loopingStr, m_looping );
        properties->getPropertyValue( speedStr, m_speed );
    }

    String Animation::getName() const
    {
        return m_name;
    }

    void Animation::setName( const String &name )
    {
        m_name = name;
    }

    f32 Animation::getLength() const
    {
        return m_length;
    }

    void Animation::setLength( f32 length )
    {
        m_length = length;
    }

    bool Animation::isLooping() const
    {
        return m_looping;
    }

    void Animation::setLooping( bool looping )
    {
        m_looping = looping;
    }

    f32 Animation::getSpeed() const
    {
        return m_speed;
    }

    void Animation::setSpeed( f32 speed )
    {
        m_speed = speed;
    }
}  // namespace workphone::scene
