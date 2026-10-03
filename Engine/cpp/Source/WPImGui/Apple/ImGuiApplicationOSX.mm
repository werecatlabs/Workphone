// Dear ImGui: standalone example application for OSX + Metal.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include <WPImGui/Apple/ImGuiApplicationOSX.hpp>
#import <Foundation/Foundation.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#else
#import <UIKit/UIKit.h>
#endif

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "imgui.h"
#include "imgui_impl_metal.h"
#if TARGET_OS_OSX
#include "imgui_impl_osx.h"
#endif

#include <Workphone/WPCore.hpp>

#if FB_GRAPHICS_SYSTEM_OGRENEXT
#include "OgreMetalRenderSystem.h"
#include "OgreMetalDevice.h"
#include "OgreMetalWindow.h"
#include "Ogre.h"

#            include <WPGraphicsOgreNext/Ogre/RenderSystems/Metal/include/OgreMetalPrerequisites.h>
#            include <WPGraphicsOgreNext/Ogre/RenderSystems/Metal/include/OgreMetalTextureGpu.h>
#elif FB_GRAPHICS_SYSTEM_OGRE
#else
#endif

namespace workphone
{
    namespace ui
    {

        void ImGuiApplicationOSX::load()
        {
            auto applicationManager = core::ApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto graphicsSystemName = graphicsSystem->getName();
            auto graphicsSystemNameLower = StringUtil::make_lower( graphicsSystemName );

            auto sceneManager = graphicsSystem->getGraphicsScene();
            WP_ASSERT( sceneManager );

            auto window = graphicsSystem->getDefaultWindow();
            WP_ASSERT( window );

            m_window = window;

            static const String ogreNextStr = "ogrenext";
            if ( StringUtil::contains(graphicsSystemNameLower, ogreNextStr) )
            {
#if FB_GRAPHICS_SYSTEM_OGRENEXT
                void *pDevice = nullptr;
                window->getCustomAttribute( "MetalDevice", (void *)&pDevice );

                auto device = (Ogre::MetalDevice *)pDevice;
                m_device = device;

                // Setup Renderer backend
                auto mtlDevice = device->mDevice;
                ImGui_ImplMetal_Init( mtlDevice );

                OgreMetalView *ogreMetalView = nullptr;
                m_window->getCustomAttribute( "UIView", &ogreMetalView );
                auto view = (NSView *)ogreMetalView;

                ImGui_ImplOSX_Init( view );
#endif
            }
        }

        void ImGuiApplicationOSX::update()
        {
#if FB_GRAPHICS_SYSTEM_OGRENEXT
            auto root = Ogre::Root::getSingletonPtr();
            auto metalRenderer = (Ogre::MetalRenderSystem*)root->getRenderSystem();
            auto currentPassDescriptor = (Ogre::MetalRenderPassDescriptor*)metalRenderer->getCurrentPassDescriptor();

            OgreMetalView      *ogreMetalView = nullptr;
            m_window->getCustomAttribute("UIView", &ogreMetalView);
            auto view = (NSView*)ogreMetalView;

            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize.x = view.bounds.size.width;
            io.DisplaySize.y = view.bounds.size.height;

#if TARGET_OS_OSX
            CGFloat framebufferScale = view.window.screen.backingScaleFactor ?: NSScreen.mainScreen.backingScaleFactor;
#else
            //CGFloat framebufferScale = view.window.screen.scale ?: UIScreen.mainScreen.scale;
            CGFloat framebufferScale = view.window.screen.backingScaleFactor ?: NSScreen.mainScreen.backingScaleFactor;
#endif
            io.DisplayFramebufferScale = ImVec2(framebufferScale, framebufferScale);

            //id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];

            //MTLRenderPassDescriptor* renderPassDescriptor = (MTLRenderPassDescriptor* )CFBridgingRelease(currentPassDescriptor); //currentPassDescriptor->getCurrentRenderPassDescriptor();
            MTLRenderPassDescriptor* renderPassDescriptor = (MTLRenderPassDescriptor* )currentPassDescriptor->getPassDescriptor();
            if (renderPassDescriptor == nil)
            {
                //[commandBuffer commit];
                return;
            }

            // Start the Dear ImGui frame
            ImGui_ImplMetal_NewFrame(renderPassDescriptor);
#if TARGET_OS_OSX
            ImGui_ImplOSX_NewFrame(view);
#endif
#endif
        }

        void ImGuiApplicationOSX::postUpdate()
        {
#if FB_GRAPHICS_SYSTEM_OGRENEXT
            auto root = Ogre::Root::getSingletonPtr();
            auto metalRenderer = (Ogre::MetalRenderSystem*)root->getRenderSystem();
            auto currentPassDescriptor = (Ogre::MetalRenderPassDescriptor*)metalRenderer->getCurrentPassDescriptor();

            MTLRenderPassDescriptor* renderPassDescriptor = (MTLRenderPassDescriptor* )currentPassDescriptor->getPassDescriptor();
            if (renderPassDescriptor == nil)
            {
                //[commandBuffer commit];
                return;
            }

            // Rendering
            ImDrawData* draw_data = ImGui::GetDrawData();

            auto commandBuffer = m_device->mCurrentCommandBuffer;

            //renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            //id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            //[renderEncoder pushDebugGroup:@"Dear ImGui rendering"];

            auto renderEncoder = m_device->mRenderEncoder;
            ImGui_ImplMetal_RenderDrawData(draw_data, commandBuffer, renderEncoder);
            //[renderEncoder popDebugGroup];
            //[renderEncoder endEncoding];

            OgreMetalView      *ogreMetalView = nullptr;
            m_window->getCustomAttribute("UIView", &ogreMetalView);
            auto view = (NSView*)ogreMetalView;

            // Present
            //[commandBuffer presentDrawable:view.currentDrawable];
            //[commandBuffer commit];
#endif
        }

        void ImGuiApplicationOSX::draw(SmartPtr<IUIRenderWindow> renderWindow)
        {
            auto renderTexture = renderWindow->getRenderTexture();
            if( renderTexture )
            {
                void* pTexture = nullptr;
                renderTexture->getTextureGPU(&pTexture);

                if( pTexture )
                {
					auto size = ImGui::GetWindowSize();
					auto avail_size = ImGui::GetContentRegionAvail();

					ImGui::Image( pTexture, avail_size );
					renderTexture->setSize( Vector2I( (s32)avail_size.x, (s32)avail_size.y ) );
				}
            }
        }

        void ImGuiApplicationOSX::handleWindowEvent(SmartPtr<render::IWindowEvent> event)
        {
            auto windowMessage  = workphone::static_pointer_cast<WindowMessageData>(event);
            auto e = (NSEvent*) windowMessage->getEvent();
            auto self = (NSView*)windowMessage->getSelf();
            ImGui_ImplOSX_HandleEvent(e, self);
        }

        void ImGuiApplicationOSX::WindowListener::handleEvent(SmartPtr<render::IWindowEvent> event)
        {
            auto windowMessage  = workphone::static_pointer_cast<WindowMessageData>(event);
            auto e = (NSEvent*) windowMessage->getEvent();
            auto self = (NSView*)windowMessage->getSelf();
            ImGui_ImplOSX_HandleEvent(e, self);
        }
    }
}
