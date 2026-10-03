#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiImage.hpp>
#include <WPImGui/ImGuiApplication.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiImage, ImGuiElement<IUIImage> );

    ImGuiImage::ImGuiImage() = default;

    ImGuiImage::~ImGuiImage() = default;

    void ImGuiImage::update()
    {
        auto size = getSize();

        if( auto texture = getTexture() )
        {
            if( !texture->isLoaded() )
            {
                texture->load( nullptr );
            }

            void *iTexture = nullptr;
            texture->getTextureGPU( &iTexture );
            m_textureHandle = reinterpret_cast<size_t>( iTexture );

            if( iTexture != nullptr )
            {
                ImGui::Image( iTexture, ImVec2( size.x, size.y ) );

                auto dropTarget = getDropTarget();
                if( dropTarget )
                {
                    if( ImGui::BeginDragDropTarget() )
                    {
                        auto payload = ImGui::AcceptDragDropPayload( "_TREENODE" );
                        if( payload )
                        {
                            auto pData = static_cast<const char *>( payload->Data );
                            auto dataSize = payload->DataSize;
                            auto data = String( pData, dataSize );

                            auto menuItemId = getElementId();

                            auto args = Array<Parameter>();
                            args.resize( 1 );

                            args[0].setStr( data );

                            dropTarget->handleEvent( EventType::UI, IEvent::handleDrop, args, this,
                                                     this, nullptr );
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }
        }
        else
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            SmartPtr<ImGuiApplication> application = ui->getApplication();
            auto emptyTexture = application->getEmptyTexture();

            ImGui::Image( emptyTexture, ImVec2( size.x, size.y ) );

            auto dropTarget = getDropTarget();
            if( dropTarget )
            {
                if( ImGui::BeginDragDropTarget() )
                {
                    auto payload = ImGui::AcceptDragDropPayload( "_TREENODE" );
                    if( payload )
                    {
                        auto pData = static_cast<const char *>( payload->Data );
                        auto dataSize = payload->DataSize;
                        auto data = String( pData, dataSize );

                        auto menuItemId = getElementId();

                        auto args = Array<Parameter>();
                        args.resize( 1 );

                        args[0].setStr( data );

                        dropTarget->handleEvent( EventType::UI, IEvent::handleDrop, args, this, this,
                                                 nullptr );
                    }

                    ImGui::EndDragDropTarget();
                }
            }
        }
    }

    String ImGuiImage::getLabel() const
    {
        return m_label;
    }

    void ImGuiImage::setLabel( const String &label )
    {
        m_label = label;
    }

    void ImGuiImage::setTexture( SmartPtr<render::ITexture> texture )
    {
        m_texture = texture;
    }

    SmartPtr<render::ITexture> ImGuiImage::getTexture() const
    {
        return m_texture;
    }

    void ImGuiImage::setUseTiling( bool useTiling )
    {
        m_useTiling = useTiling;
    }

    bool ImGuiImage::getUseTiling() const
    {
        return m_useTiling;
    }

    void ImGuiImage::setResource( SmartPtr<IResource> resource )
    {
        m_resource = resource;
    }

    SmartPtr<IResource> ImGuiImage::getResource() const
    {
        return m_resource;
    }

    Vector2I ImGuiImage::getSpriteSize() const
    {
        return {};
    }

    void ImGuiImage::setSpriteSize( const Vector2I &spriteSize )
    {
    }

    f32 ImGuiImage::getBorderLeft() const
    {
        return 0.0f;
    }

    void ImGuiImage::setBorderLeft( f32 borderLeft )
    {
    }

    f32 ImGuiImage::getBorderRight() const
    {
        return 0.0f;
    }

    void ImGuiImage::setBorderRight( f32 borderRight )
    {
    }

    f32 ImGuiImage::getBorderTop() const
    {
        return 0.0f;
    }

    void ImGuiImage::setBorderTop( f32 borderTop )
    {
    }

    f32 ImGuiImage::getBorderBottom() const
    {
        return 0.0f;
    }

    void ImGuiImage::setBorderBottom( f32 borderBottom )
    {
    }
}  // namespace workphone::ui
