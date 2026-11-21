#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiAnimationGraph.hpp>

namespace workphone
{
    namespace ui
    {

        ImGuiAnimationGraph::ImGuiAnimationGraph()
        {
        }

        ImGuiAnimationGraph::~ImGuiAnimationGraph()
        {
        }

        void ImGuiAnimationGraph::load( SmartPtr<ISharedObject> data )
        {
            //#if WP_BUILD_IMGUI
            //            if( auto native = workphone::dynamic_pointer_cast<ui::ImGuiWindow>( window ) )
            //            {
            //                auto properties = window->getProperties();
            //                properties->setProperty( "initialPlacement", String( "floating" ) );
            //                window->setProperties( properties );
            //                AtomicWeakPtr<AnimationGraphWindow> weak;
            //                weak = this;
            //                native->setContentRenderer( [weak]( const ui::ImGuiWindow::ContentContext &frame ) {
            //                    if( auto owner = weak.load().lock() )
            //                    {
            //                        const animation_graph_ui::ScopedImGuiContext scope( frame );
            //                        owner->draw();
            //                    }
            //                } );
            //            }
            //            else
            //#endif
        }

        void ImGuiAnimationGraph::unload( SmartPtr<ISharedObject> data )
        {
        }

        void ImGuiAnimationGraph::update()
        {
            draw();
        }

        workphone::SmartPtr<workphone::Properties> ImGuiAnimationGraph::getProperties() const
        {
            return nullptr;
        }

        void ImGuiAnimationGraph::setProperties( SmartPtr<Properties> properties )
        {
        }

    }  // namespace ui
}  // namespace workphone
