#ifndef __GraphicsSystemConfiguration_h__
#define __GraphicsSystemConfiguration_h__

#include <Workphone/System/Director.hpp>

namespace workphone
{
    namespace render
    {
        /** Interface for graphics settings. */
        class WPCore_API GraphicsSettings : public Director
        {
        public:
            /** Constructor. */
            GraphicsSettings();

            /** Virtual destructor. */
            ~GraphicsSettings() override;

            /** Gets a boolean to know if to create a window. */
            bool getCreateWindow() const;

            /** Sets a boolean to know if to create a window. */
            void setCreateWindow( bool createWindow );

            /** Gets a boolean to know if to show the configuration dialog. */
            bool getShowDialog() const;

            /** Sets a boolean to know if to show the configuration dialog. */
            void setShowDialog( bool showDialog );

            bool getCreateRenderUI() const;

            void setCreateRenderUI( bool createRenderUI );

            /** Gets whether presentation waits for vertical synchronization. */
            bool getVSync() const;

            /** Sets whether presentation waits for vertical synchronization. */
            void setVSync( bool enabled );

            WP_CLASS_REGISTER_DECL;

        protected:
            bool m_showDialog = false;
            bool m_createWindow = true;
            bool m_createRenderUI = false;
            bool m_vsync = false;
        };
    }  // namespace render
}  // namespace workphone

#endif  // IGraphicsSystemConfiguration_h__
