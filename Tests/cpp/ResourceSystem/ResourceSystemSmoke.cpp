#include "../ResourceRuntimeContracts.hpp"
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>

namespace
{
    namespace fs = std::filesystem;
    using namespace workphone;
    using namespace workphone::resource;

    class FaultDatabase final : public IResourceCompilationDatabase
    {
    public:
        ResourceCompilationDatabase inner;
        bool rejectCommit=false;
        size_t batchCommits=0;
        size_t lastBatchSize=0;
        bool connect(const String &path) override { return inner.connect(path); }
        void disconnect() override { inner.disconnect(); }
        bool isConnected() const override { return inner.isConnected(); }
        String getLastError() const override { return rejectCommit?"Injected index commit failure":inner.getLastError(); }
        bool reset() override { return inner.reset(); }
        bool getRecord(const String &id,CompiledResourceRecord &record) const override { return inner.getRecord(id,record); }
        bool commitCompilation(const CompiledResourceRecord &record,const Array<CompileDependencyRecord> &edges) override
        { return !rejectCommit && inner.commitCompilation(record,edges); }
        bool commitCompilations(const Array<CompilationCommit> &compilations) override
        {
            ++batchCommits;
            lastBatchSize=compilations.size();
            return !rejectCommit && inner.commitCompilations(compilations);
        }
        bool removeRecord(const String &id) override { return inner.removeRecord(id); }
        bool getDependencies(const String &id,Array<CompileDependencyRecord> &edges) const override
        { return inner.getDependencies(id,edges); }
        bool getDependents(const String &path,Array<String> &ids) const override { return inner.getDependents(path,ids); }
        bool getRecord(const String &id,const CompilationVariant &variant,CompiledResourceRecord &record) const override
        { return inner.getRecord(id,variant,record); }
        bool commitCompilation(const CompiledResourceRecord &record,const CompilationVariant &variant,
                               const Array<CompileDependencyRecord> &edges) override
        { return !rejectCommit && inner.commitCompilation(record,variant,edges); }
        bool removeRecord(const String &id,const CompilationVariant &variant) override
        { return inner.removeRecord(id,variant); }
        bool getDependencies(const String &id,const CompilationVariant &variant,Array<CompileDependencyRecord> &edges) const override
        { return inner.getDependencies(id,variant,edges); }
        bool getDependents(const String &path,const CompilationVariant &variant,Array<String> &ids) const override
        { return inner.getDependents(path,variant,ids); }
    };

    void require( bool condition, const std::string &message )
    {
        if( !condition )
            throw std::runtime_error( message );
    }

    void writeFile( const fs::path &path, const std::string &contents )
    {
        std::ofstream stream( path, std::ios::binary | std::ios::trunc );
        stream.write( contents.data(), static_cast<std::streamsize>( contents.size() ) );
        require( stream.good(), "Failed to write test input" );
    }

    void containerPinContracts(const fs::path &root)
    {
        const auto payload=root / "pins.payload", container=root / "pins.wprs";
        writeFile(payload,"container pin payload\n");
        CompiledResourceHeader header;
        header.formatVersion=2;
        header.resourceId=ResourceID("data://pins.txtres");
        header.resourceType=header.resourceId.type();
        header.compilerVersion=1;
        header.sourceHash=2;
        String error;
        require(CompiledResourceIO::hashFile(payload.u8string().c_str(),header.payloadHash,header.payloadSize,error),error.c_str());
        header.installDependencies={ResourceID("data://child.txtres")};
        header.installArtifacts={{"artifacts/v2/test/editor/child/123.wprs",3,4}};
        require(CompiledResourceIO::writeAtomic(container.u8string().c_str(),header,payload.u8string().c_str(),error),error.c_str());
        CompiledResourceHeader read;
        require(CompiledResourceIO::validate(container.u8string().c_str(),read,error) && read.formatVersion==2 &&
                read.installArtifacts==header.installArtifacts,"V2 dependency pins must round trip");
        const auto good=resource_runtime_contracts::read(container);
        auto damaged=good;
        const auto offset=damaged.find("artifacts/v2/");
        require(offset!=std::string::npos,"Find encoded artifact pin");
        damaged.replace(offset,3,"../");
        writeFile(container,damaged);
        require(!CompiledResourceIO::readHeader(container.u8string().c_str(),read,error),"Reader accepted a traversal dependency pin");
        writeFile(container,good.substr(0,offset+5));
        require(!CompiledResourceIO::readHeader(container.u8string().c_str(),read,error),"Reader accepted a truncated dependency pin");
        damaged=good;
        damaged[4]=3;
        writeFile(container,damaged);
        require(!CompiledResourceIO::readHeader(container.u8string().c_str(),read,error),"Reader accepted a future container format");
        header.installArtifacts[0].path="../escape.wprs";
        require(!CompiledResourceIO::writeAtomic(container.u8string().c_str(),header,payload.u8string().c_str(),error),
                "Writer accepted a traversal dependency pin");
        header.installArtifacts.clear();
        require(!header.isValid(),"V2 dependencies require complete artifact pins");
        header.formatVersion=1;
        require(CompiledResourceIO::writeAtomic(container.u8string().c_str(),header,payload.u8string().c_str(),error) &&
                CompiledResourceIO::validate(container.u8string().c_str(),read,error) && read.formatVersion==1 &&
                read.installArtifacts.empty(),"Legacy container compatibility must remain intact");
    }

    void variantDatabaseContracts(const fs::path &path)
    {
        ResourceCompilationDatabase database;
        require(database.connect(path.u8string().c_str()),"Open variant database");
        const CompilationVariant editor{"windows",false}, package{"windows",true}, linux{"linux",false};
        CompiledResourceRecord record;
        record.resourceId="data://variant.txtres";
        record.resourceType="txtres";
        record.compilerVersion=1;
        record.sourceHash=2;
        record.outputHash=3;
        record.outputPath="editor.wprs";
        require(database.commitCompilation(record,editor,{{"data://editor-input.bin",false}}),"Commit Editor variant");
        record.outputPath="package.wprs";
        require(database.commitCompilation(record,package,{{"data://package-input.bin",false}}),"Commit package variant");
        record.outputPath="linux.wprs";
        require(database.commitCompilation(record,linux,{{"data://linux-input.bin",false}}),"Commit Linux variant");
        record.outputPath="failed.wprs";
        require(!database.commitCompilation(record,package,{{"data://duplicate.bin",false},{"data://duplicate.bin",true}}),
                "Duplicate dependency must roll back record and edge transaction");
        database.disconnect();
        require(database.connect(path.u8string().c_str()),"Reopen variant database");
        CompiledResourceRecord read;
        require(database.getRecord(record.resourceId,package,read) && read.outputPath=="package.wprs",
                "Failed transaction must retain package record after reopening");
        require(database.getRecord(record.resourceId,editor,read) && read.outputPath=="editor.wprs",
                "Independent Editor record must persist");
        Array<CompileDependencyRecord> edges;
        require(database.getDependencies(record.resourceId,package,edges) && edges.size()==1 &&
                edges[0].path=="data://package-input.bin", "Rollback must retain exact package dependency edges");
        Array<String> ids;
        require(database.getDependents("data://editor-input.bin",package,ids) && ids.empty(),
                "Reverse queries must not cross build modes");
        require(database.getDependents("data://editor-input.bin",linux,ids) && ids.empty(),
                "Reverse queries must not cross targets");
        require(database.getDependents("data://editor-input.bin",editor,ids) && ids.size()==1,
                "Reverse query must find the selected variant");
        require(database.removeRecord(record.resourceId,editor),"Remove selected variant");
        require(database.getRecord(record.resourceId,editor,read) && !read.isValid(),"Selected variant must be removed");
        require(database.getDependents("data://editor-input.bin",editor,ids) && ids.empty(),
                "Variant deletion must cascade its edges");
        require(database.getRecord(record.resourceId,package,read) && read.outputPath=="package.wprs",
                "Variant deletion must preserve package record");
        require(!database.getRecord(record.resourceId,read) && !read.isValid(),"Legacy ambiguous read must fail closed");
        require(database.removeRecord(record.resourceId),"Remove all logical resource variants");
        require(database.getRecord(record.resourceId,linux,read) && !read.isValid(),"Legacy removal must remove all variants");
    }

    std::string reportMessage( const CompilationReport &report )
    {
        std::string message = "status=" + std::to_string( static_cast<int>( report.status ) );
        for( const auto &entry : report.messages )
            message += std::string( "; " ) + entry.c_str();
        return message;
    }

    void batchDatabaseContracts(const fs::path &path)
    {
        ResourceCompilationDatabase database;
        require(database.connect(path.u8string().c_str()),"Open batch database");
        const CompilationVariant variant{"windows",false};
        CompiledResourceRecord first;
        first.resourceId="data://first.txtres";
        first.resourceType="txtres";
        first.outputPath="first-old.wprs";
        first.compilerVersion=1;
        first.sourceHash=2;
        first.outputHash=3;
        auto second=first;
        second.resourceId="data://second.txtres";
        second.outputPath="second-old.wprs";
        require(database.commitCompilations({{first,variant,{{"data://first-old.bin",false}}},
                                            {second,variant,{{"data://second-old.bin",false}}}}),"Commit baseline batch");
        first.outputPath="first-new.wprs";
        second.outputPath="second-new.wprs";
        require(!database.commitCompilations({{first,variant,{{"data://first-new.bin",false}}},
            {second,variant,{{"data://duplicate.bin",false},{"data://duplicate.bin",true}}}}),
            "Failure in the last member must abort the entire transaction");
        database.disconnect();
        require(database.connect(path.u8string().c_str()),"Reopen rolled-back batch");
        CompiledResourceRecord read;
        Array<CompileDependencyRecord> edges;
        require(database.getRecord(first.resourceId,variant,read) && read.outputPath=="first-old.wprs" &&
            database.getDependencies(first.resourceId,variant,edges) && edges.size()==1 && edges[0].path=="data://first-old.bin",
            "Rollback must retain the first member's record and dependency edges after reopen");
        require(database.getRecord(second.resourceId,variant,read) && read.outputPath=="second-old.wprs",
            "Rollback must retain the last member's record");
        require(!database.commitCompilations({{first,variant,{}},{first,variant,{}}}),
            "Duplicate variants within a batch must be rejected");
        require(database.commitCompilations({{first,variant,{}},{second,variant,{}}}),"Publish complete replacement batch");
        require(database.getRecord(first.resourceId,variant,read) && read.outputPath==first.outputPath &&
            database.getRecord(second.resourceId,variant,read) && read.outputPath==second.outputPath,
            "Successful batch must publish all members");
    }

    class TextCompiler final : public IResourceCompiler
    {
    public:
        String name() const override
        {
            return "SmokeTextCompiler";
        }

        Array<CompilerOutput> outputs() const override
        {
            return { { ResourceTypeID( "txtres" ), 11 } };
        }

        bool getDependencies( const CompileContext &context, DependencySet &dependencies,
                              String &error ) const override
        {
            if(context.resourceId.str()=="data://mode-root.txtres")
                dependencies.compileDependencies.push_back(CompileDependency::data(
                    context.packagedBuild ? "data://package-only.bin" : "data://editor-only.bin"));
            std::ifstream input( context.sourcePath.c_str() );
            if( !input )
            {
                error = "Unable to scan descriptor";
                return false;
            }

            std::string line;
            while( std::getline( input, line ) )
            {
                if( line.rfind( "resource=", 0 ) == 0 )
                {
                    ResourceID id( line.substr( 9 ).c_str() );
                    if( !id.isValid() )
                    {
                        error = "Invalid resource dependency";
                        return false;
                    }
                    dependencies.compileDependencies.push_back( CompileDependency::resource( id ) );
                }
                else if( line.rfind( "install=", 0 ) == 0 )
                {
                    ResourceID id( line.substr( 8 ).c_str() );
                    if( !id.isValid() )
                    {
                        error = "Invalid install dependency";
                        return false;
                    }
                    dependencies.installDependencies.push_back( id );
                }
                else if( line.rfind( "data=", 0 ) == 0 )
                {
                    dependencies.compileDependencies.push_back(
                        CompileDependency::data( line.substr( 5 ).c_str() ) );
                }
            }
            return true;
        }

        CompilationStatus compile( const CompileContext &context, std::ostream &output,
                                   Array<String> & ) const override
        {
            std::ifstream input( context.sourcePath.c_str(), std::ios::binary );
            if( !input )
                return CompilationStatus::Failure;
            const std::string contents((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
            if(contents.find("fail-compile")!=std::string::npos)
                return CompilationStatus::Failure;
            if(context.resourceId.str().find("data://mode-")==0)
                output << (context.packagedBuild ? "package\n" : "editor\n");
            output << contents;
            input.close();
            if(context.resourceId.str()=="data://mutating.txtres")
                writeFile(fs::u8path(context.sourcePath.c_str()),"changed-during-compile\n");
            return output ? CompilationStatus::Success : CompilationStatus::Failure;
        }
    };
}  // namespace

int main(int argc, char **argv)
{
    using namespace workphone;
    using namespace workphone::resource;

    fs::path root;
    try
    {
        // Used by ResourceCompilationMigrationContracts.py to exercise the native
        // migration against independently generated old/new SQLite schemas.
        if(argc==3 && std::string(argv[1])=="--open-index")
        {
            ResourceCompilationDatabase database;
            const bool connected=database.connect(argv[2]);
            require(connected,database.getLastError().c_str());
            return 0;
        }
        if(argc>=5 && (std::string(argv[1])=="--hold-writer" || std::string(argv[1])=="--probe-writer"))
        {
            const bool hold=std::string(argv[1])=="--hold-writer";
            require(!hold || argc==6,"Holding a writer requires a readiness path");
            ResourceSystem writer(std::make_shared<ResourceCompilerRegistry>(),std::make_shared<ResourceCompilationDatabase>());
            ResourceSystemConfig configuration;
            configuration.sourceRoot=argv[2];
            configuration.compiledRoot=argv[3];
            configuration.target=argv[4];
            if(!hold && argc==6) configuration.databasePath=argv[5];
            String writerError;
            const bool initialized=writer.initialize(configuration,writerError);
            require(initialized,writerError.c_str());
            if(hold)
            {
                writeFile(fs::u8path(argv[5]),"ready");
                std::cin.get();
            }
            return 0;
        }
        const ResourceID canonicalId( "DATA://models\\car.mesh:lod0.mesh" );
        require( canonicalId.compiledRelativePath() == "models/car_lod0.mesh",
                 std::string( "Resource ID canonicalization failed: " ) + canonicalId.str().c_str() +
                     " => " + canonicalId.compiledRelativePath().c_str() );
        require( !ResourceID( "data://../escape.mesh" ).isValid(),
                 "Traversal resource ID was accepted" );

        const auto unique = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        root = fs::temp_directory_path() /
               ( std::string( "lioncat_wpresource_smoke_" ) + std::to_string( unique ) );
        const fs::path source = root / "source";
        const fs::path compiled = root / "compiled";
        fs::create_directories( source / "data" );
        fs::create_directories( compiled );
        variantDatabaseContracts(root / "variant-contracts.sqlite");
        batchDatabaseContracts(root / "batch-contracts.sqlite");
        containerPinContracts(root);

        writeFile( source / "dependency.txtres",
                   "data=data://data/shared.txt\n"
                   "dependency payload\n" );
        writeFile( source / "data" / "shared.txt", "shared-v1\n" );
        writeFile( source / "root.txtres",
                   "resource=data://dependency.txtres\n"
                   "install=data://dependency.txtres\n"
                   "root payload\n" );

        std::shared_ptr<IResourceCompilerRegistry> registry =
            std::make_shared<ResourceCompilerRegistry>();
        String error;
        require( registry->registerCompiler( std::make_shared<TextCompiler>(), &error ), error.c_str() );
        require( !registry->registerCompiler( std::make_shared<TextCompiler>(), &error ),
                 "Duplicate compiler registration was accepted" );

        auto database = std::make_shared<FaultDatabase>();
        ResourceSystem system( registry, database );
        ResourceSystemConfig config;
        config.sourceRoot = source.u8string().c_str();
        config.compiledRoot = compiled.u8string().c_str();
        config.target = "smoke";
        error.clear();
        require( system.initialize( config, error ), error.c_str() );

        {
            ResourceSystem contender(registry,std::make_shared<ResourceCompilationDatabase>());
            require(!contender.initialize(config,error) && error.find("writer lease")!=String::npos,
                "A second same-target authoring session must be rejected before opening its database");
        }

        const ResourceID rootId( "data://root.txtres" );
        const auto first = system.compile( rootId );
        require( first.status == CompilationStatus::Success,
                 std::string( "Initial compilation failed: " ) + reportMessage( first ) );
        require( fs::is_regular_file( fs::u8path(system.compile(ResourceID("data://dependency.txtres")).outputPath.c_str()) ),
                 "Resource dependency was not compiled" );
        require( fs::is_regular_file( fs::u8path(first.outputPath.c_str()) ), "Root resource was not compiled" );

        const auto second = system.compile( rootId );
        require( second.status == CompilationStatus::UpToDate,
                 "Unchanged resource was not detected as up to date" );

        error.clear();
        auto loaded = system.load( rootId, error );
        require( loaded != nullptr, error.c_str() );
        require( loaded->dependencies.size() == 1,
                 "Install dependency was not loaded with the root resource" );

        writeFile( source / "data" / "shared.txt", "shared-v2\n" );
        const auto affected = system.compileAffected( "data://data/shared.txt" );
        require( affected.size() == 2, "Transitive affected-resource set was incomplete" );
        const auto rootRebuild = std::find_if(
            affected.begin(), affected.end(),
            [&]( const CompilationReport &report ) { return report.resourceId == rootId; } );
        require( rootRebuild != affected.end() && rootRebuild->status == CompilationStatus::Success,
                 "Changed raw dependency did not rebuild its transitive dependent" );
        require( rootRebuild->sourceHash != first.sourceHash,
                 "Changed transitive dependency did not alter the root source hash" );

        system.unload( rootId );
        std::fstream corrupt( fs::u8path(rootRebuild->outputPath.c_str()),
                              std::ios::binary | std::ios::in | std::ios::out );
        require( static_cast<bool>( corrupt ), "Failed to open output for corruption test" );
        corrupt.seekp( -1, std::ios::end );
        const char badByte = 0;
        corrupt.write( &badByte, 1 );
        corrupt.close();
        error.clear();
        require( !system.load( rootId, error ), "Corrupt payload was accepted" );

        const auto repaired = system.compile( rootId );
        require( repaired.status == CompilationStatus::Success,
                 "Corrupt compiled output was not automatically rebuilt" );
        error.clear();
        require( system.load( rootId, error ) != nullptr, error.c_str() );

        writeFile(source / "atomic-child.txtres","child-old\n");
        writeFile(source / "atomic-root.txtres","install=data://atomic-child.txtres\nroot-old\n");
        const ResourceID atomicRoot("data://atomic-root.txtres"), atomicChild("data://atomic-child.txtres");
        const CompilationVariant atomicVariant{config.target,false};
        const auto commitsBefore=database->batchCommits;
        require(system.compile(atomicRoot).succeeded() && database->batchCommits==commitsBefore+1 &&
            database->lastBatchSize==2,"A parent and child must publish in one batch");
        CompiledResourceRecord originalParent, originalChild, atomicRead;
        require(database->getRecord(atomicRoot.str(),atomicVariant,originalParent) &&
            database->getRecord(atomicChild.str(),atomicVariant,originalChild),"Read original closure records");
        writeFile(source / "atomic-child.txtres","child-new\n");
        writeFile(source / "atomic-root.txtres","install=data://atomic-child.txtres\nfail-compile\n");
        const auto beforeParentFailure=database->batchCommits;
        require(!system.compile(atomicRoot).succeeded() && database->batchCommits==beforeParentFailure,
            "A parent compiler failure must abandon all staged child records");
        require(database->getRecord(atomicChild.str(),atomicVariant,atomicRead) &&
            atomicRead.outputPath==originalChild.outputPath,"A failed parent must retain the last-good child index entry");
        system.clearRuntimeCache();
        auto oldClosure=system.load(atomicRoot,error);
        require(oldClosure && oldClosure->dependencies.size()==1 &&
            std::string(oldClosure->dependencies[0]->payload.begin(),oldClosure->dependencies[0]->payload.end())=="child-old\n",
            "Cache eviction after parent failure must load the entire prior closure");
        writeFile(source / "atomic-root.txtres","install=data://atomic-child.txtres\nroot-new\n");
        database->rejectCommit=true;
        require(!system.compile(atomicRoot).succeeded() && database->lastBatchSize==2,"Inject whole-closure index commit failure");
        database->rejectCommit=false;
        require(database->getRecord(atomicRoot.str(),atomicVariant,atomicRead) && atomicRead.outputPath==originalParent.outputPath &&
            database->getRecord(atomicChild.str(),atomicVariant,atomicRead) && atomicRead.outputPath==originalChild.outputPath,
            "Index commit failure must retain both root and child generations");
        require(!system.writeRuntimeManifest((compiled / "failed-closure.wprm").u8string().c_str(),{atomicRoot},error),
            "An abandoned closure must not retain package publication evidence");
        require(system.compile(atomicRoot).succeeded() && database->lastBatchSize==2,"Publish recovered closure together");
        auto newClosure=system.load(atomicRoot,error);
        require(newClosure && newClosure->dependencies.size()==1 &&
            std::string(newClosure->dependencies[0]->payload.begin(),newClosure->dependencies[0]->payload.end())=="child-new\n" &&
            std::string(oldClosure->dependencies[0]->payload.begin(),oldClosure->dependencies[0]->payload.end())=="child-old\n",
            "Successful replacement must install the new closure while retained objects remain valid");

        // Independently construct a valid-container but incompatible diamond to
        // exercise closure selection, rather than only corrupt payload rejection.
        const ResourceID diamond("data://pin-diamond.txtres"), diamondLeaf("data://pin-leaf.txtres");
        writeFile(source / "pin-leaf.txtres","pin-leaf-old\n");
        writeFile(source / "pin-left.txtres","install=data://pin-leaf.txtres\nleft\n");
        writeFile(source / "pin-right.txtres","install=data://pin-leaf.txtres\nright\n");
        writeFile(source / "pin-diamond.txtres","install=data://pin-left.txtres\ninstall=data://pin-right.txtres\n");
        require(system.compile(diamond).succeeded(),"Cook compatible diamond baseline");
        CompiledResourceRecord rightRecord, latestLeaf;
        require(database->getRecord("data://pin-right.txtres",atomicVariant,rightRecord),"Read diamond branch record");
        const auto rightPath=compiled / fs::u8path(rightRecord.outputPath.c_str());
        const auto goodRight=resource_runtime_contracts::read(rightPath);
        RuntimeResource right;
        require(CompiledResourceIO::read(rightPath.u8string().c_str(),right,error),error.c_str());
        writeFile(source / "pin-leaf.txtres","pin-leaf-new\n");
        require(system.compile(diamondLeaf).succeeded() &&
                database->getRecord(diamondLeaf.str(),atomicVariant,latestLeaf),"Independently advance the diamond leaf");
        right.header.installArtifacts[0]={latestLeaf.outputPath,latestLeaf.sourceHash,latestLeaf.outputHash};
        const auto rightPayload=root / "right.payload";
        writeFile(rightPayload,std::string(right.payload.begin(),right.payload.end()));
        require(CompiledResourceIO::writeAtomic(rightPath.u8string().c_str(),right.header,rightPayload.u8string().c_str(),error),error.c_str());
        system.clearRuntimeCache();
        require(!system.load(diamond,error) && error.find("conflicting artifact generations")!=String::npos,
                "Incompatible diamond dependency generations must fail with a conflict diagnostic");
        writeFile(rightPath,goodRight);
        require(system.load(diamond,error)!=nullptr,"An incompatible closure must not publish partial cache entries");

        writeFile(source / "legacy-parent.txtres","install=data://pin-leaf.txtres\nlegacy-parent\n");
        const ResourceID legacyParent("data://legacy-parent.txtres");
        require(system.compile(legacyParent).succeeded(),"Cook legacy migration fixture");
        CompiledResourceRecord legacyRecord;
        require(database->getRecord(legacyParent.str(),atomicVariant,legacyRecord),"Read legacy migration record");
        RuntimeResource legacyResource;
        require(CompiledResourceIO::read((compiled / fs::u8path(legacyRecord.outputPath.c_str())).u8string().c_str(),
                                        legacyResource,error),error.c_str());
        legacyResource.header.formatVersion=1;
        legacyResource.header.installArtifacts.clear();
        legacyResource.header.sourceHash=123;
        legacyRecord.sourceHash=123;
        legacyRecord.outputPath=legacyRecord.outputPath.substr(0,legacyRecord.outputPath.find_last_of('/')+1) +
                                "legacy-123.wprs";
        const auto legacyPath=compiled / fs::u8path(legacyRecord.outputPath.c_str());
        const auto legacyPayload=root / "legacy-parent.payload";
        writeFile(legacyPayload,std::string(legacyResource.payload.begin(),legacyResource.payload.end()));
        require(CompiledResourceIO::writeAtomic(legacyPath.u8string().c_str(),legacyResource.header,legacyPayload.u8string().c_str(),error) &&
                database->commitCompilation(legacyRecord,atomicVariant,{{diamondLeaf.str(),true}}),"Publish an old unpinned authoring parent");
        const auto legacyBytes=resource_runtime_contracts::read(legacyPath);
        system.clearRuntimeCache();
        require(!system.load(legacyParent,error) && error.find("recook")!=String::npos,
                "Unpinned authoring parents must request a recook rather than resolve the current child");
        require(system.compile(legacyParent).succeeded(),"Recook old authoring parent into the pinned format");
        auto migratedParent=system.load(legacyParent,error);
        require(migratedParent && migratedParent->header.formatVersion==2 &&
                resource_runtime_contracts::read(legacyPath)==legacyBytes,"Migration must retain the old container unchanged");
        writeFile(source / "affected-shared.bin","before\n");
        writeFile(source / "affected-a.txtres","data=data://affected-shared.bin\na-old\n");
        writeFile(source / "affected-b.txtres","data=data://affected-shared.bin\nb-old\n");
        const ResourceID affectedA("data://affected-a.txtres"), affectedB("data://affected-b.txtres");
        require(system.compile(affectedA).succeeded() && system.compile(affectedB).succeeded(),"Build affected-root baselines");
        CompiledResourceRecord previousA, previousB;
        require(database->getRecord(affectedA.str(),atomicVariant,previousA) &&
            database->getRecord(affectedB.str(),atomicVariant,previousB),"Read affected-root baselines");
        writeFile(source / "affected-shared.bin","after\n");
        writeFile(source / "affected-b.txtres","data=data://affected-shared.bin\nfail-compile\n");
        const auto abandoned=system.compileAffected("data://affected-shared.bin");
        require(abandoned.size()==2 && !abandoned[0].succeeded() && !abandoned[1].succeeded(),
            "A failed affected root must report the entire unpublished build set as failed");
        require(database->getRecord(affectedA.str(),atomicVariant,atomicRead) && atomicRead.outputPath==previousA.outputPath &&
            database->getRecord(affectedB.str(),atomicVariant,atomicRead) && atomicRead.outputPath==previousB.outputPath,
            "A multi-root failure must retain every original index entry");
        writeFile(source / "affected-b.txtres","data=data://affected-shared.bin\nb-new\n");
        const auto affectedRecovered=system.compileAffected("data://affected-shared.bin");
        require(affectedRecovered.size()==2 && affectedRecovered[0].succeeded() && affectedRecovered[1].succeeded() &&
            database->lastBatchSize==2,"Publish an affected-root replacement in one transaction");

        writeFile( source / "cycle_a.txtres", "resource=data://cycle_b.txtres\n" );
        writeFile( source / "cycle_b.txtres", "resource=data://cycle_a.txtres\n" );
        const auto cycle = system.compile( ResourceID( "data://cycle_a.txtres" ) );
        require( cycle.status == CompilationStatus::Failure, "Resource dependency cycle was accepted" );

        writeFile( source / "quote's.txtres", "quoted resource payload\n" );
        require( system.compile( ResourceID( "data://quote's.txtres" ) ).succeeded(),
                 "Prepared database statements did not handle a quoted resource ID" );

        fs::create_directories(source / "models");
        writeFile(source / "models" / "car.mesh","subasset payload\n");
        writeFile(source / "models" / "car_lod0.txtres","independent payload\n");
        const auto subasset=system.compile(ResourceID("data://models/car.mesh:lod0.txtres"));
        const auto alias=system.compile(ResourceID("data://models/car_lod0.txtres"));
        require(subasset.succeeded(),"Subasset: "+reportMessage(subasset));
        require(alias.succeeded(),"Alias: "+reportMessage(alias));
        require(subasset.succeeded() && alias.succeeded() && subasset.outputPath!=alias.outputPath &&
                fs::is_regular_file(fs::u8path(subasset.outputPath.c_str())),"Flattened subasset aliases must retain independent artifacts");
        writeFile(source / "case.txtres","case fixture\n");
        writeFile(source / "CASE.txtres","case fixture\n");
        const auto lower=system.compile(ResourceID("data://case.txtres"));
        const auto upper=system.compile(ResourceID("data://CASE.txtres"));
        require(lower.succeeded() && upper.succeeded() &&
                !fs::equivalent(fs::u8path(lower.outputPath.c_str()),fs::u8path(upper.outputPath.c_str())),
                "Case-distinct logical IDs must not alias on Windows artifact storage");
        writeFile(source / "generation.txtres","last-good\n");
        const ResourceID generationId("data://generation.txtres");
        const auto good=system.compile(generationId);
        require(good.succeeded(),"Generation baseline");
        const auto pinnedManifest=compiled / "pinned.wprm";
        require(system.writeRuntimeManifest(pinnedManifest.u8string().c_str(),{generationId},error),error.c_str());
        writeFile(source / "generation.txtres","replacement\n");
        database->rejectCommit=true;
        require(!system.compile(generationId).succeeded(),"Injected database failure must fail build");
        database->rejectCommit=false;
        require(!system.writeRuntimeManifest((compiled / "failed-build.wprm").u8string().c_str(),{generationId},error),
                "Failed recompile must revoke stale manifest publication evidence");
        system.clearRuntimeCache();
        auto preserved=system.load(generationId,error);
        require(preserved && std::string(preserved->payload.begin(),preserved->payload.end())=="last-good\n",
                "Index commit failure must retain last-good disk generation after cache eviction");
        const auto replacement=system.compile(generationId);
        require(replacement.succeeded() && replacement.outputPath!=good.outputPath &&
                fs::is_regular_file(fs::u8path(good.outputPath.c_str())),"Successful replacement must not overwrite prior generation");
        CompilationOptions packaged;
        packaged.packagedBuild=true;
        const auto packageVariant=system.compile(generationId,packaged);
        require(packageVariant.succeeded() && packageVariant.outputPath!=replacement.outputPath,
                "Packaged and Editor variants must coexist on disk");
        auto targetDatabase=std::make_shared<ResourceCompilationDatabase>();
        ResourceSystem otherTarget(registry,targetDatabase);
        auto otherConfig=config;
        otherConfig.target="another-target";
        require(otherTarget.initialize(otherConfig,error),error.c_str());
        const auto targetVariant=otherTarget.compile(generationId);
        require(targetVariant.succeeded() && targetVariant.outputPath!=replacement.outputPath,
                "Targets sharing a cooked root must retain independent artifacts");
        require(system.compile(generationId).status==CompilationStatus::UpToDate,
                "Switching targets/modes must reuse the retained Editor index entry");
        require(system.compile(generationId,packaged).status==CompilationStatus::UpToDate,
                "Switching to package mode must reuse its retained index entry");
        CompiledResourceRecord variantRecord;
        require(!database->getRecord(generationId.str(),variantRecord) && !variantRecord.isValid(),
                "An unqualified read must reject multiple variants");
        require(database->getRecord(generationId.str(),CompilationVariant{config.target,false},variantRecord) &&
                variantRecord.outputPath.size()>0, "Explicit Editor lookup must succeed");
        require(otherTarget.compile(generationId).status==CompilationStatus::UpToDate,
                "Other target index entry must survive builds through a second connection");
        otherTarget.shutdown();
        writeFile(source / "mode-root.txtres","install=data://mode-child.txtres\n");
        writeFile(source / "mode-child.txtres","child payload\n");
        writeFile(source / "editor-only.bin","editor input\n");
        writeFile(source / "package-only.bin","package input\n");
        const ResourceID modeRoot("data://mode-root.txtres");
        require(system.compile(modeRoot).succeeded(),"Compile Editor mode closure");
        auto editorLoaded=system.load(modeRoot,error);
        require(editorLoaded && editorLoaded->dependencies.size()==1,"Load Editor mode closure");
        require(system.compile(modeRoot,packaged).succeeded(),"Compile package mode closure");
        auto packageLoaded=system.load(modeRoot,error);
        require(packageLoaded && packageLoaded!=editorLoaded && packageLoaded->dependencies.size()==1 &&
                packageLoaded->dependencies[0]!=editorLoaded->dependencies[0],
                "Retained Editor instances must not alias package root or child instances");
        require(std::string(packageLoaded->dependencies[0]->payload.begin(),packageLoaded->dependencies[0]->payload.end())==
                "package\nchild payload\n","Entire install closure must use the selected mode");
        database->rejectCommit=true;
        CompilationOptions forceEditor;
        forceEditor.force=true;
        require(!system.compile(modeRoot,forceEditor).succeeded(),"Injected Editor-mode build failure");
        database->rejectCommit=false;
        system.clearRuntimeCache();
        auto afterFailedSwitch=system.load(modeRoot,error);
        require(afterFailedSwitch && std::string(afterFailedSwitch->payload.begin(),afterFailedSwitch->payload.end()).find("package\n")==0,
                "Failed build must preserve the previously selected package mode");
        require(system.compile(modeRoot).status==CompilationStatus::UpToDate,"Restore Editor mode without rebuilding");
        auto editorAgain=system.load(modeRoot,error);
        require(editorAgain && std::string(editorAgain->dependencies[0]->payload.begin(),editorAgain->dependencies[0]->payload.end())==
                "editor\nchild payload\n","Mode switch must not reuse package cache entries");
        CompiledResourceRecord editorRecord, packageRecord;
        Array<CompileDependencyRecord> editorEdges;
        const CompilationVariant editorMode{config.target,false}, packageMode{config.target,true};
        require(database->getRecord(modeRoot.str(),editorMode,editorRecord) &&
                database->getRecord(modeRoot.str(),packageMode,packageRecord) &&
                database->getDependencies(modeRoot.str(),editorMode,editorEdges),"Read mode fixture records");
        require(database->commitCompilation(packageRecord,editorMode,editorEdges),"Inject wrong-mode artifact mapping");
        system.clearRuntimeCache();
        require(!system.load(modeRoot,error),"Authoring load must reject an artifact mapped to the wrong mode");
        auto mismatched=editorRecord;
        mismatched.outputHash ^= 1;
        require(database->commitCompilation(mismatched,editorMode,editorEdges),"Inject mismatched committed fingerprint");
        require(!system.load(modeRoot,error),"Authoring load must verify the selected committed fingerprint");
        require(database->commitCompilation(editorRecord,editorMode,editorEdges),"Restore Editor fixture record");
        writeFile(source / "editor-only.bin","changed editor input\n");
        require(system.compileAffected("data://editor-only.bin",packaged).empty(),
                "Package affected builds must ignore Editor-only dependencies");
        const auto editorAffected=system.compileAffected("data://editor-only.bin");
        require(editorAffected.size()==1 && editorAffected[0].succeeded(),
                "Editor affected builds must rebuild their own dependency graph");
        require(system.compile(modeRoot,packaged).status==CompilationStatus::UpToDate,
                "Editor dependency change must preserve package build record");
        const auto modeManifest=compiled / "mode-package.wprm";
        require(system.writeRuntimeManifest(modeManifest.u8string().c_str(),{modeRoot},error),error.c_str());
        ResourceSystem modeRuntime(nullptr,nullptr);
        RuntimeResourceConfig modeConfig;
        modeConfig.compiledRoot=config.compiledRoot;
        modeConfig.manifestPath=modeManifest.u8string().c_str();
        modeConfig.target=config.target;
        require(modeRuntime.initializeRuntime(modeConfig,error),error.c_str());
        auto exportedPackage=modeRuntime.load(modeRoot,error);
        require(exportedPackage && exportedPackage->dependencies.size()==1 &&
                std::string(exportedPackage->dependencies[0]->payload.begin(),exportedPackage->dependencies[0]->payload.end())==
                "package\nchild payload\n","Manifest export must pin the selected package-mode closure");
        modeRuntime.shutdown();
        ResourceSystem pinnedRuntime(nullptr,nullptr);
        RuntimeResourceConfig runtimeConfig;
        runtimeConfig.compiledRoot=config.compiledRoot;
        runtimeConfig.manifestPath=pinnedManifest.u8string().c_str();
        runtimeConfig.target=config.target;
        require(pinnedRuntime.initializeRuntime(runtimeConfig,error),error.c_str());
        auto pinned=pinnedRuntime.load(generationId,error);
        require(pinned && std::string(pinned->payload.begin(),pinned->payload.end())=="last-good\n",
                "An existing manifest must retain its exact generation across builds, modes and targets");
        pinnedRuntime.shutdown();
        writeFile(source / "mutating.txtres","original\n");
        require(!system.compile(ResourceID("data://mutating.txtres")).succeeded(),
                "Compiler-time source changes must reject publication");
        CompiledResourceRecord uncommitted;
        require(database->getRecord("data://mutating.txtres",uncommitted) && !uncommitted.isValid(),
                "Changed-input compilation must not create a committed index record");

        system.shutdown();
        error.clear();
        require( system.initialize( config, error ), error.c_str() );
        require( system.compile( rootId ).status == CompilationStatus::UpToDate,
                 "Persistent compilation metadata was not reused after restart" );
        require(system.compile(generationId,packaged).status==CompilationStatus::UpToDate,
                "Package variant must persist across authoring restart");
        require(system.compile(generationId).status==CompilationStatus::UpToDate,
                "Editor variant must persist across authoring restart");

        system.shutdown();
        std::error_code ignored;
        fs::remove_all( root, ignored );
        resource_runtime_contracts::run();
        std::cout << "WPResource authoring and cooked-only runtime contracts passed" << std::endl;
        return 0;
    }
    catch( const std::exception &exception )
    {
        std::error_code ignored;
        if( !root.empty() )
            fs::remove_all( root, ignored );
        std::cerr << "WPResource smoke test failure: " << exception.what() << std::endl;
        return 1;
    }
}
