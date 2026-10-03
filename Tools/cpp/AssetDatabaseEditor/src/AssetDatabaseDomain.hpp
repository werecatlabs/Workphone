// ---------------------------------------------------------------------------
//  AssetDatabaseDomain.hpp
//
//  C++17 mirror of the data classes declared at the top of
//  Tools/csharp/AssetDatabaseTool/MainWindow.xaml.cs.  These POD-style
//  records are populated by the database wrapper and consumed by the
//  editor windows.
// ---------------------------------------------------------------------------

#ifndef AssetDatabaseDomain_h__
#define AssetDatabaseDomain_h__

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace adbeditor
    {
        /** Row from the @c configured_actors table. */
        struct ConfiguredModelRow
        {
            s32 id = 0;
            s32 parentId = 0;
            String name;
            s32 refComponentId = 0;
            s32 lft = 0;
            s32 rgt = 0;
            String data;
            String type;
            s32 index = 0;
        };

        /** Row from the @c ref_components table. */
        struct RefComponent
        {
            s32 id = 0;
            String type;
            String title;
            s32 refComponentId = 0;
            bool enabled = false;
        };

        /** Row from the @c model_objects / @c actor_objects table. */
        struct ModelObject : ISharedObject
        {
            s32 id = 0;
            s32 modelId = 0;
            String className;
            String ident;
            String groupType;
            s32 originalId = 0;
        };

        /** Top-level model entry (configured_actors where parent_id = 1). */
        struct Model : ISharedObject
        {
            s32 id = 0;
            s32 parentId = 0;
            String name;
            String resource;
        };

        /** Generic attribute (attribs / ref_attribs / object_attributes). */
        struct Attrib
        {
            s32 id = 0;
            String name;
            String value;
            String groupType;
            s32 refComponentId = 0;
            s32 modelId = 0;
            s32 configuredModelId = 0;
            s32 objectId = 0;
            bool applied = false;
        };

        /** Component group (component_groups). */
        struct ComponentGroup
        {
            s32 id = 0;
            String title;
            String groupType;
        };

        /** Aircraft / vehicle data structures that mirror the C# @c json. */
        struct TransformData
        {
            Vector3F position;
            Vector3F rotation;
            Vector3F scale{ 1.0f, 1.0f, 1.0f };
        };

        struct CurveValue
        {
            f32 time = 0.0f;
            f32 value = 0.0f;
        };

        struct AircraftAirfoilData
        {
            std::vector<CurveValue> cl;
            std::vector<CurveValue> cd;
            std::vector<CurveValue> cm;
            std::vector<CurveValue> clPlus10;
            std::vector<CurveValue> cdPlus10;
            std::vector<CurveValue> cmPlus10;
            std::vector<CurveValue> clMinus10;
            std::vector<CurveValue> cdMinus10;
            std::vector<CurveValue> cmMinus10;
        };

        struct AircraftWingData
        {
            String name;
            TransformData localTransform;
            s32 sectionCount = 0;
            f32 liftLineChordPosition = 0.0f;
            f32 wingTipWidthZeroToOne = 0.0f;
            f32 wingTipSweep = 0.0f;
            f32 wingTipAngle = 0.0f;
            f32 aoaMultiplier = 0.0f;
            f32 cdMultiplier = 0.0f;
            f32 clMultiplier = 0.0f;
            f32 cmMultiplier = 0.0f;
            f32 stallControlCL = 0.0f;
            f32 stallControlCD = 0.0f;
            f32 stallControlCM = 0.0f;
            f32 stallThreshold = 0.0f;
            String aerofoilName;
            Vector3F subDivision;
        };

        struct AircraftControlSurfaceData
        {
            String name;
            bool reverse = false;
            f32 minDeflectionDegrees = 0.0f;
            f32 maxDeflectionDegrees = 0.0f;
            f32 rootHingeDistanceFromTrailingEdge = 0.0f;
            f32 tipHingeDistanceFromTrailingEdge = 0.0f;
            Vector3F modelRotationAxis;
            s32 surfaceId = 0;
            f32 aoaMultiplier = 0.0f;
            f32 cdMultiplier = 0.0f;
            f32 clMultiplier = 0.0f;
            f32 cmMultiplier = 0.0f;
            f32 stallControlCL = 0.0f;
            f32 stallControlCD = 0.0f;
            f32 stallControlCM = 0.0f;
            f32 stallThreshold = 0.0f;
            std::vector<s32> affectedSections;
            std::vector<f32> clLookup;
            std::vector<f32> cdLookup;
            std::vector<f32> cmLookup;
        };

        struct AircraftPropWashData
        {
            String name;
            String propwashSource;
            f32 strength = 0.0f;
            std::vector<s32> affectedSections;
            std::vector<f32> sectionMultipliers;
        };

        struct AircraftEngineData
        {
            String name;
            TransformData localTransform;
            f32 thrustMultiplier = 0.0f;
            f32 torqueMultiplier = 0.0f;
        };

        struct AircraftWheelData
        {
            String name;
            TransformData localTransform;
            f32 radius = 0.0f;
            f32 wheelDamping = 0.0f;
            f32 mass = 0.0f;
            f32 suspensionDistance = 0.0f;
            f32 springRate = 0.0f;
            f32 suspensionDamper = 0.0f;
            f32 forwardExtremumSlip = 0.0f;
            f32 forwardExtrememValue = 0.0f;
            f32 forwardAsymptoteSlip = 0.0f;
            f32 forwardAsymptoteValue = 0.0f;
            f32 forwardStiffness = 0.0f;
            f32 sidewaysExtremumSlip = 0.0f;
            f32 sidewaysExtrememValue = 0.0f;
            f32 sidewaysAsymptoteSlip = 0.0f;
            f32 sidewaysAsymptoteValue = 0.0f;
            f32 sidewaysStiffness = 0.0f;
        };

        struct AircraftData
        {
            Vector3F cgPosition;
            Vector3F drag;
            f32 rollwiseDamping = 0.0f;
            f32 sectionMultiplier = 0.0f;
            std::vector<AircraftWingData> wingData;
            std::vector<AircraftControlSurfaceData> controlSurfaceData;
            std::vector<AircraftPropWashData> propWashData;
            std::vector<AircraftEngineData> engineData;
            std::vector<AircraftWheelData> wheelData;
        };

        /** Transmitter profile loaded from a JSON file (rx_map import). */
        struct TransmitterProfile
        {
            std::vector<Attrib> functions;
        };
    }  // namespace adbeditor
}  // namespace workphone

#endif  // AssetDatabaseDomain_h__
