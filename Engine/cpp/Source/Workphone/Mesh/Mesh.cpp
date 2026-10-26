#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshPose.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Interface/Animation/IAnimationInterface.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/Mesh/IVertexBoneAssignment.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <new>
#include <mutex>

namespace workphone
{
    namespace
    {
        // Meshes may outlive the application's manager during factory teardown.
        // Keep synchronization available without constructing a dynamic owner.
        std::recursive_mutex &detachedMeshMutex()
        {
            // Retain image storage through late static/factory destruction.
            alignas( std::recursive_mutex ) static unsigned char storage[sizeof( std::recursive_mutex )];
            static auto *mutex = new( storage ) std::recursive_mutex;
            return *mutex;
        }

        void meshRetirementError( const char *message ) noexcept
        {
            try
            {
                WP_LOG_ERROR( message );
            }
            catch( ... )
            {
            }
        }

        void meshRetirementError( const std::exception &error ) noexcept
        {
            try
            {
                WP_LOG_EXCEPTION( error );
            }
            catch( ... )
            {
            }
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, Mesh, IMesh );

    Mesh::Mesh() :
        m_subMeshes( Allocator<SmartPtr<ISubMesh>>( sourceBudget ) ),
        m_animations( Allocator<SmartPtr<IAnimation>>( sourceBudget ) ),
        m_boneAssignments( Allocator<SmartPtr<IVertexBoneAssignment>>( sourceBudget ) ),
        m_poses( Allocator<SmartPtr<IMeshPose>>( sourceBudget ) )
    {
    }

    Mesh::~Mesh() = default;

    void Mesh::unload( SmartPtr<ISharedObject> data )
    {
        // Loading notifications are optional new work; their denial must not
        // strand already admitted inventories during physical retirement.
        try
        {
            setLoadingState( LoadingState::Unloading );
        }
        catch( const std::bad_alloc & )
        {
        }

        auto subMeshes = makeSourceBudgetSnapshot<SmartPtr<ISubMesh>>();
        auto animations = makeSourceBudgetSnapshot<SmartPtr<IAnimation>>();
        auto bones = makeSourceBudgetSnapshot<SmartPtr<IVertexBoneAssignment>>();
        auto poses = makeSourceBudgetSnapshot<SmartPtr<IMeshPose>>();
        SmartPtr<IVertexBuffer> sharedVertexBuffer;
        SmartPtr<IAnimationInterface> animationInterface;
        SmartPtr<ISkeleton> skeleton;
        {
            ScopedLock lock( this );
            // Transfer the complete generation without copying or admitting
            // teardown scratch. Reentrant unload sees empty inventories.
            subMeshes.swap( m_subMeshes );
            animations.swap( m_animations );
            bones.swap( m_boneAssignments );
            poses.swap( m_poses );
            std::swap( sharedVertexBuffer, m_sharedVertexBuffer );
            std::swap( animationInterface, m_animationInterface );
            std::swap( skeleton, m_skeleton );
            m_skeletonName.clear();
            m_hasSkeleton = false;
            m_hasSharedVertexData = false;
        }

        for( auto subMesh : subMeshes )
        {
            try
            {
                if( subMesh )
                {
                    // The mesh and this loop hold two references. A retained
                    // submesh keeps its buffers (and their budget leases) alive.
                    if( subMesh->getReferences() == 2 && !subMesh->getWeakReferences() )
                    {
                        subMesh->unload( nullptr );
                    }
                }
                else
                {
                    meshRetirementError( "Null subMesh encountered during unload." );
                }
            }
            catch( const std::exception &e )
            {
                meshRetirementError( e );
            }
        }

        if( sharedVertexBuffer )
        {
            try
            {
                if( sharedVertexBuffer->getReferences() == 1 &&
                    !sharedVertexBuffer->getWeakReferences() )
                {
                    sharedVertexBuffer->unload( nullptr );
                }
            }
            catch( const std::exception &e )
            {
                meshRetirementError( e );
            }
        }

        if( animationInterface )
        {
            try
            {
                if( animationInterface->getReferences() == 1 &&
                    !animationInterface->getWeakReferences() )
                {
                    animationInterface->unload( nullptr );
                }
            }
            catch( const std::exception &e )
            {
                meshRetirementError( e );
            }
        }

        for( auto animation : animations )
        {
            try
            {
                if( animation )
                {
                    if( animation->getReferences() == 2 && !animation->getWeakReferences() )
                    {
                        animation->unload( nullptr );
                    }
                }
                else
                {
                    meshRetirementError( "Null animation encountered during unload." );
                }
            }
            catch( const std::exception &e )
            {
                meshRetirementError( e );
            }
        }

        // Clean up poses
        for( auto pose : poses )
        {
            try
            {
                if( pose )
                {
                    if( pose->getReferences() == 2 && !pose->getWeakReferences() )
                    {
                        pose->unload( nullptr );
                    }
                }
                else
                {
                    meshRetirementError( "Null pose encountered during unload." );
                }
            }
            catch( const std::exception &e )
            {
                meshRetirementError( e );
            }
        }

        try
        {
            setLoadingState( LoadingState::Unloaded );
        }
        catch( const std::bad_alloc & )
        {
        }
    }

    void Mesh::addSubMesh( SmartPtr<ISubMesh> subMesh )
    {
        ScopedLock lock( this );
        if( !subMesh )
        {
            WP_LOG_ERROR( "Attempted to add null subMesh." );
            return;
        }
        m_subMeshes.push_back( subMesh );
    }

    void Mesh::removeSubMesh( SmartPtr<ISubMesh> subMesh )
    {
        ScopedLock lock( this );
        auto it = std::find( m_subMeshes.begin(), m_subMeshes.end(), subMesh );
        if( it != m_subMeshes.end() )
        {
            std::move( it + 1, m_subMeshes.end(), it );
            m_subMeshes.pop_back();
        }
        else
        {
            WP_LOG_ERROR( "Attempted to remove subMesh that does not exist." );
        }
    }

    void Mesh::removeAllSubMeshes()
    {
        ScopedLock lock( this );
        m_subMeshes.clear();
    }

    Array<SmartPtr<ISubMesh>> Mesh::getSubMeshes() const
    {
        ScopedLock lock( this );
        return m_subMeshes;
    }

    SmartPtr<ISubMesh> Mesh::getSubMesh( u32 index ) const
    {
        ScopedLock lock( this );
        if( index < m_subMeshes.size() )
        {
            return m_subMeshes[index];
        }
        WP_LOG_ERROR( "getSubMesh: Index out of range (" + StringUtil::toString( index ) + ")." );
        return nullptr;
    }

    u32 Mesh::getNumSubMeshes() const
    {
        ScopedLock lock( this );
        return static_cast<u32>( m_subMeshes.size() );
    }

    void Mesh::updateAABB( bool forceSubMeshUpdate )
    {
        ScopedLock lock( this );

        if( !m_subMeshes.empty() )
        {
            m_aabb.setMinimum( Vector3<real_Num>( 1e10, 1e10, 1e10 ) );
            m_aabb.setMaximum( Vector3<real_Num>( -1e10, -1e10, -1e10 ) );
        }
        else
        {
            m_aabb.reset( Vector3<real_Num>::zero() );
        }

        for( auto &subMesh : m_subMeshes )
        {
            if( !subMesh )
            {
                WP_LOG_ERROR( "Null subMesh encountered during updateAABB." );
                continue;
            }

            if( forceSubMeshUpdate )
            {
                try
                {
                    subMesh->updateAABB();
                }
                catch( const std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }

            try
            {
                auto aabb = subMesh->getAABB();
                m_aabb.merge( aabb );
            }
            catch( const std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
    }

    AABB3<real_Num> Mesh::getAABB() const
    {
        ScopedLock lock( this );
        return m_aabb;
    }

    void Mesh::setAABB( const AABB3<real_Num> &aabb )
    {
        ScopedLock lock( this );
        m_aabb = aabb;
    }

    SmartPtr<IMesh> Mesh::clone() const
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "ApplicationManager instance is null in Mesh::clone." );
            return nullptr;
        }
        auto factoryManager = applicationManager->getFactoryManager();
        if( !factoryManager )
        {
            WP_LOG_ERROR( "FactoryManager is null in Mesh::clone." );
            return nullptr;
        }

        auto newMesh = factoryManager->make_ptr<Mesh>();
        if( !newMesh )
        {
            WP_LOG_ERROR( "Failed to create new Mesh in Mesh::clone." );
            return nullptr;
        }

        auto subMeshes = getSubMeshes();

        for( auto &subMesh : subMeshes )
        {
            if( !subMesh )
            {
                WP_LOG_ERROR( "Null subMesh encountered during clone." );
                continue;
            }
            auto newSubMesh = subMesh->clone();
            if( !newSubMesh )
            {
                WP_LOG_ERROR( "Failed to clone subMesh in Mesh::clone." );
                continue;
            }
            newMesh->addSubMesh( newSubMesh );
        }

        newMesh->setHasSharedVertexData( getHasSharedVertexData() );
        if( auto sharedVertexBuffer = getSharedVertexBuffer() )
        {
            auto cloneSharedVertexBuffer = sharedVertexBuffer->clone();
            if( !cloneSharedVertexBuffer )
            {
                WP_LOG_ERROR( "Failed to clone shared vertex buffer in Mesh::clone." );
            }
            else
            {
                newMesh->setSharedVertexBuffer( cloneSharedVertexBuffer );
            }
        }

        auto aabb = getAABB();
        newMesh->setAABB( aabb );
        newMesh->setBoundingSphereRadius( getBoundingSphereRadius() );
        newMesh->setHasSkeleton( hasSkeleton() );
        newMesh->setSkeletonName( getSkeletonName() );
        newMesh->setSkeleton( getSkeleton() );

        for( const auto &boneAssignment : getBoneAssignments() )
        {
            if( boneAssignment )
            {
                newMesh->addBoneAssignment( boneAssignment );
            }
            else
            {
                WP_LOG_ERROR( "Null bone assignment encountered during clone." );
            }
        }

        if( auto animationInterface = getAnimationInterface() )
        {
            auto cloneAnimationInterface = animationInterface->clone();
            if( !cloneAnimationInterface )
            {
                WP_LOG_ERROR( "Failed to clone animationInterface in Mesh::clone." );
            }
            else
            {
                newMesh->setAnimationInterface( cloneAnimationInterface );
            }
        }

        for( auto &animation : m_animations )
        {
            if( !animation )
            {
                WP_LOG_ERROR( "Null animation encountered during clone." );
                continue;
            }

            auto clonedAnimation = animation->clone( animation->getName() );
            if( !clonedAnimation )
            {
                WP_LOG_ERROR( "Failed to clone animation in Mesh::clone." );
                continue;
            }

            SmartPtr<IAnimation> clonedAnimationPtr( clonedAnimation );
#if WP_GARBAGE_COLLECTION == 1
            // clone() transfers the initial intrusive reference, just as new
            // does in make_ptr. The SmartPtr now owns that transferred result.
            clonedAnimation->removeReference();
#endif
            clonedAnimationPtr->setName( animation->getName() );
            newMesh->m_animations.push_back( clonedAnimationPtr );
        }

        // Clone poses
        for( auto &pose : m_poses )
        {
            if( !pose )
            {
                WP_LOG_ERROR( "Null pose encountered during clone." );
                continue;
            }
            auto clonedPose = pose->clone();
            if( !clonedPose )
            {
                WP_LOG_ERROR( "Failed to clone pose in Mesh::clone." );
                continue;
            }
            newMesh->addPose( clonedPose );
        }

        return newMesh;
    }

    void Mesh::setAnimationInterface( SmartPtr<IAnimationInterface> animationInterface )
    {
        m_animationInterface = animationInterface;
    }

    SmartPtr<IAnimationInterface> Mesh::getAnimationInterface() const
    {
        return m_animationInterface;
    }

    void Mesh::setHasSharedVertexData( bool hasSharedVertexData )
    {
        m_hasSharedVertexData = hasSharedVertexData;
    }

    SmartPtr<IVertexBuffer> Mesh::getSharedVertexBuffer() const
    {
        return m_sharedVertexBuffer;
    }

    void Mesh::setHasSkeleton( bool hasSkeleton )
    {
        m_hasSkeleton = hasSkeleton;
    }

    void Mesh::setSharedVertexBuffer( SmartPtr<IVertexBuffer> sharedVertexBuffer )
    {
        m_sharedVertexBuffer = sharedVertexBuffer;
    }

    bool Mesh::hasSkeleton() const
    {
        return m_hasSkeleton;
    }

    String Mesh::getSkeletonName() const
    {
        return m_skeletonName;
    }

    void Mesh::setSkeletonName( const String &skeletonName )
    {
        m_skeletonName = skeletonName;
    }

    u32 Mesh::getNumLodLevels() const
    {
        // LOD level 0 is the base mesh. This implementation does not currently store extra LODs.
        return 1;
    }

    bool Mesh::isEdgeListBuilt() const
    {
        return false;
    }

    bool Mesh::hasVertexAnimation() const
    {
        // Check if we have animations or poses
        ScopedLock lock( this );
        if( !m_animations.empty() || !m_poses.empty() )
        {
            return true;
        }

        return m_animationInterface && m_animationInterface->getNumAnimations() > 0;
    }

    bool Mesh::getHasSharedVertexData() const
    {
        return m_hasSharedVertexData;
    }

    bool Mesh::compare( SmartPtr<IMesh> other ) const
    {
        if( !other )
        {
            WP_LOG_ERROR( "Null mesh passed to compare." );
            return false;
        }

        auto subMeshes = getSubMeshes();
        auto otherSubMeshes = other->getSubMeshes();
        if( subMeshes.size() != otherSubMeshes.size() )
        {
            return false;
        }

        if( getHasSharedVertexData() != other->getHasSharedVertexData() ||
            hasSkeleton() != other->hasSkeleton() || getSkeletonName() != other->getSkeletonName() ||
            getBoundingSphereRadius() != other->getBoundingSphereRadius() )
        {
            return false;
        }

        auto sharedVertexBuffer = getSharedVertexBuffer();
        auto otherSharedVertexBuffer = other->getSharedVertexBuffer();
        if( ( sharedVertexBuffer != nullptr ) != ( otherSharedVertexBuffer != nullptr ) )
        {
            return false;
        }

        if( sharedVertexBuffer && !sharedVertexBuffer->compare( otherSharedVertexBuffer ) )
        {
            return false;
        }

        for( size_t i = 0; i < subMeshes.size(); ++i )
        {
            auto subMesh = subMeshes[i];
            auto otherSubMesh = otherSubMeshes[i];

            if( !subMesh || !otherSubMesh )
            {
                WP_LOG_ERROR( "Null subMesh encountered during compare." );
                return false;
            }

            if( !subMesh->compare( otherSubMesh ) )
            {
                return false;
            }
        }

        auto boneAssignments = getBoneAssignments();
        auto otherBoneAssignments = other->getBoneAssignments();
        if( boneAssignments.size() != otherBoneAssignments.size() )
        {
            return false;
        }

        for( size_t i = 0; i < boneAssignments.size(); ++i )
        {
            if( ( boneAssignments[i] != nullptr ) != ( otherBoneAssignments[i] != nullptr ) )
            {
                return false;
            }

            if( boneAssignments[i] &&
                ( boneAssignments[i]->getVertexIndex() != otherBoneAssignments[i]->getVertexIndex() ||
                  boneAssignments[i]->getBoneIndex() != otherBoneAssignments[i]->getBoneIndex() ||
                  boneAssignments[i]->getWeight() != otherBoneAssignments[i]->getWeight() ) )
            {
                return false;
            }
        }

        if( getNumAnimations() != other->getNumAnimations() )
        {
            return false;
        }

        for( u32 i = 0; i < getNumAnimations(); ++i )
        {
            auto animation = getAnimation( i );
            auto otherAnimation = other->getAnimation( i );
            if( !animation || !otherAnimation )
            {
                WP_LOG_ERROR( "Null animation encountered during compare." );
                return false;
            }

            if( animation->getName() != otherAnimation->getName() ||
                animation->getLength() != otherAnimation->getLength() ||
                animation->getNumNodeTracks() != otherAnimation->getNumNodeTracks() ||
                animation->getNumNumericTracks() != otherAnimation->getNumNumericTracks() ||
                animation->getNumVertexTracks() != otherAnimation->getNumVertexTracks() )
            {
                return false;
            }
        }

        // Compare poses
        if( m_poses.size() != other->getNumPoses() )
        {
            return false;
        }

        for( u32 i = 0; i < static_cast<u32>( m_poses.size() ); ++i )
        {
            auto pose = m_poses[i];
            auto otherPose = other->getPose( i );

            if( !pose || !otherPose )
            {
                WP_LOG_ERROR( "Null pose encountered during compare." );
                return false;
            }

            // Compare pose properties
            if( pose->getName() != otherPose->getName() || pose->getTarget() != otherPose->getTarget() ||
                pose->getIncludesNormals() != otherPose->getIncludesNormals() ||
                pose->getNumVertexOffsets() != otherPose->getNumVertexOffsets() )
            {
                return false;
            }
        }

        return true;
    }

    void Mesh::setBoundingSphereRadius( real_Num radius )
    {
        ScopedLock lock( this );
        if( radius < 0 )
        {
            WP_LOG_ERROR( "Attempted to set negative bounding sphere radius." );
            radius = 0;
        }

        m_boundingSphereRadius = radius;
    }

    real_Num Mesh::getBoundingSphereRadius() const
    {
        ScopedLock lock( this );
        if( m_boundingSphereRadius < 0 )
        {
            WP_LOG_ERROR( "Invalid bounding sphere radius (negative value)." );
            return 0.0;
        }
        return m_boundingSphereRadius;
    }

    void Mesh::addBoneAssignment( SmartPtr<IVertexBoneAssignment> boneAssignment )
    {
        ScopedLock lock( this );
        if( !boneAssignment )
        {
            WP_LOG_ERROR( "Attempted to add null bone assignment." );
            return;
        }

        try
        {
            m_boneAssignments.push_back( boneAssignment );
        }
        catch( const std::bad_alloc & )
        {
            throw;
        }
        catch( const std::exception &e )
        {
            WP_LOG_ERROR( "Failed to add bone assignment: " + String( e.what() ) );
        }
    }

    void Mesh::removeBoneAssignment( SmartPtr<IVertexBoneAssignment> boneAssignment )
    {
        ScopedLock lock( this );
        if( !boneAssignment )
        {
            WP_LOG_ERROR( "Attempted to remove null bone assignment." );
            return;
        }

        auto it = std::find( m_boneAssignments.begin(), m_boneAssignments.end(), boneAssignment );
        if( it != m_boneAssignments.end() )
        {
            std::move( it + 1, m_boneAssignments.end(), it );
            m_boneAssignments.pop_back();
        }
        else
        {
            WP_LOG_ERROR( "Bone assignment not found during removal attempt." );
        }
    }

    Array<SmartPtr<IVertexBoneAssignment>> Mesh::getBoneAssignments() const
    {
        ScopedLock lock( this );
        return m_boneAssignments;
    }

    u32 Mesh::getNumAnimations() const
    {
        ScopedLock lock( this );
        return static_cast<u32>( m_animations.size() );
    }

    SmartPtr<IAnimation> Mesh::getAnimation( u32 index ) const
    {
        ScopedLock lock( this );
        if( index >= m_animations.size() )
        {
            WP_LOG_ERROR( "Animation index out of range: " + StringUtil::toString( index ) );
            return nullptr;
        }
        return m_animations[index];
    }

    SmartPtr<IAnimation> Mesh::getAnimationByName( const String &name ) const
    {
        ScopedLock lock( this );
        if( name.empty() )
        {
            WP_LOG_ERROR( "Empty animation name provided." );
            return nullptr;
        }

        auto it = std::find_if(
            m_animations.begin(), m_animations.end(),
            [&name]( const SmartPtr<IAnimation> &anim ) { return anim && anim->getName() == name; } );

        if( it != m_animations.end() )
        {
            return *it;
        }

        WP_LOG_ERROR( "Animation not found: " + name );
        return nullptr;
    }

    SmartPtr<IAnimation> Mesh::createAnimation( const String &name, f32 length )
    {
        ScopedLock lock( this );
        if( name.empty() )
        {
            WP_LOG_ERROR( "Empty name provided for new animation." );
            return nullptr;
        }

        if( length <= 0.0f )
        {
            WP_LOG_ERROR( "Invalid animation length: " + StringUtil::toString( length ) );
            return nullptr;
        }

        // Check for existing animation with the same name
        auto existingAnimation = std::find_if(
            m_animations.begin(), m_animations.end(),
            [&name]( const SmartPtr<IAnimation> &anim ) { return anim && anim->getName() == name; } );
        if( existingAnimation != m_animations.end() )
        {
            WP_LOG_ERROR( "Animation with name '" + name + "' already exists." );
            return nullptr;
        }

        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "ApplicationManager instance is null in createAnimation." );
            return nullptr;
        }

        auto factoryManager = applicationManager->getFactoryManager();
        if( !factoryManager )
        {
            WP_LOG_ERROR( "FactoryManager is null in createAnimation." );
            return nullptr;
        }

        auto animation = factoryManager->make_ptr<Animation>();
        if( !animation )
        {
            WP_LOG_ERROR( "Failed to create animation instance." );
            return nullptr;
        }

        animation->setName( name );
        animation->setLength( length );
        m_animations.push_back( animation );

        return animation;
    }

    void Mesh::removeAnimation( SmartPtr<IAnimation> animation )
    {
        ScopedLock lock( this );
        if( !animation )
        {
            WP_LOG_ERROR( "Attempted to remove null animation." );
            return;
        }

        auto it = std::find( m_animations.begin(), m_animations.end(), animation );
        if( it != m_animations.end() )
        {
            std::move( it + 1, m_animations.end(), it );
            m_animations.pop_back();
        }
        else
        {
            WP_LOG_ERROR( "Animation not found during removal attempt." );
        }
    }

    void Mesh::removeAllAnimations()
    {
        ScopedLock lock( this );
        m_animations.clear();
    }

    SmartPtr<ISkeleton> Mesh::getSkeleton() const
    {
        return m_skeleton;
    }

    void Mesh::setSkeleton( SmartPtr<ISkeleton> skeleton )
    {
        m_skeleton = skeleton;
    }

    //--------------------------------------------------------------------------
    // Pose Management Implementation
    //--------------------------------------------------------------------------

    SmartPtr<IMeshPose> Mesh::createPose( u16 target, const String &name )
    {
        ScopedLock lock( this );

        // Check for existing pose with the same name
        auto existingPose =
            std::find_if( m_poses.begin(), m_poses.end(), [&name]( const SmartPtr<IMeshPose> &pose ) {
                return !name.empty() && pose && pose->getName() == name;
            } );
        if( existingPose != m_poses.end() )
        {
            WP_LOG_ERROR( "Pose with name '" + name + "' already exists." );
            return nullptr;
        }

        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "ApplicationManager instance is null in createPose." );
            return nullptr;
        }

        auto factoryManager = applicationManager->getFactoryManager();
        if( !factoryManager )
        {
            WP_LOG_ERROR( "FactoryManager is null in createPose." );
            return nullptr;
        }

        auto pose = factoryManager->make_ptr<MeshPose>( target, name );
        if( !pose )
        {
            WP_LOG_ERROR( "Failed to create pose instance." );
            return nullptr;
        }

        m_poses.push_back( pose );
        return pose;
    }

    u32 Mesh::getNumPoses() const
    {
        ScopedLock lock( this );
        return static_cast<u32>( m_poses.size() );
    }

    SmartPtr<IMeshPose> Mesh::getPose( u32 index ) const
    {
        ScopedLock lock( this );
        if( index >= m_poses.size() )
        {
            WP_LOG_ERROR( "Pose index out of range: " + StringUtil::toString( index ) );
            return nullptr;
        }
        return m_poses[index];
    }

    SmartPtr<IMeshPose> Mesh::getPoseByName( const String &name ) const
    {
        ScopedLock lock( this );
        if( name.empty() )
        {
            WP_LOG_ERROR( "Empty pose name provided." );
            return nullptr;
        }

        auto it = std::find_if(
            m_poses.begin(), m_poses.end(),
            [&name]( const SmartPtr<IMeshPose> &pose ) { return pose && pose->getName() == name; } );

        if( it != m_poses.end() )
        {
            return *it;
        }

        WP_LOG_ERROR( "Pose not found: " + name );
        return nullptr;
    }

    void Mesh::addPose( SmartPtr<IMeshPose> pose )
    {
        ScopedLock lock( this );
        if( !pose )
        {
            WP_LOG_ERROR( "Attempted to add null pose." );
            return;
        }

        // Check for existing pose with the same name
        const String poseName = pose->getName();
        auto existingPose = std::find_if(
            m_poses.begin(), m_poses.end(), [&poseName]( const SmartPtr<IMeshPose> &existingPose ) {
                return !poseName.empty() && existingPose && existingPose->getName() == poseName;
            } );
        if( existingPose != m_poses.end() )
        {
            WP_LOG_ERROR( "Pose with name '" + poseName + "' already exists." );
            return;
        }

        try
        {
            m_poses.push_back( pose );
        }
        catch( const std::bad_alloc & )
        {
            throw;
        }
        catch( const std::exception &e )
        {
            WP_LOG_ERROR( "Failed to add pose: " + String( e.what() ) );
        }
    }

    void Mesh::removePose( SmartPtr<IMeshPose> pose )
    {
        ScopedLock lock( this );
        if( !pose )
        {
            WP_LOG_ERROR( "Attempted to remove null pose." );
            return;
        }

        auto it = std::find( m_poses.begin(), m_poses.end(), pose );
        if( it != m_poses.end() )
        {
            std::move( it + 1, m_poses.end(), it );
            m_poses.pop_back();
        }
        else
        {
            WP_LOG_ERROR( "Pose not found during removal attempt." );
        }
    }

    void Mesh::removeAllPoses()
    {
        ScopedLock lock( this );
        m_poses.clear();
    }

    void Mesh::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto meshManager = applicationManager ? applicationManager->getMeshManager() : nullptr;
        if( meshManager )
        {
            meshManager->lock();
        }
        else
        {
            detachedMeshMutex().lock();
        }
    }

    bool Mesh::try_lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto meshManager = applicationManager ? applicationManager->getMeshManager() : nullptr;
        return meshManager ? meshManager->try_lock() : detachedMeshMutex().try_lock();
    }

    void Mesh::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto meshManager = applicationManager ? applicationManager->getMeshManager() : nullptr;
        if( meshManager )
        {
            meshManager->unlock();
        }
        else
        {
            detachedMeshMutex().unlock();
        }
    }

    bool Mesh::isValid() const
    {
        auto subMeshes = getSubMeshes();
        for( auto &subMesh : subMeshes )
        {
            if( !subMesh )
            {
                WP_LOG_ERROR( "Null subMesh encountered during isValid." );
                return false;
            }
            if( !subMesh->isValid() )
            {
                return false;
            }
        }

        if( m_hasSharedVertexData && !m_sharedVertexBuffer )
        {
            WP_LOG_ERROR( "Mesh has shared vertex data enabled without a shared vertex buffer." );
            return false;
        }

        for( auto &animation : m_animations )
        {
            if( !animation )
            {
                WP_LOG_ERROR( "Null animation encountered during isValid." );
                return false;
            }
        }

        // Validate poses
        for( auto &pose : m_poses )
        {
            if( !pose )
            {
                WP_LOG_ERROR( "Null pose encountered during isValid." );
                return false;
            }
            if( !pose->isValid() )
            {
                return false;
            }
        }

        return true;
    }
}  // namespace workphone
