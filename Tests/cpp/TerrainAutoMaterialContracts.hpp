#ifndef WPTerrainAutoMaterialContracts_h__
#define WPTerrainAutoMaterialContracts_h__

#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainEditing.hpp>
#include <Workphone/System/CommandManager.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace terrain_auto_material_contracts
{
    using namespace workphone;

    inline void require( bool condition, const char *message )
    {
        if( !condition )
            throw std::runtime_error( message );
    }

    inline render::TerrainData flat( f32 height )
    {
        render::TerrainData data;
        data.dimensions = { 3, 3 };
        data.spacing = { 2, 4 };
        data.origin = { -2, -4 };
        data.heights.assign( 9, height );
        return data;
    }

    inline void run()
    {
        String error;
        auto data = flat( 20 );
        data.layers[0].albedo = "data://authored-sand.texres";
        data.layers[1].tint = { .2f, .4f, .1f, 1 };
        data.layerWeights.assign( 9, { .2f, .3f, .4f, .1f } );
        const auto authored = data;
        require( render::applyTerrainAutoMaterial( data, error ) &&
                     data.layerWeights == authored.layerWeights && data.layers == authored.layers,
                 "Disabled auto material must preserve manual weights and authored layers" );
        const auto legacy = render::serializeTerrainData( data );
        require( legacy.find( "autoMaterial" ) == String::npos,
                 "Default-disabled terrains retain their earlier source representation" );
        render::TerrainData decoded;
        require( render::deserializeTerrainData( legacy, decoded, error ) &&
                     !decoded.autoMaterial.enabled && decoded.layerWeights == authored.layerWeights,
                 "Source assets without auto material remain manually painted" );

        data.autoMaterial.enabled = true;
        for( const auto entry : { std::make_pair( 0.f, 0 ), std::make_pair( 20.f, 1 ),
                                  std::make_pair( 200.f, 3 ) } )
        {
            data.heights.assign( 9, entry.first );
            require( render::applyTerrainAutoMaterial( data, error ),
                     "Flat auto material fixture generates" );
            for( const auto &weight : data.layerWeights )
                require( weight[entry.second] > .999f,
                         "Sea-level sand, lowland grass and upland soil use their assigned layers" );
        }
        data.heights.assign( 9, 6 );
        require( render::applyTerrainAutoMaterial( data, error ) &&
                     std::abs( data.layerWeights[4].x - .5f ) < 1.e-6f &&
                     std::abs( data.layerWeights[4].y - .5f ) < 1.e-6f,
                 "Beach transition blends smoothly at its configured midpoint" );
        data.autoMaterial.seaLevel = 30;
        data.heights.assign( 9, 36 );
        require( render::applyTerrainAutoMaterial( data, error ) &&
                     std::abs( data.layerWeights[4].x - .5f ) < 1.e-6f,
                 "Beach elevation follows local sea level" );
        data.autoMaterial.seaLevel = 0;
        data.heightScale = 2;
        for( s32 z = 0; z < 3; ++z )
            for( s32 x = 0; x < 3; ++x )
                data.heights[size_t( z ) * 3 + x] = 20 + x * data.spacing.x;
        require( render::applyTerrainAutoMaterial( data, error ),
                 "Slope auto material fixture generates" );
        for( const auto &weight : data.layerWeights )
            require( weight.z > .999f,
                     "Slope classification accounts for sample spacing and height scale at grid edges" );
        require( data.layers == authored.layers,
                 "Weight generation must not replace authored texture paths or material tints" );
        const auto generated = data.layerWeights;
        require( render::applyTerrainAutoMaterial( data, error ) && data.layerWeights == generated,
                 "Auto material generation must be deterministic" );
        const auto json = render::serializeTerrainData( data );
        require( render::deserializeTerrainData( json, decoded, error ) &&
                     decoded.autoMaterial == data.autoMaterial && decoded.layerWeights == generated &&
                     decoded.layers == data.layers,
                 "Terrain source/cooked JSON retains auto rules, generated weights and layer references" );
        auto disabledRules = data;
        disabledRules.autoMaterial.enabled = false;
        require( render::deserializeTerrainData( render::serializeTerrainData( disabledRules ), decoded,
                                                 error ) &&
                     decoded.autoMaterial == disabledRules.autoMaterial,
                 "Disabled customized rules persist for subsequent re-enabling" );
        auto oldVersion = json;
        const auto version = oldVersion.find( "\"version\":3" );
        require( version != String::npos, "Terrain source version fixture exists" );
        oldVersion.replace( version, 11, "\"version\":2" );
        require( render::deserializeTerrainData( oldVersion, decoded, error ) &&
                     !decoded.autoMaterial.enabled,
                 "Version-two migration does not activate newer auto rules" );

        auto terrain = make_ptr<scene::TerrainSystem>();
        require( terrain->applyTerrainData( authored, error ), "Manual terrain fixture publishes" );
        auto history = make_ptr<CommandManager>();
        render::TerrainAutoMaterial rules;
        rules.enabled = true;
        require( scene::applyTerrainAutoMaterial( terrain, rules, history, error ),
                 "Enabling auto material is one undoable operation" );
        const auto enabled = terrain->getTerrainSnapshot();
        require( enabled->layerWeights[4].y > .999f && enabled->autoMaterial == rules,
                 "Component publication applies enabled rules" );
        for( const auto mode : { scene::TerrainBrushMode::Paint, scene::TerrainBrushMode::ClearPaint,
                                scene::TerrainBrushMode::Normalize } )
        {
            scene::TerrainBrush paint;
            paint.mode = mode;
            paint.layer = 0;
            require( !scene::applyTerrainBrush( terrain, paint, history, error ) &&
                         error.find( "Disable auto material" ) != String::npos &&
                         terrain->getTerrainSnapshot() == enabled,
                     "Layer paint tools must reject auto mode without changing terrain or revision" );
        }
        auto manualPayload = *enabled;
        manualPayload.layerWeights.assign( manualPayload.heights.size(), { 1, 0, 0, 0 } );
        require( terrain->applyTerrainData( manualPayload, error ) &&
                     terrain->getTerrainSnapshot() == enabled,
                 "Direct weight payloads in auto mode must be corrected without publishing a false edit" );
        history->getPreviousCommand()->undo();
        require( !terrain->getTerrainSnapshot()->autoMaterial.enabled &&
                     terrain->getTerrainSnapshot()->layerWeights == authored.layerWeights,
                 "Undoing enable restores previous manual painting and disabled state" );
        history->getNextCommand()->redo();
        require( terrain->getTerrainSnapshot()->layerWeights == enabled->layerWeights,
                 "Redoing enable restores the complete generated surface" );
        scene::TerrainBrush brush;
        brush.radius = 1;
        brush.strength = 180;
        require( scene::applyTerrainBrush( terrain, brush, history, error ),
                 "Sculpting an automatic surface succeeds" );
        const auto sculpted = terrain->getTerrainSnapshot();
        require( sculpted->layerWeights != enabled->layerWeights && sculpted->layerWeights[4].w > .999f,
                 "Sculpting refreshes height and neighboring slope weights" );
        history->getPreviousCommand()->undo();
        require( terrain->getTerrainSnapshot()->heights == enabled->heights &&
                     terrain->getTerrainSnapshot()->layerWeights == enabled->layerWeights,
                 "Sculpt undo recomputes weights from restored heights without stale caches" );
        history->getNextCommand()->redo();
        require( terrain->getTerrainSnapshot()->layerWeights == sculpted->layerWeights,
                 "Sculpt redo restores deterministic automatic weights" );
        auto adjusted = rules;
        adjusted.rockSlope = 80;
        adjusted.seaLevel = 25;
        require( scene::applyTerrainAutoMaterial( terrain, adjusted, history, error ),
                 "Changing auto material rules commits" );
        history->getPreviousCommand()->undo();
        require( terrain->getTerrainSnapshot()->autoMaterial == rules &&
                     terrain->getTerrainSnapshot()->layerWeights == sculpted->layerWeights,
                 "Rule undo restores settings together with the matching weight map" );
        const auto retained = terrain->getTerrainSnapshot();
        for( int invalidCase = 0; invalidCase < 7; ++invalidCase )
        {
            auto invalid = *retained;
            switch( invalidCase )
            {
            case 0: invalid.autoMaterial.seaLevel = std::numeric_limits<float>::quiet_NaN(); break;
            case 1: invalid.autoMaterial.beachHeight = -1; break;
            case 2: invalid.autoMaterial.beachTransition = 0; break;
            case 3: invalid.autoMaterial.rockSlope = 91; break;
            case 4: invalid.autoMaterial.rockTransition = 0; break;
            case 5: invalid.autoMaterial.soilHeight = std::numeric_limits<float>::infinity(); break;
            case 6: invalid.autoMaterial.soilTransition = -1; break;
            }
            const auto weights = invalid.layerWeights;
            require( !render::applyTerrainAutoMaterial( invalid, error ) &&
                         invalid.layerWeights == weights && !terrain->applyTerrainData( invalid, error ) &&
                         terrain->getTerrainSnapshot() == retained,
                     "Invalid rules leave generation and component publication unchanged" );
        }
        auto malformed = json;
        const auto enabledFlag = malformed.find( "\"enabled\":true" );
        require( enabledFlag != String::npos, "Auto material JSON fixture exists" );
        malformed.replace( enabledFlag, 14, "\"enabled\":1" );
        decoded = *retained;
        require( !render::deserializeTerrainData( malformed, decoded, error ) &&
                     decoded.autoMaterial == retained->autoMaterial &&
                     decoded.layerWeights == retained->layerWeights,
                 "Malformed optional settings fail transactionally" );

        auto paletteTerrain = make_ptr<scene::TerrainSystem>();
        require( paletteTerrain->applyTerrainData( authored, error ) &&
                     scene::applyTerrainIslandMaterials( paletteTerrain, history, error ),
                 "Island palette is applied through command history" );
        const auto palette = paletteTerrain->getTerrainSnapshot();
        require( palette->layers[0].albedo == authored.layers[0].albedo &&
                     palette->layerWeights == authored.layerWeights && !palette->autoMaterial.enabled &&
                     palette->appearance.projection == render::TerrainProjection::Triplanar,
                 "Island defaults retain imported texture references, manual weights and opt-in state" );
        history->getPreviousCommand()->undo();
        require( paletteTerrain->getTerrainSnapshot()->layers == authored.layers &&
                     paletteTerrain->getTerrainSnapshot()->appearance == authored.appearance,
                 "Island palette undo restores all four layers and appearance together" );
        auto replacements = authored.layers;
        replacements[0].albedo = "data://generated-sand.texres";
        replacements[3].roughness = .7f;
        require( scene::applyTerrainLayers( paletteTerrain, replacements, history, error ) &&
                     paletteTerrain->getTerrainSnapshot()->layers == replacements,
                 "Generated material layers install as one transaction" );
        history->getPreviousCommand()->undo();
        require( paletteTerrain->getTerrainSnapshot()->layers == authored.layers,
                 "Generated material layers undo together" );

        auto large = flat( 20 );
        large.dimensions = { 257, 257 };
        large.heights.assign( 257u * 257u, 20 );
        auto largeTerrain = make_ptr<scene::TerrainSystem>();
        require( largeTerrain->applyTerrainData( large, error ) &&
                     scene::applyTerrainAutoMaterial( largeTerrain, rules, history, error ),
                 "Whole-terrain auto rules support grids larger than the sparse brush undo limit" );
        history->getPreviousCommand()->undo();
        require( !largeTerrain->getTerrainSnapshot()->autoMaterial.enabled &&
                     largeTerrain->getTerrainSnapshot()->layerWeights.empty(),
                 "Whole-terrain auto undo restores implicit original weights" );
        history->clearAll();
    }
}

#endif
