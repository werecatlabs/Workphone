#pragma once
#include <string>
namespace workphone::advanced
{
    bool captureFrame( const std::string &path );
    std::string captureDeviceEvidence();
    std::string captureRadianceEvidence( void *textureView, bool &valid );
}
