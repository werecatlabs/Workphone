#include "../ResourceRuntimeContracts.hpp"
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourcePayloadStream.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <thread>
#if WP_RESOURCE_AUDIO_CONTRACTS
#include <WPAudio/AudioResourceCompiler.hpp>
#endif
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

    class SnapshotCompiler final : public IResourceCompiler
    {
    public:
        enum class Mutation { None, DescriptorABA, RawABA, PersistentRaw, OptionalAppears, EarlierRoot };
        explicit SnapshotCompiler(fs::path source) : authored(std::move(source)) {}
        mutable Mutation mutation=Mutation::None;
        mutable Array<fs::path> roots;
        String name() const override { return "SnapshotContracts"; }
        Array<CompilerOutput> outputs() const override { return {{ResourceTypeID("snapres"),1}}; }
        bool getDependencies(const CompileContext &context,DependencySet &dependencies,String &error) const override
        {
            require(static_cast<bool>(context.snapshotInput),"Dependency scanning requires the capture hook");
            require(fs::u8path(context.sourcePath.c_str())!=authored / context.resourceId.sourceRelativePath().c_str(),
                    "Scanner descriptor must be a captured file");
            String optional;
            if(!context.snapshotInput("data://snapshot.optional",false,optional,error)) return false;
            dependencies.compileDependencies.push_back(CompileDependency::data("data://snapshot.raw"));
            return true;
        }
        CompilationStatus compile(const CompileContext &context,std::ostream &output,Array<String> &) const override
        {
            require(!context.snapshotInput,"Compilation must not capture undeclared inputs");
            const auto root=fs::u8path(context.sourceRoot.c_str());
            roots.push_back(root);
            require(root!=authored && fs::u8path(context.sourcePath.c_str())==root / context.resourceId.sourceRelativePath().c_str(),
                    "Compile context must identify the isolated input tree");
            const auto capturedSource=resource_runtime_contracts::read(fs::u8path(context.sourcePath.c_str()));
            const auto capturedRaw=resource_runtime_contracts::read(root / "snapshot.raw");
            if(mutation==Mutation::DescriptorABA)
            {
                const auto source=authored / context.resourceId.sourceRelativePath().c_str();
                const auto original=resource_runtime_contracts::read(source);
                writeFile(source,"transient descriptor bytes\n");
                require(resource_runtime_contracts::read(fs::u8path(context.sourcePath.c_str()))==capturedSource,
                        "Live descriptor ABA must not affect compiler input bytes");
                writeFile(source,original);
            }
            if(mutation==Mutation::RawABA || mutation==Mutation::PersistentRaw)
            {
                const auto original=resource_runtime_contracts::read(authored / "snapshot.raw");
                writeFile(authored / "snapshot.raw","transient raw bytes\n");
                require(resource_runtime_contracts::read(root / "snapshot.raw")==capturedRaw,
                        "Live raw-input edits must not affect compiler input bytes");
                if(mutation==Mutation::RawABA) writeFile(authored / "snapshot.raw",original);
            }
            if(mutation==Mutation::OptionalAppears) writeFile(authored / "snapshot.optional","appeared\n");
            if(mutation==Mutation::EarlierRoot && context.resourceId.str()=="data://snapshot-b.snapres")
                writeFile(authored / "snapshot-a.snapres","changed after its compiler completed\n");
            output << capturedSource << capturedRaw;
            if(fs::exists(root / "snapshot.optional"))
                output << resource_runtime_contracts::read(root / "snapshot.optional");
            return output ? CompilationStatus::Success : CompilationStatus::Failure;
        }
        fs::path authored;
    };

    void snapshotContracts(ResourceSystem &system, const std::shared_ptr<FaultDatabase> &database,
                           const std::shared_ptr<SnapshotCompiler> &compiler, const fs::path &source,
                           const CompilationVariant &variant)
    {
        const ResourceID first("data://snapshot-a.snapres"), second("data://snapshot-b.snapres");
        writeFile(source / "snapshot-a.snapres","snapshot-a\n");
        writeFile(source / "snapshot-b.snapres","snapshot-b\n");
        writeFile(source / "snapshot.raw","raw-original\n");
        String error;
        CompilationOptions forced;
        forced.force=true;
        require(system.compile(first).succeeded() && system.compile(second).succeeded(),"Cook snapshot baselines");
        auto original=system.load(first,error);
        require(original && std::string(original->payload.begin(),original->payload.end())=="snapshot-a\nraw-original\n",
                "Captured descriptor/raw bytes must become the cooked payload");
        Array<String> dependents;
        require(database->getDependents("data://snapshot.optional",variant,dependents) && dependents.size()==2,
                "Absent auxiliary scan inputs must persist reverse dependency edges");
        for(const auto action:{SnapshotCompiler::Mutation::DescriptorABA,SnapshotCompiler::Mutation::RawABA})
        {
            compiler->mutation=action;
            require(system.compile(first,forced).succeeded(),"Restored live-input edits must leave captured bytes valid");
            auto loaded=system.load(first,error);
            require(loaded && loaded->payload==original->payload,"ABA edits must not stamp transient bytes with original fingerprints");
        }
        CompiledResourceRecord previous,read;
        require(database->getRecord(first.str(),variant,previous),"Read snapshot baseline record");
        compiler->mutation=SnapshotCompiler::Mutation::PersistentRaw;
        const auto before=database->batchCommits;
        require(!system.compile(first,forced).succeeded() && database->batchCommits==before &&
                database->getRecord(first.str(),variant,read) && read.outputPath==previous.outputPath,
                "Persistent raw edits must abandon publication and retain the previous index record");
        writeFile(source / "snapshot.raw","raw-original\n");
        compiler->mutation=SnapshotCompiler::Mutation::OptionalAppears;
        require(!system.compile(first,forced).succeeded(),"Optional input appearance must reject an obsolete absent-input build");
        compiler->mutation=SnapshotCompiler::Mutation::None;
        compiler->roots.clear();
        const auto affected=system.compileAffected("data://snapshot.optional");
        require(affected.size()==2 && affected[0].succeeded() && affected[1].succeeded() &&
                compiler->roots.size()==2 && compiler->roots[0]==compiler->roots[1],
                "Optional input creation must rebuild both dependents using one shared request snapshot");
        auto present=system.load(first,error);
        require(present && std::string(present->payload.begin(),present->payload.end())==
                "snapshot-a\nraw-original\nappeared\n","Newly present optional bytes must affect the cooked payload");
        fs::remove(source / "snapshot.optional");
        const auto removed=system.compileAffected("data://snapshot.optional");
        require(removed.size()==2 && removed[0].succeeded() && removed[1].succeeded(),
                "Optional input deletion must invalidate its previously present dependents");
        require(database->getRecord(first.str(),variant,previous),"Read absent optional-input fingerprint");
        writeFile(source / "snapshot.optional","");
        const auto emptyPresent=system.compileAffected("data://snapshot.optional");
        require(emptyPresent.size()==2 && emptyPresent[0].succeeded() && emptyPresent[1].succeeded() &&
                database->getRecord(first.str(),variant,read) && read.sourceHash!=previous.sourceHash,
                "An empty optional file must have distinct identity from an absent optional file");
        fs::remove(source / "snapshot.optional");
        const auto emptyRemoved=system.compileAffected("data://snapshot.optional");
        require(emptyRemoved.size()==2 && emptyRemoved[0].succeeded() && emptyRemoved[1].succeeded(),
                "Recover absent optional-input identity after removing an empty file");
        CompiledResourceRecord previousSecond;
        require(database->getRecord(first.str(),variant,previous) && database->getRecord(second.str(),variant,previousSecond),
                "Read pre-failure shared snapshot records");
        writeFile(source / "snapshot.raw","new shared bytes\n");
        compiler->mutation=SnapshotCompiler::Mutation::EarlierRoot;
        const auto failed=system.compileAffected("data://snapshot.raw");
        require(failed.size()==2 && !failed[0].succeeded() && !failed[1].succeeded() &&
                database->getRecord(first.str(),variant,read) && read.outputPath==previous.outputPath &&
                database->getRecord(second.str(),variant,read) && read.outputPath==previousSecond.outputPath,
                "Final request verification must reject edits to a root that compiled before another root");
        compiler->mutation=SnapshotCompiler::Mutation::None;
        writeFile(source / "snapshot-a.snapres","snapshot-a\n");
        require(system.compile(first).succeeded(),"Recover after abandoned snapshot publication");
        for(const auto &root:compiler->roots)
            require(!fs::exists(root),"Successful and failed builds must clean up their private snapshots");
    }
    void audioContracts(const fs::path &root)
    {
#if WP_RESOURCE_AUDIO_CONTRACTS
        fs::create_directories(root/"source");
        std::string wave="RIFF"; wave.append("\x28\0\0\0",4); wave+="WAVEfmt ";
        wave.append("\x10\0\0\0\x01\0\x01\0\x40\x1f\0\0\x80\x3e\0\0\x02\0\x10\0",20);
        wave+="data"; wave.append("\x04\0\0\0\0\0\xff\x7f",8);
        writeFile(root/"source"/"tone.wav",wave);
        auto registry=std::make_shared<ResourceCompilerRegistry>(); String error;
        require(registry->registerCompiler(std::make_shared<audio::AudioResourceCompiler>(),&error),error.c_str());
        ResourceSystem system(registry,std::make_shared<ResourceCompilationDatabase>());
        ResourceSystemConfig config; config.sourceRoot=(root/"source").u8string().c_str();
        config.compiledRoot=(root/"cooked").u8string().c_str(); config.target="audio";
        require(system.initialize(config,error),error.c_str());
        CompilationOptions options; options.packagedBuild=true; const ResourceID id("data://tone.wav");
        require(system.compile(id,options).succeeded(),"Native PCM16 compiler accepts valid WAVE");
        writeFile(root/"source"/"broken.wav","malformed WAVE input");
        require(!system.compile(ResourceID("data://broken.wav"),options).succeeded(),"Malformed audio fails compilation");
        const auto package=root/"package";
        require(system.writeRuntimePackage(package.u8string().c_str(),{id},{},error),error.c_str());
        system.shutdown(); fs::rename(root/"source",root/"hidden-source"); fs::rename(root/"cooked",root/"hidden-cooked");
        ResourceSystem runtime(nullptr,nullptr); RuntimeResourceConfig mount;
        mount.compiledRoot=package.u8string().c_str(); mount.manifestPath=(package/"runtime.wprm").u8string().c_str(); mount.target="audio";
        require(runtime.initializeRuntime(mount,error),error.c_str());
        auto clip=runtime.load(id,error);
        require(clip!=nullptr,error.c_str());
        const bool audioValid=audio::AudioResourceCompiler::validate(*clip,error);
        require(audioValid,std::string(error.c_str())+"; type="+clip->header.resourceType.str().c_str()+
            "; version="+std::to_string(clip->header.compilerVersion)+"; bytes="+std::to_string(clip->payload.size()));
        RuntimeResource corrupt=*clip; corrupt.payload[0]=0;
        require(!audio::AudioResourceCompiler::validate(corrupt,error),"Typed consumer rejects malformed audio");
        corrupt=*clip; corrupt.header.compilerVersion=2;
        require(!audio::AudioResourceCompiler::validate(corrupt,error),"Typed consumer rejects unknown versions");
#endif
    }

    void buildProfileContracts(const fs::path &root)
    {
        class ProfileProbe final : public IResourceCompiler
        {
        public:
            String name() const override { return "ProfileContextProbe"; }
            Array<CompilerOutput> outputs() const override { return {{ResourceTypeID("txtres"),1}}; }
            bool getDependencies(const CompileContext &context,DependencySet &dependencies,String &) const override
            {
                if(context.resourceId==ResourceID("data://asset.txtres"))
                    dependencies.installDependencies.push_back(ResourceID("data://child.txtres"));
                return true;
            }
            CompilationStatus compile(const CompileContext &context,std::ostream &output,Array<String>&) const override
            { output << "profile=" << context.buildProfile.key(); return CompilationStatus::Success; }
        };
        fs::create_directories(root/"source");
        writeFile(root/"source"/"asset.txtres","profile payload\n");
        writeFile(root/"source"/"child.txtres","child payload\n");
        auto registry=std::make_shared<ResourceCompilerRegistry>(); String error;
        require(registry->registerCompiler(std::make_shared<ProfileProbe>(),&error),error.c_str());
        ResourceSystemConfig config;
        config.sourceRoot=(root/"source").u8string().c_str();
        config.compiledRoot=(root/"cooked").u8string().c_str(); config.target="profiles";
        const ResourceID id("data://asset.txtres");
        std::vector<String> paths;
        fs::path childArtifact;
        for(int pass : {0,1,2,3,0,1,2,3})
        {
            config.buildProfile={};
            if(pass) { config.buildProfile.profile="shipping"; config.buildProfile.toolchain=pass==3?"encoder-2":"encoder-1";
                config.buildProfile.settings["quality"]=pass==2?"high":"low"; }
            auto database=std::make_shared<ResourceCompilationDatabase>(); ResourceSystem system(registry,database);
            require(system.initialize(config,error),error.c_str());
            const auto report=system.compile(id);
            require(report.succeeded(),"Profile compilation succeeds");
            auto loaded=system.load(id,error);
            const std::string expected="profile="+std::string(config.buildProfile.key().c_str());
            require(loaded && std::string(reinterpret_cast<const char*>(loaded->payload.data()),loaded->payload.size())==expected,
                "Compiler receives the captured effective build profile");
            if(paths.size()<4) { require(report.status!=CompilationStatus::UpToDate,"New profile cooks separately"); paths.push_back(report.outputPath); }
            else require(report.status==CompilationStatus::UpToDate && report.outputPath==paths[pass],"Profile survives restart without recooking");
            CompiledResourceRecord record;
            require(database->getRecord(id.str(),{config.target,false,config.buildProfile.key()},record)&&
                fs::equivalent(fs::u8path(config.compiledRoot.c_str())/fs::u8path(record.outputPath.c_str()),fs::u8path(report.outputPath.c_str())),
                "Explicit index lookup selects the full profile");
            if(pass==0) {
                require(database->getRecord("data://child.txtres",{config.target,false,{}},record)&&record.isValid(),"Child generation is indexed");
                childArtifact=fs::u8path(config.compiledRoot.c_str())/fs::u8path(record.outputPath.c_str());
            }
            require(system.writeRuntimeManifest((root/("manifest-"+std::to_string(pass)+".wprm")).u8string().c_str(),{id},
                {{"35d8e41d-e0e5-43fe-9013-5bc80c42c401",id}},error),error.c_str());
            system.shutdown();
        }
        require(std::set<String>(paths.begin(),paths.end()).size()==4,"Settings and toolchain artifact variants coexist");
        RuntimeResourceConfig runtime;
        runtime.compiledRoot=config.compiledRoot; runtime.target=config.target;
        runtime.manifestPath=(root/"manifest-0.wprm").u8string().c_str();
        auto scheduler=std::make_shared<WorldStreamingScheduler>();
        auto stream=ResourcePayloadStream::create(runtime,scheduler,{},error);
        require(stream!=nullptr,error.c_str());
        ResourceID selected;
        require(stream->resolveUUID("35d8e41d-e0e5-43fe-9013-5bc80c42c401",selected,error)&&selected==id,
            "Stream demand resolves UUIDs through its own immutable mount");
        fs::rename(root/"source",root/"source-unavailable");
        fs::rename(childArtifact,fs::path(childArtifact.wstring()+L".unavailable"));
        require(stream->replace({{19,id,1024*1024,0}},error),error.c_str());
        ResourcePayloadResult ready;
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
        while(!stream->take(ready)&&std::chrono::steady_clock::now()<deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        require(ready.key==19&&ready.resource&&!ready.resource->payload.empty()&&ready.resource->dependencies.empty(),
            "Shared scheduler delivers a shallow source-free payload");
        require(!stream->replace({{20,id,9ull<<20,0}},error)&&!error.empty(),"Demand above producer residency budget is rejected");
        Array<ResourcePayloadDemand> oversized;
        for(u32 key=0;key<257;++key) oversized.push_back({100+key,id,1024,0});
        const auto generation=stream->stats().generation;
        require(!stream->replace(oversized,error)&&stream->stats().generation==generation,
            "Oversized demand batches are rejected before preparation without replacing accepted demand");
        require(!stream->replace({{20,ResourceID("data://"+String(1100,'a')+".txtres"),1024,0}},error),
            "Oversized pending identity metadata is rejected before queueing");
        require(stream->replace({{21,ResourceID("data://absent.txtres"),1024*1024,0}},error),error.c_str());
        ResourcePayloadResult missing;
        const auto missingDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
        while(!stream->take(missing)&&std::chrono::steady_clock::now()<missingDeadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        require(missing.key==21&&!missing.resource&&!missing.error.empty(),"Missing pinned resources report worker failures");
        require(stream->replace({{22,id,1024*1024,0}},error),error.c_str());
        stream->cancel();
        stream->drain(); stream.reset();
        require(scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)]!=0,
            "Caller-held result retains its memory reservation after stream destruction");
        ready={};
        require(scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)]==0,
            "Last consumer releases stream residency reservation");
        ResourceBuildProfile left,right;
        left.profile=right.profile="shipping"; left.toolchain=right.toolchain="tools";
        left.settings={{"a","b:c"},{"d","e"}}; right.settings={{"a","b"},{"c:d","e"}};
        require(left.key()!=right.key(),"Length framing prevents setting aliases");
        right=left; right.toolchain="tools-next";
        require(left.key()!=right.key(),"Toolchain identity participates in key");
        config.buildProfile.profile="missing-toolchain"; config.buildProfile.toolchain.clear();
        ResourceSystem rejected(registry,std::make_shared<ResourceCompilationDatabase>());
        require(!rejected.initialize(config,error)&&!error.empty(),"Incomplete profiles fail before cooking");
    }
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
        buildProfileContracts(root/"profile-contracts");
        audioContracts(root/"audio-contracts");

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
        auto snapshotCompiler=std::make_shared<SnapshotCompiler>(source);
        require(registry->registerCompiler(snapshotCompiler,&error),error.c_str());
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
        snapshotContracts(system,database,snapshotCompiler,source,{config.target,false});

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
        const ResourceID longId((std::string("data://models/car.mesh:")+std::string(180,'s')+".txtres").c_str());
        const auto longArtifact=system.compile(longId);
        require(longArtifact.succeeded(),"Long logical resource identity: "+reportMessage(longArtifact));
        const auto longLoaded=system.load(longId,error);
        require(longArtifact.outputPath.size()<260 && longLoaded && longLoaded->header.resourceId==longId,
            "Long logical IDs retain exact identity without expanding Windows artifact paths");
        writeFile(source / "case.txtres","case fixture\n");
        writeFile(source / "CASE.txtres","case fixture\n");
        const auto lower=system.compile(ResourceID("data://case.txtres"));
        const auto upper=system.compile(ResourceID("data://CASE.txtres"));
        require(lower.succeeded() && upper.succeeded() &&
                !fs::equivalent(fs::u8path(lower.outputPath.c_str()),fs::u8path(upper.outputPath.c_str())),
                "Case-distinct logical IDs must not alias on Windows artifact storage");
        {
            writeFile(source / "legacy-cache.txtres","retained legacy cache\n");
            const ResourceID id("data://legacy-cache.txtres");
            const auto cooked=system.compile(id);
            require(cooked.succeeded(),"Build cache-layout migration fixture");
            CompiledResourceRecord record;
            require(database->getRecord(id.str(),atomicVariant,record),"Read cache-layout migration record");
            // Freeze the pre-compact layout, independently of the current path builder.
            record.outputPath="artifacts/v2/rsmoke/editor/rdata~3a~2f~2flegacy-cache~2etxtres/" +
                String(fs::u8path(cooked.outputPath.c_str()).filename().u8string().c_str());
            const auto oldPath=compiled / fs::u8path(record.outputPath.c_str());
            fs::create_directories(oldPath.parent_path());
            fs::copy_file(fs::u8path(cooked.outputPath.c_str()),oldPath);
            require(database->commitCompilation(record,atomicVariant,{}),"Retain the pre-compact index entry");
            system.clearRuntimeCache();
            require(bool(system.load(id,error)),error.c_str());
            const auto reused=system.compile(id);
            require(reused.status==CompilationStatus::UpToDate && fs::equivalent(fs::u8path(reused.outputPath.c_str()),oldPath),
                "Existing encoded cache paths must load and remain incremental");
            const auto manifest=compiled / "legacy-cache.wprm";
            require(system.writeRuntimeManifest(manifest.u8string().c_str(),{id},error),error.c_str());
            ResourceSystem runtime(nullptr,nullptr);
            RuntimeResourceConfig configuration;
            configuration.compiledRoot=config.compiledRoot;
            configuration.manifestPath=manifest.u8string().c_str();
            configuration.target=config.target;
            require(runtime.initializeRuntime(configuration,error),error.c_str());
            const auto loaded=runtime.load(id,error);
            require(loaded && loaded->header.resourceId==id &&
                std::string(loaded->payload.begin(),loaded->payload.end())=="retained legacy cache\n",
                "Installed manifests must preserve pre-compact artifact paths");
        }
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
        require(resource_runtime_contracts::read(source / "mutating.txtres")=="original\n",
                "Compiler writes must not modify the authored descriptor through its captured path");
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
