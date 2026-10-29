#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/ClawMaterial.hpp"
#include "WPGraphics/ClawUtil.hpp"
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_material.h"
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include "ClawAssetBudget.hpp"
#include "MaterialShaderEditorDX11.hpp"

namespace workphone
{
    namespace render
    {
        struct ClawMaterial::RuntimeState
        {
            StreamBudget cpu;
            MaterialProgramDX11::Ptr program;
            std::shared_ptr<MaterialProgramResourcesDX11> resources;
            MaterialProgramDX11::PublicationPtr publication;
            MaterialProgramReloadDX11::BindingPtr reload;
            MaterialProgramReloadDX11::Ptr controller;
            u64 shaderIDRevision = 0;
            u64 revision = 0;

            RuntimeState()
            {
                String error;
                cpu = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::SourceCpu,
                                                                  sizeof( RuntimeState ), error );
                if( !cpu )
                {
                    throw std::bad_alloc();
                }
            }
        };

        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawMaterial, Material );

        const String ClawMaterial::diffuseColourStr = "diffuseColour";
        const String ClawMaterial::specularColourStr = "specularColour";
        const String ClawMaterial::ambientColourStr = "ambientColour";
        const String ClawMaterial::emissiveColourStr = "emissiveColour";
        const String ClawMaterial::metalnessStr = "metalness";
        const String ClawMaterial::roughnessStr = "roughness";
        const String ClawMaterial::flagsStr = "flags";
        const String ClawMaterial::blendModeStr = "blendMode";
        const String ClawMaterial::cullModeStr = "cullMode";
        const String ClawMaterial::textureNamesStr = "textureNames";
        const String ClawMaterial::dirtyStr = "dirty";

        ClawMaterial::ClawMaterial() : m_material( asset_budget::createMaterial() )
        {
            if( !m_material )
            {
                WP_LOG_ERROR( "ClawMaterial::ClawMaterial: failed to create native material." );
            }
        }

        ClawMaterial::~ClawMaterial()
        {
            detail::forgetShaderReload( this );
            if( m_material )
            {
                wp_graphics_material_destroy( m_material );
                m_material = nullptr;
            }
        }

        void ClawMaterial::setRuntimeProgram( MaterialProgramDX11::Ptr program,
                                              std::shared_ptr<MaterialProgramResourcesDX11> resources )
        {
            if( !program )
            {
                m_runtime.reset();
                detail::shaderReloadAttached( this, 0 );
                return;
            }
            auto candidate = std::make_unique<RuntimeState>();
            candidate->shaderIDRevision = getShaderIDRevision();
            candidate->publication = program->Capture( getShaderParameters(), resources );
            candidate->program = std::move( program );
            candidate->resources = std::move( resources );
            candidate->revision = getShaderParameters().GetRevision();
            m_runtime = std::move( candidate );
            detail::shaderReloadAttached( this, 0 );
        }

        std::shared_ptr<MaterialProgramResourcesDX11> ClawMaterial::runtimeShaderResources() const
        {
            return m_runtime ? m_runtime->resources : nullptr;
        }

        void ClawMaterial::copyRuntimeShaderTo( ClawMaterial &target ) const
        {
            if( !m_runtime )
            {
                return;
            }
            target.setShaderID( getShaderID() );
            target.getShaderParameters() = getShaderParameters();
            if( m_runtime->controller )
            {
                detail::copyShaderFiles( this, &target );
                target.setRuntimeReload( m_runtime->controller, m_runtime->resources );
            }
            else
            {
                target.setRuntimeProgram( m_runtime->program, m_runtime->resources );
            }
        }

        bool ClawMaterial::hasRuntimeProgram() const
        {
            return bool( m_runtime );
        }

        bool ClawMaterial::refreshRuntimeShaderSelection()
        {
            if( !m_runtime )
            {
                return false;
            }
            // A logical shader switch can retain the same layout. Its numeric
            // authoring token keeps unchanged warm submissions free of strings.
            if( m_runtime->shaderIDRevision != getShaderIDRevision() )
            {
                m_runtime.reset();
                detail::forgetShaderReload( this );
                return true;
            }
            return false;
        }

        void ClawMaterial::setRuntimeReload( MaterialProgramReloadDX11::Ptr reload,
                                             std::shared_ptr<MaterialProgramResourcesDX11> resources )
        {
            if( !reload )
            {
                m_runtime.reset();
                detail::shaderReloadAttached( this, 0 );
                return;
            }
            auto candidate = std::make_unique<RuntimeState>();
            candidate->shaderIDRevision = getShaderIDRevision();
            detail::prepareShaderReloadStatus( this );
            candidate->resources = resources;
            candidate->controller = reload;
            candidate->reload = reload->Attach( getShaderParameters(), std::move( resources ) );
            m_runtime = std::move( candidate );
            detail::shaderReloadAttached( this, reload->GetId() );
        }

        MaterialProgramDX11::PublicationPtr ClawMaterial::getRuntimePublication()
        {
            refreshRuntimeShaderSelection();
            if( !m_runtime )
            {
                return {};
            }
            if( m_runtime->reload )
            {
                return m_runtime->reload->GetPublication();
            }
            const auto &values = getShaderParameters();
            if( !m_runtime->program->IsCurrent( values.GetLayout() ) )
            {
                return {};
            }
            if( m_runtime->revision != values.GetRevision() )
            {
                MaterialProgramDX11::PublicationPtr candidate;
                try
                {
                    candidate = m_runtime->program->Capture( values, m_runtime->resources );
                }
                catch( const std::runtime_error & )
                {
                    return {};
                }  // incompatible schema: defer
                m_runtime->publication = std::move( candidate );
                m_runtime->revision = values.GetRevision();
            }
            return m_runtime->publication;
        }

        void ClawMaterial::loadResource( IResource *resource )
        {
            if( !m_material )
            {
                m_material = asset_budget::createMaterial();
                if( !m_material )
                {
                    WP_LOG_ERROR( "ClawMaterial::loadResource: failed to create native material." );
                }
            }
        }

        wp_graphics_material *ClawMaterial::getNativeMaterial() const
        {
            syncNativeMaterial();
            return m_material;
        }

        void ClawMaterial::createMaterialByType()
        {
            // All material setters write pass state. A newly-created Claw material
            // previously had no passes, so edits were silently discarded.
            if( getNumTechniques() == 0u )
            {
                createTechnique();
            }
            for( auto technique : getTechniques() )
            {
                if( technique && technique->getNumPasses() == 0u )
                {
                    technique->createPass();
                }
            }
            Material::createMaterialByType();
        }

        void ClawMaterial::load( SmartPtr<ISharedObject> data )
        {
            auto application = core::IApplicationManager::instance();
            ScopedLock lock( application->getGraphicsSystem() );
            // Selection may load a resource that is already in the graphics queue.
            // Reprocessing that entry must not reload its file over live edits.
            if( isLoaded() )
            {
                return;
            }
            try
            {
                setLoadingState( LoadingState::Loading );
                loadResource( nullptr );
                if( !m_material )
                {
                    throw std::bad_alloc();
                }
                createMaterialByType();
                Material::load( data );
                syncNativeMaterial();
                setLoadingState( LoadingState::Loaded );
                if( m_documentWorkingBytes )
                {
                    String error;
                    auto working = WorldStreamingScheduler::shared()->reserve(
                        StreamMemoryClass::SourceCpu, m_documentWorkingBytes, error );
                    if( !working )
                    {
                        throw std::runtime_error( error.c_str() );
                    }
                    auto document = authoringDocument();
                    if( document.capacity() + 1 > m_documentWorkingBytes )
                    {
                        throw std::runtime_error(
                            "Material authoring document exceeds its preparation allowance" );
                    }
                    auto retained = WorldStreamingScheduler::shared()->reserve(
                        StreamMemoryClass::SourceCpu, document.capacity() + 1, error );
                    if( !retained )
                    {
                        throw std::runtime_error( error.c_str() );
                    }
                    m_residentDocument.swap( document );
                    m_documentBudget = std::move( retained );
                    m_fileResident = true;
                }
            }
            catch( ... )
            {
                // An admission failure must remain retryable by normal manager
                // lookup/load after the owner thread retires other resources.
                unload( nullptr );
                throw;
            }
        }

        void ClawMaterial::loadFromFile( const String &filePath )
        {
            auto application = core::IApplicationManager::instance();
            auto files = application->getFileSystem();
            // The preparation guard compares physical byte counts. Binary reads
            // preserve CRLF bytes on Windows so a stable text file is not rejected.
            auto stream = files->open( filePath, true, true, false, false, false );
            if( !stream )
            {
                stream = files->open( filePath, true, true, false, true, true );
            }
            if( !stream )
            {
                return;  // Procedural names are not reloadable assets.
            }
            const u64 bytes = stream->size();
            // Bound descriptor parsing independently of the city cell count.
            // This allowance covers text, parsed properties and serialization;
            // immutable render versions carry their own exact reservations.
            if( bytes > ( 1ull << 20 ) )
            {
                throw std::runtime_error( "Material source exceeds the 1 MiB descriptor limit" );
            }
            const u64 preparationBytes = bytes * 32 + ( 64ull << 10 );
            String error;
            auto preparation = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::SourceCpu,
                                                                           preparationBytes, error );
            if( !preparation )
            {
                throw std::runtime_error( error.c_str() );
            }
            String text;
            text.resize( static_cast<size_t>( bytes ) );
            stream->seek( 0 );
            size_t offset = 0;
            while( offset < text.size() )
            {
                const auto read = stream->read( &text[offset], text.size() - offset );
                if( !read )
                {
                    throw std::runtime_error( "Material source changed during preparation" );
                }
                offset += read;
            }
            char extra;
            if( stream->read( &extra, 1 ) )
            {
                throw std::runtime_error( "Material source grew during preparation" );
            }
            auto properties = make_ptr<Properties>();
            DataUtil::parse( text, properties.get() );
            fromData( properties );
            FileInfo fileInfo;
            if( files->findFileInfo( filePath, fileInfo ) )
            {
                setFileSystemId( fileInfo.fileId );
            }
            setFilePath( filePath );
            m_documentWorkingBytes = Path::getFileExtension( filePath ) == ".mat" ? preparationBytes : 0;
        }

        String ClawMaterial::authoringDocument() const
        {
            auto properties = static_pointer_cast<Properties>( toData() );
            if( !properties )
            {
                throw std::runtime_error( "Material serialization failed" );
            }
            // Include shader parameters and native/editor aliases as well as
            // the pass graph. Equality protects every serialized authoring edit.
            auto editor = getProperties();
            if( !editor )
            {
                throw std::runtime_error( "Material properties serialization failed" );
            }
            editor->removeProperty( ISharedObject::referencesStr );
            editor->removeProperty( ISharedObject::weakReferencesStr );
            editor->removeProperty( ISharedObject::loadingStateStr );
            editor->removeProperty( dirtyStr );
            editor->setName( "residentEditorProperties" );
            properties->addChild( editor );
            const auto sanitize = [&]( const auto &self, Properties &node ) -> void {
                node.removeProperty( ISharedObject::referencesStr );
                node.removeProperty( ISharedObject::weakReferencesStr );
                node.removeProperty( ISharedObject::loadingStateStr );
                node.removeProperty( ISharedObject::loadedStr );
                if( node.getName() == "textures" && node.getProperty( "texturePath" ).empty() )
                {
                    node.removeProperty( "texturePath" );  // Empty lazy facade equals an empty slot.
                }
                for( const auto &child : node.getChildren() )
                {
                    if( child )
                    {
                        self( self, *child );
                    }
                }
            };
            sanitize( sanitize, *properties );
            return DataUtil::toString( properties.get(), false );
        }

        void ClawMaterial::unload( SmartPtr<ISharedObject> data )
        {
            m_runtime.reset();
            Material::unload( data );
            // The base class clears the list but also caches its primary graph.
            m_pass = nullptr;
            m_technique = nullptr;
            m_cubeTexture = nullptr;
            std::atomic_store( &m_cubicTextures, CubicTextureSnapshot{} );
            m_syncedRenderState.reset();
            m_syncedNativeRevision = 0;
            if( m_material )
            {
                wp_graphics_material_destroy( m_material );
            }
            m_material = nullptr;
            m_fileResident = false;
            m_documentWorkingBytes = 0;
            String().swap( m_residentDocument );
            m_documentBudget.reset();
        }

        bool ClawMaterial::releaseUnusedResources()
        {
            if( !m_fileResident || !isLoaded() || isLoadLocked() || getReferences() != 2 ||
                m_root.load() )
            {
                return false;
            }
            try
            {
                auto application = core::IApplicationManager::instance();
                auto files = application->getFileSystem();
                // Never destroy the sole copy after its backing source disappears.
                auto source = files->open( getFilePath(), true, false, false, false, false );
                if( !source )
                {
                    source = files->open( getFilePath(), true, false, false, true, true );
                }
                if( !source || !source->size() || source->size() > ( 1ull << 20 ) )
                {
                    return false;
                }

                s32 materialWeakOwners = 0;
                if( auto context = getStateContext() )
                {
                    if( context->getOwnerPtr() == this )
                    {
                        ++materialWeakOwners;
                    }
                    for( const auto &listener : context->getStateListeners() )
                    {
                        if( auto owner = dynamic_cast<MaterialStateListener *>( listener.get() ) )
                        {
                            if( owner->getOwner().get() == this )
                            {
                                ++materialWeakOwners;
                            }
                        }
                    }
                }
                const auto techniques = getTechniques();
                for( const auto &technique : techniques )
                {
                    if( !technique )
                    {
                        continue;
                    }
                    // One list owner, this bounded inspection and the optional
                    // primary cache are the only permitted strong owners.
                    const s32 techniqueOwners = 2 + ( m_technique.get() == technique.get() );
                    if( technique->isLoadLocked() || !technique->isEnabled() ||
                        technique->getReferences() != techniqueOwners )
                    {
                        return false;
                    }
                    if( technique->getMaterial().get() == this )
                    {
                        ++materialWeakOwners;
                    }
                    else
                    {
                        return false;
                    }
                    s32 techniqueWeakOwners = 0;
                    const auto passes = technique->getPasses();
                    for( const auto &pass : passes )
                    {
                        if( !pass )
                        {
                            continue;
                        }
                        const s32 passOwners = 2 + ( m_pass.get() == pass.get() );
                        if( pass->isLoadLocked() || !pass->isEnabled() ||
                            pass->getReferences() != passOwners )
                        {
                            return false;
                        }
                        if( pass->getMaterial().get() == this )
                        {
                            ++materialWeakOwners;
                        }
                        else
                        {
                            return false;
                        }
                        if( pass->getParentPtr() == technique.get() )
                        {
                            ++techniqueWeakOwners;
                        }
                        else
                        {
                            return false;
                        }
                        auto concretePass = dynamic_cast<MaterialPass *>( pass.get() );
                        if( !concretePass )
                        {
                            return false;
                        }
                        s32 passWeakOwners = 0;
                        if( auto context = pass->getStateContext() )
                        {
                            auto state = context->getStateById( pass->getId() );
                            if( !state || state->getOwnerPtr() != pass.get() )
                            {
                                return false;
                            }
                            // The managed state and its texture binding observer
                            // each retain an intrinsic weak back-reference.
                            if( !state->getData() ||
                                !state->getData()->isDerived<MaterialPassStateData>() )
                            {
                                return false;
                            }
                            passWeakOwners += 2;
                        }
                        const auto units = concretePass->getMaterializedTextureUnits();
                        for( const auto &unit : units )
                        {
                            if( !unit )
                            {
                                continue;
                            }
                            auto concreteUnit = dynamic_cast<MaterialTexture *>( unit.get() );
                            // Scale/tint/animation facades are not fully represented
                            // by legacy .mat serialization; retain those edits.
                            if( !concreteUnit || concreteUnit->getTextureAuthoringStorageBytes() ||
                                unit->isLoadLocked() || !unit->isEnabled() ||
                                unit->getReferences() != 2 || unit->getWeakReferences() != 0 )
                            {
                                return false;
                            }
                            if( unit->getMaterial().get() == this )
                            {
                                ++materialWeakOwners;
                            }
                            else
                            {
                                return false;
                            }
                            if( unit->getParentPtr() == pass.get() )
                            {
                                ++passWeakOwners;
                            }
                            else
                            {
                                return false;
                            }
                        }
                        if( pass->getWeakReferences() != passWeakOwners )
                        {
                            return false;
                        }
                    }
                    if( technique->getWeakReferences() != techniqueWeakOwners )
                    {
                        return false;
                    }
                }
                if( getWeakReferences() != materialWeakOwners )
                {
                    return false;
                }
                String error;
                auto working = WorldStreamingScheduler::shared()->reserve(
                    StreamMemoryClass::WorkingCpu, m_documentWorkingBytes, error );
                if( !working )
                {
                    return false;
                }
                if( authoringDocument() != m_residentDocument )
                {
                    return false;
                }
                // All serialized authoring values still match the reload source.
                // Independently retained draw snapshots keep their own textures
                // and SourceCpu reservations through their final reader.
                unload( nullptr );
                m_shaderAuthoring.reset();
                m_legacySurface.reset();
                m_shaderParameters = MaterialShaderParametersInstance{};
                m_shaderWorkflow = ShaderWorkflow::Legacy;
                return true;
            }
            catch( const std::exception &error )
            {
                WP_LOG_WARNING( "Material eviction deferred: " + String( error.what() ) );
                return false;
            }
        }

        void ClawMaterial::syncNativeMaterial() const
        {
            if( !m_material )
            {
                return;
            }
            const auto state = getRenderState();
            if( !state )
            {
                return;
            }
            if( m_syncedRenderState == state &&
                m_syncedNativeRevision == wp_graphics_material_get_revision( m_material ) )
            {
                return;
            }
            wp_graphics_material_values values;
            values.struct_size = sizeof( values );
            wp_graphics_material_get_values( m_material, &values );
            values.diffuse = ClawUtil::toCColour( state->diffuseColour );
            values.specular = ClawUtil::toCColour( state->specularColour );
            values.emissive = ClawUtil::toCColour( state->emissiveColour );
            values.metalness = state->metalness;
            values.roughness = state->roughness;
            values.normal_scale = state->normalStrength;
            values.occlusion_strength = state->aoStrength;
            values.emissive_intensity = state->emissionIntensity;
            values.alpha_cutoff = state->alphaClip;
            values.clearcoat = state->clearCoat;
            values.clearcoat_roughness = state->clearCoatRoughness;
            values.transmission = state->refractionAmount;
            values.index_of_refraction = state->refractionIor;
            values.blend_mode = ClawUtil::toCBlendMode( state->blendMode );
            values.cull_mode = state->getFlag( doubleSidedFlag )
                                   ? WORKPHONE_CULL_MODE_NONE
                                   : ClawUtil::toCCullMode( state->cullMode );
            const auto setFlag = [&]( wp_u32 flag, bool enabled ) {
                values.flags = enabled ? values.flags | flag : values.flags & ~flag;
            };
            setFlag( WORKPHONE_MATERIAL_FLAG_TRANSPARENT, state->getFlag( render::transparentFlag ) );
            setFlag( WORKPHONE_MATERIAL_FLAG_CUTOUT, state->getFlag( render::cutoutFlag ) );
            setFlag( WORKPHONE_MATERIAL_FLAG_EMISSION, state->getFlag( emissionEnabledFlag ) );
            setFlag( WORKPHONE_MATERIAL_FLAG_REFRACTION, state->getFlag( refractionEnabledFlag ) );
            setFlag( WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE, state->getFlag( depthWriteFlag ) );
            setFlag( WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK, state->depthTest != 0u );
            wp_graphics_material_set_values( m_material, &values );
            m_syncedRenderState = state;
            m_syncedNativeRevision = wp_graphics_material_get_revision( m_material );
        }

        /*
        void ClawMaterial::setMetalness( f32 metalness )
        {
            Material::setMetalness( metalness );
            if( m_material )
                wp_graphics_material_set_metalness( m_material, metalness );
        }

        f32 ClawMaterial::getMetalness() const
        {
            return m_material ? wp_graphics_material_get_metalness( m_material )
                              : Material::getMetalness();
        }

        void ClawMaterial::setRoughness( f32 roughness )
        {
            Material::setRoughness( roughness );
            if( m_material )
                wp_graphics_material_set_roughness( m_material, roughness );
        }

        f32 ClawMaterial::getRoughness() const
        {
            return m_material ? wp_graphics_material_get_roughness( m_material )
                              : Material::getRoughness();
        }

        void ClawMaterial::setDiffuse( const ColourF &diffuse )
        {
            Material::setDiffuse( diffuse );
            if( m_material )
                wp_graphics_material_set_diffuse( m_material, toCColour( diffuse ) );
        }

        ColourF ClawMaterial::getDiffuse() const
        {
            return m_material ? fromCColour( wp_graphics_material_get_diffuse( m_material ) )
                              : Material::getDiffuse();
        }

        void ClawMaterial::setSpecular( const ColourF &specular )
        {
            Material::setSpecular( specular );
            if( m_material )
                wp_graphics_material_set_specular( m_material, toCColour( specular ) );
        }

        ColourF ClawMaterial::getSpecular() const
        {
            return m_material ? fromCColour( wp_graphics_material_get_specular( m_material ) )
                              : Material::getSpecular();
        }

        void ClawMaterial::setEmissive( const ColourF &emissive )
        {
            Material::setEmissive( emissive );
            if( m_material )
                wp_graphics_material_set_emissive( m_material, toCColour( emissive ) );
        }

        ColourF ClawMaterial::getEmissive() const
        {
            return m_material ? fromCColour( wp_graphics_material_get_emissive( m_material ) )
                              : Material::getEmissive();
        }

        void ClawMaterial::setNormalStrength( f32 value )
        {
            Material::setNormalStrength( value );
            if( m_material )
                wp_graphics_material_set_normal_scale( m_material, value );
        }

        f32 ClawMaterial::getNormalStrength() const
        {
            return m_material ? wp_graphics_material_get_normal_scale( m_material )
                              : Material::getNormalStrength();
        }

        void ClawMaterial::setAlphaClip( f32 value )
        {
            Material::setAlphaClip( value );
            if( m_material )
                wp_graphics_material_set_alpha_cutoff( m_material, value );
        }

        f32 ClawMaterial::getAlphaClip() const
        {
            return m_material ? wp_graphics_material_get_alpha_cutoff( m_material )
                              : Material::getAlphaClip();
        }

        void ClawMaterial::setOpacity( f32 value )
        {
            Material::setOpacity( value );
            if( m_material )
            {
                auto colour = wp_graphics_material_get_diffuse( m_material );
                colour.a = value;
                wp_graphics_material_set_diffuse( m_material, colour );
                wp_graphics_material_set_flag( m_material, WORKPHONE_MATERIAL_FLAG_TRANSPARENT,
                                               value < 0.999f ? 1 : 0 );
            }
        }

        f32 ClawMaterial::getOpacity() const
        {
            return m_material ? wp_graphics_material_get_diffuse( m_material ).a
                              : Material::getOpacity();
        }

        void ClawMaterial::setBlendMode( u32 mode )
        {
            Material::setBlendMode( mode );
            if( m_material )
                wp_graphics_material_set_blend_mode( m_material, toCBlendMode( mode ) );
        }

        u32 ClawMaterial::getBlendMode() const
        {
            return m_material ? fromCBlendMode( wp_graphics_material_get_blend_mode( m_material ) )
                              : Material::getBlendMode();
        }

        void ClawMaterial::setDepthWrite( bool enabled )
        {
            Material::setDepthWrite( enabled );
            if( m_material )
                wp_graphics_material_set_flag( m_material, WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE,
                                               enabled ? 1 : 0 );
        }

        bool ClawMaterial::getDepthWrite() const
        {
            return m_material ? wp_graphics_material_has_flag( m_material,
                                                               WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE ) != 0
                              : Material::getDepthWrite();
        }

        void ClawMaterial::setDepthTest( u32 mode )
        {
            Material::setDepthTest( mode );
            if( m_material )
                wp_graphics_material_set_flag( m_material, WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK,
                                               mode != 0u ? 1 : 0 );
        }

        u32 ClawMaterial::getDepthTest() const
        {
            return m_material && !wp_graphics_material_has_flag( m_material,
                                                                 WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK )
                       ? 0u
                       : Material::getDepthTest();
        }

        void ClawMaterial::setCullMode( u32 mode )
        {
            Material::setCullMode( mode );
            if( m_material )
                wp_graphics_material_set_cull_mode( m_material, toCCullMode( mode ) );
        }

        u32 ClawMaterial::getCullMode() const
        {
            return m_material ? fromCCullMode( wp_graphics_material_get_cull_mode( m_material ) )
                              : Material::getCullMode();
        }

        void ClawMaterial::setTransparent( bool transparent )
        {
            Material::setTransparent( transparent );
            if( m_material )
                wp_graphics_material_set_flag( m_material, WORKPHONE_MATERIAL_FLAG_TRANSPARENT,
                                               transparent ? 1 : 0 );
        }

        bool ClawMaterial::isTransparent() const
        {
            return m_material ? wp_graphics_material_has_flag( m_material,
                                                               WORKPHONE_MATERIAL_FLAG_TRANSPARENT ) != 0
                              : Material::isTransparent();
        }

        void ClawMaterial::setCutout( bool cutout )
        {
            Material::setCutout( cutout );
            if( m_material )
                wp_graphics_material_set_flag( m_material, WORKPHONE_MATERIAL_FLAG_CUTOUT,
                                               cutout ? 1 : 0 );
        }

        bool ClawMaterial::isCutout() const
        {
            return m_material
                       ? wp_graphics_material_has_flag( m_material, WORKPHONE_MATERIAL_FLAG_CUTOUT ) != 0
                       : Material::isCutout();
        }
        */

        SmartPtr<Properties> ClawMaterial::getProperties() const
        {
            try
            {
                syncNativeMaterial();
                auto properties = Material::getProperties();
                if( !properties )
                {
                    WP_LOG_WARNING(
                        "ClawMaterial::getProperties: base class returned null properties." );
                    return {};
                }

                if( !m_material )
                {
                    WP_LOG_WARNING(
                        "ClawMaterial::getProperties: native material is null, returning base "
                        "properties." );
                    detail::appendShaderFiles( this, properties );
                    return properties;
                }

                properties->setProperty(
                    diffuseColourStr,
                    ClawUtil::fromCColour( wp_graphics_material_get_diffuse( m_material ) ) );
                properties->setProperty(
                    specularColourStr,
                    ClawUtil::fromCColour( wp_graphics_material_get_specular( m_material ) ) );
                properties->setProperty(
                    ambientColourStr,
                    ClawUtil::fromCColour( wp_graphics_material_get_ambient( m_material ) ) );
                properties->setProperty(
                    emissiveColourStr,
                    ClawUtil::fromCColour( wp_graphics_material_get_emissive( m_material ) ) );
                properties->setProperty( metalnessStr,
                                         wp_graphics_material_get_metalness( m_material ) );
                properties->setProperty( roughnessStr,
                                         wp_graphics_material_get_roughness( m_material ) );
                properties->setProperty( flagsStr, wp_graphics_material_get_flags( m_material ) );
                properties->setProperty( blendModeStr, getBlendMode() );
                properties->setProperty( cullModeStr, getCullMode() );

                auto textureProperties = workphone::make_ptr<Properties>();
                textureProperties->setName( textureNamesStr );
                properties->addChild( textureProperties );

                for( s32 layer = 0; layer < WP_MATERIAL_MAX_TEXTURES; ++layer )
                {
                    const auto name = getTextureName( static_cast<u32>( layer ) );
                    textureProperties->setProperty( "texture" + String( std::to_string( layer ) ),
                                                    name );
                }

                properties->setProperty( dirtyStr, wp_graphics_material_is_dirty( m_material ) != 0 );
                detail::appendShaderFiles( this, properties );
                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return {};
        }

        void ClawMaterial::setProperties( SmartPtr<Properties> properties )
        {
            try
            {
                if( !properties )
                {
                    WP_LOG_WARNING( "ClawMaterial::setProperties: null properties supplied." );
                    return;
                }

                MaterialPublicationScope publication;
                Material::setProperties( properties );
                if( detail::restoreShaderSchema( this, properties ) )
                {
                    restoreShaderParameterProperties( properties );
                }
                String pixel, vertex;
                bool watching = true;
                if( properties->getPropertyValue( "runtimeShaderPixelFile", pixel ) && !pixel.empty() )
                {
                    properties->getPropertyValue( "runtimeShaderVertexFile", vertex );
                    requestRuntimeShaderFiles( vertex, pixel );
                    if( properties->getPropertyValue( "runtimeShaderWatching", watching ) )
                    {
                        setRuntimeShaderWatching( watching );
                    }
                }

                if( !m_material )
                {
                    WP_LOG_WARNING(
                        "ClawMaterial::setProperties: native material is null, only base properties "
                        "applied." );
                    return;
                }

                // A saved bag contains both shader values and native aliases.
                // Restore aliases as pass overrides when an authored semantic is
                // present, rather than feeding those mirrors back into its owner.
                // A partial legacy-only bag remains a material-wide editor edit.
                const auto authored = [&]( const String &name ) {
                    const bool active =
                        m_shaderWorkflow == ShaderWorkflow::Pbr ||
                        ( m_shaderWorkflow == ShaderWorkflow::Unlit &&
                          ( name == shaderEmissiveParamStr || name == shaderOpacityParamStr ) ) ||
                        ( m_shaderWorkflow == ShaderWorkflow::ColorOnly &&
                          ( name == shaderColorParamStr || name == shaderOpacityParamStr ) );
                    return active && properties->hasProperty( shaderParamPrefixStr + name );
                };
                const auto restorePasses = [&]( const auto &update ) {
                    for( auto technique : getTechniques() )
                    {
                        if( technique )
                        {
                            for( auto pass : technique->getPasses() )
                            {
                                if( pass )
                                {
                                    update( *pass );
                                }
                            }
                        }
                    }
                };
                auto diffuseColour = getDiffuse();
                if( properties->getPropertyValue( diffuseColourStr, diffuseColour ) )
                {
                    if( authored( shaderAlbedoParamStr ) || authored( shaderColorParamStr ) ||
                        authored( shaderOpacityParamStr ) )
                    {
                        restorePasses(
                            [&]( IMaterialPass &pass ) { pass.setDiffuse( diffuseColour ); } );
                    }
                    else
                    {
                        setDiffuse( diffuseColour );
                    }
                }

                auto specularColour = getSpecular();
                if( properties->getPropertyValue( specularColourStr, specularColour ) )
                {
                    setSpecular( specularColour );
                }

                auto ambientColour =
                    ClawUtil::fromCColour( wp_graphics_material_get_ambient( m_material ) );
                if( properties->getPropertyValue( ambientColourStr, ambientColour ) )
                {
                    wp_graphics_material_set_ambient( m_material, ClawUtil::toCColour( ambientColour ) );
                }

                auto emissiveColour = getEmissive();
                if( properties->getPropertyValue( emissiveColourStr, emissiveColour ) )
                {
                    if( authored( shaderEmissiveParamStr ) || authored( shaderColorParamStr ) )
                    {
                        restorePasses(
                            [&]( IMaterialPass &pass ) { pass.setEmissive( emissiveColour ); } );
                    }
                    else
                    {
                        setEmissive( emissiveColour );
                    }
                }

                auto metalness = getMetalness();
                if( properties->getPropertyValue( metalnessStr, metalness ) )
                {
                    if( authored( shaderMetalnessParamStr ) )
                    {
                        restorePasses( [&]( IMaterialPass &pass ) { pass.setMetalness( metalness ); } );
                    }
                    else
                    {
                        setMetalness( metalness );
                    }
                }

                auto roughness = getRoughness();
                if( properties->getPropertyValue( roughnessStr, roughness ) )
                {
                    if( authored( shaderRoughnessParamStr ) )
                    {
                        restorePasses( [&]( IMaterialPass &pass ) { pass.setRoughness( roughness ); } );
                    }
                    else
                    {
                        setRoughness( roughness );
                    }
                }

                auto flags = wp_graphics_material_get_flags( m_material );
                if( properties->getPropertyValue( flagsStr, flags ) )
                {
                    wp_graphics_material_set_flags( m_material, flags );
                    setTransparent( ( flags & WORKPHONE_MATERIAL_FLAG_TRANSPARENT ) != 0 );
                    setCutout( ( flags & WORKPHONE_MATERIAL_FLAG_CUTOUT ) != 0 );
                    setEmissionEnabled( ( flags & WORKPHONE_MATERIAL_FLAG_EMISSION ) != 0 );
                    setRefractionEnabled( ( flags & WORKPHONE_MATERIAL_FLAG_REFRACTION ) != 0 );
                    setDepthWrite( ( flags & WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE ) != 0 );
                    if( !( flags & WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK ) )
                    {
                        setDepthTest( 0u );
                    }
                }

                auto blendMode = static_cast<u32>( wp_graphics_material_get_blend_mode( m_material ) );
                if( properties->getPropertyValue( blendModeStr, blendMode ) )
                {
                    setBlendMode( blendMode );
                }

                auto cullMode = static_cast<u32>( wp_graphics_material_get_cull_mode( m_material ) );
                if( properties->getPropertyValue( cullModeStr, cullMode ) )
                {
                    setCullMode( cullMode );
                }

                if( auto textureProperties = properties->getChild( textureNamesStr ) )
                {
                    for( s32 layer = 0; layer < WP_MATERIAL_MAX_TEXTURES; ++layer )
                    {
                        auto textureName = String();
                        String propertyName = "texture" + String( std::to_string( layer ) );
                        if( textureProperties->getPropertyValue( propertyName, textureName ) )
                        {
                            auto cStr = static_cast<const wp_c8 *>(
                                static_cast<const void *>( textureName.c_str() ) );
                            wp_graphics_material_set_texture_name( m_material, layer, cStr );
                            if( textureName.empty() )
                            {
                                setTexture( SmartPtr<ITexture>(), static_cast<u32>( layer ) );
                            }
                            else
                            {
                                setTexture( textureName, static_cast<u32>( layer ) );
                            }
                        }
                    }
                }

                bool dirty = wp_graphics_material_is_dirty( m_material ) != 0;
                if( properties->getPropertyValue( dirtyStr, dirty ) )
                {
                    if( dirty )
                    {
                        wp_graphics_material_mark_dirty( m_material );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void ClawMaterial::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = getNativeMaterial();
            }
        }

        bool ClawMaterial::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool ClawMaterial::handleStateChanged( SmartPtr<IState> &state )
        {
            return Material::handleStateChanged( state );
        }

    }  // namespace render
}  // namespace workphone
