#include <WPAudio/SharedClipCache.hpp>
#include <array>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)
using Clip=std::vector<float>;
using Cache=workphone::audio::SharedClipCache<Clip>;
static bool equal(const Clip &a,const Clip &b) { return a==b; }
int main() {
    try {
        Cache cache(16,2);
        std::shared_ptr<const Clip> first, alias, changed, rejected;
        CHECK(cache.intern(std::make_shared<Clip>(Clip{1,2}),8,equal,first)==Cache::Result::Ready);
        CHECK(cache.intern(std::make_shared<Clip>(Clip{1,2}),8,equal,alias)==Cache::Result::Ready);
        CHECK(first==alias && cache.stats().residentBytes==8 && cache.stats().reuses==1);
        CHECK(cache.intern(std::make_shared<Clip>(Clip{3,4}),8,equal,changed)==Cache::Result::Ready);
        CHECK(first!=changed && (*first)[0]==1 && (*changed)[0]==3);
        CHECK(cache.intern(std::make_shared<Clip>(Clip{5,6}),8,equal,rejected)==Cache::Result::BudgetExceeded);
        CHECK(!rejected && cache.stats().rejections==1 && cache.stats().residentBytes==16);
        first.reset(); alias.reset(); CHECK(cache.stats().residentBytes==8);
        CHECK(cache.intern(std::make_shared<Clip>(Clip{5,6}),8,equal,rejected)==Cache::Result::Ready);
        changed.reset(); rejected.reset(); CHECK(cache.stats().pinnedClips==0);
        Cache concurrent(1024,4);
        std::array<std::shared_ptr<const Clip>,8> pins;
        std::array<std::thread,8> workers;
        std::array<Cache::Result,8> results{};
        for(size_t i=0;i<workers.size();++i) workers[i]=std::thread([&,i] {
            results[i]=concurrent.intern(std::make_shared<Clip>(Clip{1,2,3,4}),16,equal,pins[i]);
        });
        for(auto &worker:workers) worker.join();
        for(size_t i=0;i<pins.size();++i) CHECK(results[i]==Cache::Result::Ready && pins[i]==pins[0]);
        CHECK(concurrent.stats().pinnedClips==1 && concurrent.stats().residentBytes==16);
        CHECK(concurrent.stats().reuses==7);
        for(auto &pin:pins) pin.reset(); CHECK(concurrent.stats().residentBytes==0);
        std::puts("Shared clip generations, budget, reclamation and concurrent admission passed");
        return 0;
    } catch(const std::exception &e) { std::fprintf(stderr,"%s\n",e.what()); return 1; }
}
