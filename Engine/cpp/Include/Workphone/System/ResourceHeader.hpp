#ifndef ResourceHeader_h__
#define ResourceHeader_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/UUID.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief Metadata describing a compiled resource file.
     *
     * Every compiled resource begins with this header so that the engine can
     * identify its version, type, dependencies, and original source hash
     * before loading the body data.
     */
    struct ResourceHeader
    {
        s32 m_version = -1;                 ///< Resource schema version
        UUID m_resourceType;                ///< Type UUID of this resource (e.g. mesh, texture)
        Array<UUID> m_installDependencies;  ///< Dependencies that must be installed first
        u64 m_sourceResourceHash = 0;       ///< Hash of the original source file

        ResourceHeader();

        explicit ResourceHeader( s32 version, UUID type, u64 hash );

        void clear();

        UUID getResourceTypeID() const;

        void addInstallDependency( UUID resourceID );
    };

}  // namespace workphone

#endif  // ResourceHeader_h__
