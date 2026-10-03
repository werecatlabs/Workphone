#ifndef CDecalCursor_h__
#define CDecalCursor_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/DecalCursor.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace render
    {

        class CDecalCursor : public DecalCursor
        {
        public:
            class CDecalCursorStateListener : public IStateListener
            {
            public:
                CDecalCursorStateListener( CDecalCursor *owner );
                ~CDecalCursorStateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

            protected:
                CDecalCursor *m_owner;
            };

            CDecalCursor();
            ~CDecalCursor() override;

            void initialise( SmartPtr<IGraphicsScene> sceneMgr, const String &terrainMaterial,
                             const String &decalTextureName, const Vector2F &size );

            bool isVisible() const override;
            void setVisible( bool visible ) override;

            Vector3F getPosition() const override;
            void setPosition( const Vector3F &position ) override;

            Vector2F getSize() const override;
            void setSize( const Vector2F &size ) override;

            String getTextureName() const;
            void setTextureName( const String &textureName );

            void addDebugEntity( const String &entityName,
                                 const Vector3F &scale = Vector3F::unit() ) override;
            void removeDebugEntity() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<IGraphicsMesh> m_entity;
            SmartPtr<IGraphicsSceneNode> m_node;
            SmartPtr<IGraphicsScene> m_sceneMgr;
            DecalCursorOgre *m_decalCursor;
            Vector3F m_position;
            Vector2F m_size;
            bool m_isVisible;
            String m_textureName;
        };

    }  // namespace render
}  // namespace workphone

#endif  // CDecalCursor_h__
