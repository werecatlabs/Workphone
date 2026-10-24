#pragma once
#include <Workphone/Database/AssetDocumentInventory.hpp>
#include <Workphone/Database/MediaAssetCatalog.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>

namespace
{
    void catalogSnapshotBudgetContracts(const std::filesystem::path &folder)
    {
        using namespace workphone;
        namespace fs = std::filesystem;
        struct Capacity
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
            size_t category = size_t(StreamMemoryClass::SourceCpu);
            u64 original = scheduler->stats().limits[category];
            u64 used() const { return scheduler->stats().used[category]; }
            void set(u64 bytes)
            {
                String error;
                require(scheduler->setBudget(StreamMemoryClass::SourceCpu, bytes, error),
                        "Catalog snapshot budget must apply");
            }
            void restore() { set(original); }
            ~Capacity() { restore(); }
        } budget;
        const auto root = folder / "snapshot-budgets";
        fs::create_directories(root / ".workphone");
        auto catalog = make_ptr<AssetDatabaseManager>();
        require(catalog->setProjectRoot(root.u8string().c_str()), "Budget fixture root");
        catalog->loadFromFile((root / ".workphone/catalog.db").u8string().c_str());
        auto bound = dynamic_cast<IParameterizedDatabase *>(catalog->getDatabase().get());
        require(bound != nullptr, "Snapshot budgets require real SQLite");
        for(int i = 0; i < 24; ++i)
        {
            const auto id = String("snapshot-") + StringUtil::toString(100 + i);
            const auto path = id + ".png";
            require(bound->queryBound("INSERT INTO resources(uuid,path,path_key,type,kind) VALUES(?,?,?,'Texture','file')",
                                     {id, path, path}) != nullptr, "Seed budget catalog");
        }
        AssetDatabaseManager::EntryQuery query;
        query.pageSize = 24;
        {
            AssetCatalogBrowser browser;
            AssetCatalogSession session;
            const auto resident = budget.used(); budget.set(resident);
            require(!browser.refresh(catalog, query) && !browser.page().succeeded &&
                        browser.page().entries.empty() && browser.page().error == "Budget exhausted" &&
                        !session.capture(catalog) && budget.used() == resident,
                    "Denied catalog observer/browser admission reports complete boolean failure without retaining partial metadata");
            budget.restore();
            require(browser.refresh(catalog, query) && session.capture(catalog),
                    "Catalog observer/browser bindings retry after admission returns");
            browser.clear(); session.clear();
        }
        {
            AssetDatabaseManager::EntryQuery navigationQuery;
            navigationQuery.pageSize = 1;
            // Warm database/string-pool helpers independently of the measured
            // cursor storage, including validation while returning to page one.
            {
                AssetCatalogBrowser warm;
                require(warm.refresh(catalog, navigationQuery), "Warm navigation first page");
                while(warm.page().hasMore)
                    require(warm.next(catalog), "Warm navigation cursor growth");
                while(warm.hasPrevious())
                    require(warm.previous(catalog), "Warm navigation backtracking");
                warm.clear();
            }
            const auto baseline = budget.used();
            AssetCatalogBrowser browser;
            require(browser.refresh(catalog, navigationQuery), "Open budgeted navigation");
            const auto firstResident = budget.used();
            require(browser.next(catalog) && browser.next(catalog) && browser.next(catalog) &&
                        browser.pageNumber() == 4 && browser.page().entries.front().uuid == "snapshot-103",
                    "Navigate with three retained continuation cursors");
            const auto cursorBytes = sourceBudgetStorageBytes(
                4 * sizeof(AssetDatabaseManager::EntryCursor), alignof(AssetDatabaseManager::EntryCursor));
            require(budget.used() == firstResident + cursorBytes,
                    "Cursor admission charges the physical four-slot buffer including its unused slot");
            const auto resident = budget.used();
            const auto rows = browser.page().entries.data();
            const auto generation = browser.page().next.generation;
            auto expectPreserved = [&]() {
                require(browser.pageNumber() == 4 && browser.hasPrevious() &&
                            browser.page().succeeded && browser.page().hasMore &&
                            browser.page().entries.data() == rows &&
                            browser.page().entries.front().uuid == "snapshot-103" &&
                            browser.page().next.afterUuid == "snapshot-103" &&
                            browser.page().error == "Budget exhausted" && budget.used() == resident,
                        "Denied navigation preserves the complete displayed page and cursor position without partial storage");
            };
            budget.set(resident);
            auto denials = sourceBudgetDenialCount();
            require(!browser.next(catalog) && sourceBudgetDenialCount() != denials,
                    "Next cursor candidate cannot bypass a full shared budget");
            expectPreserved();
            denials = sourceBudgetDenialCount();
            require(!browser.previous(catalog) && sourceBudgetDenialCount() != denials,
                    "Denied backtracking validation cannot consume the previous cursor");
            expectPreserved();
            budget.set(resident + cursorBytes);
            denials = sourceBudgetDenialCount();
            require(!browser.next(catalog) && sourceBudgetDenialCount() != denials,
                    "A complete admitted cursor candidate is discarded when its new page cannot be admitted");
            expectPreserved();
            budget.restore();
            require(catalog->getCatalogGeneration() == generation,
                    "Denied revision reads preserve the catalog generation and retained continuation identities");
            const auto backtracked = browser.previous(catalog);
            require(backtracked, "Backtracking retries after navigation admission is restored");
            require(browser.pageNumber() == 3 && browser.page().entries.front().uuid == "snapshot-102" &&
                        browser.page().error.empty(),
                    "Backtracking retry publishes the expected previous page and clears its transient error");
            require(budget.used() == resident,
                    "Backtracking retains its charged four-slot spare capacity");
            require(browser.next(catalog) && browser.pageNumber() == 4 &&
                        browser.page().entries.front().uuid == "snapshot-103" && budget.used() == resident,
                    "Forward navigation retries without skipping a denied page");
            auto retained = browser.page();
            const auto pageBytes = sourceBudgetStorageBytes(
                retained.entries.capacity() * sizeof(AssetDatabaseManager::EntrySnapshot),
                alignof(AssetDatabaseManager::EntrySnapshot));
            budget.set(budget.used());
            denials = sourceBudgetDenialCount();
            browser.clear();
            require(sourceBudgetDenialCount() == denials && browser.pageNumber() == 1 &&
                        !browser.hasPrevious() && browser.page().entries.empty() &&
                        budget.used() == baseline + pageBytes &&
                        retained.entries.front().uuid == "snapshot-103" &&
                        retained.entries.get_allocator().isSourceBudgeted(),
                    "Full-budget clear releases cursor and binding metadata while detached rows retain their own charge");
            retained = {};
            require(budget.used() == baseline, "Final detached navigation page retires its physical buffer");
            budget.restore();
        }
        { auto warm = catalog->queryEntries(query); require(warm.succeeded, "Warm detached page"); }
        auto page = catalog->queryEntries(query);
        require(page.succeeded && page.entries.size() == 24 && page.entries.get_allocator().isSourceBudgeted(),
                "Catalog page owns admitted caller storage");
        const auto pageResident = budget.used();
        budget.set(pageResident);
        auto failedPage = catalog->queryEntries(query);
        require(!failedPage.succeeded && failedPage.entries.empty() && !failedPage.hasMore &&
                    page.entries.size() == 24 && page.entries.front().uuid == "snapshot-100",
                "Denied page publishes no truncated rows and preserves earlier detached pages");
        bool denied = false;
        try { auto copy = page; } catch(const std::bad_alloc &) { denied = true; }
        require(denied && page.entries.size() == 24, "Caller page copies cannot bypass the shared limit");
        budget.restore();
        auto copiedPage = page;
        require(copiedPage.succeeded && copiedPage.entries.get_allocator().isSourceBudgeted(),
                "Catalog page copying retries with admitted storage");

        resource::ResourceCompilationDatabase compilation;
        require(compilation.connect((root / ".workphone/compilation.db").u8string().c_str()),
                "Open compilation snapshot fixture");
        resource::CompiledResourceRecord record{"snapshot-id", "Texture", "compiled.bin", 1, 2, 3};
        require(compilation.commitCompilation(record, {{"source.png", false}, {"shared.png", false}}),
                "Seed dependency snapshots");
        Array<resource::CompileDependencyRecord> dependencies{{"kept", true}};
        Array<String> dependents{"kept"};
        budget.set(budget.used());
        require(!compilation.getDependencies("snapshot-id", dependencies) && dependencies.size() == 1 &&
                    dependencies[0].path == "kept" && !compilation.getDependents("shared.png", dependents) &&
                    dependents.size() == 1 && dependents[0] == "kept",
                "Denied dependency snapshots preserve caller outputs without partial rows");
        budget.restore();
        require(compilation.getDependencies("snapshot-id", dependencies) && dependencies.size() == 2 &&
                    dependencies.get_allocator().isSourceBudgeted() &&
                    compilation.getDependents("shared.png", dependents) && dependents.size() == 1 &&
                    dependents[0] == "snapshot-id" && dependents.get_allocator().isSourceBudgeted(),
                "Dependency snapshots retry and transfer the admitted allocation policy to callers");

        Properties document;
        document.setProperty("name", "budgeted-document");
        const auto json = DataUtil::toString(&document, true);
        {
            auto serialized = make_ptr<Properties>();
            *serialized = document;
            const String deniedPath = (root / "denied.material").u8string().c_str();
            String saveError;
            budget.set(budget.used());
            require(!saveAssetDocumentText(deniedPath, serialized, json, saveError) && !saveError.empty(),
                    "Denied text-document byte snapshot reports failure before file publication");
            budget.restore();
            require(!fs::exists(root / "denied.material"), "Denied document snapshot leaves no installed file");
        }
        auto write = [](const fs::path &path, const std::string &text) {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output.write(text.data(), static_cast<std::streamsize>(text.size()));
            require(bool(output), "Write snapshot budget fixture");
        };
        write(root / "a.material", std::string(131072, ' ') + json.c_str());
        write(root / "b.material", json.c_str());
        { auto warm = scanAssetDocuments(catalog); require(warm.succeeded && warm.ready == 2, "Warm complete scan"); }
        auto scan = scanAssetDocuments(catalog);
        require(scan.succeeded && scan.ready == 2 && scan.documents.get_allocator().isSourceBudgeted(),
                "Document preview retains admitted displayed and authorization snapshots");
        const auto firstDigest = scan.documents.front().digest;
        unsigned failedScans = 0, completeScans = 0;
        for(u64 headroom : {0ULL, 512ULL, 2048ULL, 8192ULL, 32768ULL, 65536ULL, 131072ULL,
                            262144ULL, 400000ULL, 500000ULL, 550000ULL, 600000ULL, 700000ULL, 1048576ULL})
        {
            const auto resident = budget.used();
            budget.set(resident + headroom);
            auto failure = scanAssetDocuments(catalog);
            if(failure.succeeded)
            {
                ++completeScans;
                require(failure.ready == 2 && failure.blocked == 0 && failure.documents.size() == 2,
                        "Every admitted threshold publishes the complete document inventory");
            }
            else
            {
                ++failedScans;
                require(failure.documents.empty() && failure.ready == 0 && failure.blocked == 0 &&
                        !failure.error.empty() && scan.documents.front().digest == firstDigest &&
                        budget.used() <= resident + headroom,
                    "Denied scan metadata/read/parse candidates never publish partial or falsely blocked inventories");
            }
            budget.restore();
        }
        require(failedScans >= 8 && completeScans > 0,
                "Threshold sweep exercises both denied scan stages and complete admitted scans");
        // The transaction begins before scan validation. Denial after BEGIN
        // must roll it back without allocating another result wrapper.
        const auto resident = budget.used();
        budget.set(resident + 8192);
        u32 indexed = 99;
        String error;
        require(!reindexAssetDocuments(catalog, scan, indexed, error) && indexed == 0,
                "Denied batch has no published reference graphs");
        budget.restore();
        require(bound->queryBound("BEGIN IMMEDIATE", {}) != nullptr &&
                    bound->queryBound("ROLLBACK", {}) != nullptr,
                "Budget denial leaves the catalog connection outside its failed transaction");
        auto retry = scanAssetDocuments(catalog);
        require(retry.succeeded && retry.ready == 2 &&
                    reindexAssetDocuments(catalog, retry, indexed, error) && indexed == 2,
                "Complete document scan and batch publication retry after capacity is restored");
        auto references = queryAssetReferences(catalog, "a.material", false);
        require(references.succeeded && references.edges.get_allocator().isSourceBudgeted(),
                "Reference pages retain the same caller-container admission policy");
        require(bound->queryBound("CREATE TEMP TABLE snapshot_rollback(value INTEGER); BEGIN IMMEDIATE; INSERT INTO snapshot_rollback VALUES(1)", {}) != nullptr,
                "Stage rollback at full admission capacity");
        auto cleanup = dynamic_cast<ITransactionCleanupDatabase *>(catalog->getDatabase().get());
        const auto beforeRollback = budget.used();
        const auto denials = sourceBudgetDenialCount();
        budget.set(beforeRollback);
        require(cleanup && cleanup->rollbackTransaction() && budget.used() == beforeRollback &&
                    sourceBudgetDenialCount() == denials,
                "Native transaction cleanup neither admits nor rejects new engine storage at capacity");
        budget.restore();
        auto rolledBack = bound->queryBound("SELECT count(*) AS n FROM snapshot_rollback", {});
        require(rolledBack && rolledBack->getFieldValueAsInt("n") == 0,
                "Full-budget rollback actually discards the staged catalog mutation");

        const auto mediaRoot = root / "media";
        fs::create_directories(mediaRoot);
        write(mediaRoot / "one.png", "one");
        const String mediaPath = (mediaRoot / ".workphone/media-assets.db").u8string().c_str();
        require(buildMediaAssetCatalog(mediaRoot.u8string().c_str(), mediaPath, error), "Build initial media inventory");
        write(mediaRoot / "two.png", "two");
        budget.set(budget.used());
        require(!buildMediaAssetCatalog(mediaRoot.u8string().c_str(), mediaPath, error),
                "Denied media scan cannot publish an incomplete inventory");
        budget.restore();
        auto media = openMediaAssetCatalog(mediaRoot.u8string().c_str(), error, mediaPath);
        require(media != nullptr, "Previously published media catalog remains readable");
        auto oldMedia = media->queryEntries({});
        require(oldMedia.succeeded && oldMedia.entries.size() == 1,
                "Denied media rebuild leaves the previously published catalog intact");
        media->unload(nullptr);
        require(buildMediaAssetCatalog(mediaRoot.u8string().c_str(), mediaPath, error),
                "Media scan retries complete inventory publication");
        // Detached reports survive producer shutdown; copies and release keep
        // their physical reservations, independently of the catalog session.
        auto retained = scan;
        const auto held = budget.used();
        catalog->unload(nullptr);
        require(page.entries.size() == 24 && copiedPage.entries.front().uuid == "snapshot-100" &&
                    retained.documents.front().digest == firstDigest,
                "Caller snapshots retain complete data after the producing catalog closes");
        scan = {};
        require(retained.documents.size() == 2 && budget.used() < held,
                "Independent displayed copies release their buffers while shared authorization remains alive");
        std::cout << "PASS: catalog/document snapshot budgets, scan denial, rollback, retry and retained caller ownership\n";
    }
}
