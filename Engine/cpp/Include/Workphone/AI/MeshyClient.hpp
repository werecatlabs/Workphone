#pragma once

#include <Workphone/WorkphonePrerequisites.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace workphone::mesh_generation
{
    inline constexpr std::size_t maximumSourceImageBytes = 20u * 1024u * 1024u;
    inline constexpr std::size_t maximumGeneratedGlbBytes = 128u * 1024u * 1024u;

    WPCore_API bool validateSourceImage( const std::uint8_t *bytes, std::size_t size,
                                         const std::string &mimeType, std::string &error );
    WPCore_API bool validateGeneratedGlb( const std::uint8_t *bytes, std::size_t size,
                                          std::string &error );

    struct Request
    {
        std::vector<std::uint8_t> imageBytes;
        std::string mimeType;
        std::uint32_t targetPolycount = 10000;
        bool textured = true;
    };

    struct Progress
    {
        std::string taskId;
        std::uint32_t percent = 0;
        std::string stage;
    };

    struct Result
    {
        std::vector<std::uint8_t> glbBytes;
        std::string taskId;
        std::string error;
        bool cancelled = false;
    };

    /** Synchronous worker service. Call away from the Editor/UI task.
     * Cancellation stops local waiting/download; it does not cancel the remote task. */
    class WPCore_API MeshyClient
    {
    public:
        virtual ~MeshyClient();
        Result generate( const Request &request, const std::atomic_bool &cancel,
                         const std::function<void( const Progress & )> &progress = {} ) const;

    protected:
        virtual bool resolveApiKey( std::string &key, std::string &error ) const;
        virtual bool request( const std::string &method, const std::string &url,
                              const std::string &bearer, const std::string &body,
                              std::size_t maximumBytes, const std::atomic_bool &cancel,
                              std::vector<std::uint8_t> &response, std::uint32_t &status,
                              std::string &error ) const;
        virtual bool waitForPoll( const std::atomic_bool &cancel ) const;
    };
}
