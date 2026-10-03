#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/LOD/LodObject.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {

        u32 LodObject::m_idExt = 0;

        LodObject::LodObject() : m_lodPage( nullptr ), m_entity( nullptr ), m_node( nullptr )
        {
            m_id = m_idExt++;
        }

        LodObject::~LodObject()
        {
        }

        std::string LodObject::getMeshName() const
        {
            return m_meshName;
        }

        void LodObject::setMeshName( const std::string &meshName )
        {
            m_meshName = meshName;
        }

    }  // namespace render
}  // namespace workphone
