#ifndef CollisionTerrain_h__
#define CollisionTerrain_h__

#include <Workphone/Scene/Components/Collision.hpp>
#include <Workphone/Graphics/TerrainData.hpp>

namespace workphone
{
    namespace scene
    {

        /** Collision terrain component backed by a heightfield physics shape. */
        class WPCore_API CollisionTerrain : public Collision
        {
        public:
            /** Property key for terrain width (number of samples along X). */
            static const String terrainWidthStr;

            /** Property key for terrain depth (number of samples along Z). */
            static const String terrainDepthStr;

            /** Property key for the world-space scale applied to the heightfield. */
            static const String terrainScaleStr;

            /** Constructor. */
            CollisionTerrain();

            /** Destructor. */
            ~CollisionTerrain() override;

            /** @copydoc Collision::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Collision::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Component::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc Collision::isValid */
            bool isValid() const override;

            /** @copydoc Component::updateTransform */
            void updateTransform() override;

            /** Prepare at the scene/physics owner synchronization point. Failure keeps
             * the active shape untouched. TerrainSystem commits all prepared consumers.
             */
            bool prepareTerrain( render::TerrainSnapshot snapshot,
                                 const Transform3<real_Num> &worldTransform, String &error );
            void commitTerrain();
            void discardPreparedTerrain();
            render::TerrainSnapshot getTerrainSnapshot() const;
            u64 getTerrainRevision() const;
            const String &getTerrainError() const;
            Transform3<real_Num> getPublishedTerrainTransform() const;
            bool isTerrainPrepared( render::TerrainSnapshot snapshot, const Transform3<real_Num> &placement ) const;

            /** @copydoc Collision::handleComponentEvent */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /** Gets the number of heightfield samples along the X axis. */
            u32 getTerrainWidth() const;

            /** Sets the number of heightfield samples along the X axis. */
            void setTerrainWidth( u32 width );

            /** Gets the number of heightfield samples along the Z axis. */
            u32 getTerrainDepth() const;

            /** Sets the number of heightfield samples along the Z axis. */
            void setTerrainDepth( u32 depth );

            /** Gets the world-space scale applied to the heightfield. */
            Vector3<real_Num> getTerrainScale() const;

            /** Sets the world-space scale applied to the heightfield. */
            void setTerrainScale( const Vector3<real_Num> &scale );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @copydoc Collision::createPhysicsShape */
            void createPhysicsShape() override;

            /** Number of heightfield samples along the X axis. */
            u32 m_terrainWidth = 0u;

            /** Number of heightfield samples along the Z axis. */
            u32 m_terrainDepth = 0u;

            /** World-space scale applied to the heightfield. */
            Vector3<real_Num> m_terrainScale = Vector3<real_Num>::unit();

            SmartPtr<physics::IPhysicsShape3> m_preparedShape;
            render::TerrainSnapshot m_terrainSnapshot;
            render::TerrainSnapshot m_preparedSnapshot;
            Transform3<real_Num> m_preparedTransform;
            Transform3<real_Num> m_publishedTransform;
            String m_terrainError;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // CollisionTerrain_h__
