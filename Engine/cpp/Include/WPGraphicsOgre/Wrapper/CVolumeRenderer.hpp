#ifndef CVolumeRenderer_h__
#define CVolumeRenderer_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IVolumeRenderer.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>
#include <OgreRenderable.h>
#include <OgreMovableObject.h>

namespace workphone
{
    namespace render
    {

        class CVolumeRenderer : public IVolumeRenderer
        {
        public:
            class ParticleVolumeRenderer : public Ogre::Renderable, public Ogre::MovableObject
            {
            public:
                ParticleVolumeRenderer( const Ogre::String &name );
                virtual ~ParticleVolumeRenderer();

                // Set the particle positions and update the texture atlas
                void setParticlePositions( const std::vector<Ogre::Vector3> &positions );

                // Render the particle volume using the specified camera
                void render( const Ogre::Camera *camera );

                // Required overrides for Renderable and MovableObject interfaces
                virtual const Ogre::MaterialPtr &getMaterial( void ) const;
                virtual void getRenderOperation( Ogre::RenderOperation &op );
                virtual void getWorldTransforms( Ogre::Matrix4 *xform ) const;
                virtual const Ogre::LightList &getLights( void ) const;
                virtual const Ogre::String &getMovableType( void ) const;

            private:
                Ogre::MaterialPtr m_material;
                Ogre::LightList m_lights;
                Ogre::String m_movableType;
            };

            CVolumeRenderer();
            ~CVolumeRenderer();

            void createTextureAtlas();
            void createMaterial();
            void createMesh();
            void updateMesh();
            void updateTextureAtlas();
            void updateMaterial();

        protected:
            Ogre::MeshPtr particleMesh;
            Ogre::MaterialPtr particleMaterial;
            ParticleTextureAtlas *particleTextureAtlas;
            int particleCount;
            int particleSize;
            int atlasSize;
            std::vector<Ogre::Vector3> particlePositions;
        };

    }  // namespace render
}  // namespace workphone

#endif  // CVolumeRenderer_h__
