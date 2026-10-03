#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Graphics/BillboardSet.hpp"
#include <Workphone/Interface/Graphics/IBillboard.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include "Workphone/Graphics/Billboard.hpp"

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, BillboardSet, IBillboardSet );

    BillboardSet::BillboardSet()
    {
        m_cullIndividually = false;
        m_sortingEnabled = false;
        m_useAccurateFacing = false;
        m_radius = 0.0f;
        m_defaultDimensions = Vector3<real_Num>::zero();
    }

    BillboardSet::~BillboardSet()
    {
        clear();
    }

    void BillboardSet::clear()
    {
        m_billboards.clear();
    }

    SmartPtr<IBillboard> BillboardSet::createBillboard( const Vector3<real_Num> &position )
    {
        // Assume there is a concrete Billboard implementation called Billboard
        // If not, this should be replaced with the correct type
        auto billboard = make_ptr<Billboard>();
        billboard->setPosition( position );
        billboard->setScale( m_defaultDimensions );
        m_billboards.push_back( billboard );
        return billboard;
    }

    bool BillboardSet::removeBillboard( SmartPtr<IBillboard> billboard )
    {
        auto it = std::find( m_billboards.begin(), m_billboards.end(), billboard );
        if( it != m_billboards.end() )
        {
            m_billboards.erase( it );
            return true;
        }
        return false;
    }

    Array<SmartPtr<IBillboard>> BillboardSet::getBillboards() const
    {
        return m_billboards;
    }

    void BillboardSet::setCullIndividually( bool cullIndividually )
    {
        m_cullIndividually = cullIndividually;
    }

    void BillboardSet::setSortingEnabled( bool sortingEnabled )
    {
        m_sortingEnabled = sortingEnabled;
    }

    void BillboardSet::setUseAccurateFacing( bool useAccurateFacing )
    {
        m_useAccurateFacing = useAccurateFacing;
    }

    void BillboardSet::setBounds( const AABB3<real_Num> &box, real_Num radius )
    {
        m_bounds = box;
        m_radius = radius;
    }

    void BillboardSet::setDefaultDimensions( const Vector3<real_Num> &dimension )
    {
        m_defaultDimensions = dimension;
    }

    void BillboardSet::setMaterialName( const String &materialName, s32 index )
    {
        if( index < 0 )
        {
            m_materialNames.clear();
            m_materialNames.push_back( materialName );
        }
        else
        {
            if( static_cast<size_t>( index ) >= m_materialNames.size() )
            {
                m_materialNames.resize( index + 1 );
            }
            m_materialNames[index] = materialName;
        }
    }

    String BillboardSet::getMaterialName( s32 index ) const
    {
        if( index < 0 )
        {
            if( !m_materialNames.empty() )
            {
                return m_materialNames[0];
            }
        }
        else if( static_cast<size_t>( index ) < m_materialNames.size() )
        {
            return m_materialNames[index];
        }
        return String();
    }

    void BillboardSet::setMaterial( SmartPtr<IMaterial> material, s32 index )
    {
        if( index < 0 )
        {
            m_materials.clear();
            m_materials.push_back( material );
        }
        else
        {
            if( static_cast<size_t>( index ) >= m_materials.size() )
            {
                m_materials.resize( index + 1 );
            }
            m_materials[index] = material;
        }
    }

    SmartPtr<IMaterial> BillboardSet::getMaterial( s32 index ) const
    {
        if( index < 0 )
        {
            if( !m_materials.empty() )
            {
                return m_materials[0];
            }
        }
        else if( static_cast<size_t>( index ) < m_materials.size() )
        {
            return m_materials[index];
        }
        return nullptr;
    }

    SmartPtr<IGraphicsObject> BillboardSet::clone( const String &name ) const
    {
        // Shallow clone for demonstration; deep copy logic may be needed
        auto cloned = make_ptr<BillboardSet>();
        cloned->m_billboards = m_billboards;
        cloned->m_materials = m_materials;
        cloned->m_materialNames = m_materialNames;
        cloned->m_cullIndividually = m_cullIndividually;
        cloned->m_sortingEnabled = m_sortingEnabled;
        cloned->m_useAccurateFacing = m_useAccurateFacing;
        cloned->m_bounds = m_bounds;
        cloned->m_radius = m_radius;
        cloned->m_defaultDimensions = m_defaultDimensions;
        return cloned;
    }

    void BillboardSet::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = nullptr;  // No native object in this implementation
        }
    }
}  // namespace workphone::render
