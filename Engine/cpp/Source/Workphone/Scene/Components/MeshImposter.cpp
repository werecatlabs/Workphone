#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/MeshImposter.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <cmath>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        struct ImposterPublicationLock
        {
            SmartPtr<IGameManager> manager;
            SmartPtr<render::IGraphicsSystem> graphics;
            ScopedLock<IGameManager> managerLock;
            ScopedLock<render::IGraphicsSystem> graphicsLock;
            ImposterPublicationLock() :
                manager( core::IApplicationManager::instancePtr()
                    ? core::IApplicationManager::instancePtr()->getGameManager() : nullptr ),
                graphics( core::IApplicationManager::instancePtr()
                    ? core::IApplicationManager::instancePtr()->getGraphicsSystem() : nullptr ),
                managerLock( manager ), graphicsLock( graphics )
            {
            }
        };

        template <class T> bool finiteVector( const Vector3<T> &value )
        {
            return std::isfinite( value.x ) && std::isfinite( value.y ) && std::isfinite( value.z );
        }

        Vector3<real_Num> realVector( const Vector3F &value )
        {
            return { real_Num( value.x ), real_Num( value.y ), real_Num( value.z ) };
        }

        bool validView( const MeshImposterView &view )
        {
            return finiteVector( view.right ) && finiteVector( view.up ) && finiteVector( view.towardCamera ) &&
                std::abs( view.right.lengthSquared() - 1.f ) <= .001f &&
                std::abs( view.up.lengthSquared() - 1.f ) <= .001f &&
                std::abs( view.towardCamera.lengthSquared() - 1.f ) <= .001f &&
                std::abs( view.right.dotProduct( view.up ) ) <= .001f &&
                view.right.crossProduct( view.up ).dotProduct( view.towardCamera ) >= .999f &&
                std::isfinite( view.minimum.x ) && std::isfinite( view.minimum.y ) &&
                std::isfinite( view.maximum.x ) && std::isfinite( view.maximum.y ) &&
                std::isfinite( view.maximum.x - view.minimum.x ) &&
                std::isfinite( view.maximum.y - view.minimum.y ) &&
                std::isfinite( view.maximumDepth ) &&
                view.maximum.x > view.minimum.x && view.maximum.y > view.minimum.y &&
                std::isfinite( view.uvMinimum.x ) && std::isfinite( view.uvMinimum.y ) &&
                std::isfinite( view.uvMaximum.x ) && std::isfinite( view.uvMaximum.y ) &&
                view.uvMinimum.x >= 0 && view.uvMinimum.y >= 0 &&
                view.uvMaximum.x <= 1 && view.uvMaximum.y <= 1 &&
                view.uvMaximum.x > view.uvMinimum.x && view.uvMaximum.y > view.uvMinimum.y;
        }
    }

    WP_CLASS_REGISTER_DERIVED( workphone::scene, MeshImposter, Component );

    MeshImposter::MeshImposter() : m_storage( MeshImposterSystem::instance().create() ) {}
    MeshImposter::~MeshImposter() { MeshImposterSystem::instance().destroy( m_storage ); }

    void MeshImposter::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() ) return;
        setLoadingState( LoadingState::Loading );
        Component::load( data );
        // Actor reconstruction usually registers before load. Direct Editor
        // unload/load cycles also need to rejoin the manager-owned roster.
        if( !getComponentSystem() )
            if( auto app = core::IApplicationManager::instancePtr() )
                if( auto manager = app->getGameManager() ) manager->addComponent( this );
        {
            ImposterPublicationLock publication;
            auto state = MeshImposterSystem::instance().access( m_storage );
            if( !state.views.empty() ) applyView( state.selectedView >= 0 ? state.selectedView : 0 );
        }
        setLoadingState( LoadingState::Loaded );
    }

    void MeshImposter::unload( SmartPtr<ISharedObject> data )
    {
        ImposterPublicationLock publication;
        if( publication.manager ) publication.manager->unregisterAllComponent( this );
        setLoadingState( LoadingState::Unloading );
        // Keep authoring/runtime configuration across Editor stop/play reloads,
        // just as LODGroup retains its levels. Destruction releases these owners.
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void MeshImposter::configure( const Array<MeshImposterView> &views, const Vector3F &centre,
                                  SmartPtr<render::IMaterial> material )
    {
        if( !material || !finiteVector( centre ) || views.empty() || views.size() > 256 )
            throw std::invalid_argument( "Mesh imposter requires a material, finite centre and 1..256 views." );
        for( const auto &view : views )
        {
            if( !validView( view ) )
                throw std::invalid_argument( "Mesh imposter view requires orthonormal capture axes, bounds and a valid UV rectangle." );
            const auto midpoint = view.minimum + ( view.maximum - view.minimum ) * .5f;
            const auto projectedCentre = centre + view.right * midpoint.x + view.up * midpoint.y;
            if( !finiteVector( projectedCentre ) ||
                !finiteVector( projectedCentre + view.towardCamera * view.maximumDepth ) )
                throw std::invalid_argument( "Mesh imposter projected centre must remain finite." );
        }
        auto admitted = makeSourceBudgetSnapshot( views );
        ImposterPublicationLock publication;
        auto state = MeshImposterSystem::instance().access( m_storage );
        state.views.swap( admitted );
        state.centre = centre;
        state.material = std::move( material );
        state.selectedView = -1;
        state.selectionChanges = 0;
        applyView( 0 );
    }

    bool MeshImposter::updateForCamera( const Vector3<real_Num> &worldPosition )
    {
        return updateFromCameraVector( worldPosition, false );
    }

    bool MeshImposter::updateForView( const LODSystem::View &view )
    {
        // A parallel projection has the same object-to-camera direction for
        // every patch, even when the camera or buildings are translated.
        return view.orthographic ? updateFromCameraVector( -view.direction, true )
                                 : updateForCamera( view.position );
    }

    bool MeshImposter::updateFromCameraVector( const Vector3<real_Num> &worldVector,
                                               bool directionOnly )
    {
        if( !finiteVector( worldVector ) ) return false;
        ImposterPublicationLock publication;
        auto state = MeshImposterSystem::instance().access( m_storage );
        auto actor = getActor();
        if( !actor || state.views.empty() || !state.material ) return false;
        auto localVector = worldVector;
        if( !actor->getFlag( IGameActor::ActorFlagWorldSpaceTransform ) )
            if( const auto parent = actor->getParent() )
            {
                const auto pose = parent->getWorldTransform();
                const auto scale = pose.getScale();
                const auto orientation = pose.getOrientation();
                const double orientationNorm = double( orientation.w ) * orientation.w +
                    double( orientation.x ) * orientation.x + double( orientation.y ) * orientation.y +
                    double( orientation.z ) * orientation.z;
                if( !finiteVector( pose.getPosition() ) || !finiteVector( scale ) ||
                    std::abs( scale.x ) < 1e-8 || std::abs( scale.y ) < 1e-8 || std::abs( scale.z ) < 1e-8 ||
                    !std::isfinite( orientation.w ) || !std::isfinite( orientation.x ) ||
                    !std::isfinite( orientation.y ) || !std::isfinite( orientation.z ) ||
                    !std::isfinite( orientationNorm ) || std::abs( orientationNorm - 1. ) > .001 ) return false;
                localVector = orientation.inverse() *
                    ( directionOnly ? worldVector : worldVector - pose.getPosition() );
                localVector.x /= scale.x;
                localVector.y /= scale.y;
                localVector.z /= scale.z;
            }
        const auto localDirection = directionOnly ? localVector : localVector - realVector( state.centre );
        const Vector3F direction( f32( localDirection.x ), f32( localDirection.y ), f32( localDirection.z ) );
        const auto directionLengthSquared = direction.lengthSquared();
        if( !finiteVector( direction ) || !std::isfinite( directionLengthSquared ) || directionLengthSquared < 1e-12f ) return false;
        const auto selected = selectMeshImposterView( state.views, direction, state.selectedView );
        if( selected < 0 ) return false;
        // The supporting plane's ray-preserving placement changes as the eye
        // moves, even while angular hysteresis retains the same atlas frame.
        return applyView( selected, &localVector, directionOnly );
    }

    bool MeshImposter::applyView( s32 selected, const Vector3<real_Num> *localCamera,
                                  bool orthographic )
    {
        auto state = MeshImposterSystem::instance().access( m_storage );
        auto actor = getActor();
        auto transform = actor ? actor->getTransform() : nullptr;
        if( !transform || !state.material || selected < 0 || size_t( selected ) >= state.views.size() ) return false;
        const auto &view = state.views[size_t( selected )];
        const auto midpoint = view.minimum + ( view.maximum - view.minimum ) * .5f;
        const auto centre = realVector( state.centre + view.right * midpoint.x + view.up * midpoint.y );
        const auto normal = realVector( view.towardCamera );
        auto projectedCentre = centre;
        real_Num projectionScale = 1;
        if( orthographic )
        {
            // Quantized frame normals and actual parallel viewing rays differ.
            // Shift along the latter so screen positions remain exactly unchanged.
            auto toward = localCamera ? *localCamera : normal;
            toward.normalise();
            const auto alignment = toward.dotProduct( normal );
            if( std::isfinite( alignment ) && alignment > 1e-6 )
                projectedCentre += toward * ( real_Num( view.maximumDepth ) / alignment );
        }
        else if( localCamera )
        {
            const auto distance = ( *localCamera - realVector( state.centre ) ).dotProduct( normal );
            const auto gap = distance - real_Num( view.maximumDepth );
            if( std::isfinite( distance ) && std::isfinite( gap ) && distance > 1e-6 && gap > 1e-4 )
            {
                // q' = E + t(q-E) lies on the same perspective ray as q, and
                // n.(q'-C) equals maximumDepth for t=(D-maximumDepth)/D.
                projectionScale = gap / distance;
                projectedCentre = *localCamera + ( centre - *localCamera ) * projectionScale;
            }
            // A near/inside camera still selects the correct frame, but retains
            // the original centre plane rather than creating an inverted quad.
            // This is a distant representation: normal LOD detail covers near use.
        }
        const Vector3<real_Num> scale( real_Num( view.maximum.x - view.minimum.x ) * projectionScale,
                                       real_Num( view.maximum.y - view.minimum.y ) * projectionScale, 1 );
        if( !finiteVector( projectedCentre ) || !finiteVector( scale ) || scale.x <= 0 || scale.y <= 0 ) return false;
        const Quaternion<real_Num> orientation( realVector( view.right ), realVector( view.up ),
                                                realVector( view.towardCamera ) );
        const Transform3<real_Num> pose( projectedCentre, orientation, scale );
        if( selected != state.selectedView )
        {
            state.material->setUVTiling( view.uvMaximum - view.uvMinimum );
            state.material->setUVOffset( view.uvMinimum );
        }
        if( transform->getLocalTransform() != pose || transform->isDirty() || transform->isLocalDirty() )
        {
            transform->setLocalTransform( pose );
            transform->setDirty( true );
            actor->updateTransform();
        }
        if( state.selectedView >= 0 && state.selectedView != selected ) ++state.selectionChanges;
        state.selectedView = selected;
        return true;
    }

    Vector3F MeshImposter::getLocalCentre() const
    {
        auto state = MeshImposterSystem::instance().access( m_storage );
        return state.centre;
    }

    Vector3F MeshImposter::getSelectedDirection() const
    {
        auto state = MeshImposterSystem::instance().access( m_storage );
        return state.selectedView >= 0 && size_t( state.selectedView ) < state.views.size()
            ? state.views[size_t( state.selectedView )].towardCamera : Vector3F::zero();
    }

    bool MeshImposter::getSelectedFrame( MeshImposterView &view ) const
    {
        auto state = MeshImposterSystem::instance().access( m_storage );
        if( state.selectedView < 0 || size_t( state.selectedView ) >= state.views.size() ) return false;
        view = state.views[size_t( state.selectedView )];
        return true;
    }

    s32 MeshImposter::getSelectedView() const
    {
        auto state = MeshImposterSystem::instance().access( m_storage );
        return state.selectedView;
    }

    u32 MeshImposter::getViewCount() const
    {
        auto state = MeshImposterSystem::instance().access( m_storage );
        return static_cast<u32>( state.views.size() );
    }

    u64 MeshImposter::getSelectionChanges() const
    {
        auto state = MeshImposterSystem::instance().access( m_storage );
        return state.selectionChanges;
    }
}
