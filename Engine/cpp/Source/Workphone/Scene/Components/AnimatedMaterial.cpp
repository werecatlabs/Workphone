#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/AnimatedMaterial.hpp>
#include <Workphone/Animation/Animator.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Animation/IAnimator.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, AnimatedMaterial, Component );

    const String AnimatedMaterial::materialNameStr = String( "materialName" );
    const String AnimatedMaterial::animatorStr = String( "animator" );

    AnimatedMaterial::AnimatedMaterial()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        m_animator = factoryManager->make_ptr<Animator>();
    }

    AnimatedMaterial::~AnimatedMaterial()
    {
    }

    void AnimatedMaterial::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            if( m_animator )
            {
                m_animator->load( data );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AnimatedMaterial::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                if( m_animator )
                {
                    m_animator->unload( data );
                    m_animator = nullptr;
                }

                Component::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AnimatedMaterial::update()
    {
        if( !isEnabled() || !m_animator )
        {
            return;
        }

        if( auto animator = workphone::dynamic_pointer_cast<Animator>( m_animator ) )
        {
            animator->update();
        }
    }

    void AnimatedMaterial::play()
    {
        if( m_animator )
        {
            m_animator->start();
        }
    }

    void AnimatedMaterial::pause()
    {
        if( auto animator = workphone::dynamic_pointer_cast<Animator>( m_animator ) )
        {
            animator->pause();
        }
    }

    void AnimatedMaterial::stop()
    {
        if( m_animator )
        {
            m_animator->stop();
        }
    }

    bool AnimatedMaterial::isPlaying() const
    {
        if( auto animator = workphone::dynamic_pointer_cast<Animator>( m_animator ) )
        {
            return animator->isPlaying();
        }

        return false;
    }

    String AnimatedMaterial::getMaterialName() const
    {
        return m_materialName;
    }

    void AnimatedMaterial::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
    }

    SmartPtr<Properties> AnimatedMaterial::getProperties() const
    {
        auto properties = Component::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( materialNameStr, m_materialName );
        properties->setPropertyAsType( animatorStr, m_animator );

        return properties;
    }

    void AnimatedMaterial::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );
        if( !properties )
        {
            return;
        }

        properties->getPropertyValue( materialNameStr, m_materialName );

        SmartPtr<IAnimator> animator;
        properties->getPropertyAsType( animatorStr, animator );
        if( animator )
        {
            setAnimator( animator );
        }
    }

    SmartPtr<IAnimator> AnimatedMaterial::getAnimator() const
    {
        return m_animator;
    }

    void AnimatedMaterial::setAnimator( SmartPtr<IAnimator> animator )
    {
        m_animator = animator;
    }

    Array<SmartPtr<ISharedObject>> AnimatedMaterial::getChildObjects() const
    {
        Array<SmartPtr<ISharedObject>> objects;
        objects.reserve( 1 );

        if( m_animator )
        {
            objects.push_back( m_animator );
        }

        return objects;
    }

}  // namespace workphone::scene
