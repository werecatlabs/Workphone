#pragma once
#include <string>
namespace workphone::advanced
{
    bool captureFrame( const std::string &path );
    std::string captureDeviceEvidence();
    std::string captureRadianceEvidence( void *textureView, bool &valid, bool cubemap = true,
                                         float *peakRadiance = nullptr );
    std::string captureLinearSceneEvidence( void *textureView, bool &valid );
}
