#include <Workphone/WorkphonePCH.hpp>
#include "ProceduralProperties.hpp"
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        struct VehicleEventScratch
        {
            Array<procedural::VehicleEffectEvent> &recycled;
            Array<procedural::VehicleEffectEvent> staged{ Allocator<procedural::VehicleEffectEvent>(
                sourceBudget ) };

            explicit VehicleEventScratch( Array<procedural::VehicleEffectEvent> &storage ) :
                recycled( storage )
            {
                staged.swap( recycled );
            }

            ~VehicleEventScratch() noexcept
            {
                if( staged.get_allocator().isSourceBudgeted() )
                {
                    staged.clear();
                    recycled.swap( staged );
                }
            }
        };
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralVehicle, ProceduralMesh );

    ProceduralVehicle::ProceduralVehicle()
    {
        m_config.physicsPreset = procedural::VehiclePhysicsPreset::Warbird;
        m_config.geometry.bodyStyle = procedural::VehicleBodyStyle::Warbird;
        m_config.geometry.bodyLength = 9.8f;
        m_config.geometry.bodyWidth = 1.2f;
        m_config.appearance.primary = { .12f, .15f, .10f, 1.0f };
        m_config.appearance.secondary = { .10f, .065f, .035f, 1.0f };
    }

    ProceduralVehicle::~ProceduralVehicle() = default;

    SmartPtr<IMesh> ProceduralVehicle::buildMesh()
    {
        using namespace procedural;

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManager();

        if( !m_generator )
        {
            m_generator = factoryManager->make_object<IVehicleGenerator>( "IVehicleGenerator" );
        }

        if( !m_generator )
        {
            throw std::runtime_error( "Load the WPProcedural plugin to generate vehicles" );
        }

        auto vehicle = m_generator->generate( m_config );
        if( !vehicle.isValid() )
        {
            String errors;
            for( const auto &issue : vehicle.issues )
            {
                if( issue.error )
                {
                    errors += String( issue.component.data(), issue.component.size() );
                    errors += ": ";
                    errors += String( issue.message.data(), issue.message.size() );
                    errors += "\n";
                }
            }
            throw std::runtime_error( errors );
        }

        if( m_lod >= vehicle.geometry.lods.size() )
        {
            throw std::runtime_error( "Selected LOD does not exist" );
        }

        auto mesh = workphone::make_ptr<workphone::Mesh>();
        for( const auto &section : vehicle.geometry.lods[m_lod].sections )
        {
            if( !section.hasGeometry() )
            {
                continue;
            }

            auto positions = makeSourceBudgetSnapshot<Vector3<real_Num>>();
            auto normals = makeSourceBudgetSnapshot<Vector3<real_Num>>();
            auto uvs = makeSourceBudgetSnapshot<Vector3<real_Num>>();
            auto tangents = makeSourceBudgetSnapshot<Vector4F>();
            auto indices = makeSourceBudgetSnapshot<u32>();
            indices.reserve( section.indices.size() );
            for( const auto index : section.indices )
            {
                indices.push_back( index );
            }
            positions.reserve( section.vertices.size() );
            normals.reserve( section.vertices.size() );
            uvs.reserve( section.vertices.size() );
            tangents.reserve( section.vertices.size() );

            for( const auto &v : section.vertices )
            {
                positions.emplace_back( v.position.x, v.position.y, v.position.z );
                normals.emplace_back( v.normal.x, v.normal.y, v.normal.z );
                tangents.emplace_back( v.tangent.x, v.tangent.y, v.tangent.z, v.tangentSign );
                uvs.emplace_back( v.uv.x, v.uv.y, 0 );
            }

            auto part = MeshUtil::createMesh( positions, normals, tangents, uvs, indices );
            if( !part )
            {
                throw std::runtime_error( "Cannot create generated vehicle geometry" );
            }
            for( auto sub : part->getSubMeshes() )
            {
                const auto &material =
                    vehicle.appearance.material( m_generator->materialSlotFor( section.material ) );
                sub->setMaterialName( String( material.name.data(), material.name.size() ) );
                mesh->addSubMesh( sub );
            }
        }

        if( !m_dynamics )
        {
            m_dynamics = factoryManager->make_object<IVehicleDynamics>( "IVehicleDynamics" );
        }

        if( !m_damage )
        {
            m_damage = factoryManager->make_object<IVehicleDamage>( "IVehicleDamage" );
        }

        if( !m_presentation )
        {
            m_presentation = factoryManager->make_object<IVehiclePresentation>( "IVehiclePresentation" );
        }

        if( !m_effects )
        {
            m_effects = factoryManager->make_object<IVehicleEffects>( "IVehicleEffects" );
        }

        if( m_dynamics )
        {
            m_dynamicsState = m_dynamics->reset( vehicle.physics );
        }

        if( m_damage )
        {
            m_damageState = m_damage->reset( m_config.seed );
        }

        if( m_presentation )
        {
            m_presentationState = m_presentation->reset( vehicle.physics );
        }

        if( m_effects )
        {
            m_effectsState = m_effects->reset( m_config.seed );
        }

        m_telemetry = {};
        m_events.clear();
        m_vehicle = std::move( vehicle );

        return mesh;
    }

    void ProceduralVehicle::applyGeneratedMaterials()
    {
        auto actor = getActor();

        auto app = core::IApplicationManager::instancePtr();
        if( !actor || !app || !app->getGraphicsSystem() || !m_generator ||
            m_lod >= m_vehicle.geometry.lods.size() )
        {
            return;
        }

        auto graphicsSystem = app->getGraphicsSystem();
        auto materialManager = graphicsSystem->getMaterialManager();
        if( !materialManager )
        {
            return;
        }

        const auto &sections = m_vehicle.geometry.lods[m_lod].sections;
        const auto required = static_cast<size_t>(
            std::count_if( sections.begin(), sections.end(),
                           []( const auto &section ) { return section.hasGeometry(); } ) );
        if( !m_generatedMaterials.get_allocator().isSourceBudgeted() )
        {
            auto admitted = makeSourceBudgetSnapshot( m_generatedMaterials );
            m_generatedMaterials.swap( admitted );
        }

        // Respect explicitly authored Material components on the actor. Discovery
        // is staged so a denied owner inventory cannot publish a partial match.
        if( m_generatedMaterials.empty() )
        {
            const auto existing = actor->getComponentsByType<Material>();
            auto discovered = makeSourceBudgetSnapshot<SmartPtr<Material>>();
            discovered.reserve( std::max( existing.capacity(), required ) );
            for( const auto &component : existing )
            {
                const auto id = ";" + component->getHandle()->getUUIDAsString() + ";";
                if( m_generatedMaterialIds.find( id ) != String::npos )
                {
                    discovered.push_back( component );
                }
            }

            if( discovered.empty() && !existing.empty() )
            {
                return;
            }

            std::sort( discovered.begin(), discovered.end(),
                       []( const auto &a, const auto &b ) { return a->getIndex() < b->getIndex(); } );
            m_generatedMaterials.swap( discovered );
        }
        // Admit every owner slot before any manager-owned material or actor
        // component is created. Later configuration failures remain cleanup-owned.
        m_generatedMaterials.reserve( required );

        u32 index = 0;
        for( const auto &section : sections )
        {
            if( !section.hasGeometry() )
            {
                continue;
            }

            const auto &descriptor =
                m_vehicle.appearance.material( m_generator->materialSlotFor( section.material ) );
            SmartPtr<Material> component;
            SmartPtr<render::IMaterial> material;
            bool createdComponent = false;
            if( index < m_generatedMaterials.size() )
            {
                component = m_generatedMaterials[index];
                material = component->getMaterial();
            }
            else
            {
                component = actor->addComponent<Material>();
                if( !component )
                {
                    throw std::runtime_error( "Cannot create generated vehicle material component" );
                }
                m_generatedMaterials.push_back( component );
                createdComponent = true;
                component->getHandle()->setUUID( StringUtil::getUUID() );
                component->setIndex( index );
                material = component->getMaterial();
            }

            if( !material )
            {
                material = workphone::dynamic_pointer_cast<render::IMaterial>(
                    materialManager->create( "ProceduralVehicle/" + StringUtil::getUUID() ) );
                if( !material )
                {
                    throw std::runtime_error( "Cannot create generated vehicle material" );
                }
                try
                {
                    component->setMaterial( material );
                }
                catch( ... )
                {
                    try
                    {
                        materialManager->destroyResource( material );
                    }
                    catch( const std::bad_alloc & )
                    {
                    }
                    throw;
                }
            }
            if( createdComponent )
            {
                component->setState( getState() );
            }

            if( material )
            {
                const auto &color = descriptor.fallbackBaseColor;
                material->setDiffuse( ColourF( color.r, color.g, color.b, color.a ) );
                material->setRoughness( descriptor.fallbackRoughness );
                material->setMetalness( descriptor.metallic );
                material->setOpacity( descriptor.opacity );
                const auto &emissive = descriptor.emissiveColor;
                material->setEmissive( ColourF(
                    emissive.r * descriptor.emissiveIntensity, emissive.g * descriptor.emissiveIntensity,
                    emissive.b * descriptor.emissiveIntensity, emissive.a ) );
            }
            ++index;
        }

        while( m_generatedMaterials.size() > index )
        {
            auto component = m_generatedMaterials.back();
            m_generatedMaterials.pop_back();
            auto material = component->getMaterial();
            try
            {
                component->unload( nullptr );
            }
            catch( const std::bad_alloc & )
            {
            }
            try
            {
                actor->removeComponentInstance( component );
            }
            catch( const std::bad_alloc & )
            {
            }
            if( material )
            {
                try
                {
                    materialManager->destroyResource( material );
                }
                catch( const std::bad_alloc & )
                {
                }
            }
        }
    }

    bool ProceduralVehicle::stepFixed( const procedural::VehicleControlInput &input,
                                       const std::array<procedural::VehicleSurfaceSample, 4> &surfaces,
                                       double deltaSeconds )
    {
        using namespace procedural;
        if( m_vehicle.physics.preset == procedural::VehiclePhysicsPreset::HelicopterLittleBird ||
            m_vehicle.physics.preset == procedural::VehiclePhysicsPreset::Warbird || !m_dynamics ||
            !m_damage || !m_presentation || !m_effects || !m_vehicle.geometry.hasGeometry() )
        {
            return false;
        }

        if( !std::isfinite( deltaSeconds ) || deltaSeconds <= 0 )
        {
            return false;
        }

        // Stage updates so failure leaves the published simulation state unchanged.
        auto dynamics = m_dynamicsState;
        auto damage = m_damageState;
        auto presentation = m_presentationState;
        auto effects = m_effectsState;
        VehicleDynamicsTelemetry telemetry;
        VehicleEventScratch stagedEvents( m_eventScratch );
        auto &events = stagedEvents.staged;
        const auto denialEpoch = sourceBudgetDenialCount();
        if( !m_damage->update( {}, deltaSeconds, damage ) )
        {
            return false;
        }

        const auto damageTelemetry = m_damage->telemetry( {}, damage );
        if( !m_dynamics->stepFixed( m_vehicle.physics, {}, input, surfaces,
                                    m_damage->dynamicsModifiers( damageTelemetry ), deltaSeconds,
                                    dynamics, telemetry ) )
        {
            return false;
        }

        VehiclePresentationInput presentationInput;
        presentationInput.throttle = input.throttle;
        if( !m_presentation->update( {}, m_vehicle.physics, dynamics, telemetry, damageTelemetry,
                                     presentationInput, deltaSeconds, presentation ) )
        {
            return false;
        }

        if( !m_effects->emit( {}, m_vehicle.physics, dynamics, telemetry, presentation, damageTelemetry,
                              {}, deltaSeconds, effects, events ) )
        {
            return false;
        }
        if( sourceBudgetDenialCount() != denialEpoch )
        {
            throw std::bad_alloc();
        }
        // Replaceable effects can assign a foreign Array. Admit its complete
        // spare capacity before consuming the staged simulation tick/RNG state.
        if( !events.get_allocator().isSourceBudgeted() )
        {
            auto admitted = makeSourceBudgetSnapshot( events );
            events.swap( admitted );
        }

        m_dynamicsState = dynamics;
        m_damageState = damage;
        m_telemetry = telemetry;
        m_presentationState = presentation;
        m_effectsState = effects;
        m_events.swap( events );
        return true;
    }

    procedural::VehicleDamageEvent ProceduralVehicle::registerImpact(
        const procedural::VehicleDamageImpact &impact )
    {
        if( !m_damage || !m_vehicle.geometry.hasGeometry() )
        {
            return {};
        }
        auto event = m_damage->registerImpact( {}, m_vehicle.physics, impact, m_damageState );
        m_damage->applyImpactResponse( event, m_dynamicsState );
        return event;
    }

    SmartPtr<Properties> ProceduralVehicle::getProperties() const
    {
        auto p = ProceduralMesh::getProperties();
        String ids = m_generatedMaterialIds;
        if( !m_generatedMaterials.empty() )
        {
            ids.clear();
            for( const auto &component : m_generatedMaterials )
            {
                ids += ";" + component->getHandle()->getUUIDAsString() + ";";
            }
        }
        p->setProperty( "Generated Material IDs", ids );
        p->getPropertyObject( "Generated Material IDs" ).setReadOnly( true );
        p->setProperty( "Seed", m_config.seed );
        procedural_properties::setEnum(
            p, "Physics Preset", static_cast<s32>( m_config.physicsPreset ),
            { "Grand Prix", "GT", "Road Sport", "Helicopter - Little Bird", "Plane - Warbird" } );
        procedural_properties::setEnum( p, "Appearance Quality",
                                        static_cast<s32>( m_config.appearance.quality ),
                                        { "Preview", "Standard", "High", "Cinematic" } );
        p->setProperty( "Texture Resolution", m_config.appearance.customMaxResolution );
        p->setProperty( "Vehicle Number", m_config.appearance.vehicleNumber );
        p->setProperty( "Wear", m_config.appearance.wear );
        p->setProperty( "Wetness", m_config.appearance.wetness );
        p->setProperty( "Synchronize From Physics", m_config.synchronizeFromPhysics );
        p->setProperty( "Wheelbase", m_config.geometry.wheelbase );
        p->setProperty( "Body Length", m_config.geometry.bodyLength );
        p->setProperty( "Body Width", m_config.geometry.bodyWidth );
        p->setProperty( "Main Rotor Radius", m_config.geometry.mainRotorRadius );
        p->setProperty( "Tail Rotor Radius", m_config.geometry.tailRotorRadius );
        p->setProperty( "Main Rotor Blades", m_config.geometry.mainRotorBlades );
        p->setProperty( "Tail Rotor Blades", m_config.geometry.tailRotorBlades );
        p->setProperty( "Wing Span", m_config.geometry.wingSpan );
        p->setProperty( "Propeller Radius", m_config.geometry.propellerRadius );
        p->setProperty( "Propeller Blades", m_config.geometry.propellerBlades );
        p->setProperty( "Landing Gear Down", m_config.geometry.landingGearDown );
        p->setProperty( "Three LODs", m_config.geometry.generateThreeLODs );
        p->setProperty( "Fine Details", m_config.geometry.includeFineDetails );
        p->setProperty( "LOD", m_lod );
        p->setProperty( "Primary Colour",
                        ColourF( m_config.appearance.primary.r, m_config.appearance.primary.g,
                                 m_config.appearance.primary.b, m_config.appearance.primary.a ) );
        p->setProperty( "Secondary Colour",
                        ColourF( m_config.appearance.secondary.r, m_config.appearance.secondary.g,
                                 m_config.appearance.secondary.b, m_config.appearance.secondary.a ) );
        return p;
    }

    void ProceduralVehicle::setProperties( SmartPtr<Properties> p )
    {
        if( !p )
        {
            return;
        }
        ProceduralMesh::setProperties( p );
        p->getPropertyValue( "Generated Material IDs", m_generatedMaterialIds );
        p->getPropertyValue( "Seed", m_config.seed );
        s32 preset = static_cast<s32>( m_config.physicsPreset ),
            quality = static_cast<s32>( m_config.appearance.quality );
        preset = procedural_properties::getEnum(
            p, "Physics Preset", preset,
            { "Grand Prix", "GT", "Road Sport", "Helicopter - Little Bird", "Plane - Warbird" } );
        quality = procedural_properties::getEnum( p, "Appearance Quality", quality,
                                                  { "Preview", "Standard", "High", "Cinematic" } );
        m_config.physicsPreset =
            static_cast<procedural::VehiclePhysicsPreset>( std::clamp( preset, 0, 4 ) );
        m_config.appearance.quality =
            static_cast<procedural::VehicleAppearanceQuality>( std::clamp( quality, 0, 3 ) );
        p->getPropertyValue( "Texture Resolution", m_config.appearance.customMaxResolution );
        m_config.appearance.customMaxResolution =
            std::min<u32>( m_config.appearance.customMaxResolution, 2048 );
        p->getPropertyValue( "Vehicle Number", m_config.appearance.vehicleNumber );
        p->getPropertyValue( "Wear", m_config.appearance.wear );
        p->getPropertyValue( "Wetness", m_config.appearance.wetness );
        p->getPropertyValue( "Synchronize From Physics", m_config.synchronizeFromPhysics );
        p->getPropertyValue( "Wheelbase", m_config.geometry.wheelbase );
        p->getPropertyValue( "Body Length", m_config.geometry.bodyLength );
        p->getPropertyValue( "Body Width", m_config.geometry.bodyWidth );
        p->getPropertyValue( "Main Rotor Radius", m_config.geometry.mainRotorRadius );
        p->getPropertyValue( "Tail Rotor Radius", m_config.geometry.tailRotorRadius );
        p->getPropertyValue( "Main Rotor Blades", m_config.geometry.mainRotorBlades );
        p->getPropertyValue( "Tail Rotor Blades", m_config.geometry.tailRotorBlades );
        m_config.geometry.mainRotorRadius = std::clamp( m_config.geometry.mainRotorRadius, 1.5f, 8.0f );
        m_config.geometry.tailRotorRadius = std::clamp( m_config.geometry.tailRotorRadius, .25f, 1.5f );
        m_config.geometry.mainRotorBlades = std::clamp<u32>( m_config.geometry.mainRotorBlades, 2, 8 );
        m_config.geometry.tailRotorBlades = std::clamp<u32>( m_config.geometry.tailRotorBlades, 2, 6 );
        p->getPropertyValue( "Wing Span", m_config.geometry.wingSpan );
        p->getPropertyValue( "Propeller Radius", m_config.geometry.propellerRadius );
        p->getPropertyValue( "Propeller Blades", m_config.geometry.propellerBlades );
        p->getPropertyValue( "Landing Gear Down", m_config.geometry.landingGearDown );
        m_config.geometry.wingSpan = std::clamp( m_config.geometry.wingSpan, 6.0f, 18.0f );
        m_config.geometry.propellerRadius = std::clamp( m_config.geometry.propellerRadius, .6f, 2.0f );
        m_config.geometry.propellerBlades = std::clamp<u32>( m_config.geometry.propellerBlades, 2, 5 );
        p->getPropertyValue( "Three LODs", m_config.geometry.generateThreeLODs );
        p->getPropertyValue( "Fine Details", m_config.geometry.includeFineDetails );
        p->getPropertyValue( "LOD", m_lod );
        m_lod = std::min<u32>( m_lod, 2 );
        auto color = ColourF( m_config.appearance.primary.r, m_config.appearance.primary.g,
                              m_config.appearance.primary.b, m_config.appearance.primary.a );
        p->getPropertyValue( "Primary Colour", color );
        m_config.appearance.primary = { color.r, color.g, color.b, color.a };
        color = ColourF( m_config.appearance.secondary.r, m_config.appearance.secondary.g,
                         m_config.appearance.secondary.b, m_config.appearance.secondary.a );
        p->getPropertyValue( "Secondary Colour", color );
        m_config.appearance.secondary = { color.r, color.g, color.b, color.a };
    }

    void ProceduralVehicle::unload( SmartPtr<ISharedObject> data )
    {
        auto retiredMaterials = makeSourceBudgetSnapshot<SmartPtr<Material>>();
        retiredMaterials.swap( m_generatedMaterials );
        auto retiredGenerator = std::move( m_generator );
        m_generator = nullptr;
        auto retiredDynamics = std::move( m_dynamics );
        m_dynamics = nullptr;
        auto retiredDamage = std::move( m_damage );
        m_damage = nullptr;
        auto retiredPresentation = std::move( m_presentation );
        m_presentation = nullptr;
        auto retiredEffects = std::move( m_effects );
        m_effects = nullptr;
        // Plain data producers retire before optional component notifications;
        // the detached material inventory remains a physical owner through them.
        m_vehicle = {};
        m_events = makeSourceBudgetSnapshot<procedural::VehicleEffectEvent>();
        m_eventScratch = makeSourceBudgetSnapshot<procedural::VehicleEffectEvent>();
        String{}.swap( m_generatedMaterialIds );
        auto actor = getActor();
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        auto manager = graphics ? graphics->getMaterialManager() : nullptr;
        for( auto &component : retiredMaterials )
        {
            if( !component )
            {
                continue;
            }
            auto material = component->getMaterial();
            try
            {
                component->unload( nullptr );
            }
            catch( const std::bad_alloc & )
            {
            }
            if( actor )
            {
                try
                {
                    actor->removeComponentInstance( component );
                }
                catch( const std::bad_alloc & )
                {
                }
            }
            if( manager && material )
            {
                try
                {
                    manager->destroyResource( material );
                }
                catch( const std::bad_alloc & )
                {
                }
            }
        }
        ProceduralMesh::unload( data );
    }

    const Array<workphone::procedural::VehicleEffectEvent> &ProceduralVehicle::getEffects() const
    {
        return m_events;
    }

    const workphone::procedural::VehiclePresentationState &ProceduralVehicle::getPresentationState()
        const
    {
        return m_presentationState;
    }

    const workphone::procedural::VehicleDynamicsState &ProceduralVehicle::getDynamicsState() const
    {
        return m_dynamicsState;
    }

    const workphone::procedural::GeneratedVehicle &ProceduralVehicle::getGeneratedVehicle() const
    {
        return m_vehicle;
    }

    void ProceduralVehicle::setGenerator( SmartPtr<procedural::IVehicleGenerator> generator )
    {
        m_generator = generator;
        requestRegeneration();
    }

    void ProceduralVehicle::setConfig( const procedural::VehicleGenerationConfig &config )
    {
        m_config = config;
        requestRegeneration();
    }

    const workphone::procedural::VehicleGenerationConfig &ProceduralVehicle::getConfig() const
    {
        return m_config;
    }

}  // namespace workphone::scene
