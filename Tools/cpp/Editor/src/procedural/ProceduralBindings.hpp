#ifndef WP_PROCEDURAL_BINDINGS_HPP
#define WP_PROCEDURAL_BINDINGS_HPP

#include <EditorPrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render  { class IMesh; }
    namespace procedural { class IProceduralTerrain; }
    namespace editor
    {

        /**
         * @class ProceduralBindings
         * @brief C++ bindings exposing procedural mesh/L-system generators to the Lua editor layer.
         *
         * This class is the single C++ entry point for all Lua-callable procedural modelling
         * operations. It wraps IProceduralManager, IMeshGenerator, ILSystem, and related
         * procedural system getters so that Lua scripts (particularly ProceduralModelEditor.lua)
         * can drive procedural generation without directly calling Interface headers.
         *
         * Lifetime: owned by the editor application; passed to Lua as a ScriptInvoker target.
         *
         * @note All methods are thin wrappers over the existing procedural runtime. No new
         *       procedural systems are invented here.
         * @note Boolean mesh operations are stubbed: the binding exists so the Lua editor
         *       can call it; the actual CSG kernel belongs in the runtime and is flagged
         *       as a deferred implementation item.
         */
        class ProceduralBindings : public IScriptReceiver
        {
        public:
            WP_CLASS_REGISTER_DECL;

            ProceduralBindings();
            ~ProceduralBindings() override;

            // --- Lua-callable methods -------------------------------------------

            /** Generate a mesh from an L-system grammar. */
            SmartPtr<render::IMesh> generateMeshFromLSystem(
                const String &axiom,
                const Array<String> &rules,
                u32 iterations,
                real_Num angleDeg,
                real_Num stepLength );

            /** Apply a CSG boolean (union/subtract/intersect) between two meshes.
             *  booleanMode: 0=union, 1=subtract, 2=intersect.
             *  @note Currently stubbed pending runtime CSG implementation. */
            SmartPtr<render::IMesh> applyBooleanToMesh(
                SmartPtr<render::IMesh> meshA,
                SmartPtr<render::IMesh> meshB,
                u32 booleanMode );

            // --- L-system grammar mutators -------------------------------------

            void addLSystemVariable( const String &var );
            void setLSystemStart( const String &axiom );
            void addLSystemRule( const String &predecessor, const String &successor );
            void setLSystemAngle( real_Num angleDeg );
            void setLSystemStepLength( real_Num length );

            // --- Mesh generator configuration ------------------------------------

            void setMeshGeneratorQuality( real_Num quality );           // 0..1
            void setMeshGeneratorLOD( u32 numLods, real_Num distance );
            void setMeshGeneratorUV( bool generateUVs, bool generateTangents );
            void setMeshGeneratorCollision( bool generate, real_Num simplification );

            /** Generate mesh from a terrain object. */
            SmartPtr<render::IMesh> generateMeshFromTerrain(
                SmartPtr<procedural::IProceduralTerrain> terrain );

            SmartPtr<render::IMesh> getMeshGeneratorLastMesh();

            SmartPtr<procedural::IProceduralTerrain> getTerrainGenerator();
            SmartPtr<procedural::ICityGenerator>    getCityGenerator();

        private:
            /** IScriptReceiver: property access by hash — not used here. */
            s32 setProperty( hash_type, const String & ) override { return 0; }
            s32 getProperty( hash_type, String & ) const override { return 0; }
            s32 setProperty( hash_type, const Parameter & ) override { return 0; }
            s32 setProperty( hash_type, const Parameters & ) override { return 0; }
            s32 setProperty( hash_type, void * ) override { return 0; }
            s32 getProperty( hash_type, Parameter & ) const override { return 0; }
            s32 getProperty( hash_type, Parameters & ) const override { return 0; }
            s32 getProperty( hash_type, void * ) const override { return 0; }
            s32 callFunction( hash_type, const Parameters &, Parameters & ) override { return 0; }
            s32 callFunction( hash_type, SmartPtr<ISharedObject>, Parameters & ) override { return 0; }
        };

    }  // namespace editor
}  // namespace workphone

#endif  // WP_PROCEDURAL_BINDINGS_HPP
