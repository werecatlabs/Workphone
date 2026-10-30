#ifndef WP_GraphicsAssetCleanup_hpp
#define WP_GraphicsAssetCleanup_hpp

#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone::render
{
    /** Periodic garbage-task cleanup under graphics lifecycle ownership.
     * Materials retire before textures; failed material inspections retain their
     * automatic retry delay. The graphics owner stays alive through the sweep. */
    WPCore_API void collectUnusedGraphicsAssets( SmartPtr<IGraphicsSystem> graphics,
                                                u32 maximumCandidates = 32 );
}

#endif
