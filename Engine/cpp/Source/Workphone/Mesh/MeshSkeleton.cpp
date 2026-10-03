#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/ILogManager.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Mesh/Bone.hpp>
#include <Workphone/Mesh/LinkedSkeletonAnimationSource.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, MeshSkeleton, ISkeleton );

    MeshSkeleton::MeshSkeleton()
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    logManager->logMessage( "MeshSkeleton: Constructor called",
                                            ILogManager::Type::Info );
                }
            }
        }
        catch( const std::exception &e )
        {
            std::cerr << "MeshSkeleton constructor error: " << e.what() << std::endl;
        }
    }

    MeshSkeleton::~MeshSkeleton()
    {
        try
        {
            // Clean up animations and bones
            m_animations.clear();
            m_animationsByName.clear();
            m_bones.clear();
            m_bonesByName.clear();
            //m_linker.reset();

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    logManager->logMessage( "MeshSkeleton: Destructor called", ILogManager::Type::Info );
                }
            }
        }
        catch( const std::exception &e )
        {
            std::cerr << "MeshSkeleton destructor error: " << e.what() << std::endl;
        }
    }

    SmartPtr<IAnimation> MeshSkeleton::createAnimation( const String &name, f32 length )
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Creating animation '" ) + name + "' with length " +
                               StringUtil::toString( length );
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            // Check if animation with this name already exists
            if( hasAnimation( name ) )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto logManager = applicationManager->getLogManager() )
                    {
                        auto msg = String( "MeshSkeleton: Animation '" ) + name + "' already exists";
                        logManager->logMessage( msg, ILogManager::Type::Warning );
                    }
                }
                return getAnimation( name );
            }

            auto factoryManager = core::IApplicationManager::instance()->getFactoryManager();
            if( !factoryManager )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto logManager = applicationManager->getLogManager() )
                    {
                        logManager->logMessage( "MeshSkeleton: Factory manager not available",
                                                ILogManager::Type::Error );
                    }
                }
                throw std::runtime_error( "Factory manager not available" );
            }

            auto animation = factoryManager->make_ptr<Animation>();
            if( !animation )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto logManager = applicationManager->getLogManager() )
                    {
                        auto msg = String( "MeshSkeleton: Failed to create animation '" ) + name + "'";
                        logManager->logMessage( msg, ILogManager::Type::Error );
                    }
                }
                throw std::runtime_error( "Failed to create animation" );
            }

            animation->setLength( length );
            animation->setName( name );
            m_animations.push_back( animation );
            m_animationsByName[name] = animation;

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Successfully created animation '" ) + name + "'";
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            return animation;
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Exception in createAnimation: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    SmartPtr<IAnimation> MeshSkeleton::getAnimation( const String &name,
                                                     const LinkedSkeletonAnimationSource **linker ) const
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Getting animation '" ) + name + "' with linker";
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            if( linker )
            {
                *linker = m_linker.get();
            }

            return getAnimation( name );
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg =
                        String( "MeshSkeleton: Exception in getAnimation (with linker): " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    SmartPtr<IAnimation> MeshSkeleton::getAnimation( const String &name ) const
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Getting animation '" ) + name + "'";
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            auto it = m_animationsByName.find( name );
            if( it != m_animationsByName.end() && it->second )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto logManager = applicationManager->getLogManager() )
                    {
                        auto msg = String( "MeshSkeleton: Found animation '" ) + name + "'";
                        logManager->logMessage( msg, ILogManager::Type::Info );
                    }
                }
                return it->second;
            }

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Animation '" ) + name + "' not found";
                    logManager->logMessage( msg, ILogManager::Type::Warning );
                }
            }

            return nullptr;
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Exception in getAnimation: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    bool MeshSkeleton::hasAnimation( const String &name ) const
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Checking if animation '" ) + name + "' exists";
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            bool hasAnim = m_animationsByName.find( name ) != m_animationsByName.end();

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Animation '" ) + name + "' " +
                               ( hasAnim ? "exists" : "does not exist" );
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            return hasAnim;
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Exception in hasAnimation: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    SmartPtr<IBone> MeshSkeleton::createBone()
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    logManager->logMessage( "MeshSkeleton: Creating bone with auto-generated handle",
                                            ILogManager::Type::Info );
                }
            }

            auto handle = static_cast<u32>( m_bones.size() );
            return createBone( handle );
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Exception in createBone(): " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    SmartPtr<IBone> MeshSkeleton::createBone( const String &name, u32 handle )
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Creating bone '" ) + name + "' with handle " +
                               StringUtil::toString( handle );
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            auto bone = createBone( handle );
            if( bone )
            {
                bone->setName( name );
                m_bonesByName[name] = bone;
            }
            return bone;
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg =
                        String( "MeshSkeleton: Exception in createBone(name, handle): " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    SmartPtr<IBone> MeshSkeleton::createBone( const String &name )
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Creating bone '" ) + name + "'";
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            auto handle = static_cast<u32>( m_bones.size() );
            return createBone( name, handle );
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Exception in createBone(name): " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    SmartPtr<IBone> MeshSkeleton::createBone( u32 handle )
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Creating bone with handle " ) +
                               StringUtil::toString( handle );
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            auto factoryManager = core::IApplicationManager::instance()->getFactoryManager();
            if( !factoryManager )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto logManager = applicationManager->getLogManager() )
                    {
                        logManager->logMessage( "MeshSkeleton: Factory manager not available",
                                                ILogManager::Type::Error );
                    }
                }
                throw std::runtime_error( "Factory manager not available" );
            }

            auto bone = factoryManager->make_ptr<Bone>();
            if( !bone )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto logManager = applicationManager->getLogManager() )
                    {
                        auto msg = String( "MeshSkeleton: Failed to create bone with handle " ) +
                                   StringUtil::toString( handle );
                        logManager->logMessage( msg, ILogManager::Type::Error );
                    }
                }
                throw std::runtime_error( "Failed to create bone" );
            }

            bone->setBoneHandle( static_cast<u16>( handle ) );

            m_bones.push_back( bone );

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Successfully created bone with handle " ) +
                               StringUtil::toString( handle );
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            return bone;
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Exception in createBone(handle): " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    SmartPtr<IBone> MeshSkeleton::getBone( const String &name ) const
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Getting bone '" ) + name + "'";
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            auto it = m_bonesByName.find( name );
            if( it != m_bonesByName.end() && it->second )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto logManager = applicationManager->getLogManager() )
                    {
                        auto msg = String( "MeshSkeleton: Found bone '" ) + name + "'";
                        logManager->logMessage( msg, ILogManager::Type::Info );
                    }
                }
                return it->second;
            }

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Bone '" ) + name + "' not found";
                    logManager->logMessage( msg, ILogManager::Type::Warning );
                }
            }

            return nullptr;
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Exception in getBone: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    bool MeshSkeleton::hasBone( const String &name ) const
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Checking if bone '" ) + name + "' exists";
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            bool hasBoneResult = m_bonesByName.find( name ) != m_bonesByName.end();

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Bone '" ) + name + "' " +
                               ( hasBoneResult ? "exists" : "does not exist" );
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            return hasBoneResult;
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "MeshSkeleton: Exception in hasBone: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }

            throw;
        }
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
