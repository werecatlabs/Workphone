#ifndef CDecalCursor_h__
#define CDecalCursor_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/DecalCursor.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {
        class CDecalCursor : public DecalCursor
        {
        public:
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

        protected:
            class CDecalCursorStateListener : public IStateListener
            {
            public:
                CDecalCursorStateListener( CDecalCursor *owner );
                ~CDecalCursorStateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

            protected:
                CDecalCursor *m_owner = nullptr;
            };

            SmartPtr<IGraphicsMesh> m_entity;
            SmartPtr<IGraphicsSceneNode> m_node;
            SmartPtr<IGraphicsScene> m_sceneMgr;
            DecalCursor *m_decalCursor = nullptr;
            Vector3F m_position;
            Vector2F m_size;
            bool m_isVisible;
            String m_textureName;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CDecalCursor_h__
