#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawMaterialDrawRecords.hpp>
#include <Workphone/Graphics/Material.hpp>
#include "ClawMeshMaterial.hpp"
#include <atomic>
#include <limits>
#include <stdexcept>

namespace workphone::render
{
    namespace
    {
        std::atomic<u64> nextPublication{ 1 };
        u64 newPublication()
        {
            // Saturate instead of reusing an identity, including after overflow.
            auto id = nextPublication.load( std::memory_order_relaxed );
            do
            {
                if( id == std::numeric_limits<u64>::max() )
                    throw std::overflow_error( "material publication identities exhausted" );
            } while( !nextPublication.compare_exchange_weak( id, id + 1, std::memory_order_relaxed ) );
            return id;
        }
    }

    ClawMaterialDrawRecords::ClawMaterialDrawRecords() : m_publication( newPublication() ) {}

    u32 ClawMaterialDrawRecords::captureMaterial( SmartPtr<IMaterial> material )
    {
        if( auto found = m_materials.find( material.get() ); found != m_materials.end() )
            return found->second;
        if( m_records.size() >= std::numeric_limits<u32>::max() )
            throw std::length_error( "too many captured materials" );
        Owner owner;
        owner.material = std::move( material );
        if( auto concrete = dynamic_cast<Material *>( owner.material.get() ) )
            owner.state = concrete->getRenderState();
        else if( owner.material )
            for( auto technique : owner.material->getTechniques() )
            {
                if( technique ) for( auto pass : technique->getPasses() )
                    if( auto context = pass ? pass->getStateContext() : nullptr )
                        if( auto data = context->getStateDataById<MaterialPassStateData>( pass->getId() ) )
                        { owner.state = data->getRenderState(); break; }
                if( owner.state ) break;
            }
        Record record;
        record.material = owner.material.get();
        record.state = owner.state.get();
        if( record.state )
        {
            record.prepared = ClawMaterialRenderCache::prepare( *record.state );
            ++m_preparations;
        }
        const auto index = static_cast<u32>( m_records.size() );
        m_owners.push_back( std::move( owner ) );
        m_records.push_back( record );
        m_materials.emplace( record.material, index );
        return index;
    }

    void ClawMaterialDrawRecords::capture( ClawMesh *mesh )
    {
        if( m_sealed ) throw std::logic_error( "captured materials are immutable" );
        if( !mesh || m_meshes.find( mesh ) != m_meshes.end() ) return;
        const auto native = mesh->getNativeMesh();
        if( !native ) return;
        const auto count = static_cast<u32>( std::max<s32>( 1, wp_graphics_mesh_get_submesh_count( native ) ) );
        if( m_bindings.size() > std::numeric_limits<u32>::max() - count )
            throw std::length_error( "too many captured material bindings" );
        Binding binding{ m_publication, mesh, static_cast<u32>( m_bindings.size() ), count };
        for( u32 section = 0; section < count; ++section )
            m_bindings.push_back( captureMaterial( resolveMeshMaterial( mesh, section ) ) );
        m_meshes.emplace( mesh, binding );
    }

    void ClawMaterialDrawRecords::clear()
    {
        const auto publication = newPublication();
        m_records.clear();
        m_owners.clear();
        m_bindings.clear();
        m_materials.clear();
        m_meshes.clear();
        m_publication = publication;
        m_preparations = 0;
        m_sealed = false;
    }

    const ClawMaterialDrawRecords::Binding *ClawMaterialDrawRecords::find( const ClawMesh *mesh ) const
    {
        if( !m_sealed ) return nullptr;
        const auto found = m_meshes.find( mesh );
        return found == m_meshes.end() ? nullptr : &found->second;
    }

    bool ClawMaterialDrawRecords::valid( const Binding &binding, const ClawMesh *mesh, u32 sections ) const
    {
        return m_sealed && binding.publication == m_publication && binding.mesh == mesh &&
            binding.count == sections && binding.first <= m_bindings.size() &&
            binding.count <= m_bindings.size() - binding.first;
    }

    const ClawMaterialDrawRecords::Record *ClawMaterialDrawRecords::get( const Binding &binding, u32 section ) const
    {
        if( !valid( binding, binding.mesh, binding.count ) || section >= binding.count ) return nullptr;
        const auto index = m_bindings[binding.first + section];
        return index < m_records.size() ? &m_records[index] : nullptr;
    }
}
