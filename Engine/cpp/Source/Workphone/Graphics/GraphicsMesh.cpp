#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsMesh.hpp>
#include <Workphone/Interface/Graphics/IAnimationController.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSkeleton.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsMesh, GraphicsObject<IGraphicsMesh> );

    GraphicsMesh::GraphicsMesh() = default;

    GraphicsMesh::~GraphicsMesh() = default;

    void GraphicsMesh::setMaterialName( const String &materialName, s32 index )
    {
        if( index == -1 )
        {
            m_materialName = materialName;
        }
        else
        {
            if( m_subMaterialNames.size() <= static_cast<size_t>( index ) )
            {
                m_subMaterialNames.resize( index + 1 );
            }

            m_subMaterialNames[index] = materialName;
        }
    }

    auto GraphicsMesh::getMaterialName( s32 index ) const -> String
    {
        if( index == -1 )
        {
            return m_materialName;
        }
        else if( index >= 0 && static_cast<size_t>( index ) < m_subMaterialNames.size() )
        {
            return m_subMaterialNames[index];
        }

        return {};
    }

    void GraphicsMesh::setMaterial( SmartPtr<IMaterial> material, s32 index )
    {
        if( index == -1 )
        {
            m_material = material;
        }
        else
        {
            if( m_subMaterials.size() <= static_cast<size_t>( index ) )
            {
                m_subMaterials.resize( index + 1 );
            }

            m_subMaterials[index] = material;
        }
    }

    auto GraphicsMesh::getMaterial( s32 index ) const -> SmartPtr<IMaterial>
    {
        if( index == -1 )
        {
            return m_material;
        }
        else if( index >= 0 && static_cast<size_t>( index ) < m_subMaterials.size() )
        {
            return m_subMaterials[index];
        }

        return nullptr;
    }

    void GraphicsMesh::setHardwareAnimationEnabled( bool enabled )
    {
        m_hardwareAnimationEnabled = enabled;
    }

    void GraphicsMesh::checkVertexProcessing()
    {
        // Implementation can be added as needed
    }

    auto GraphicsMesh::getAnimationController() -> SmartPtr<IAnimationController>
    {
        return m_animationController;
    }

    auto GraphicsMesh::getMeshName() const -> String
    {
        return m_meshName;
    }

    void GraphicsMesh::setMeshName( const String &meshName )
    {
        m_meshName = meshName;
    }

    ProgressiveMeshOptions GraphicsMesh::getProgressiveMeshOptions() const
    {
        return m_progressiveMeshOptions.load();
    }

    void GraphicsMesh::setProgressiveMeshOptions( const ProgressiveMeshOptions &options )
    {
        m_progressiveMeshOptions.store( options );
    }

    SmartPtr<IGraphicsSkeleton> GraphicsMesh::getSkeleton() const
    {
        return m_skeleton;
    }

    void GraphicsMesh::setSkeleton( SmartPtr<IGraphicsSkeleton> skeleton )
    {
        m_skeleton = skeleton;
    }

}  // namespace workphone::render
