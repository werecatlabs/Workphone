#ifndef ClawWindow_h__
#define ClawWindow_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>

struct wp_graphics_window;

#if defined WP_PLATFORM_WIN32
struct wp_platform_window_win32;
#endif

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawWindow
         * @brief Graphics window implementation for the ClawHammer backend.
         *
         * ClawWindow reuses the backend-agnostic GraphicsWindow base for window
         * state/event handling and supplies a working addViewport() implementation.
         * The base RenderTarget::addViewport() returns null, which would trip the
         * engine's viewport assertions, so this override creates ClawViewport
         * instances and manages them in a local collection.
         */
        class WPGraphics_API ClawWindow : public GraphicsWindow
        {
        public:
            /** @brief Default constructor. */
            ClawWindow();

            /** @brief Destructor. Removes any attached viewports. */
            ~ClawWindow() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            String getTitle() const override;
            void setTitle( const String &title ) override;
            void destroy() override;
            void resize( u32 width, u32 height ) override;
            void maximize() override;
            bool isVisible() const override;
            void setVisible( bool visible ) override;
            bool isClosed() const override;
            Vector2I getSize() const override;
            void setSize( const Vector2I &size ) override;
            void getWindowHandle( void *pData ) override;
            String getWindowHandleAsString() const override;

            /** Pump native window events. Returns false after the window closes. */
            bool messagePump();

            /** @copydoc IRenderTarget::addViewport */
            SmartPtr<IViewport> addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera,
                                             s32 ZOrder = -1, f32 left = 0.0f, f32 top = 0.0f,
                                             f32 width = 1.0f, f32 height = 1.0f ) override;

            SmartPtr<IViewport> getViewport( u32 index ) override;
            Array<SmartPtr<IViewport>> getViewports() const override;
            u32 getNumViewports() const override;

            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            wp_graphics_window *m_window = nullptr;

#if defined WP_PLATFORM_WIN32
            wp_platform_window_win32 *m_platformWindow = nullptr;
#endif
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawWindow_h__
