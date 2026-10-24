#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <set>
#include <map>
#include <limits>
using workphone::procedural::OpenCityLayout;
static void require(bool condition, const char *message="Open-city layout contract failed")
{ if(!condition) throw std::runtime_error(message); }
static bool sameLot(const OpenCityLayout::Lot &a,const OpenCityLayout::Lot &b)
{
    return a.x==b.x && a.z==b.z && a.width==b.width && a.depth==b.depth && a.height==b.height &&
        a.park==b.park && a.id==b.id && a.variant==b.variant && a.district==b.district && a.frontage==b.frontage &&
        a.parcelX==b.parcelX && a.parcelZ==b.parcelZ && a.parcelWidth==b.parcelWidth && a.parcelDepth==b.parcelDepth;
}
static void checkMasses(const OpenCityLayout &city,const OpenCityLayout::Lot &lot)
{
    const auto volumes=OpenCityLayout::buildingVolumes(lot);
    const auto repeated=OpenCityLayout::buildingVolumes(lot);
    require(volumes.size()==repeated.size(),"Building massing is not deterministic");
    if(lot.park) {require(volumes.empty(),"Park generated occupied building volumes");return;}
    require(!volumes.empty() && volumes.size()<=3,"City building exceeded bounded massing tier");
    require(volumes.size()==(lot.district==OpenCityLayout::District::Residential || lot.height<12?1u:
        lot.district==OpenCityLayout::District::Civic?2u:3u),"District massing policy changed");
    float top=0;
    for(size_t i=0;i<volumes.size();++i)
    {
        const auto &v=volumes[i], &r=repeated[i];
        require(v.x==r.x && v.z==r.z && v.width==r.width && v.depth==r.depth &&
            v.bottom==r.bottom && v.height==r.height,"Repeated building mass changed");
        require(std::isfinite(v.x) && std::isfinite(v.z) && std::isfinite(v.width) && std::isfinite(v.depth) &&
            std::isfinite(v.bottom) && std::isfinite(v.height) && v.width>0 && v.depth>0 && v.height>0,
            "Building generated invalid occupied volume");
        require(std::abs(v.x-lot.x)+v.width*.5f<=lot.width*.5f+.001f &&
            std::abs(v.z-lot.z)+v.depth*.5f<=lot.depth*.5f+.001f,"Building mass escaped its parcel footprint");
        require(std::abs(v.bottom-top)<.001f,"Building tiers overlap or leave vertical gaps");
        top=v.bottom+v.height;
        require(top<=lot.height+.001f,"Building mass exceeded authored height");
        for(int a : {-1,1}) for(int b : {-1,1})
            require(city.roadDistance(v.x+a*v.width*.5f,v.z+b*v.depth*.5f)>=OpenCityLayout::roadSetback-.001f,
                "Shared render/collision volume intrudes onto street or sidewalk");
        for(size_t j=0;j<i;++j)
        {
            const auto &other=volumes[j];
            require(v.bottom>=other.bottom+other.height-.001f || other.bottom>=top-.001f ||
                std::abs(v.x-other.x)>=(v.width+other.width)*.5f-.001f ||
                std::abs(v.z-other.z)>=(v.depth+other.depth)*.5f-.001f,
                "Building occupied volumes overlap");
        }
    }
    require(std::abs(top-lot.height)<.001f,"Building massing lost authored skyline height");
}
int main()
{
    for(unsigned seed : {0u, 7u, 23u, 4294967295u})
    {
        const auto large=OpenCityLayout::generate(seed,10,0);
        std::map<std::uint64_t,OpenCityLayout::Lot> largestLots;
        for(const auto &lot:large.lots) largestLots.emplace(lot.id,lot);
        for(int blocks : {6, 8, 10})
        {
            float previousLength=0;
            const auto base=OpenCityLayout::generate(seed,blocks,0);
            for(int route=0;route<3;++route)
            {
                const auto city=OpenCityLayout::generate(seed,blocks,route);
                const auto copy=OpenCityLayout::generate(seed,blocks,route);
                require(city.lots.size()==size_t(blocks*blocks*4));
                require(city.spacing>=88 && city.spacing<=104 && city.extent==blocks*city.spacing*.5f);
                require(city.route.front().x==0 && city.route.front().z==0);
                require(city.route[1].x==0 && city.route[1].z<0);
                float length=0;
                for(size_t i=0;i<city.route.size();++i)
                {
                    const auto p=city.route[i], q=city.route[(i+1)%city.route.size()];
                    require(city.roadDistance(p.x,p.z)<.001f);
                    const auto step=std::hypot(q.x-p.x,q.z-p.z);
                    require(step>0 && step<=2.001f);
                    require(std::abs(q.x-p.x)<.001f || std::abs(q.z-p.z)<.001f,
                        "City route leaves connected axis-aligned street network");
                    require(std::abs(p.x)<=city.extent+.001f && std::abs(p.z)<=city.extent+.001f,
                        "City route escaped generated road bounds");
                    require(p.x==copy.route[i].x && p.z==copy.route[i].z);
                    length+=step;
                }
                require(length>previousLength);
                previousLength=length;
                std::set<std::uint64_t> identities;
                bool commercial=false,residential=false;
                for(size_t i=0;i<city.lots.size();++i)
                {
                    const auto lot=city.lots[i];
                    require(lot.height>0 && std::isfinite(lot.height));
                    require(sameLot(lot,copy.lots[i]),"City lot or metadata changed on repeated generation");
                    require(sameLot(lot,base.lots[i]),"Route choice reshuffled city lots");
                    const auto stable=largestLots.find(lot.id);
                    require(stable!=largestLots.end() && sameLot(lot,stable->second),
                        "Expanding city bounds changed an existing signed-cell parcel");
                    require(lot.id && identities.insert(lot.id).second,"City parcel identity collision");
                    require(unsigned(lot.district)<=unsigned(OpenCityLayout::District::Civic) &&
                        unsigned(lot.frontage)<=unsigned(OpenCityLayout::Frontage::South),"Invalid city metadata");
                    require(lot.district==city.lots[i/4*4].district,"District changed inside one city block");
                    commercial|=lot.district==OpenCityLayout::District::Commercial;
                    residential|=lot.district==OpenCityLayout::District::Residential;
                    require(lot.width>0 && lot.depth>0 && lot.parcelWidth>0 && lot.parcelDepth>0 &&
                        std::abs(lot.x-lot.parcelX)+lot.width*.5f<=lot.parcelWidth*.5f+.001f &&
                        std::abs(lot.z-lot.parcelZ)+lot.depth*.5f<=lot.parcelDepth*.5f+.001f,
                        "Building footprint escaped shared parcel envelope");
                    // Every footprint clears the carriageway and sidewalks, including boundary streets.
                    for(int a : {-1,1}) for(int b : {-1,1})
                    {
                        const float x=lot.x+a*lot.width*.5f,z=lot.z+b*lot.depth*.5f;
                        require(city.roadDistance(x,z)>=OpenCityLayout::roadSetback-.001f,
                            "Parcel footprint violates street setback");
                        require(std::abs(x)<=city.extent-OpenCityLayout::roadSetback+.001f &&
                            std::abs(z)<=city.extent-OpenCityLayout::roadSetback+.001f,"Parcel escaped city bounds");
                    }
                    for(size_t j=0;j<i;++j)
                    {
                        const auto other=city.lots[j];
                        require(std::abs(lot.x-other.x)>=(lot.width+other.width)*.5f-.001f ||
                            std::abs(lot.z-other.z)>=(lot.depth+other.depth)*.5f-.001f,"City lot footprints overlap");
                    }
                    if(!lot.park)
                    {
                        const float face=lot.frontage==OpenCityLayout::Frontage::West?lot.x-lot.width*.5f:
                            lot.frontage==OpenCityLayout::Frontage::East?lot.x+lot.width*.5f:
                            lot.frontage==OpenCityLayout::Frontage::North?lot.z-lot.depth*.5f:lot.z+lot.depth*.5f;
                        require(std::abs(std::abs(face-std::round(face/city.spacing)*city.spacing)-OpenCityLayout::roadSetback)<.001f,
                            "Building frontage does not align with its street setback");
                    }
                    checkMasses(city,lot);
                }
                require(commercial && residential,"City lost coherent centre and outer district variation");
                require(city.roadDistance(0,0)==0);
                require(city.roadDistance(city.extent+20,0)>100);
                for(int x=-blocks/2;x<=blocks/2;++x) for(int z=-blocks/2;z<=blocks/2;++z)
                    require(city.roadDistance(x*city.spacing,z*city.spacing)==0,"Signed intersection disconnected");
            }
        }
    }
    require(OpenCityLayout::generate(1,-2,-1).blocks==6);
    require(OpenCityLayout::generate(1,99,99).blocks==10);
    require(OpenCityLayout::generate(7).lots[0].height!=OpenCityLayout::generate(8).lots[0].height);
    for(unsigned seed : {0u,7u,42u,4294967295u}) for(int blocks:{6,8,10})
    {
        const auto natural=OpenCityLayout::generate(seed,blocks,0,true);
        const auto repeated=OpenCityLayout::generate(seed,blocks,0,true);
        const auto alternate=OpenCityLayout::generate(seed,blocks,2,true);
        const auto parks=std::count_if(natural.lots.begin(),natural.lots.end(),
            [](const OpenCityLayout::Lot &lot){return lot.park;});
        std::cout << "Natural city seed=" << seed << " blocks=" << blocks << " nodes=" << natural.roads.nodes.size()
            << " edges=" << natural.roads.edges.size() << " parcels=" << natural.lots.size()
            << " buildings=" << natural.lots.size()-parks << " parks=" << parks << std::endl;
        require(natural.naturalRoads && !natural.roads.edges.empty() && natural.lots.size()>20,
            "L-system city failed to populate connected street frontages");
        if(seed==42 && blocks==6) require(natural.lots.size()>=70,
            "Representative natural city lost its populated street frontages");
        require(natural.lots.size()<=size_t(blocks*blocks*4),"Natural city exceeded parcel budget");
        require(natural.lots.size()==repeated.lots.size() && natural.lots.size()==alternate.lots.size(),
            "Natural route choice changed city parcels");
        std::set<std::uint64_t> identities;
        for(size_t i=0;i<natural.lots.size();++i)
        {
            const auto &lot=natural.lots[i];
            require(sameLot(lot,repeated.lots[i]) && sameLot(lot,alternate.lots[i]),"Natural parcel determinism failed");
            require(lot.id && identities.insert(lot.id).second,"Natural parcel identity collision");
            require(lot.width>0 && lot.depth>0 && lot.parcelWidth>0 && lot.parcelDepth>0 &&
                std::abs(lot.x-lot.parcelX)+lot.width*.5f<=lot.parcelWidth*.5f+.001f &&
                std::abs(lot.z-lot.parcelZ)+lot.depth*.5f<=lot.parcelDepth*.5f+.001f,
                "Natural building escaped its parcel envelope");
            require(natural.footprintRoadClearance(lot.parcelX,lot.parcelZ,lot.parcelWidth,lot.parcelDepth)>=OpenCityLayout::roadSetback-.001f,
                "Natural road crosses a building/park envelope");
            require(std::abs(lot.parcelX)+lot.parcelWidth*.5f<=natural.extent-OpenCityLayout::roadSetback+.001f &&
                std::abs(lot.parcelZ)+lot.parcelDepth*.5f<=natural.extent-OpenCityLayout::roadSetback+.001f,
                "Natural parcel escaped city bounds");
            for(size_t j=0;j<i;++j) {const auto &other=natural.lots[j];
                require(std::abs(lot.parcelX-other.parcelX)>=(lot.parcelWidth+other.parcelWidth)*.5f+.749f ||
                    std::abs(lot.parcelZ-other.parcelZ)>=(lot.parcelDepth+other.parcelDepth)*.5f+.749f,
                    "Natural parcel envelopes overlap or lose their service separation");}
            checkMasses(natural,lot);
        }
        require(natural.route.size()>10 && natural.route[0].x==0 && natural.route[0].z==0 &&
            natural.route[1].x==0 && natural.route[1].z<0,"Natural route reset orientation changed");
        for(size_t i=0;i<natural.route.size();++i) {
            const auto &a=natural.route[i],&b=natural.route[(i+1)%natural.route.size()];
            require(natural.roadDistance(a.x,a.z)<.002f && std::hypot(a.x-b.x,a.z-b.z)<=2.002f,
                "Natural route leaves its graph or fails closure");}
    }
    auto crossing=OpenCityLayout::generate(1);
    crossing.naturalRoads=true;crossing.roads.nodes.clear();crossing.roads.edges.clear();
    crossing.roads.nodes.push_back({-20,-20});crossing.roads.nodes.push_back({20,20});
    crossing.roads.edges.push_back({0,1});
    require(crossing.footprintRoadClearance(0,0,10,10)==0,
        "Footprint query missed a diagonal road crossing between clear corners");
    require(std::abs(crossing.footprintRoadClearance(-10,10,2,2)-std::sqrt(162.f))<.001f,
        "Segment-to-footprint distance is incorrect");
    const auto city=OpenCityLayout::generate(1);
    require(OpenCityLayout{}.roadDistance(1,1)==1000,"Uninitialized city road query divided by zero");
    require(city.roadDistance(std::numeric_limits<float>::quiet_NaN(),0)==1000 &&
        city.roadDistance(0,std::numeric_limits<float>::infinity())==1000,"Non-finite city road query accepted");
    auto invalid=city; invalid.spacing=0;
    require(invalid.roadDistance(0,0)==1000,"Zero-spacing city road query accepted");
    invalid.spacing=std::numeric_limits<float>::quiet_NaN();
    require(invalid.roadDistance(0,0)==1000,"Non-finite city spacing accepted");
    auto badLot=city.lots.front();badLot.park=false;badLot.width=-1;
    require(OpenCityLayout::buildingVolumes(badLot).empty(),"Invalid building footprint accepted");
    badLot.width=10;badLot.height=std::numeric_limits<float>::infinity();
    require(OpenCityLayout::buildingVolumes(badLot).empty(),"Non-finite building height accepted");
    std::cout << "Open city: stable signed parcels/districts, road setbacks, nonoverlap, shared building masses, connected routes: PASS\n";
}
