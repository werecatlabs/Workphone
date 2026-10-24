#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <stdexcept>
#include <cmath>
#include <iostream>
using workphone::procedural::OpenCityLayout;
static void require(bool condition)
{ if(!condition) throw std::runtime_error("Open-city layout contract failed"); }
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
                require(city.lots.size()==size_t(blocks*blocks*4));
                require(city.route.front().x==0 && city.route.front().z==0);
                require(city.route[1].x==0 && city.route[1].z<0);
                float length=0;
                for(size_t i=0;i<city.route.size();++i)
                {
                    const auto p=city.route[i], q=city.route[(i+1)%city.route.size()];
                    require(city.roadDistance(p.x,p.z)<.001f);
                    const auto step=std::hypot(q.x-p.x,q.z-p.z);
                    require(step>0 && step<=2.001f);
                    require(p.x==copy.route[i].x && p.z==copy.route[i].z);
                    length+=step;
                }
                require(length>previousLength);
                previousLength=length;
                for(size_t i=0;i<city.lots.size();++i)
                {
                    const auto lot=city.lots[i];
                    require(lot.height>0 && std::isfinite(lot.height));
                    require(lot.x==copy.lots[i].x && lot.height==copy.lots[i].height);
                    // Every footprint clears the 12m carriageway and sidewalks.
                    for(int a : {-1,1}) for(int b : {-1,1})
                        require(city.roadDistance(lot.x+a*lot.width*.5f,lot.z+b*lot.depth*.5f)>9);
                }
                require(city.roadDistance(0,0)==0);
                require(city.roadDistance(city.extent+20,0)>100);
            }
        }
    require(OpenCityLayout::generate(1,-2,-1).blocks==6);
    require(OpenCityLayout::generate(1,99,99).blocks==10);
    require(OpenCityLayout::generate(7).lots[0].height!=OpenCityLayout::generate(8).lots[0].height);
    std::cout << "Open city deterministic layout, street clearance, closed routes and bounds: PASS\n";
}
