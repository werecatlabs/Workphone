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

    SmartPtr<IAnimation> AnimationInterface::createAnimation( const String &name, f32 length )
    {
        auto anim = workphone::make_ptr<Animation>();
        anim->setLength( length );
        m_animations[name] = anim;
        m_animationNames.push_back( name );
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
