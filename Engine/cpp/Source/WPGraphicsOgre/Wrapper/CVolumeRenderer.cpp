#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CVolumeRenderer.hpp>

namespace workphone
{
    namespace render
    {
        CVolumeRenderer::CVolumeRenderer()
        {
        }

        CVolumeRenderer::~CVolumeRenderer()
        {
        }

        void CVolumeRenderer::createTextureAtlas()
        {
        }

        void CVolumeRenderer::createMaterial()
        {
        }

        void CVolumeRenderer::createMesh()
        {
        }

        void CVolumeRenderer::updateMesh()
        {
        }

        void CVolumeRenderer::updateTextureAtlas()
        {
        }

        void CVolumeRenderer::updateMaterial()
        {
        }

        CVolumeRenderer::ParticleVolumeRenderer::ParticleVolumeRenderer( const Ogre::String &name )
        {
        }

        CVolumeRenderer::ParticleVolumeRenderer::~ParticleVolumeRenderer()
        {
        }

        void CVolumeRenderer::ParticleVolumeRenderer::setParticlePositions(
            const std::vector<Ogre::Vector3> &positions )
        {
        }

        void CVolumeRenderer::ParticleVolumeRenderer::render( const Ogre::Camera *camera )
        {
        }

        const Ogre::MaterialPtr &CVolumeRenderer::ParticleVolumeRenderer::getMaterial( void ) const
        {
            return m_material;
        }

        void CVolumeRenderer::ParticleVolumeRenderer::getRenderOperation( Ogre::RenderOperation &op )
        {
        }

        void CVolumeRenderer::ParticleVolumeRenderer::getWorldTransforms( Ogre::Matrix4 *xform ) const
        {
        }

        const Ogre::LightList &CVolumeRenderer::ParticleVolumeRenderer::getLights( void ) const
        {
            return m_lights;
        }

        const Ogre::String &CVolumeRenderer::ParticleVolumeRenderer::getMovableType( void ) const
        {
            return m_movableType;
        }

    }  // namespace render
}  // namespace workphone
