#ifndef WP_CLAW_MATERIAL_DRAW_RECORDS_HPP
#define WP_CLAW_MATERIAL_DRAW_RECORDS_HPP

#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <unordered_map>
#include <vector>

namespace workphone::render
{
    class ClawMesh;

    /** Owner-thread builder, then immutable scene-job publication. Draws borrow
     * dense records; ownership is acquired once per distinct material. No native
     * views are retained. The publication must outlive every borrowed draw.
     */
    class WPGraphics_API ClawMaterialDrawRecords
    {
    public:
        struct Binding
        {
            u64 publication = 0;
            const ClawMesh *mesh = nullptr;
            u32 first = 0, count = 0;
        };
        struct Record
        {
            IMaterial *material = nullptr;
            const MaterialRenderState *state = nullptr;
            ClawPreparedMaterialDX11 prepared;
        };

        ClawMaterialDrawRecords();
        ClawMaterialDrawRecords( const ClawMaterialDrawRecords & ) = delete;
        ClawMaterialDrawRecords &operator=( const ClawMaterialDrawRecords & ) = delete;
        void capture( ClawMesh *mesh );
        void seal() { m_sealed = true; }
        // Retire owners and invalidate old bindings, retaining reusable capacity.
        void clear();
        const Binding *find( const ClawMesh *mesh ) const;
        bool valid( const Binding &, const ClawMesh *, u32 sections ) const;
        const Record *get( const Binding &, u32 section ) const;

        size_t getRecordCount() const { return m_records.size(); }
        size_t getBindingCount() const { return m_bindings.size(); }
        size_t getMeshCount() const { return m_meshes.size(); }
        u64 getPreparationCount() const { return m_preparations; }

    private:
        struct Owner
        {
            SmartPtr<IMaterial> material;
            MaterialRenderState::Ptr state;
        };
        u32 captureMaterial( SmartPtr<IMaterial> material );
        std::vector<Record> m_records;
        std::vector<Owner> m_owners;
        std::vector<u32> m_bindings;
        std::unordered_map<const IMaterial *, u32> m_materials;
        std::unordered_map<const ClawMesh *, Binding> m_meshes;
        u64 m_publication = 0, m_preparations = 0;
        bool m_sealed = false;
    };
}
#endif
