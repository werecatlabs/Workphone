#ifndef CDebugOgre_h__
#define CDebugOgre_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/Debug.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <OgreWireBoundingBox.h>

namespace workphone
{
    namespace render
    {

        class CDebugOgre : public Debug
        {
        public:
            class StateListener : public IStateListener
            {
            public:
                StateListener();
                ~StateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                SmartPtr<CDebugOgre> getOwner() const;

                void setOwner( SmartPtr<CDebugOgre> owner );

            protected:
                WeakPtr<CDebugOgre> m_owner;
            };

            CDebugOgre();
            CDebugOgre( const CDebugOgre &other ) = delete;
            ~CDebugOgre() override;

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

            void drawText( hash_type id, const Vector2<real_Num> &position, const String &text,
                           u32 color ) override;

        private:
            void createLineMaterial();

            ConcurrentArray<SmartPtr<IDebugLine>> getDebugLines() const;
            void setDebugLines( ConcurrentArray<SmartPtr<IDebugLine>> debugLines );

            ConcurrentArray<SmartPtr<IDebugCircle>> getDebugCircles() const;
            void setDebugCircles( ConcurrentArray<SmartPtr<IDebugCircle>> debugCircles );

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

            AtomicSmartPtr<IOverlay> m_overlay;
            ConcurrentArray<SmartPtr<IOverlayElement>> m_overlayElements;

            ConcurrentQueue<SmartPtr<IDebugLine>> m_removeQueue;
            ConcurrentArray<SmartPtr<IDebugLine>> m_debugLines;
            ConcurrentArray<SmartPtr<IDebugCircle>> m_debugCircles;

            Ogre::WireBoundingBox *m_wireBoundingBox = nullptr;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CDebug_h__
