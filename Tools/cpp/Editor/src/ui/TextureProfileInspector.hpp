#pragma once

#if WP_EDITOR_TEXTURE_PROFILE
#    include <WPGraphics/Resources/TextureProfileAuthoring.hpp>
#    include <Workphone/Memory/SmartPtr.hpp>
#    include <Workphone/System/WorldStreamingScheduler.hpp>

namespace workphone::editor
{
    // Application-task owner. Workers only receive immutable request values.
    class TextureProfileInspector
    {
    public:
        TextureProfileInspector( SmartPtr<ui::IUIManager> ui, SmartPtr<ui::IUIWindow> parent,
                                 SmartPtr<IEventListener> listener );
        ~TextureProfileInspector();

        bool update( SmartPtr<ISharedObject> selection );
        bool ownsEvent( SmartPtr<ISharedObject> sender ) const;
        bool handle( SmartPtr<ISharedObject> sender, SmartPtr<Properties> displayed,
                     const String &name, const String &value, bool isButton );
        static bool isTextureDescriptor( SmartPtr<ISharedObject> selection );

    private:
        void reload();
        void refresh();
        void requestPreview();
        void takePreview();
        void discardPreview();
        void retireImage();
        void setStatus( const String &status );

        SmartPtr<ui::IUIPropertyGrid> m_grid;
        SmartPtr<ui::IUIImage> m_image;
        SmartPtr<Properties> m_displayed;
        std::shared_ptr<WorldStreamProducer> m_jobs;
        SmartPtr<ISharedObject> m_projectIdentity;
        String m_project, m_path, m_customTarget = "pc", m_status;
        render::TextureProfileDocument m_document;
        render::TextureProfileSettings m_draft;
        s32 m_target = 0, m_mip = 0, m_channel = 0;
        u64 m_pending = 0, m_next = 0, m_documentGeneration = 0;
    };
}
#endif
