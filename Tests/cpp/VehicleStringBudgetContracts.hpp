#pragma once
#include <Workphone/System/SourceBudgetStdString.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

namespace vehicle_string_budget_contracts
{
    using namespace workphone;
    inline void require(bool condition, const char *message)
    { if(!condition) throw std::runtime_error(message); }
    struct Budget
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler=WorldStreamingScheduler::shared();
        static constexpr auto source=size_t(StreamMemoryClass::SourceCpu);
        u64 original=scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set(u64 limit)
        { String error; require(scheduler->setBudget(StreamMemoryClass::SourceCpu,limit,error),"vehicle string budget applies"); }
        void restore() { set(original); }
        ~Budget() { restore(); }
    };
    template<class F> void denied(F operation, const char *message)
    { bool rejected=false;try{operation();}catch(const std::bad_alloc&){rejected=true;}require(rejected,message); }
    inline u64 bytes(size_t capacity)
    { return sourceBudgetStorageBytes(capacity+1,alignof(char)); }

    inline void run()
    {
        Budget budget;
        const auto baseline=budget.used();
        SourceBudgetStdString inlineText("small");
        require(budget.used()==baseline,"inline vehicle text lives in the enclosing object");
        std::string foreign("raw\0name",8);foreign.reserve(4095);
        const auto imported=bytes(foreign.capacity());
        budget.set(baseline+imported-1);
        denied([&]{SourceBudgetStdString value(foreign);},"foreign text imports its physical spare and terminator");
        require(budget.used()==baseline,"denied foreign import retires no caller memory");
        budget.set(baseline+imported);
        SourceBudgetStdString name(std::move(foreign));
        require(budget.used()==baseline+imported && foreign.size()==8 && name.size()==8 && name[3]==0 &&
                    name.capacity()==foreign.capacity() && name.data()!=foreign.data(),
                "foreign rvalue canonicalizes into independent Source storage with complete capacity");
        budget.set(budget.used());
        const auto data=name.data();
        name=name;
        require(name.data()==data && name.size()==8,"self assignment requires no admission");
        denied([&]{name.append(name.capacity()+1,'x');},"string growth cannot exceed Source headroom");
        require(name.data()==data && name.size()==8 && name[3]==0,"denied append preserves embedded-NUL content and storage");
        const auto inlineData=inlineText.data();
        denied([&]{inlineText=name;},"owning string copy stages physical capacity before replacement");
        require(inlineText=="small" && inlineText.data()==inlineData,"denied text replacement preserves previous value");
        require(name==foreign && foreign==name && name==std::string_view(foreign) &&
                    name.compare(std::string_view(foreign))==0 && name.find("name")==4,
                "comparisons and read-only view interop allocate no storage at the cap");
        std::string escaped=static_cast<std::string>(name);
        require(escaped==foreign && budget.used()==baseline+imported,"foreign string output is explicit and independent");
        name.clear();
        require(name.empty() && name.capacity()==foreign.capacity() && budget.used()==baseline+imported,
                "logical text clear retains its complete physical charge");
        name.assign(foreign.data(),foreign.size());
        budget.set(budget.used()+imported);
        std::optional<SourceBudgetStdString> snapshot(name);
        require(snapshot->capacity()==name.capacity() && snapshot->data()!=name.data(),"caller text snapshot retains spare ownership");
        budget.set(budget.used());const auto denials=sourceBudgetDenialCount();
        name.releaseStorage();
        require(budget.used()==baseline+imported && *snapshot==foreign,"producer release leaves caller capacity charged");
        std::thread finalOwner([value=std::move(snapshot)]() mutable { value.reset(); });
        finalOwner.join();snapshot.reset();
        require(budget.used()==baseline && sourceBudgetDenialCount()==denials,"last cross-thread text release retires without admission");

        const auto body=sourceBudgetStorageBytes(sizeof(SourceBudgetStdString),alignof(std::max_align_t));
        budget.set(baseline+body+imported-1);
        denied([&]{auto owner=std::make_unique<SourceBudgetStdString>(foreign);},"body success followed by text denial must unwind both");
        require(budget.used()==baseline,"failed heap text construction retires its body");
        budget.set(baseline+body+imported);
        auto owner=std::make_unique<SourceBudgetStdString>(foreign);
        require(budget.used()==baseline+body+imported,"heap text body and capacity each own their exact admission");
        budget.set(budget.used());owner.reset();
        require(budget.used()==baseline,"heap text destruction needs no replacement storage");

        budget.restore();
        SourceBudgetStdString prefix(127,'p');prefix.reserve(1023);
        const auto resident=budget.used();const auto prefixData=prefix.data();
        budget.set(resident);
        denied([&]{auto combined=prefix+"-suffix";},"concatenation cannot create unbudgeted diagnostic scratch");
        require(prefix.data()==prefixData && prefix.size()==127 && budget.used()==resident,
                "denied diagnostic concatenation leaves source text unchanged");
        budget.restore();
        const auto combined=std::string("head-")+prefix+"-tail";
        require(combined.size()==137 && combined.find("head-")==0 && combined.substr(132)=="-tail",
                "foreign-prefix diagnostic concatenation stays in Source-owned result storage");
        SourceBudgetStdString emptyView(std::string_view{});
        emptyView.append(std::string_view{});emptyView.assign(std::string_view{});
        require(emptyView.empty(),"empty views need no nonnull external buffer");
        emptyView={'a','\0','b'};
        require(emptyView.size()==3 && emptyView[1]==0,"brace text assignment preserves arbitrary bytes");
        emptyView={};
        require(emptyView.empty(),"empty brace assignment clears rather than constructing from a null pointer");
    }
}
