#pragma once

#include <Workphone/AI/MeshyClient.hpp>

namespace workphone::mesh_generation
{
    /** Hosted TRELLIS 2 through fal.ai. Run on a worker, not the Editor/UI task.
     * Request::targetPolycount is the target vertex count (5000..2000000).
     * Texturing is required; resolution must be 512, 1024 or 1536.
     * Cancellation stops local waiting/download and leaves the remote task running. */
    class WPCore_API Trellis2Client
    {
    public:
        virtual ~Trellis2Client();
        Result generate( const Request &request, std::uint32_t resolution,
                         const std::atomic_bool &cancel,
                         const std::function<void( const Progress & )> &progress = {} ) const;

    protected:
        virtual bool resolveApiKey( std::string &key, std::string &error ) const;
        virtual bool request( const std::string &method, const std::string &url,
                              const std::string &authorization, const std::string &body,
                              std::size_t maximumBytes, const std::atomic_bool &cancel,
                              std::vector<std::uint8_t> &response, std::uint32_t &status,
                              std::string &error ) const;
        virtual bool waitForPoll( const std::atomic_bool &cancel ) const;
    };
}
