#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiWindow.hpp>
#include <WPImGui/ImGuiApplication.hpp>
#include <WPImGui/ImGuiDropJob.hpp>
#include <WPImGui/ImGuiListenerJob.hpp>
#include "Workphone/Interface/UI/IUIManager.hpp"
#include "Workphone/Core/LogManager.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiWindow, ImGuiWindowT<IUIWindow> );

    ImGuiWindow::ImGuiWindow() = default;

    ImGuiWindow::~ImGuiWindow() = default;

    SmartPtr<Properties> ImGuiWindow::getProperties() const
    {
        auto properties = ImGuiWindowT<IUIWindow>::getProperties();
        properties->setProperty( "initialPlacement", String( m_initiallyFloating ? "floating" : "default" ) );
        return properties;
    }

    void ImGuiWindow::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties ) return;
        String placement;
        const bool supplied = properties->getPropertyValue( "initialPlacement", placement );
        if( supplied && placement != "floating" && placement != "default" )
            throw std::invalid_argument( "Unsupported window initialPlacement; use floating or default." );
        ImGuiWindowT<IUIWindow>::setProperties( properties );
        if( supplied ) m_initiallyFloating = placement == "floating";
    }

    bool ImGuiWindow::applyInitialPlacement() const
    {
        if( !m_initiallyFloating ) return false;
        ImGui::SetNextWindowDockID( 0, ImGuiCond_FirstUseEver );
        const auto viewport = ImGui::GetMainViewport();
        const auto requested = getSize();
        if( std::isfinite( requested.X() ) && std::isfinite( requested.Y() ) &&
            requested.X() > 0 && requested.Y() > 0 &&
            viewport->WorkSize.x > 0 && viewport->WorkSize.y > 0 )
        {
            ImGui::SetNextWindowSize( ImVec2( std::min( requested.X(), viewport->WorkSize.x ),
                                             std::min( requested.Y(), viewport->WorkSize.y ) ),
                                      ImGuiCond_FirstUseEver );
            ImGui::SetNextWindowPos( ImVec2( viewport->WorkPos.x + viewport->WorkSize.x * .5f,
                                           viewport->WorkPos.y + viewport->WorkSize.y * .5f ),
                                     ImGuiCond_FirstUseEver, ImVec2( .5f, .5f ) );
        }
        return true;
    }

    void ImGuiWindow::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto ui = applicationManager->getUI();
        auto uiApplication = workphone::static_pointer_cast<ImGuiApplication>( ui->getApplication() );

        auto window = this;
        if( window->isVisible() )
        {
            auto windowParent = window->getParent();
            if( windowParent == nullptr )
            {
                if( !window->applyInitialPlacement() && !window->isDocked() )
                {
                    ImGui::SetNextWindowDockID( uiApplication->m_dockLeftIdRight, ImGuiCond_FirstUseEver );
                    window->setDocked( true );
                }

                auto label = window->getLabel();
                if( StringUtil::isNullOrEmpty( label ) )
                {
                    label = "Untitled";
                }

                auto windowFlags = static_cast<s32>( ImGuiWindowFlags_None );
                windowFlags |= ImGuiWindowFlags_AlwaysAutoResize;

                auto visible = window->isVisible();
                if( ImGui::Begin( label.c_str(), &visible, windowFlags ) )
                {
                    try
                    {
                        auto children = window->getChildren();
                        for( auto &child : children )
                        {
                            uiApplication->createElement( child );
                        }

                        auto dropTarget = window->getDropTarget();
                        if( dropTarget )
                        {
                            auto imguiWindow = ImGui::GetCurrentWindow();
                            if( ImGui::BeginDragDropTargetCustom( imguiWindow->InnerRect,
                                                                 imguiWindow->ID ) )
                            {
                                auto payload = ImGui::AcceptDragDropPayload( "_TREENODE" );
                                if( payload )
                                {
                                    auto pData = static_cast<const char *>( payload->Data );
                                    auto dataSize = payload->DataSize;
                                    auto data = String( pData, dataSize );

                                    auto args = Array<Parameter>();
                                    args.reserve( 1 );

                                    args.emplace_back( data );

                                    ImGuiDropJob::queue( dropTarget, std::move( args ), window,
                                                         window );
                                }

                                ImGui::EndDragDropTarget();
                            }
                        }

                        if( auto contextMenu = window->getContextMenu() )
                        {
                            if( ImGui::BeginPopupContextWindow( label.c_str(),
                                                                ImGuiPopupFlags_MouseButtonRight ) )
                            {
                                try
                                {
                                    auto label = contextMenu->getLabel();
                                    if( StringUtil::isNullOrEmpty( label ) )
                                    {
                                        label = "Untitled";
                                    }

                                    //if( ImGui::BeginMenu( label.c_str() ) )
                                    {
                                        try
                                        {
                                            auto menuItems = contextMenu->getMenuItems();
                                            for( auto menuItemElement : menuItems )
                                            {
                                                uiApplication->createMenuItem( contextMenu,
                                                                               menuItemElement );
                                            }
                                        }
                                        catch( std::exception &e )
                                        {
                                            WP_LOG_EXCEPTION( e );
                                        }

                                        //ImGui::EndMenu();
                                    }
                                }
                                catch( std::exception &e )
                                {
                                    WP_LOG_EXCEPTION( e );
                                }

                                ImGui::EndPopup();
                            }
                        }

                        const bool is_hovered = ImGui::IsItemHovered();  // Hovered
                        const bool is_active = ImGui::IsItemActive();    // Held

                        if( is_hovered && ImGui::IsMouseClicked( 0 ) )
                        {
                            auto listeners = window->getObjectListeners();
                            if( !listeners.empty() )
                            {
                                auto args = Array<Parameter>();
                                args.resize( 1 );

                                args[0].object = window;

                                ImGuiListenerJob::queue( std::move( listeners ), EventType::UI,
                                                         IEvent::handleMouseClicked, std::move( args ),
                                                         window, window );
                            }
                        }

                        window->setVisible( visible, false );
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }

                ImGui::End();
            }
        }
    }

    void ImGuiWindow::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loaded );
    }

}  // namespace workphone::ui
