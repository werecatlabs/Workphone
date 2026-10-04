#ifndef RenderTargetListener_h__
#define RenderTargetListener_h__

#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>

namespace workphone
{
    namespace render
    {

        class WPCore_API RenderTargetListener : public IStateListener
        {
        public:
            RenderTargetListener();
            ~RenderTargetListener() override;

            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            SmartPtr<render::IRenderTarget> getOwner() const;
            void setOwner( SmartPtr<render::IRenderTarget> owner );

        protected:
            AtomicWeakPtr<render::IRenderTarget> m_owner;
        };

    }  // namespace render
}  // namespace workphone

#endif  // RenderTargetListener_h__
