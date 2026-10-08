#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <cassert>
#include <cmath>
#include <iostream>
using workphone::procedural::OpenCityLayout;
int main()
{
    for(unsigned seed : {0u, 7u, 23u, 4294967295u})
        for(int blocks : {6, 8, 10})
        {
            float previousLength=0;
            for(int route=0;route<3;++route)
            {
                const auto city=OpenCityLayout::generate(seed,blocks,route);
                const auto copy=OpenCityLayout::generate(seed,blocks,route);
                assert(city.lots.size()==size_t(blocks*blocks*4));
                assert(city.route.front().x==0 && city.route.front().z==0);
                assert(city.route[1].x==0 && city.route[1].z<0);
                float length=0;
                for(size_t i=0;i<city.route.size();++i)
                {
                    const auto p=city.route[i], q=city.route[(i+1)%city.route.size()];
                    assert(city.roadDistance(p.x,p.z)<.001f);
                    const auto step=std::hypot(q.x-p.x,q.z-p.z);
                    assert(step>0 && step<=2.001f);
                    assert(p.x==copy.route[i].x && p.z==copy.route[i].z);
                    length+=step;
                }
                assert(length>previousLength);
                previousLength=length;
                for(size_t i=0;i<city.lots.size();++i)
                {
                    const auto lot=city.lots[i];
                    assert(lot.height>0 && std::isfinite(lot.height));
                    assert(lot.x==copy.lots[i].x && lot.height==copy.lots[i].height);
                    // Every footprint clears the 12m carriageway and sidewalks.
                    for(int a : {-1,1}) for(int b : {-1,1})
                        assert(city.roadDistance(lot.x+a*lot.width*.5f,lot.z+b*lot.depth*.5f)>9);
                }
                assert(city.roadDistance(0,0)==0);
                assert(city.roadDistance(city.extent+20,0)>100);
            }
        }
    assert(OpenCityLayout::generate(1,-2,-1).blocks==6);
    assert(OpenCityLayout::generate(1,99,99).blocks==10);
    assert(OpenCityLayout::generate(7).lots[0].height!=OpenCityLayout::generate(8).lots[0].height);
    std::cout << "Open city deterministic layout, street clearance, closed routes and bounds: PASS\n";
}
