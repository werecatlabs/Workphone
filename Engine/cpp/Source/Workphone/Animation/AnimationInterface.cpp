#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationInterface.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, AnimationInterface, IAnimationInterface );

    AnimationInterface::AnimationInterface() = default;
    AnimationInterface::~AnimationInterface() = default;

    void AnimationInterface::unload( SmartPtr<ISharedObject> data )
    {
        if( m_unloading )
            return;
        m_unloading = true;
        struct ResetFlag
        {
            bool &value;
            ~ResetFlag() { value = false; }
        } reset{ m_unloading };
        // Releasing an inventory must work at the cap. In particular, do not
        // construct a second Source map: MSVC allocates its empty sentinel.
        m_animationNames = makeSourceBudgetSnapshot<String>();
        while( !m_animations.empty() )
        {
            auto node = m_animations.extract( m_animations.begin() );
            auto animation = node.mapped();
            node.mapped() = nullptr;
            if( animation && animation->getReferences() == 1 && !animation->getWeakReferences() )
            {
                try { animation->unload( data ); }
                catch( const std::bad_alloc & ) { /* optional unload notification */ }
            }
        }
    }

    SmartPtr<IAnimation> AnimationInterface::createAnimation( const String &name, f32 length )
    {
        if( m_unloading )
            throw std::bad_alloc();
        auto anim = workphone::make_ptr<Animation>();
        anim->setName( name );
        anim->setLength( length );

        const auto existing = m_animations.find( name );
        if( existing != m_animations.end() )
        {
            existing->second = anim;
            return anim;
        }

        String stagedName( name );
        if( m_animationNames.size() == m_animationNames.capacity() )
        {
            auto names = makeSourceBudgetSnapshot( m_animationNames );
            names.push_back( std::move( stagedName ) );
            m_animations.emplace( name, anim );
            m_animationNames.swap( names );
        }
        else
        {
            // All throwing work precedes publication; the final in-capacity
            // String move cannot allocate after the map node becomes visible.
            m_animations.emplace( name, anim );
            m_animationNames.push_back( std::move( stagedName ) );
        }
        return anim;
    }

    SmartPtr<IAnimation> AnimationInterface::getAnimation( const String &name ) const
    {
        auto it = m_animations.find( name );
        if( it != m_animations.end() )
            return it->second;
        return nullptr;
    }

    bool AnimationInterface::hasAnimation( const String &name ) const
    {
        return m_animations.find( name ) != m_animations.end();
    }

    void AnimationInterface::removeAnimation( const String &name )
    {
        m_animations.erase( name );
        m_animationNames.erase( std::remove( m_animationNames.begin(), m_animationNames.end(), name ),
                                m_animationNames.end() );
    }

    u16 AnimationInterface::getNumAnimations() const
    {
        return static_cast<u16>( m_animations.size() );
    }

    SmartPtr<IAnimation> AnimationInterface::getAnimation( u16 index ) const
    {
        if( index < m_animationNames.size() )
        {
            const String &name = m_animationNames[index];
            return getAnimation( name );
        }
        return nullptr;
    }

    void *AnimationInterface::getVertexDataByTrackHandle( u16 handle )
    {
        // TODO: Implement vertex data lookup. This requires access to the mesh's vertex buffers,
        // which are not currently managed by AnimationInterface.
        return nullptr;
    }

    SmartPtr<IAnimationInterface> AnimationInterface::clone()
    {
        auto cloned = workphone::make_ptr<AnimationInterface>();
        cloned->m_animations = m_animations;
        cloned->m_animationNames = m_animationNames;
        return cloned;
    }
}  // namespace workphone
