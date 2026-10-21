#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/ProceduralComponent.hpp>

namespace workphone
{
    namespace procedural
    {

        WP_CLASS_REGISTER_DERIVED( workphone::procedural, ProceduralComponent, scene::Component );

        ProceduralComponent::ProceduralComponent() = default;

        ProceduralComponent::~ProceduralComponent() = default;

        SmartPtr<ISharedObject> ProceduralComponent::getObject() const
        {
            return m_object;
        }

        void ProceduralComponent::setObject( SmartPtr<ISharedObject> object )
        {
            m_object = object;
        }

        FSMReturnType ProceduralComponent::handleComponentEvent( u32 state, FSMEvent eventType )
        {
            return FSMReturnType::Ok;
        }

    }  // namespace procedural
}  // namespace workphone
