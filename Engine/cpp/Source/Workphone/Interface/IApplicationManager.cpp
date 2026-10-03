#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/DebugTrace.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>

namespace workphone::core
{

    WP_CLASS_REGISTER_DERIVED( workphone::core, IApplicationManager, ISharedObject );

    std::atomic<IApplicationManager *> WPCore_API IApplicationManager::m_instance;

    IApplicationManager::IApplicationManager() : ISharedObject( IApplicationManager::typeInfo() )
    {
    }

    IApplicationManager::~IApplicationManager() = default;

    void IApplicationManager::setInstance( SmartPtr<IApplicationManager> instance )
    {
        m_instance = instance.get();
    }

    void IApplicationManager::setInstancePtr( IApplicationManager *instance )
    {
        m_instance.store( instance );
    }

    SmartPtr<IApplicationManager> IApplicationManager::instance()
    {
        return m_instance.load();
    }

    core::IApplicationManager *IApplicationManager::instancePtr()
    {
        return m_instance.load();
    }

}  // namespace workphone::core
