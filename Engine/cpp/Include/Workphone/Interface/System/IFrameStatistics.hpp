//
// Created by Zane Desir on 10/11/2021.
//

#ifndef WP_IFRAMESTATISTICS_H
#define WP_IFRAMESTATISTICS_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /** An interface class to show frame statistics.
     */
    class WPCore_API IFrameStatistics : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IFrameStatistics() override;

        /** Tells this object whether to be visible or not, if it has a renderable component. */
        virtual void setVisible( bool visible ) = 0;

        /** Return a boolean indicating whether or not this object is visible. */
        virtual bool isVisible() const = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // WP_IFRAMESTATISTICS_H
