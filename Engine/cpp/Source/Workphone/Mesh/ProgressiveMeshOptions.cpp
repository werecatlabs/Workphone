#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/ProgressiveMeshOptions.hpp>
#include <Workphone/Core/Properties.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace workphone
{
    namespace
    {
        const String lodLevelName = "progressiveMeshLodLevel";

        template <class T>
        void hashValue( u64 &hash, const T &value )
        {
            const auto *bytes = reinterpret_cast<const u8 *>( &value );
            for( size_t i = 0; i < sizeof( value ); ++i )
            {
                hash ^= bytes[i];
                hash *= 1099511628211ull;
            }
        }

        void hashString( u64 &hash, const String &value )
        {
            for( auto character : value )
            {
                hash ^= static_cast<u8>( character );
                hash *= 1099511628211ull;
            }
        }

        ProgressiveMeshLodLevel makePercentageLevel( f32 screenHeight, f32 screenArea, f32 distance,
                                                     f32 remainingGeometry )
        {
            ProgressiveMeshLodLevel level;
            level.screenHeight = screenHeight;
            level.screenArea = screenArea;
            level.distance = distance;
            level.reductionMode = MeshLodReductionMode::Percentage;
            level.remainingGeometry = remainingGeometry;
            return level;
        }
    }  // namespace

    bool ProgressiveMeshLodLevel::operator==( const ProgressiveMeshLodLevel &other ) const
    {
        return enabled == other.enabled && screenHeight == other.screenHeight &&
               screenArea == other.screenArea && distance == other.distance &&
               reductionMode == other.reductionMode && remainingGeometry == other.remainingGeometry &&
               verticesToRemove == other.verticesToRemove && maximumError == other.maximumError &&
               manualMesh == other.manualMesh;
    }

    bool ProgressiveMeshLodLevel::operator!=( const ProgressiveMeshLodLevel &other ) const
    {
        return !( *this == other );
    }

    bool ProgressiveMeshOptions::isEnabled() const
    {
        return generationMode != MeshLodGenerationMode::Disabled;
    }

    Array<ProgressiveMeshLodLevel> ProgressiveMeshOptions::getResolvedLevels() const
    {
        auto resolvedLevels =
            generationMode == MeshLodGenerationMode::Automatic && preset != MeshLodPreset::Custom
                ? getPresetLevels( preset )
                : levels;

        resolvedLevels.erase(
            std::remove_if( resolvedLevels.begin(), resolvedLevels.end(),
                            []( const ProgressiveMeshLodLevel &level ) { return !level.enabled; } ),
            resolvedLevels.end() );
        return resolvedLevels;
    }

    bool ProgressiveMeshOptions::validate( String *errorMessage ) const
    {
        if( !isEnabled() )
        {
            return true;
        }

        const auto resolvedLevels = getResolvedLevels();
        if( resolvedLevels.empty() )
        {
            if( errorMessage )
            {
                *errorMessage = "Progressive mesh generation requires at least one enabled LOD level.";
            }
            return false;
        }

        auto previousTransition = transitionMode == MeshLodTransitionMode::Distance
                                      ? -std::numeric_limits<f32>::max()
                                      : std::numeric_limits<f32>::max();
        auto previousRemaining = std::numeric_limits<f32>::max();
        u32 previousVertexReduction = 0;
        auto previousError = -std::numeric_limits<f32>::max();

        for( const auto &level : resolvedLevels )
        {
            const auto transition = transitionMode == MeshLodTransitionMode::Distance ? level.distance
                                    : transitionMode == MeshLodTransitionMode::ScreenArea
                                        ? level.screenArea
                                        : level.screenHeight;

            const auto transitionValid =
                transitionMode == MeshLodTransitionMode::Distance
                    ? transition >= 0.0f && transition > previousTransition
                    : transition > 0.0f && transition <= 1.0f && transition < previousTransition;
            if( !transitionValid )
            {
                if( errorMessage )
                {
                    *errorMessage = transitionMode == MeshLodTransitionMode::Distance
                                        ? "Progressive mesh distances must be non-negative and "
                                          "strictly increasing."
                                        : "Progressive mesh screen thresholds must be in (0, 1] and "
                                          "strictly decreasing.";
                }
                return false;
            }
            previousTransition = transition;

            if( generationMode == MeshLodGenerationMode::Manual )
            {
                if( StringUtil::isNullOrEmpty( level.manualMesh ) )
                {
                    if( errorMessage )
                    {
                        *errorMessage = "Every manual progressive mesh level needs a mesh path.";
                    }
                    return false;
                }
                continue;
            }

            switch( level.reductionMode )
            {
            case MeshLodReductionMode::Percentage:
                if( level.remainingGeometry <= 0.0f || level.remainingGeometry >= 1.0f ||
                    level.remainingGeometry >= previousRemaining )
                {
                    if( errorMessage )
                    {
                        *errorMessage =
                            "Remaining geometry must be in (0, 1) and strictly decrease per LOD.";
                    }
                    return false;
                }
                previousRemaining = level.remainingGeometry;
                break;
            case MeshLodReductionMode::FixedVertexReduction:
                if( level.verticesToRemove == 0 || level.verticesToRemove <= previousVertexReduction )
                {
                    if( errorMessage )
                    {
                        *errorMessage = "Fixed vertex reduction must be positive and increase per LOD.";
                    }
                    return false;
                }
                previousVertexReduction = level.verticesToRemove;
                break;
            case MeshLodReductionMode::ErrorThreshold:
                if( level.maximumError < 0.0f || level.maximumError <= previousError )
                {
                    if( errorMessage )
                    {
                        *errorMessage = "Error thresholds must be non-negative and increase per LOD.";
                    }
                    return false;
                }
                previousError = level.maximumError;
                break;
            }
        }

        return true;
    }

    u64 ProgressiveMeshOptions::getHash() const
    {
        u64 hash = 1469598103934665603ull;
        hashValue( hash, generationMode );
        hashValue( hash, preset );
        hashValue( hash, transitionMode );
        hashValue( hash, simplificationMetric );
        hashValue( hash, minimumSourceTriangleCount );
        hashValue( hash, minimumLodTriangleCount );
        hashValue( hash, preserveBorders );
        hashValue( hash, preserveUvSeams );
        hashValue( hash, preserveHardEdges );
        hashValue( hash, preserveMaterialBoundaries );
        hashValue( hash, preserveSkinning );
        hashValue( hash, borderImportance );
        hashValue( hash, uvImportance );
        hashValue( hash, normalImportance );
        hashValue( hash, skinningImportance );
        hashValue( hash, hardEdgeAngleDegrees );
        hashValue( hash, useVertexNormals );
        hashValue( hash, compressIndexBuffers );
        hashValue( hash, optimiseHiddenInterior );
        hashValue( hash, outsideImportance );
        hashValue( hash, outsideWalkAngleDegrees );
        hashValue( hash, generateShadowLods );
        hashValue( hash, shadowLodOffset );
        hashValue( hash, recalculateBounds );
        hashValue( hash, halfPosition );
        hashValue( hash, halfTexCoords );
        hashValue( hash, qTangents );
        hashValue( hash, halfPoseData );

        const auto resolvedLevels = getResolvedLevels();
        for( const auto &level : resolvedLevels )
        {
            hashValue( hash, level.enabled );
            hashValue( hash, level.screenHeight );
            hashValue( hash, level.screenArea );
            hashValue( hash, level.distance );
            hashValue( hash, level.reductionMode );
            hashValue( hash, level.remainingGeometry );
            hashValue( hash, level.verticesToRemove );
            hashValue( hash, level.maximumError );
            hashString( hash, level.manualMesh );
        }

        return hash;
    }

    SmartPtr<Properties> ProgressiveMeshOptions::toProperties( const String &name ) const
    {
        auto properties = workphone::make_ptr<Properties>();
        properties->setName( name );
        properties->setProperty( "generationMode", static_cast<u32>( generationMode ) );
        properties->setProperty( "preset", static_cast<u32>( preset ) );
        properties->setProperty( "transitionMode", static_cast<u32>( transitionMode ) );
        properties->setProperty( "simplificationMetric", static_cast<u32>( simplificationMetric ) );
        properties->setProperty( "minimumSourceTriangleCount", minimumSourceTriangleCount );
        properties->setProperty( "minimumLodTriangleCount", minimumLodTriangleCount );
        properties->setProperty( "preserveBorders", preserveBorders );
        properties->setProperty( "preserveUvSeams", preserveUvSeams );
        properties->setProperty( "preserveHardEdges", preserveHardEdges );
        properties->setProperty( "preserveMaterialBoundaries", preserveMaterialBoundaries );
        properties->setProperty( "preserveSkinning", preserveSkinning );
        properties->setProperty( "borderImportance", borderImportance );
        properties->setProperty( "uvImportance", uvImportance );
        properties->setProperty( "normalImportance", normalImportance );
        properties->setProperty( "skinningImportance", skinningImportance );
        properties->setProperty( "hardEdgeAngleDegrees", hardEdgeAngleDegrees );
        properties->setProperty( "useVertexNormals", useVertexNormals );
        properties->setProperty( "compressIndexBuffers", compressIndexBuffers );
        properties->setProperty( "optimiseHiddenInterior", optimiseHiddenInterior );
        properties->setProperty( "outsideImportance", outsideImportance );
        properties->setProperty( "outsideWalkAngleDegrees", outsideWalkAngleDegrees );
        properties->setProperty( "generateShadowLods", generateShadowLods );
        properties->setProperty( "shadowLodOffset", shadowLodOffset );
        properties->setProperty( "recalculateBounds", recalculateBounds );
        properties->setProperty( "halfPosition", halfPosition );
        properties->setProperty( "halfTexCoords", halfTexCoords );
        properties->setProperty( "qTangents", qTangents );
        properties->setProperty( "halfPoseData", halfPoseData );

        for( const auto &level : levels )
        {
            auto levelProperties = workphone::make_ptr<Properties>();
            levelProperties->setName( lodLevelName );
            levelProperties->setProperty( "enabled", level.enabled );
            levelProperties->setProperty( "screenHeight", level.screenHeight );
            levelProperties->setProperty( "screenArea", level.screenArea );
            levelProperties->setProperty( "distance", level.distance );
            levelProperties->setProperty( "reductionMode", static_cast<u32>( level.reductionMode ) );
            levelProperties->setProperty( "remainingGeometry", level.remainingGeometry );
            levelProperties->setProperty( "verticesToRemove", level.verticesToRemove );
            levelProperties->setProperty( "maximumError", level.maximumError );
            levelProperties->setProperty( "manualMesh", level.manualMesh );
            properties->addChild( levelProperties );
        }

        return properties;
    }

    void ProgressiveMeshOptions::fromProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        auto generationModeValue = static_cast<u32>( generationMode );
        auto presetValue = static_cast<u32>( preset );
        auto transitionModeValue = static_cast<u32>( transitionMode );
        auto simplificationMetricValue = static_cast<u32>( simplificationMetric );

        properties->getPropertyValue( "generationMode", generationModeValue );
        properties->getPropertyValue( "preset", presetValue );
        properties->getPropertyValue( "transitionMode", transitionModeValue );
        properties->getPropertyValue( "simplificationMetric", simplificationMetricValue );

        generationMode = static_cast<MeshLodGenerationMode>(
            std::min( generationModeValue, static_cast<u32>( MeshLodGenerationMode::Manual ) ) );
        preset = static_cast<MeshLodPreset>(
            std::min( presetValue, static_cast<u32>( MeshLodPreset::Custom ) ) );
        transitionMode = static_cast<MeshLodTransitionMode>(
            std::min( transitionModeValue, static_cast<u32>( MeshLodTransitionMode::Distance ) ) );
        simplificationMetric = static_cast<MeshSimplificationMetric>( std::min(
            simplificationMetricValue, static_cast<u32>( MeshSimplificationMetric::QuadricError ) ) );

        properties->getPropertyValue( "minimumSourceTriangleCount", minimumSourceTriangleCount );
        properties->getPropertyValue( "minimumLodTriangleCount", minimumLodTriangleCount );
        properties->getPropertyValue( "preserveBorders", preserveBorders );
        properties->getPropertyValue( "preserveUvSeams", preserveUvSeams );
        properties->getPropertyValue( "preserveHardEdges", preserveHardEdges );
        properties->getPropertyValue( "preserveMaterialBoundaries", preserveMaterialBoundaries );
        properties->getPropertyValue( "preserveSkinning", preserveSkinning );
        properties->getPropertyValue( "borderImportance", borderImportance );
        properties->getPropertyValue( "uvImportance", uvImportance );
        properties->getPropertyValue( "normalImportance", normalImportance );
        properties->getPropertyValue( "skinningImportance", skinningImportance );
        properties->getPropertyValue( "hardEdgeAngleDegrees", hardEdgeAngleDegrees );
        properties->getPropertyValue( "useVertexNormals", useVertexNormals );
        properties->getPropertyValue( "compressIndexBuffers", compressIndexBuffers );
        properties->getPropertyValue( "optimiseHiddenInterior", optimiseHiddenInterior );
        properties->getPropertyValue( "outsideImportance", outsideImportance );
        properties->getPropertyValue( "outsideWalkAngleDegrees", outsideWalkAngleDegrees );
        properties->getPropertyValue( "generateShadowLods", generateShadowLods );
        properties->getPropertyValue( "shadowLodOffset", shadowLodOffset );
        properties->getPropertyValue( "recalculateBounds", recalculateBounds );
        properties->getPropertyValue( "halfPosition", halfPosition );
        properties->getPropertyValue( "halfTexCoords", halfTexCoords );
        properties->getPropertyValue( "qTangents", qTangents );
        properties->getPropertyValue( "halfPoseData", halfPoseData );

        minimumSourceTriangleCount = std::max( minimumSourceTriangleCount, 1u );
        minimumLodTriangleCount = std::max( minimumLodTriangleCount, 1u );
        borderImportance = std::max( borderImportance, 0.0f );
        uvImportance = std::max( uvImportance, 0.0f );
        normalImportance = std::max( normalImportance, 0.0f );
        skinningImportance = std::max( skinningImportance, 0.0f );
        hardEdgeAngleDegrees = std::clamp( hardEdgeAngleDegrees, 0.0f, 180.0f );
        outsideImportance = std::max( outsideImportance, 0.0f );
        outsideWalkAngleDegrees = std::clamp( outsideWalkAngleDegrees, 0.0f, 180.0f );

        const auto levelProperties = properties->getChildrenByName( lodLevelName );
        Array<ProgressiveMeshLodLevel> newLevels;
        newLevels.reserve( levelProperties.size() );
        for( const auto &levelData : levelProperties )
        {
            ProgressiveMeshLodLevel level;
            auto reductionModeValue = static_cast<u32>( level.reductionMode );
            levelData->getPropertyValue( "enabled", level.enabled );
            levelData->getPropertyValue( "screenHeight", level.screenHeight );
            levelData->getPropertyValue( "screenArea", level.screenArea );
            levelData->getPropertyValue( "distance", level.distance );
            levelData->getPropertyValue( "reductionMode", reductionModeValue );
            levelData->getPropertyValue( "remainingGeometry", level.remainingGeometry );
            levelData->getPropertyValue( "verticesToRemove", level.verticesToRemove );
            levelData->getPropertyValue( "maximumError", level.maximumError );
            levelData->getPropertyValue( "manualMesh", level.manualMesh );

            level.reductionMode = static_cast<MeshLodReductionMode>( std::min(
                reductionModeValue, static_cast<u32>( MeshLodReductionMode::ErrorThreshold ) ) );
            level.screenHeight = std::clamp( level.screenHeight, 0.0001f, 1.0f );
            level.screenArea = std::clamp( level.screenArea, 0.0001f, 1.0f );
            level.distance = std::max( level.distance, 0.0f );
            level.remainingGeometry = std::clamp( level.remainingGeometry, 0.0001f, 0.9999f );
            level.maximumError = std::max( level.maximumError, 0.0f );
            newLevels.push_back( std::move( level ) );
        }
        levels = std::move( newLevels );
    }

    bool ProgressiveMeshOptions::operator==( const ProgressiveMeshOptions &other ) const
    {
        return generationMode == other.generationMode && preset == other.preset &&
               transitionMode == other.transitionMode &&
               simplificationMetric == other.simplificationMetric && levels == other.levels &&
               minimumSourceTriangleCount == other.minimumSourceTriangleCount &&
               minimumLodTriangleCount == other.minimumLodTriangleCount &&
               preserveBorders == other.preserveBorders && preserveUvSeams == other.preserveUvSeams &&
               preserveHardEdges == other.preserveHardEdges &&
               preserveMaterialBoundaries == other.preserveMaterialBoundaries &&
               preserveSkinning == other.preserveSkinning &&
               borderImportance == other.borderImportance && uvImportance == other.uvImportance &&
               normalImportance == other.normalImportance &&
               skinningImportance == other.skinningImportance &&
               hardEdgeAngleDegrees == other.hardEdgeAngleDegrees &&
               useVertexNormals == other.useVertexNormals &&
               compressIndexBuffers == other.compressIndexBuffers &&
               optimiseHiddenInterior == other.optimiseHiddenInterior &&
               outsideImportance == other.outsideImportance &&
               outsideWalkAngleDegrees == other.outsideWalkAngleDegrees &&
               generateShadowLods == other.generateShadowLods &&
               shadowLodOffset == other.shadowLodOffset &&
               recalculateBounds == other.recalculateBounds && halfPosition == other.halfPosition &&
               halfTexCoords == other.halfTexCoords && qTangents == other.qTangents &&
               halfPoseData == other.halfPoseData;
    }

    bool ProgressiveMeshOptions::operator!=( const ProgressiveMeshOptions &other ) const
    {
        return !( *this == other );
    }

    Array<ProgressiveMeshLodLevel> ProgressiveMeshOptions::getPresetLevels( MeshLodPreset preset )
    {
        switch( preset )
        {
        case MeshLodPreset::Performance:
            return { makePercentageLevel( 0.45f, 0.16f, 12.0f, 0.65f ),
                     makePercentageLevel( 0.20f, 0.04f, 30.0f, 0.35f ),
                     makePercentageLevel( 0.08f, 0.008f, 70.0f, 0.15f ) };
        case MeshLodPreset::Quality:
            return { makePercentageLevel( 0.65f, 0.32f, 25.0f, 0.90f ),
                     makePercentageLevel( 0.40f, 0.13f, 60.0f, 0.70f ),
                     makePercentageLevel( 0.22f, 0.04f, 120.0f, 0.50f ),
                     makePercentageLevel( 0.10f, 0.01f, 240.0f, 0.30f ) };
        case MeshLodPreset::Balanced:
            return { makePercentageLevel( 0.60f, 0.28f, 15.0f, 0.80f ),
                     makePercentageLevel( 0.35f, 0.10f, 35.0f, 0.55f ),
                     makePercentageLevel( 0.18f, 0.025f, 70.0f, 0.30f ),
                     makePercentageLevel( 0.07f, 0.004f, 140.0f, 0.12f ) };
        case MeshLodPreset::Custom:
        default:
            return {};
        }
    }
}  // namespace workphone
