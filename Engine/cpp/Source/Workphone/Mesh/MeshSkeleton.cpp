#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Mesh/Bone.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, MeshSkeleton, ISkeleton );

    namespace
    {
        template <class T>
        SmartPtr<T> makeSkeletonObject()
        {
            auto application = core::IApplicationManager::instance();
            auto factory = application ? application->getFactoryManager() : nullptr;
            if( !factory )
            {
                throw std::runtime_error( "Factory manager not available" );
            }
            const auto denials = sourceBudgetDenialCount();
            auto result = factory->make_ptr<T>();
            if( !result )
            {
                if( sourceBudgetDenialCount() != denials )
                {
                    throw std::bad_alloc();
                }
                throw std::runtime_error( "Failed to create skeleton object" );
            }
            return result;
        }

        template <class T, class Map>
        void appendNamed( Array<SmartPtr<T>> &objects, Map &index, const String &name,
                          const SmartPtr<T> &object )
        {
            // Index insertion and Array growth each have a strong guarantee.
            // Until append succeeds, a new node is private to this serialized
            // owner operation and is erased on failure. Existing aliases are
            // replaced only after the owner reference is safely retained.
            const auto entry = index.try_emplace( name, object );
            try
            {
                objects.push_back( object );
            }
            catch( ... )
            {
                if( entry.second )
                {
                    index.erase( entry.first );
                }
                throw;
            }
            if( !entry.second )
            {
                entry.first->second = object;
            }
        }
    }  // namespace

    MeshSkeleton::MeshSkeleton() :
        m_animations( Allocator<SmartPtr<IAnimation>>( sourceBudget ) ),
        m_bones( Allocator<SmartPtr<IBone>>( sourceBudget ) )
    {
    }

    MeshSkeleton::~MeshSkeleton()
    {
        unload( nullptr );
    }

    void MeshSkeleton::unload( SmartPtr<ISharedObject> data )
    {
        auto animations = makeSourceBudgetSnapshot<SmartPtr<IAnimation>>();
        auto bones = makeSourceBudgetSnapshot<SmartPtr<IBone>>();
        animations.swap( m_animations );
        bones.swap( m_bones );
        auto linker = m_linker;
        m_linker = nullptr;
        // Every published owner inventory is detached before final references
        // can invoke callbacks through map clearing or local retirement.
        m_animationsByName.clear();
        m_bonesByName.clear();
    }

    SmartPtr<IAnimation> MeshSkeleton::createAnimation( const String &name, f32 length )
    {
        if( auto existing = getAnimation( name ) )
        {
            return existing;
        }
        auto animation = makeSkeletonObject<Animation>();
        animation->setLength( length );
        animation->setName( name );
        appendNamed( m_animations, m_animationsByName, name, SmartPtr<IAnimation>( animation ) );
        return animation;
    }

    SmartPtr<IAnimation> MeshSkeleton::getAnimation( const String &name,
                                                     const LinkedSkeletonAnimationSource **linker ) const
    {
        if( linker )
        {
            *linker = m_linker.get();
        }
        return getAnimation( name );
    }

    SmartPtr<IAnimation> MeshSkeleton::getAnimation( const String &name ) const
    {
        const auto found = m_animationsByName.find( name );
        return found == m_animationsByName.end() ? nullptr : found->second;
    }

    bool MeshSkeleton::hasAnimation( const String &name ) const
    {
        return m_animationsByName.find( name ) != m_animationsByName.end();
    }

    SmartPtr<IBone> MeshSkeleton::createBone()
    {
        return createBone( static_cast<u32>( m_bones.size() ) );
    }

    SmartPtr<IBone> MeshSkeleton::createBone( const String &name, u32 handle )
    {
        auto bone = makeSkeletonObject<Bone>();
        bone->setBoneHandle( static_cast<u16>( handle ) );
        bone->setName( name );
        appendNamed( m_bones, m_bonesByName, name, SmartPtr<IBone>( bone ) );
        return bone;
    }

    SmartPtr<IBone> MeshSkeleton::createBone( const String &name )
    {
        return createBone( name, static_cast<u32>( m_bones.size() ) );
    }

    SmartPtr<IBone> MeshSkeleton::createBone( u32 handle )
    {
        auto bone = makeSkeletonObject<Bone>();
        bone->setBoneHandle( static_cast<u16>( handle ) );
        m_bones.push_back( bone );
        return bone;
    }

    SmartPtr<IBone> MeshSkeleton::getBone( const String &name ) const
    {
        const auto found = m_bonesByName.find( name );
        return found == m_bonesByName.end() ? nullptr : found->second;
    }

    bool MeshSkeleton::hasBone( const String &name ) const
    {
        return m_bonesByName.find( name ) != m_bonesByName.end();
    }

    const Array<SmartPtr<IAnimation>> &MeshSkeleton::getAnimations() const
    {
        return m_animations;
    }

    const Array<SmartPtr<IBone>> &MeshSkeleton::getBones() const
    {
        return m_bones;
    }

    SkeletonAnimationBlendMode MeshSkeleton::getBlendMode() const
    {
        return m_blendMode;
    }

    void MeshSkeleton::setBlendMode( SkeletonAnimationBlendMode mode )
    {
        m_blendMode = mode;
    }
}  // namespace workphone
