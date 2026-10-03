#include <Workphone/WorkphonePCH.hpp>
#include "ProceduralProperties.hpp"
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace workphone::scene
{
    namespace
    {
        template <class T>
        SmartPtr<T> service( const String &name )
        {
            auto app = core::IApplicationManager::instancePtr();
            if( !app || !app->getFactoryManager() )
                return nullptr;
            return app->getFactoryManager()->createObjectFromType<T>( name );
        }
    }  // namespace

    ProceduralVehicle::ProceduralVehicle() = default;
    ProceduralVehicle::~ProceduralVehicle() = default;

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralVehicle, ProceduralMeshComponent );
    SmartPtr<IMesh> ProceduralVehicle::buildMesh()
    {
        using namespace procedural;
        if( !m_generator )
            m_generator = service<IVehicleGenerator>( "IVehicleGenerator" );
        if( !m_generator )
            throw std::runtime_error( "Load the WPProcedural plugin to generate vehicles" );
        auto vehicle = m_generator->generate( m_config );
        if( !vehicle.isValid() )
        {
            String errors;
            for( const auto &issue : vehicle.issues )
                if( issue.error )
                    errors += issue.component + ": " + issue.message + "\n";
            throw std::runtime_error( errors );
        }
        if( m_lod >= vehicle.geometry.lods.size() )
            throw std::runtime_error( "Selected LOD does not exist" );
        auto mesh = workphone::make_ptr<workphone::Mesh>();
        for( const auto &section : vehicle.geometry.lods[m_lod].sections )
        {
            if( !section.hasGeometry() )
                continue;
            Array<Vector3<real_Num>> positions, normals, uvs;
            Array<Vector4F> tangents;
            Array<u32> indices( section.indices.begin(), section.indices.end() );
            for( const auto &v : section.vertices )
            {
                positions.emplace_back( v.position.x, v.position.y, v.position.z );
                normals.emplace_back( v.normal.x, v.normal.y, v.normal.z );
                tangents.emplace_back( v.tangent.x, v.tangent.y, v.tangent.z, v.tangentSign );
                uvs.emplace_back( v.uv.x, v.uv.y, 0 );
            }
            auto part = MeshUtil::createMesh( positions, normals, tangents, uvs, indices );
            for( auto sub : part->getSubMeshes() )
            {
                const auto &material =
                    vehicle.appearance.material( m_generator->materialSlotFor( section.material ) );
                sub->setMaterialName( material.name );
                mesh->addSubMesh( sub );
            }
        }
        if( !m_dynamics )
            m_dynamics = service<IVehicleDynamics>( "IVehicleDynamics" );
        if( !m_damage )
            m_damage = service<IVehicleDamage>( "IVehicleDamage" );
        if( !m_presentation )
            m_presentation = service<IVehiclePresentation>( "IVehiclePresentation" );
        if( !m_effects )
            m_effects = service<IVehicleEffects>( "IVehicleEffects" );
        if( m_dynamics )
            m_dynamicsState = m_dynamics->reset( vehicle.physics );
        if( m_damage )
            m_damageState = m_damage->reset( m_config.seed );
        if( m_presentation )
            m_presentationState = m_presentation->reset( vehicle.physics );
        if( m_effects )
            m_effectsState = m_effects->reset( m_config.seed );
        m_telemetry = {};
        m_events.clear();
        m_vehicle = std::move( vehicle );
        return mesh;
    }
    void ProceduralVehicle::applyGeneratedMaterials()
    {
        auto actor = getActor();
        auto app = core::IApplicationManager::instancePtr();
        if( !actor || !app || !app->getGraphicsSystem() )
            return;
        auto manager = app->getGraphicsSystem()->getMaterialManager();
        if( !manager )
            return;
        // Respect explicitly authored Material components on the actor.
        if( m_generatedMaterials.empty() )
        {
            const auto existing = actor->getComponentsByType<Material>();
            for( const auto &component : existing )
            {
                const auto id = ";" + component->getHandle()->getUUIDAsString() + ";";
                if( m_generatedMaterialIds.find( id ) != String::npos )
                    m_generatedMaterials.push_back( component );
            }
            if( m_generatedMaterials.empty() && !existing.empty() )
                return;
            std::sort( m_generatedMaterials.begin(), m_generatedMaterials.end(),
                       []( const auto &a, const auto &b ) { return a->getIndex() < b->getIndex(); } );
        }
        u32 index = 0;
        for( const auto &section : m_vehicle.geometry.lods[m_lod].sections )
        {
            if( !section.hasGeometry() )
                continue;
            const auto &descriptor =
                m_vehicle.appearance.material( m_generator->materialSlotFor( section.material ) );
            SmartPtr<Material> component;
            SmartPtr<render::IMaterial> material;
            if( index < m_generatedMaterials.size() )
            {
                component = m_generatedMaterials[index];
                material = component->getMaterial();
            }
            else
            {
                const String name = "ProceduralVehicle/" + StringUtil::getUUID();
                material = workphone::dynamic_pointer_cast<render::IMaterial>( manager->create( name ) );
                if( !material )
                    return;
                component = actor->addComponent<Material>();
                component->getHandle()->setUUID( StringUtil::getUUID() );
                component->setIndex( index );
                component->setMaterial( material );
                component->setState( getState() );
                m_generatedMaterials.push_back( component );
            }
            if( !material )
            {
                material = workphone::dynamic_pointer_cast<render::IMaterial>(
                    manager->create( "ProceduralVehicle/" + StringUtil::getUUID() ) );
                if( !material )
                    return;
                component->setMaterial( material );
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
            auto material = component->getMaterial();
            component->unload( nullptr );
            actor->removeComponentInstance( component );
            if( material )
                manager->destroyResource( material );
            m_generatedMaterials.pop_back();
        }
    }
    bool ProceduralVehicle::stepFixed( const procedural::VehicleControlInput &input,
                                       const std::array<procedural::VehicleSurfaceSample, 4> &surfaces,
                                       double deltaSeconds )
    {
        using namespace procedural;
        if( !m_dynamics || !m_damage || !m_presentation || !m_effects ||
            !m_vehicle.geometry.hasGeometry() )
            return false;
        if( !std::isfinite( deltaSeconds ) || deltaSeconds <= 0 )
            return false;
        // Stage updates so failure leaves the published simulation state unchanged.
        auto dynamics = m_dynamicsState;
        auto damage = m_damageState;
        auto presentation = m_presentationState;
        auto effects = m_effectsState;
        VehicleDynamicsTelemetry telemetry;
        std::vector<VehicleEffectEvent> events;
        if( !m_damage->update( {}, deltaSeconds, damage ) )
            return false;
        const auto damageTelemetry = m_damage->telemetry( {}, damage );
        if( !m_dynamics->stepFixed( m_vehicle.physics, {}, input, surfaces,
                                    m_damage->dynamicsModifiers( damageTelemetry ), deltaSeconds,
                                    dynamics, telemetry ) )
            return false;
        VehiclePresentationInput presentationInput;
        presentationInput.throttle = input.throttle;
        if( !m_presentation->update( {}, m_vehicle.physics, dynamics, telemetry, damageTelemetry,
                                     presentationInput, deltaSeconds, presentation ) )
            return false;
        if( !m_effects->emit( {}, m_vehicle.physics, dynamics, telemetry, presentation, damageTelemetry,
                              {}, deltaSeconds, effects, events ) )
            return false;
        m_dynamicsState = dynamics;
        m_damageState = damage;
        m_telemetry = telemetry;
        m_presentationState = presentation;
        m_effectsState = effects;
        m_events = std::move( events );
        return true;
    }
    procedural::VehicleDamageEvent ProceduralVehicle::registerImpact(
        const procedural::VehicleDamageImpact &impact )
    {
        if( !m_damage || !m_vehicle.geometry.hasGeometry() )
            return {};
        auto event = m_damage->registerImpact( {}, m_vehicle.physics, impact, m_damageState );
        m_damage->applyImpactResponse( event, m_dynamicsState );
        return event;
    }
    SmartPtr<Properties> ProceduralVehicle::getProperties() const
    {
        auto p = ProceduralMeshComponent::getProperties();
        String ids = m_generatedMaterialIds;
        if( !m_generatedMaterials.empty() )
        {
            ids.clear();
            for( const auto &component : m_generatedMaterials )
                ids += ";" + component->getHandle()->getUUIDAsString() + ";";
        }
        p->setProperty( "Generated Material IDs", ids );
        p->getPropertyObject( "Generated Material IDs" ).setReadOnly( true );
        p->setProperty( "Seed", m_config.seed );
        procedural_properties::setEnum( p, "Physics Preset", static_cast<s32>( m_config.physicsPreset ),
                                        { "Grand Prix", "GT", "Road Sport" } );
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
        p->setProperty( "Three LODs", m_config.geometry.generateThreeLODs );
        p->setProperty( "Fine Details", m_config.geometry.includeFineDetails );
        p->setProperty( "LOD", m_lod );
        p->setProperty( "Primary Colour",
                        ColourF( m_config.appearance.primary.r, m_config.appearance.primary.g,
                                 m_config.appearance.primary.b, m_config.appearance.primary.a ) );
        return p;
    }
    void ProceduralVehicle::setProperties( SmartPtr<Properties> p )
    {
        if( !p )
            return;
        ProceduralMeshComponent::setProperties( p );
        p->getPropertyValue( "Generated Material IDs", m_generatedMaterialIds );
        p->getPropertyValue( "Seed", m_config.seed );
        s32 preset = static_cast<s32>( m_config.physicsPreset ),
            quality = static_cast<s32>( m_config.appearance.quality );
        preset = procedural_properties::getEnum( p, "Physics Preset", preset,
                                                 { "Grand Prix", "GT", "Road Sport" } );
        quality = procedural_properties::getEnum( p, "Appearance Quality", quality,
                                                  { "Preview", "Standard", "High", "Cinematic" } );
        m_config.physicsPreset =
            static_cast<procedural::VehiclePhysicsPreset>( std::clamp( preset, 0, 2 ) );
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
        p->getPropertyValue( "Three LODs", m_config.geometry.generateThreeLODs );
        p->getPropertyValue( "Fine Details", m_config.geometry.includeFineDetails );
        p->getPropertyValue( "LOD", m_lod );
        m_lod = std::min<u32>( m_lod, 2 );
        auto color = ColourF( m_config.appearance.primary.r, m_config.appearance.primary.g,
                              m_config.appearance.primary.b, m_config.appearance.primary.a );
        p->getPropertyValue( "Primary Colour", color );
        m_config.appearance.primary = { color.r, color.g, color.b, color.a };
    }
    void ProceduralVehicle::unload( SmartPtr<ISharedObject> data )
    {
        auto actor = getActor();
        auto app = core::IApplicationManager::instancePtr();
        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        auto manager = graphics ? graphics->getMaterialManager() : nullptr;
        for( auto &component : m_generatedMaterials )
        {
            auto material = component->getMaterial();
            component->unload( nullptr );
            if( actor )
                actor->removeComponentInstance( component );
            if( manager && material )
                manager->destroyResource( material );
        }
        m_generatedMaterials.clear();
        m_generatedMaterialIds.clear();
        ProceduralMeshComponent::unload( data );
        m_generator = nullptr;
        m_dynamics = nullptr;
        m_damage = nullptr;
        m_presentation = nullptr;
        m_effects = nullptr;
        m_vehicle = {};
        m_events.clear();
    }
}  // namespace workphone::scene
