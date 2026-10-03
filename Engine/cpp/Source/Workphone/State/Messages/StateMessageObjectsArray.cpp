#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageObjectsArray.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageObjectsArray, StateMessage );

    StateMessageObjectsArray::StateMessageObjectsArray() = default;

    StateMessageObjectsArray::~StateMessageObjectsArray() = default;

    auto StateMessageObjectsArray::getObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        return m_objects;
    }

    void StateMessageObjectsArray::setObjects( const Array<SmartPtr<ISharedObject>> &objects )
    {
        m_objects = objects;
    }
}  // namespace workphone
