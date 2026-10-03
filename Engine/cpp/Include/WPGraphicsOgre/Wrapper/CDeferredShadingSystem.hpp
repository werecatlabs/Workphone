#ifndef CDeferredShadingSystem_h__
#define CDeferredShadingSystem_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IGraphicsDeferredShading.hpp>

namespace workphone
{
    namespace render
    {

        class CDeferredShadingSystem : public IGraphicsDeferredShading
        {
        public:
            CDeferredShadingSystem( SmartPtr<IViewport> viewport );
            ~CDeferredShadingSystem();

            void load( SmartPtr<ISharedObject> data );
            void unload( SmartPtr<ISharedObject> data );

            u32 getMode() const;
            void setMode( u32 mode );

            bool getSSAO() const;
            void setSSAO( bool ssao );

            bool getActive() const;
            void setActive( bool active );

            bool getShadowsEnabled() const;
            void setShadowsEnabled( bool enabled );

            DeferredShadingSystem *getDeferredShading() const;
            void setDeferredShading( DeferredShadingSystem *deferredShading );

            void update();

            String getGBufferCompositorName() const;
            void setGBufferCompositorName( const String &gBufferCompositorName );

            String getShowLightingCompositorName() const;
            void setShowLightingCompositorName( const String &showLightingCompositorName );

            String getShowNormalsCompositorName() const;
            void setShowNormalsCompositorName( const String &showNormalsCompositorName );

            String getShowDepthSpecularCompositorName() const;
            void setShowDepthSpecularCompositorName( const String &showDepthSpecularCompositorName );

            String getShowColourCompositorName() const;
            void setShowColourCompositorName( const String &showColourCompositorName );

            virtual void initialise( SmartPtr<IBuildDirector> objectTemplate );

            virtual void initialise( SmartPtr<IBuildDirector> objectTemplate,
                                     SmartPtr<Properties> instanceProperties );

            WP_CLASS_REGISTER_DECL;

        protected:
            DeferredShadingSystem *mSystem;
            time_interval m_nextShadowUpdate = 0.0;
        };

    }  // namespace render
}  // namespace workphone

#endif  // CDeferredShadingSystem_h__
