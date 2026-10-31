#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    struct WPCore_API ResourcePackageHistoryResult
    {
        Array<String> selected, removed, skipped, warnings;
        u64 retainedBytes = 0;
    };

    /** Completion receipts adopt only this worker's closed log/report files.
     * Interrupted requests and unreceipted legacy files are never pruned. */
    WPCore_API bool recordResourcePackageCompletion(const String &projectRoot,
        const String &snapshotPath, const String &logPath, const String &previewPath,
        const String &status, String &error);

    /** Keep newest owned completed bundles within both limits. Dry-run makes no
     * persistent changes. protectedId pins the current Editor result. Hashes and
     * exclusive file handles protect modified, open and unrelated files. */
    WPCore_API bool pruneResourcePackageHistory(const String &projectRoot, u32 keepCount,
        u64 maximumBytes, bool dryRun, const String &protectedId,
        ResourcePackageHistoryResult &result, String &error);
}
