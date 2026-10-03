#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageLoad.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageLoad, StateMessage );
    const hash_type StateMessageLoad::LOAD_HASH = StringUtil::getHash( "load" );
    const hash_type StateMessageLoad::RELOAD_HASH = StringUtil::getHash( "reload" );

    const hash_type StateMessageLoad::LOADED_HASH = StringUtil::getHash( "loaded" );
    const hash_type StateMessageLoad::UNLOADED_HASH = StringUtil::getHash( "unloaded" );

    StateMessageLoad::StateMessageLoad() = default;

    StateMessageLoad::~StateMessageLoad() = default;

    auto StateMessageLoad::getObject() const -> SmartPtr<ISharedObject>
    {
        return m_object;
    }

    void StateMessageLoad::setObject( SmartPtr<ISharedObject> object )
    {
        m_object = object;
    }
}  // namespace workphone
