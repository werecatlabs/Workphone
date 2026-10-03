#ifndef CDebug_h__
#define CDebug_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IDebug.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace render
    {

        class CDebugOgreNext : public SharedGraphicsObject<IDebug>
        {
        public:
            class StateListener : public IStateListener
            {
            public:
                StateListener();
                ~StateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                SmartPtr<CDebugOgreNext> getOwner() const;

                void setOwner( SmartPtr<CDebugOgreNext> owner );

            protected:
                AtomicWeakPtr<CDebugOgreNext> m_owner;
            };

            CDebugOgreNext();
            CDebugOgreNext( const CDebugOgreNext &other ) = delete;
            ~CDebugOgreNext() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::preUpdate */
            void preUpdate() override;

            /** @copydoc ISharedObject::update */
            void update() override;

            /** @copydoc ISharedObject::postUpdate */
            void postUpdate() override;

            /** @copydoc IDebug::clear */
            void clear() override;

            /** @copydoc IDebug::drawPoint */
            void drawPoint( hash_type id, const Vector3<real_Num> &positon, u32 color ) override;

            /** @copydoc IDebug::drawLine */
            SmartPtr<IDebugLine> drawLine( hash_type id, const Vector3<real_Num> &start,
                                           const Vector3<real_Num> &end, u32 colour ) override;

            SmartPtr<IDebugCircle> drawCircle( hash_type id, const Vector3<real_Num> &position,
                                               const Quaternion<real_Num> &orientation, real_Num radius,
                                               u32 color ) override;

            Ogre::HlmsUnlitDatablock *getDatablock() const;
            void setDatablock( Ogre::HlmsUnlitDatablock *datablock );

            void drawText( hash_type id, const Vector2<real_Num> &position, const String &text,
                           u32 color ) override;

        private:
            void createLineMaterial();

            SmartPtr<IDebugLine> addLine( hash_type id );
            void removeLine( hash_type id );
            void removeLine( SmartPtr<IDebugLine> debugLine );
            SmartPtr<IDebugLine> getLine( hash_type id ) const;

            SmartPtr<IDebugCircle> addCircle( hash_type id );
            void removeCircle( hash_type id );
            void removeCircle( SmartPtr<IDebugCircle> debugCircle );
            SmartPtr<IDebugCircle> getCircle( hash_type id ) const;

            SmartPtr<IOverlay> getOverlay() const;

            void setOverlay( SmartPtr<IOverlay> overlay );

            void addOverlayElement( SmartPtr<IOverlayElement> element );
            SmartPtr<IOverlayElement> getElementById( hash_type id ) const;

            SmartPtr<IOverlay> m_overlay;

            ConcurrentArray<SmartPtr<IDebugLine>> m_debugLines;
            ConcurrentArray<SmartPtr<IDebugCircle>> m_debugCircles;
            ConcurrentArray<SmartPtr<IOverlayElement>> m_overlayElements;

            Ogre::HlmsUnlitDatablock *m_datablock = nullptr;

            Ogre::ObjectMemoryManager *m_memoryManager = nullptr;

            Ogre::v1::WireBoundingBox *m_wireBoundingBox = nullptr;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CDebug_h__
