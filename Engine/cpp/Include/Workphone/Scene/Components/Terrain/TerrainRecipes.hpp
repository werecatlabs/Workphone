#ifndef WPTerrainRecipes_h__
#define WPTerrainRecipes_h__

#include <Workphone/Scene/Components/Terrain/TerrainEditing.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone::scene
{
    struct TerrainRecipeOperation
    {
        enum class Kind
        {
            Brush,
            Erosion,
            Grade
        };
        Kind kind = Kind::Brush;
        TerrainBrush brush;
        TerrainErosionRecipe erosion;
        TerrainGradeRecipe grade;
        bool enabled = true;
    };

    /** Ordered, non-destructive terrain recipe. Workers only own CPU snapshots;
     * owner-thread commit checks the captured source revision and one bounded undo patch.
     * Cancel/unload drains workers; stale generations can never publish.
     */
    class WPCore_API TerrainRecipeStack final : public ISharedObject
    {
    public:
        TerrainRecipeStack();
        ~TerrainRecipeStack() override;
        void unload( SmartPtr<ISharedObject> data ) override;
        bool setBase( SmartPtr<TerrainSystem> terrain, String &error );
        bool append( const TerrainRecipeOperation &operation, String &error );
        bool move( u32 from, u32 to, String &error );
        bool remove( u32 index, String &error );
        bool setEnabled( u32 index, bool enabled, String &error );
        u32 getOperationCount() const;
        u64 getGeneration() const;
        bool requestPreview( String &error );
        bool poll( String &error );
        bool isBusy() const;
        render::TerrainSnapshot getPreview() const;
        render::TerrainSnapshot getPreviewFor( SmartPtr<TerrainSystem> terrain ) const;
        bool commit( SmartPtr<TerrainSystem> terrain, SmartPtr<ICommandManager> commands,
                     String &error );
        void cancel();
        String serialize() const;
        bool deserialize( const String &json, SmartPtr<TerrainSystem> target, String &error );
        bool save( const String &path, String &error ) const;
        bool load( const String &path, SmartPtr<TerrainSystem> target, String &error );
        WP_CLASS_REGISTER_DECL;

    private:
        struct State;
        std::unique_ptr<State> m_state;
    };
}  // namespace workphone::scene
#endif
