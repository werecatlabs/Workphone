#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/Bone.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, Bone, IBone );

    Bone::Bone() :
        m_position( Vector3<real_Num>::zero() ),
        m_orientation( Quaternion<real_Num>::identity() ),
        m_initialPosition( Vector3<real_Num>::zero() ),
        m_initialOrientation( Quaternion<real_Num>::identity() ),
        m_bindingPosition( Vector3<real_Num>::zero() ),
        m_bindingOrientation( Quaternion<real_Num>::identity() ),
        m_iHandle( 0 ),
        m_manuallyControlled( false ),
        m_children( Allocator<SmartPtr<IBone>>( sourceBudget ) )
    {
    }

    Bone::~Bone()
    {
        unload( nullptr );
    }

    void Bone::unload( SmartPtr<ISharedObject> data )
    {
        auto children = makeSourceBudgetSnapshot<SmartPtr<IBone>>();
        children.swap( m_children );
        m_parent = nullptr;
        // Escaped child snapshots remain owners. Detach their weak back links
        // before releasing this hierarchy's physical child inventory.
        for( const auto &child : children )
        {
            if( auto native = dynamic_cast<Bone *>( child.get() ) )
                if( native->m_parent.get() == this ) native->m_parent = nullptr;
        }
    }

    SmartPtr<IBone> Bone::createChild( u16 handle, const Vector3<real_Num> &translate,
                                     const Quaternion<real_Num> &rotate )
    {
        auto application = core::IApplicationManager::instance();
        auto factory = application ? application->getFactoryManager() : nullptr;
        if( !factory ) throw std::runtime_error( "Factory manager not available" );
        const auto denials = sourceBudgetDenialCount();
        auto child = factory->make_ptr<Bone>();
        if( !child )
        {
            if( sourceBudgetDenialCount() != denials ) throw std::bad_alloc();
            throw std::runtime_error( "Failed to create child bone" );
        }
        child->m_iHandle = handle;
        child->m_position = child->m_initialPosition = translate;
        child->m_orientation = child->m_initialOrientation = rotate;
        m_children.push_back( child );
        // Weak assignment cannot allocate. No parent relationship is exposed
        // until the strong child inventory has successfully published.
        child->m_parent = this;
        return child;
    }

    void Bone::setBindingPose()
    {
        m_bindingPosition = m_position;
        m_bindingOrientation = m_orientation;
        for( auto &child : m_children ) if( child ) child->setBindingPose();
    }

    void Bone::reset()
    {
        m_position = m_bindingPosition;
        m_orientation = m_bindingOrientation;
        for( auto &child : m_children ) if( child ) child->reset();
    }

    void Bone::setManuallyControlled( bool value ) { m_manuallyControlled = value; }
    bool Bone::isManuallyControlled() const { return m_manuallyControlled; }
    Vector3<real_Num> Bone::getPosition() const { return m_position; }
    void Bone::setPosition( const Vector3<real_Num> &position ) { m_position = position; }
    Quaternion<real_Num> Bone::getOrientation() const { return m_orientation; }
    void Bone::setOrientation( const Quaternion<real_Num> &orientation ) { m_orientation = orientation; }
    u16 Bone::getBoneHandle() const { return m_iHandle; }
    void Bone::setBoneHandle( u16 handle ) { m_iHandle = handle; }
    SmartPtr<IBone> Bone::getParent() const { return m_parent.lock(); }
    Array<SmartPtr<IBone>> Bone::getChildren() const
    {
        return makeSourceBudgetSnapshot( m_children );
    }
}  // namespace workphone
