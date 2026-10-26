#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    struct WPCore_API ResourcePackageRecoveryResult
    {
        Array<String> recovered, skipped, warnings;
    };

    /** Windows request-snapshot recovery, without cooking or consulting a catalog.
     * Archives only UUID.json profiles with versioned ownership markers, bound
     * to this project's owned job folder. Unmarked legacy records are preserved.
     * Live request locks are skipped. Logs, reports, packages and staging trees
     * are preserved; an interrupted request's publication outcome is unknown. */
    WPCore_API bool recoverResourcePackageJobs( const String &projectRoot,
                                                ResourcePackageRecoveryResult &result, String &error );
}  // namespace workphone
