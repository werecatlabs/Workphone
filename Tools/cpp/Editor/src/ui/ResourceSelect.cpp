#include <EditorPCH.hpp>
#include "ui/ResourceSelect.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{

    ResourceSelect::ResourceSelect()
    {
    }

    ResourceSelect::~ResourceSelect()
    {
    }

    void ResourceSelect::load( SmartPtr<ISharedObject> data )
    {
        // Get the UI manager
        auto applicationManager = core::IApplicationManager::instance();
        auto uiManager = applicationManager->getUI();

        // Create UI elements
        m_text = uiManager->addElementByType<ui::IUIText>();
        m_image = uiManager->addElementByType<ui::IUIImage>();
        m_button = uiManager->addElementByType<ui::IUIButton>();
        m_label = uiManager->addElementByType<ui::IUIText>();

        // Setup button for selection
        if( m_button )
        {
            m_button->setLabel( "Select Resource" );
            // TODO: Add event listener for selection
        }

        // Setup image for drag-and-drop
        if( m_image )
        {
            // TODO: Add drag-and-drop support
        }
    }

    void ResourceSelect::unload( SmartPtr<ISharedObject> data )
    {
        m_text = nullptr;
        m_image = nullptr;
        m_button = nullptr;
        m_label = nullptr;
        m_material = nullptr;
        m_terrain = nullptr;
    }

}  // namespace workphone::editor
