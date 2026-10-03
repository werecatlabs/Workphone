#ifndef CDebugCircle_h__
#define CDebugCircle_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IDebugCircle.hpp>

namespace workphone
{
    namespace render
    {
        class CDebugCircle : public IDebugCircle
        {
        public:
            CDebugCircle();
            CDebugCircle( hash_type id, const Vector3<real_Num> &position, real_Num radius, u32 color );

            ~CDebugCircle() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            void update() override;

            /** @copydoc IDebugLine::getLifeTime */
            f64 getLifeTime() const override;

            /** @copydoc IDebugLine::setLifeTime */
            void setLifeTime( f64 lifeTime ) override;

            /** @copydoc IDebugLine::getMaxLifeTime */
            f64 getMaxLifeTime() const override;

            /** @copydoc IDebugLine::setMaxLifeTime */
            void setMaxLifeTime( f64 maxLifeTime ) override;

            /** @copydoc IDebugLine::isVisible */
            bool isVisible() const override;

            /** @copydoc IDebugLine::setVisible */
            void setVisible( bool visible ) override;

            void setPosition( const Vector3<real_Num> &position ) override;
            void setRadius( real_Num radius ) override;
            void setColor( u32 color ) override;

            Vector3<real_Num> getPosition() const override;
            real_Num getRadius() const override;
            u32 getColor() const override;

            Ogre::HlmsUnlitDatablock *getDatablock() const;
            void setDatablock( Ogre::HlmsUnlitDatablock *datablock );

            String getMaterialName() const;
            void setMaterialName( const String &materialName );

            bool isDirty() const override;
            void setDirty( bool dirty ) override;

            Ogre::ObjectMemoryManager *getMemoryManager() const;
            void setMemoryManager( Ogre::ObjectMemoryManager *memoryManager );

            Quaternion<real_Num> getOrientation() const override;

            void setOrientation( const Quaternion<real_Num> &orientation ) override;

        private:
            Ogre::ManualObject *m_manualObject = nullptr;

            Ogre::HlmsUnlitDatablock *m_datablock = nullptr;

            Ogre::ObjectMemoryManager *m_memoryManager = nullptr;

            RawPtr<Ogre::SceneNode> m_sceneNode;

            f64 m_lifeTime = 0.0;
            f64 m_maxLifeTime = 1.0;

            f32 m_radius = 1.0f;

            // Number of line segments to approximate the circle.
            s32 segments = 36;

            Vector3<real_Num> m_position;
            Quaternion<real_Num> m_orientation;

            u32 m_colour = 0;
            atomic_bool m_dirty = false;
            atomic_bool m_isVisible = false;

            String m_materialName;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CDebugCircle_h__
