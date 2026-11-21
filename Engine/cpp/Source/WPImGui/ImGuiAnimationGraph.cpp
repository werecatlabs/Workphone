#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiAnimationGraph.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiAnimationGraph, ImGuiElement<IUIAnimationGraph> );

    void ImGuiAnimationGraph::update()
    {
        if( isVisible() )
        {
            ScopedLock lock( this );
            draw();
        }
    }

    void ImGuiAnimationGraph::unlock()
    {
        m_mutex.unlock();
    }

    bool ImGuiAnimationGraph::try_lock()
    {
        return m_mutex.try_lock();
    }

    void ImGuiAnimationGraph::lock()
    {
        m_mutex.lock();
    }

}  // namespace workphone::ui
