// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/ImguiOgre/ImGuiOverlayOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRenderSystem.h>
#include <OgreTextureManager.h>
#include <OgreMaterialManager.h>
#include <OgreOverlayManager.h>
#include <OgreFontManager.h>
#include <OgreTechnique.h>
#include <OgreTextureUnitState.h>
#include <OgreFont.h>
#include <OgreRenderQueue.h>
#include <OgreFrameListener.h>
#include <OgreRoot.h>
#include <OgreTimer.h>
#include <imgui.h>
#include <imgui_freetype.h>

namespace workphone
{
    using namespace Ogre;

    u32 ImGuiOverlayOgre::m_frameCount = 0;
    ImGuiOverlayOgre *ImGuiOverlayOgre::m_overlay = nullptr;

    ImGuiOverlayOgre::ImGuiOverlayOgre() : Overlay( "ImGuiOverlay" )
    {
        m_overlay = this;
    }

    ImGuiOverlayOgre::~ImGuiOverlayOgre()
    {
        unload();
    }

    void ImGuiOverlayOgre::load()
    {
        setLoadingState( LoadingState::Loading );

        auto applicationManager = core::IApplicationManager::instance();
        auto ui = applicationManager->getUI();

        auto root = Root::getSingletonPtr();
        auto smgrs = root->getSceneManagers();
        SceneManager *smgr = nullptr;

        auto it = smgrs.begin();
        if( it != smgrs.end() )
        {
            smgr = it->second;
        }

        WP_ASSERT( smgr );

        mRenderable = new ImGUIRenderable;
        m_sceneNode = smgr->getRootSceneNode()->createChildSceneNode();
        m_sceneNode->attachObject( mRenderable );

        auto listener = new RenderQueueListener;
        listener->setOwner( this );
        m_renderQueueListener = listener;

        smgr->addRenderQueueListener( m_renderQueueListener );

        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();

        // Load Font Awesome font
        String fontPath = "media/fonts/FontAwesome.ttf";
        if( !ui->loadFont( fontPath, "awesome" ) )
        {
            WP_LOG_ERROR( "Failed to load Font Awesome font" );
        }

        io.BackendPlatformName = "OGRE";

        auto timer = root->getTimer();

        m_lastTime = timer->getMilliseconds();

        show();

        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiOverlayOgre::unload()
    {
        setLoadingState( LoadingState::Unloading );
        //UnloadFontAwesomeFont();
        ImGui::DestroyContext();
        setLoadingState( LoadingState::Unloaded );
    }

    void ImGuiOverlayOgre::initialise()
    {
        if( !mInitialised )
        {
            mRenderable->initialise();
            mCodePointRanges.clear();
        }

        mInitialised = true;
    }

    void ImGuiOverlayOgre::_findVisibleObjects( Camera *cam, RenderQueue *queue, Viewport *vp )
    {
        if( !mVisible )
            return;

        mRenderable->_update();
        queue->addRenderable( mRenderable, RENDER_QUEUE_SKIES_EARLY, mZOrder * 100 );
    }

    LoadingState ImGuiOverlayOgre::getLoadingState() const
    {
        return m_loadingState;
    }

    void ImGuiOverlayOgre::setLoadingState( LoadingState loadingState )
    {
        m_loadingState = loadingState;
    }

    ImGuiOverlayOgre *ImGuiOverlayOgre::getOverlay()
    {
        return m_overlay;
    }

    void ImGuiOverlayOgre::setOverlay( ImGuiOverlayOgre *overlay )
    {
        m_overlay = overlay;
    }

    void ImGuiOverlayOgre::ImGUIRenderable::createMaterial()
    {
        mMaterial = MaterialManager::getSingleton().create( "ImGui/material", RGN_INTERNAL );
        Pass *mPass = mMaterial->getTechnique( 0 )->getPass( 0 );
        mPass->setCullingMode( CULL_NONE );
        mPass->setVertexColourTracking( TVC_DIFFUSE );
        mPass->setSceneBlending( SBT_TRANSPARENT_ALPHA );
        mPass->setSeparateSceneBlendingOperation( SBO_ADD, SBO_ADD );
        mPass->setSeparateSceneBlending( SBF_SOURCE_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA,
                                         SBF_ONE_MINUS_SOURCE_ALPHA, SBF_ZERO );

        TextureUnitState *mTexUnit = mPass->createTextureUnitState();
        mTexUnit->setTexture( mFontTex );
        mTexUnit->setTextureFiltering( TFO_NONE );

        mMaterial->load();
        mMaterial->setLightingEnabled( false );
        mMaterial->setDepthCheckEnabled( false );
        mMaterial->setReceiveShadows( false );
    }

    ImFont *ImGuiOverlayOgre::addFont( const String &name, const String &group )
    {
        FontPtr font = FontManager::getSingleton().getByName( name.c_str(), group.c_str() );
        OgreAssert( font, "font does not exist" );
        OgreAssert( font->getType() == FT_TRUETYPE, "font must be of FT_TRUETYPE" );
        DataStreamPtr dataStreamPtr =
            ResourceGroupManager::getSingleton().openResource( font->getSource(), font->getGroup() );
        MemoryDataStream ttfchunk( dataStreamPtr, false );  // transfer ownership to imgui

        // convert codepoint ranges for imgui
        CodePointRange cprange;
        for( const auto &r : font->getCodePointRangeList() )
        {
            cprange.push_back( r.first );
            cprange.push_back( r.second );
        }

        ImGuiIO &io = ImGui::GetIO();
        const ImWchar *cprangePtr = io.Fonts->GetGlyphRangesDefault();
        if( !cprange.empty() )
        {
            cprange.push_back( 0 );  // terminate
            mCodePointRanges.push_back( cprange );
            // ptr must persist until createFontTexture
            cprangePtr = mCodePointRanges.back().data();
        }

        float vpScale = OverlayManager::getSingleton().getPixelRatio();

        ImFontConfig cfg;
        strncpy( cfg.Name, name.c_str(), IM_ARRAYSIZE( cfg.Name ) - 1 );
        return io.Fonts->AddFontFromMemoryTTF( ttfchunk.getPtr(), ttfchunk.size(),
                                               font->getTrueTypeSize() * vpScale, &cfg, cprangePtr );
    }

    void ImGuiOverlayOgre::ImGUIRenderable::createFontTexture()
    {
        // Build texture atlas
        ImGuiIO &io = ImGui::GetIO();
        if( io.Fonts->Fonts.empty() )
            io.Fonts->AddFontDefault();
        unsigned char *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

        mFontTex = TextureManager::getSingleton().createManual(
            "ImGui/FontTex", RGN_INTERNAL, TEX_TYPE_2D, width, height, 1, 1, PF_BYTE_RGBA );

        mFontTex->getBuffer()->blitFromMemory(
            PixelBox( Box( 0, 0, width, height ), PF_BYTE_RGBA, pixels ) );
    }

    const MaterialPtr &ImGuiOverlayOgre::ImGUIRenderable::getMaterial() const
    {
        return mMaterial;
    }

    bool ImGuiOverlayOgre::NewFrame()
    {
        auto root = Root::getSingletonPtr();
        auto timer = root->getTimer();

        auto overlay = ImGuiOverlayOgre::getOverlay();
        if( overlay->getLoadingState() == LoadingState::Loaded )
        {
            auto now = timer->getMilliseconds();

            ImGuiIO &io = ImGui::GetIO();
            io.DeltaTime = std::max<f32>( static_cast<f32>( now - overlay->m_lastTime ) / 1000.0f,
                                          1e-4f );  // see
            // https://github.com/ocornut/imgui/commit/3c07ec6a6126fb6b98523a9685d1f0f78ca3c40c

            overlay->m_lastTime = now;

            // Read keyboard modifiers inputs
            io.KeyAlt = false;
            io.KeySuper = false;

            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto window = graphicsSystem->getDefaultWindow();
            auto windowSize = window->getSize();

            auto displaySize = ImVec2( (f32)windowSize.X(), (f32)windowSize.Y() );
            if( displaySize.x > 0 && displaySize.y > 0 )
            {
                io.DisplaySize = displaySize;
            }
            else
            {
                io.DisplaySize = ImVec2( 0, 0 );
            }

            if( io.DisplaySize.x > 0 && io.DisplaySize.y > 0 )
            {
                if( m_frameCount == 0 )
                {
                    // Start the frame
                    ImGui::NewFrame();
                    m_frameCount++;

                    return true;
                }
            }
        }

        return false;
    }

    Ogre::Real ImGuiOverlayOgre::ImGUIRenderable::getBoundingRadius( void ) const
    {
        return (Ogre::Real)1e10;
    }

    Ogre::Real ImGuiOverlayOgre::ImGUIRenderable::getSquaredViewDepth( const Camera * ) const
    {
        return (Ogre::Real)1e10;
    }

    void ImGuiOverlayOgre::ImGUIRenderable::_update()
    {
        if( mMaterial->getSupportedTechniques().empty() )
        {
            mMaterial->load();  // Support for adding lights run time
        }

        RenderSystem *rSys = Root::getSingleton().getRenderSystem();

        // Construct projection matrix, taking texel offset corrections in account (important for
        // DirectX9) See also:
        //     - OGRE-API specific hint: http://www.ogre3d.org/forums/viewtopic.php?f=5&p=536881#p536881
        //     - IMGUI Dx9 demo solution:
        //     https://github.com/ocornut/imgui/blob/v1.50/examples/directx9_example/imgui_impl_dx9.cpp#L127-L138
        ImGuiIO &io = ImGui::GetIO();
        float texelOffsetX = rSys->getHorizontalTexelOffset();
        float texelOffsetY = rSys->getVerticalTexelOffset();
        float L = texelOffsetX;
        float R = io.DisplaySize.x + texelOffsetX;
        float T = texelOffsetY;
        float B = io.DisplaySize.y + texelOffsetY;

        mXform = Ogre::Matrix4( 2.0f / ( R - L ), 0.0f, 0.0f, ( L + R ) / ( L - R ), 0.0f,
                                -2.0f / ( B - T ), 0.0f, ( T + B ) / ( B - T ), 0.0f, 0.0f, -1.0f, 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f );
    }

    bool ImGuiOverlayOgre::ImGUIRenderable::preRender( SceneManager *sm, RenderSystem *rsys )
    {
        if( m_frameCount == 1 )
        {
            ImGuiIO &io = ImGui::GetIO();
            if( io.DisplaySize.x > 0 && io.DisplaySize.y > 0 )
            {
                m_frameCount--;
                WP_ASSERT( m_frameCount == 0 );

                Viewport *vp = rsys->_getViewport();

                // Instruct ImGui to Render() and process the resulting CmdList-s
                // Adopted from https://bitbucket.org/ChaosCreator/imgui-ogre2.1-binding
                // ... Commentary on OGRE forums:
                // http://www.ogre3d.org/forums/viewtopic.php?f=5&t=89081#p531059
                ImGui::Render();
                ImDrawData *draw_data = ImGui::GetDrawData();
                int vpWidth = vp->getActualWidth();
                int vpHeight = vp->getActualHeight();

                //vpWidth = vpWidth * 2.0;
                //vpHeight = vpHeight * 2.0;

                TextureUnitState *tu =
                    mMaterial->getBestTechnique()->getPass( 0 )->getTextureUnitState( 0 );

                updateVertexData( draw_data );

                mRenderOp.indexData->indexStart = 0;
                mRenderOp.vertexData->vertexStart = 0;

                for( int i = 0; i < draw_data->CmdListsCount; ++i )
                {
                    const ImDrawList *draw_list = draw_data->CmdLists[i];
                    mRenderOp.vertexData->vertexCount = draw_list->VtxBuffer.size();

                    for( int j = 0; j < draw_list->CmdBuffer.Size; ++j )
                    {
                        // Create a renderable and fill it's buffers
                        const ImDrawCmd *drawCmd = &draw_list->CmdBuffer[j];

                        // Set scissoring
                        Ogre::Rect scissor( drawCmd->ClipRect.x, drawCmd->ClipRect.y,
                                            drawCmd->ClipRect.z, drawCmd->ClipRect.w );

                        // Clamp bounds to viewport dimensions
                        scissor = scissor.intersect( Ogre::Rect( 0, 0, vpWidth, vpHeight ) );

                        if( drawCmd->TextureId )
                        {
                            auto handle = *static_cast<ResourceHandle *>( drawCmd->TextureId );
                            auto tex = static_pointer_cast<Texture>(
                                TextureManager::getSingleton().getByHandle( handle ) );
                            if( tex )
                            {
                                rsys->_setTexture( 0, true, tex );
                                rsys->_setSampler( 0,
                                                   *TextureManager::getSingleton().getDefaultSampler() );
                            }
                        }

                        rsys->setScissorTest( true, scissor );

                        // Render!
                        mRenderOp.indexData->indexCount = drawCmd->ElemCount;

                        rsys->_render( mRenderOp );

                        if( drawCmd->TextureId )
                        {
                            // reset to pass state
                            rsys->_setTexture( 0, true, mFontTex );
                            rsys->_setSampler( 0, *tu->getSampler() );
                        }

                        // Update counts
                        mRenderOp.indexData->indexStart += drawCmd->ElemCount;
                    }
                    mRenderOp.vertexData->vertexStart += draw_list->VtxBuffer.size();
                }
                rsys->setScissorTest( false );
                return false;
            }
        }

        return false;
    }

    void ImGuiOverlayOgre::ImGUIRenderable::getWorldTransforms( Ogre::Matrix4 *xform ) const
    {
        *xform = mXform;
    }

    void ImGuiOverlayOgre::ImGUIRenderable::getRenderOperation( RenderOperation &op )
    {
        op = mRenderOp;
    }

    const LightList &ImGuiOverlayOgre::ImGUIRenderable::getLights() const
    {
        // Overlayelements should not be lit by the scene, this will not get called
        static LightList ll;
        return ll;
    }

    ImGuiOverlayOgre::ImGUIRenderable::ImGUIRenderable()
    {
        // default overlays to preserve their own detail level
        mPolygonModeOverrideable = false;

        // use identity projection and view matrices
        mUseIdentityProjection = true;
        mUseIdentityView = true;

        setVisibilityFlags( render::IGraphicsObject::UiFlag );
    }

    void ImGuiOverlayOgre::ImGUIRenderable::initialise( void )
    {
        createFontTexture();
        createMaterial();

        mRenderOp.vertexData = OGRE_NEW VertexData();
        mRenderOp.indexData = OGRE_NEW IndexData();

        mRenderOp.vertexData->vertexCount = 0;
        mRenderOp.vertexData->vertexStart = 0;

        mRenderOp.indexData->indexCount = 0;
        mRenderOp.indexData->indexStart = 0;
        mRenderOp.operationType = RenderOperation::OT_TRIANGLE_LIST;
        mRenderOp.useIndexes = true;

        auto decl = mRenderOp.vertexData->vertexDeclaration;

        // vertex declaration
        size_t offset = 0;
        decl->addElement( 0, offset, VET_FLOAT2, VES_POSITION );
        offset += Ogre::VertexElement::getTypeSize( VET_FLOAT2 );
        decl->addElement( 0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0 );
        offset += Ogre::VertexElement::getTypeSize( VET_FLOAT2 );
        decl->addElement( 0, offset, VET_UBYTE4_NORM, VES_DIFFUSE );
    }

    ImGuiOverlayOgre::ImGUIRenderable::~ImGUIRenderable()
    {
        OGRE_DELETE mRenderOp.vertexData;
        OGRE_DELETE mRenderOp.indexData;
    }

    void ImGuiOverlayOgre::ImGUIRenderable::updateVertexData( ImDrawData *draw_data )
    {
        if( !draw_data->TotalVtxCount )
            return;

        VertexBufferBinding *bind = mRenderOp.vertexData->vertexBufferBinding;

        if( bind->getBindings().empty() ||
            bind->getBuffer( 0 )->getNumVertices() < static_cast<size_t>( draw_data->TotalVtxCount ) )
        {
            bind->setBinding( 0, HardwareBufferManager::getSingleton().createVertexBuffer(
                                     sizeof( ImDrawVert ), draw_data->TotalVtxCount, HBU_CPU_TO_GPU ) );
        }
        if( !mRenderOp.indexData->indexBuffer || mRenderOp.indexData->indexBuffer->getNumIndexes() <
                                                     static_cast<size_t>( draw_data->TotalIdxCount ) )
        {
            mRenderOp.indexData->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(
                HardwareIndexBuffer::IT_16BIT, draw_data->TotalIdxCount, HBU_CPU_TO_GPU );
        }

        // Copy all vertices
        size_t vtx_offset = 0;
        size_t idx_offset = 0;
        for( int i = 0; i < draw_data->CmdListsCount; ++i )
        {
            const ImDrawList *draw_list = draw_data->CmdLists[i];
            bind->getBuffer( 0 )->writeData( vtx_offset, draw_list->VtxBuffer.size_in_bytes(),
                                             draw_list->VtxBuffer.Data,
                                             i == 0 );  // discard on first write
            mRenderOp.indexData->indexBuffer->writeData(
                idx_offset, draw_list->IdxBuffer.size_in_bytes(), draw_list->IdxBuffer.Data, i == 0 );
            vtx_offset += draw_list->VtxBuffer.size_in_bytes();
            idx_offset += draw_list->IdxBuffer.size_in_bytes();
        }
    }

    ImGuiOverlayOgre::RenderQueueListener::RenderQueueListener() = default;

    ImGuiOverlayOgre::RenderQueueListener::~RenderQueueListener() = default;

    void ImGuiOverlayOgre::RenderQueueListener::renderQueueStarted( uint8 queueGroupId,
                                                                    const Ogre::String &invocation,
                                                                    bool &skipThisInvocation )
    {
        if( queueGroupId == RENDER_QUEUE_SKIES_EARLY )
        {
            try
            {
                auto root = Root::getSingletonPtr();
                auto renderSystem = root->getRenderSystem();
                auto vp = renderSystem->_getViewport();
                if( vp != nullptr )
                {
                    auto cam = vp->getCamera();

                    auto applicationManager = core::IApplicationManager::instance();
                    WP_ASSERT( applicationManager );

                    auto graphicsSystem = applicationManager->getGraphicsSystem();
                    WP_ASSERT( graphicsSystem );

                    auto sceneManager = graphicsSystem->getGraphicsScene();
                    auto cameras = sceneManager->getGraphicsObjectsByType<render::IGraphicsCamera>();

                    auto renderUI = false;
                    for( auto c : cameras )
                    {
                        Camera *pCam = nullptr;
                        c->_getObject( (void **)&pCam );

                        if( pCam == cam )
                        {
                            if( c->getRenderUI() )
                            {
                                renderUI = true;
                            }
                        }
                    }

                    if( cam )
                    {
                        if( renderUI )
                        {
                            auto sceneMgr = cam->getSceneManager();

                            if( vp->getOverlaysEnabled() )
                            {
                                auto rq = sceneMgr->getRenderQueue();
                                m_owner->_findVisibleObjects( cam, rq, vp );
                            }
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
    }

    ImGuiOverlayOgre *ImGuiOverlayOgre::RenderQueueListener::getOwner() const
    {
        return m_owner;
    }

    void ImGuiOverlayOgre::RenderQueueListener::setOwner( ImGuiOverlayOgre *owner )
    {
        m_owner = owner;
    }

    SmartPtr<render::IGraphicsCamera> ImGuiOverlayOgre::RenderQueueListener::getCamera() const
    {
        auto camera = m_camera.lock();
        return camera;
    }

    void ImGuiOverlayOgre::RenderQueueListener::setCamera( SmartPtr<render::IGraphicsCamera> camera )
    {
        m_camera = camera;
    }

}  // namespace workphone
