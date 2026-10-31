#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

using namespace workphone;
using namespace workphone::resource;
namespace fs = std::filesystem;
namespace
{
    constexpr const char *crashUuid = "5811d94f-ffcd-4e63-8dd7-d1cb93168134";
    void require(bool value,const String &message) { if(!value) throw std::runtime_error(message.c_str()); }
    void barrier(const fs::path &ready,const char *phase)
    {
        std::ofstream marker(ready); marker<<phase; marker.close();
        require(static_cast<bool>(marker),"Cannot write crash-test readiness marker");
        std::cin.get();
    }
    class Compiler final : public IResourceCompiler
    {
    public:
        std::string phase;
        fs::path ready;
        Compiler(std::string value,fs::path marker):phase(std::move(value)),ready(std::move(marker)) {}
        String name() const override { return "PublicationCrashContractCompiler"; }
        Array<CompilerOutput> outputs() const override { return {{ResourceTypeID("crashres"),1}}; }
        CompilationStatus compile(const CompileContext &context,std::ostream &output,Array<String>&) const override
        {
            std::ifstream source(fs::u8path(context.sourcePath.c_str()),std::ios::binary);
            if(!source) return CompilationStatus::Failure;
            output<<source.rdbuf();
            const std::string block(65536,'x');
            output.write(block.data(),block.size()); output.flush();
            if(phase=="compile") barrier(ready,"compile");
            for(int index=0;index<15;++index) output.write(block.data(),block.size());
            return output ? CompilationStatus::Success : CompilationStatus::Failure;
        }
    };
    struct Admission
    {
        bool pauseAfterPublication;
        fs::path output,ready;
        ~Admission()
        {
            // The existing resolver lifetime is retained through exclusive
            // publication. No production fault hook or environment backdoor.
            if(pauseAfterPublication&&fs::is_regular_file(output/"runtime.wprm"))
                barrier(ready,"published");
        }
    };
}
int main(int argc,char **argv)
{
    try
    {
        if(argc==4&&std::string(argv[1])=="--verify")
        {
            ResourceSystem runtime(nullptr,nullptr); RuntimeResourceConfig config;
            config.compiledRoot=fs::absolute(argv[2]).u8string().c_str();
            config.manifestPath=(fs::absolute(argv[2])/"runtime.wprm").u8string().c_str();
            config.target="crash-contract"; String error;
            require(runtime.initializeRuntime(config,error),error);
            auto resource=runtime.loadUUID(crashUuid,error); require(resource!=nullptr,error);
            const std::string expected=std::string(argv[3])+"\n";
            require(resource->payload.size()==expected.size()+1024*1024,"Unexpected crash-contract payload size");
            require(std::equal(expected.begin(),expected.end(),resource->payload.begin()),"Unexpected compiled generation");
            return 0;
        }
        require(argc==6&&std::string(argv[1])=="--export","Expected --export project phase output readiness");
        const fs::path project=fs::absolute(argv[2]),output=fs::absolute(argv[4]),ready=fs::absolute(argv[5]);
        const std::string phase=argv[3];
        auto registry=std::make_shared<ResourceCompilerRegistry>(); String error;
        require(registry->registerCompiler(std::make_shared<Compiler>(phase,ready),&error),error);
        ResourceSystem resources(registry,std::make_shared<ResourceCompilationDatabase>());
        ResourceSystemConfig config; config.sourceRoot=(project/"source").u8string().c_str();
        config.compiledRoot=(project/"cooked").u8string().c_str(); config.target="crash-contract";
        require(resources.initialize(config,error),error);
        const ResourceID root("data://asset.crashres"); CompilationOptions options; options.packagedBuild=true;
        const auto report=resources.compile(root,options);
        if(!report.succeeded()) { for(const auto &message:report.messages) std::cerr<<message<<'\n'; return 1; }
        RuntimePackageResolver resolver=[&](const RuntimePackageContext &context,RuntimePackageAdmission &result,String&) {
            require(context.resources.size()==1&&context.resources.front()==root,"Unexpected pinned install closure");
            result.bindings.push_back({crashUuid,root});
            auto admission=std::make_shared<Admission>(); admission->pauseAfterPublication=phase=="published";
            admission->output=output; admission->ready=ready; result.lifetime=admission;
            if(phase=="admission") barrier(ready,"admission");
            return true;
        };
        require(resources.writeRuntimePackageChecked(output.u8string().c_str(),{root},resolver,error),error);
        std::cout<<"Exported crash-contract package\n";
        return 0;
    }
    catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
