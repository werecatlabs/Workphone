#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIAnimationGraph.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIAnimationGraph, IUIElement );

    IUIAnimationGraph::~IUIAnimationGraph() = default;

    IUIAnimationGraph::IUIAnimationGraph( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIAnimationGraph::IUIAnimationGraph() : IUIAnimationGraph( typeInfo() )
    {
    }

    IUIAnimationGraph::Model &IUIAnimationGraph::getModel()
    {
        return m_model;
    }

void IUIAnimationGraph::Model::unlock_shared()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app->getGraphicsSystemPtr();
        graphics->unlock();
    }

    void IUIAnimationGraph::Model::unlock()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app->getGraphicsSystemPtr();
        graphics->unlock();
    }

    void IUIAnimationGraph::Model::lock_shared()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app->getGraphicsSystemPtr();
        graphics->lock();
    }

    bool IUIAnimationGraph::Model::try_lock()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app->getGraphicsSystemPtr();
        return graphics->try_lock();
    }

    void IUIAnimationGraph::Model::lock()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app->getGraphicsSystemPtr();
        graphics->lock();
    }

}  // namespace workphone::ui
