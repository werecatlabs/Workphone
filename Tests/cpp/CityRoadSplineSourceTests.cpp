#include <Workphone/Interface/Procedural/CityRoadSplineSource.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

using namespace workphone;
using namespace workphone::procedural;
namespace
{
    void require( bool value, const char *message ) { if( !value ) throw std::runtime_error(message); }
    void check( bool value, const String &error ) { if( !value ) throw std::runtime_error(error.c_str()); }
    bool pointsNear( Vector3F a, Vector3F b, float tolerance = .0001f ) { return (a-b).length() <= tolerance; }
    CityRoadSplineSource source( bool drape = false )
    {
        CityRoadSplineSource s;
        s.id="10000000-0000-4000-8000-000000000001";s.revision=17;
        if(drape)s.terrainResourceUuid="20000000-0000-4000-8000-000000000001";
        s.terrainRevision=42;
        CityRoadSpline r;r.id="30000000-0000-4000-8000-000000000001";
        r.width=8;r.maxGrade=.08f;r.heightMode=drape?CityRoadHeightMode::Drape:CityRoadHeightMode::Fixed;
        r.knots.push_back({"40000000-0000-4000-8000-000000000001",{0,0,0},{0,0,0},{0,0,-20}});
        r.knots.push_back({"40000000-0000-4000-8000-000000000002",{30,0,-40},{-20,0,0},{0,0,0}});
        s.roads.push_back(std::move(r));return s;
    }
    SourceBudgetText encode( const CityRoadSplineSource &s )
    { SourceBudgetText json;String error;check(serializeCityRoadSplineSource(s,json,error),error);return json; }
    void sameBake( const CityRoadSplineBake &a, const CityRoadSplineBake &b )
    {
        require(a.sourceRevision==b.sourceRevision && a.roads.size()==b.roads.size(),"Failed bake replaced last-good revision/roads");
        for(size_t i=0;i<a.roads.size();++i)
        {
            require(a.roads[i].roadId==b.roads[i].roadId && a.roads[i].points.size()==b.roads[i].points.size(),"Failed bake changed road identity/sample count");
            for(size_t j=0;j<a.roads[i].points.size();++j)
            {
                const auto &x=a.roads[i].points[j],&y=b.roads[i].points[j];
                require(pointsNear(x.position,y.position,0) && pointsNear(x.tangent,y.tangent,0) && pointsNear(x.left,y.left,0) && pointsNear(x.right,y.right,0) && x.distance==y.distance,
                    "Failed bake modified a last-good sample");
            }
        }
    }
    void codec()
    {
        auto edited=source();edited.roads[0].width=10.25f;edited.roads[0].knots[0].outHandle.x=6.25f;
        edited.roads[0].knots[1].position={37.5f,2.25f,-41};edited.revision++;
        auto json=encode(edited);CityRoadSplineSource loaded;String error;
        check(deserializeCityRoadSplineSource(std::string_view(json.data(),json.size()),loaded,error),error);
        require(encode(loaded)==json,"JSON roundtrip lost manual Bezier/width/identity/revision edits");
        const auto reject=[&](const std::string &bad)
        {
            require(!deserializeCityRoadSplineSource(bad,loaded,error) && !error.empty(),"Malformed source was accepted without a diagnostic");
            require(encode(loaded)==json,"Malformed source replaced last-good source");
        };
        const std::string original(json.data(),json.size());
        reject("{");reject(std::string(1024*1024+1,' '));
        auto bad=original;bad.insert(bad.size()-1,",\"unknownField\":true");reject(bad);
        bad=original;const auto version=bad.find("\"version\":1");require(version!=std::string::npos,"Fixture version field missing");bad.replace(version,11,"\"version\":2");reject(bad);
        bad=original;auto id=bad.find("40000000-0000-4000-8000-000000000002");bad.replace(id,36,"40000000-0000-4000-8000-000000000001");reject(bad);
        bad=original;bad.insert(bad.size()-1,",\"id\":\"10000000-0000-4000-8000-000000000001\"");reject(bad);
        bad=original;bad.insert(1,1,char(0xff));reject(bad);
        bad=original;bad.insert(1,1,'\0');reject(bad);
        bad=original;const auto mode=bad.find("\"Fixed\"");bad.replace(mode,7,"\"Bridge\"");reject(bad);
        bad=std::string(25,'[')+std::string(25,']');reject(bad);
        auto invalid=edited;invalid.roads[0].width=std::numeric_limits<float>::quiet_NaN();
        SourceBudgetText output="retained";require(!serializeCityRoadSplineSource(invalid,output,error) && output=="retained","Failed serialization replaced last-good JSON");
        invalid=edited;invalid.roads[0].knots[0].id=invalid.roads[0].id;
        require(!validateCityRoadSplineSource(invalid,error),"Cross-kind duplicate GUID accepted");
        invalid=edited;invalid.id="00000000-0000-0000-0000-000000000000";
        require(!validateCityRoadSplineSource(invalid,error),"Nil source identity accepted");
        invalid=edited;invalid.roads[0].knots[0].position.x=std::numeric_limits<float>::infinity();
        require(!validateCityRoadSplineSource(invalid,error),"Infinite Bezier position accepted");
    }
    CityRoadSplineSource roundaboutSource()
    {
        auto s=source();s.version=2;s.roads.clear();
        CityRoadRoundaboutSpec junction;
        junction.junctionId="50000000-0000-4000-8000-000000000001";
        junction.center={3,2,-7};junction.clockwise=false;junction.ringSamples=128;
        junction.islandHeight=.22f;junction.kerbWidth=.3f;junction.gradeX=.01f;junction.gradeZ=-.02f;
        junction.maxGrade=.07f;junction.maxTerrainDeviation=.2f;junction.terrainSampleStep=1;
        junction.maximumTerrainSamples=32768;
        for(unsigned i=0;i<3;++i)
        {
            CityRoadSpline road;road.id="30000000-0000-4000-8000-000000000001";road.id.back()=char('1'+i);
            road.width=8;road.heightMode=CityRoadHeightMode::Fixed;
            const float angle=6.283185307179586f*i/3;
            const Vector3F outward{std::cos(angle),0,std::sin(angle)};
            const float length=18+i;
            auto a=junction.center+outward*(junction.outerRadius+length),b=a+outward*24;
            const auto fit=[&](Vector3F &p){p.y=junction.center.y+junction.gradeX*(p.x-junction.center.x)+junction.gradeZ*(p.z-junction.center.z);};
            fit(a);fit(b);const auto tangent=(b-a)/3.f;
            CityRoadSplineKnot first,second;first.id="40000000-0000-4000-8000-000000000001";
            second.id=first.id;first.id.back()=char('1'+i*2);second.id.back()=char('2'+i*2);
            first.position=a;first.outHandle=tangent;second.position=b;second.inHandle=-tangent;
            road.knots={first,second};junction.ports.push_back({road.id,outward,road.width,length});s.roads.push_back(std::move(road));
        }
        s.roundabouts.push_back(std::move(junction));return s;
    }
    void versionTwoCodec()
    {
        auto empty=source();empty.version=2;empty.roads.clear();String error;
        const auto emptyJson=encode(empty);CityRoadSplineSource loaded;
        check(deserializeCityRoadSplineSource(std::string_view(emptyJson.data(),emptyJson.size()),loaded,error),error);
        require(loaded.version==2 && loaded.roads.empty() && loaded.roundabouts.empty() && encode(loaded)==emptyJson,
            "Canonical empty version 2 source cannot be saved/reopened");
        CityRoadSplineBake emptyBake;emptyBake.sourceRevision=999;
        check(sampleCityRoadSplines(loaded,{}, {},emptyBake,error),error);
        require(emptyBake.sourceRevision==empty.revision && emptyBake.roads.empty(),"Empty version 2 retained stale sampled roads");
        const auto authored=roundaboutSource();const auto canonical=encode(authored);
        check(deserializeCityRoadSplineSource(std::string_view(canonical.data(),canonical.size()),loaded,error),error);
        require(encode(loaded)==canonical && loaded.version==2 && loaded.roads.size()==3 && loaded.roundabouts.size()==1,
            "Roundabout source did not roundtrip all version 2 fields");
        const auto &junction=loaded.roundabouts[0];
        require(junction.junctionId==authored.roundabouts[0].junctionId && !junction.clockwise && junction.ringSamples==128 &&
            junction.maximumTerrainSamples==32768 && junction.islandHeight==.22f && junction.kerbWidth==.3f &&
            junction.gradeX==.01f && junction.gradeZ==-.02f && junction.ports.size()==3,
            "Version 2 lost authored junction identity/profile/circulation");
        for(size_t i=0;i<junction.ports.size();++i)
            require(junction.ports[i].approachId==loaded.roads[i].id &&
                junction.ports[i].length==18+i && junction.ports[i].width==loaded.roads[i].width &&
                pointsNear(junction.ports[i].outward,authored.roundabouts[0].ports[i].outward,0),
                "Roundabout port road GUID/profile changed on reopen");
        const auto reject=[&](const std::string &bad)
        {
            require(!deserializeCityRoadSplineSource(bad,loaded,error) && !error.empty(),"Invalid version 2 junction record accepted");
            require(encode(loaded)==canonical,"Invalid version 2 import replaced last-good source");
        };
        const std::string original(canonical.data(),canonical.size());
        auto bad=original;auto field=bad.find("\"version\":2");require(field!=std::string::npos,"Version 2 tag missing");
        bad.replace(field,11,"\"version\":3");reject(bad);
        bad=original;bad.replace(field,11,"\"version\":1");reject(bad); // Version 1 cannot hide/drop junctions.
        bad=original;field=bad.find("\"roundabouts\"");bad.replace(field,13,"\"junctions\"");reject(bad);
        bad=original;field=bad.find("\"center\"");bad.replace(field,8,"\"centre\"");reject(bad);
        bad=original;field=bad.find("\"center\"");bad.insert(field,"\"type\":\"trafficSignals\",");reject(bad);
        bad=original;field=bad.find("\"ports\"");bad.replace(field,7,"\"portz\"");reject(bad);
        bad=original;field=bad.find("\"roadId\":\"30000000-0000-4000-8000-000000000001\"");
        require(field!=std::string::npos,"Roundabout fixture road reference missing");
        bad.replace(field+10,36,"30000000-0000-4000-8000-000000000009");reject(bad);
        bad=original;field=bad.find("\"roadId\":\"30000000-0000-4000-8000-000000000002\"");
        require(field!=std::string::npos,"Second roundabout reference missing");
        bad.replace(field+10,36,"30000000-0000-4000-8000-000000000001");reject(bad);
        bad=original;field=bad.find("\"roadId\"");bad.insert(field,"\"nodeIndex\":17,");reject(bad);
        auto invalid=authored;invalid.roundabouts[0].ports[0].approachId="30000000-0000-4000-8000-000000000009";
        require(!validateCityRoadSplineSource(invalid,error),"Missing source road GUID accepted as a roundabout port");
        invalid=authored;invalid.roundabouts[0].ports[1].approachId=invalid.roundabouts[0].ports[0].approachId;
        require(!validateCityRoadSplineSource(invalid,error),"Duplicate roundabout port road GUID accepted");
        invalid=authored;invalid.roundabouts[0].junctionId=invalid.roads[0].id;
        require(!validateCityRoadSplineSource(invalid,error),"Junction GUID aliased a road GUID");
        invalid=authored;invalid.roundabouts.push_back(invalid.roundabouts[0]);
        require(!validateCityRoadSplineSource(invalid,error),"Duplicate stable junction GUID accepted");
        invalid=authored;invalid.version=1;SourceBudgetText retained="retained";
        require(!serializeCityRoadSplineSource(invalid,retained,error) && retained=="retained","Version 1 serialization silently discarded roundabouts");
    }
    void sampling()
    {
        auto s=source();String error;CityRoadSplineSampling options;options.tolerance=.02f;options.maxStep=.75f;
        CityRoadSplineBake bake;
        check(sampleCityRoadSplines(s,options,{},bake,error),error);
        require(bake.sourceRevision==s.revision && bake.roads.size()==1,"Bake lost source provenance");
        const auto &points=bake.roads[0].points;require(points.size()>50,"Curved road was not subdivided");
        require(pointsNear(points.front().position,s.roads[0].knots.front().position) && pointsNear(points.back().position,s.roads[0].knots.back().position),"Sampling changed spline endpoints");
        for(size_t i=0;i<points.size();++i)
        {
            const auto &p=points[i];require(std::isfinite(p.distance) && std::abs(p.tangent.length()-1)<.0001f,"Nonfinite distance or unnormalized tangent");
            require(std::abs((p.left-p.right).length()-8)<.0001f && pointsNear((p.left+p.right)*.5f,p.position),"Road cross-section width/centre changed");
            if(i)require((p.position-points[i-1].position).length()<=options.maxStep+.0001f && p.distance>points[i-1].distance,"Maximum step or arc-length ordering failed");
        }
        const auto &a=s.roads[0].knots[0],&b=s.roads[0].knots[1];
        for(int i=0;i<=1000;++i)
        {
            const float t=i/1000.f,u=1-t;
            const auto exact=a.position*(u*u*u)+(a.position+a.outHandle)*(3*u*u*t)+(b.position+b.inHandle)*(3*u*t*t)+b.position*(t*t*t);
            float closest=100000;
            for(size_t j=1;j<points.size();++j)
            {
                const auto p=points[j-1].position,d=points[j].position-p;
                const float t2=std::clamp((exact-p).dotProduct(d)/d.lengthSquared(),0.f,1.f);
                closest=std::min(closest,(exact-(p+d*t2)).length());
            }
            require(closest<=options.tolerance+.0001f,"Bezier curve exceeded requested flattening tolerance");
        }
        auto coarse=options;coarse.tolerance=.5f;coarse.maxStep=50;
        CityRoadSplineBake coarseBake;check(sampleCityRoadSplines(s,coarse,{},coarseBake,error),error);
        require(coarseBake.roads[0].points.size()<points.size(),"Tolerance/max-step controls do not affect sampling");
        const auto old=bake;
        const auto reject=[&](const CityRoadSplineSource &candidate,const CityRoadSplineSampling &limits)
        {require(!sampleCityRoadSplines(candidate,limits,{},bake,error) && !error.empty(),"Invalid spline/limits unexpectedly sampled");sameBake(old,bake);};
        auto bad=options;bad.tolerance=std::numeric_limits<float>::quiet_NaN();reject(s,bad);
        bad=options;bad.maxStep=0;reject(s,bad);bad=options;bad.maximumSamples=4;reject(s,bad);
        auto invalid=s;invalid.roads[0].heightMode=CityRoadHeightMode::Bridge;reject(invalid,options);
        invalid=s;invalid.roads[0].knots[1].position.y=20;reject(invalid,options);
        auto folded=source();folded.roads[0].width=12;
        folded.roads[0].knots[0].position={-3,0,0};folded.roads[0].knots[0].outHandle={0,0,-6};
        folded.roads[0].knots[1].position={3,0,0};folded.roads[0].knots[1].inHandle={0,0,-6};reject(folded,options);
        auto closed=source();closed.roads[0].closed=true;const float c=16.568543f;
        closed.roads[0].knots={
            {"40000000-0000-4000-8000-000000000001",{30,0,0},{0,0,c},{0,0,-c}},
            {"40000000-0000-4000-8000-000000000002",{0,0,-30},{c,0,0},{-c,0,0}},
            {"40000000-0000-4000-8000-000000000003",{-30,0,0},{0,0,-c},{0,0,c}},
            {"40000000-0000-4000-8000-000000000004",{0,0,30},{-c,0,0},{c,0,0}}};
        check(sampleCityRoadSplines(closed,options,{},bake,error),error);
        const auto &loop=bake.roads[0].points;
        require(pointsNear(loop.front().position,loop.back().position) && pointsNear(loop.front().left,loop.back().left) && loop.back().distance>100,"Closed curve has a seam or missing closing span");
    }
    void terrain()
    {
        auto s=source(true);s.roads[0].heightOffset=.025f;
        const auto original=encode(s);String error;CityRoadSplineSampling options;CityRoadSplineBake bake;
        const auto plane=[](const Vector3F &p,float &y){y=8+.02f*p.x-.03f*p.z;return true;};
        check(sampleCityRoadSplines(s,options,plane,bake,error),error);
        for(const auto &p:bake.roads[0].points)for(const auto q:{p.position,p.left,p.right})
        {float expected=0;plane(q,expected);require(std::abs(q.y-expected-.025f)<.0001f,"Drape did not fit centre and both edges to sloped terrain");}
        require(encode(s)==original,"Terrain fitting modified authored controls");
        const auto old=bake;
        const auto reject=[&](const CityRoadHeightSampler &sampler)
        {require(!sampleCityRoadSplines(s,options,sampler,bake,error) && !error.empty(),"Unsupported terrain profile sampled successfully");sameBake(old,bake);};
        reject({});
        reject([](const Vector3F &p,float &y){y=0;return p.z>=-20;});
        reject([](const Vector3F &p,float &y){y=0;return p.x>=-1;}); // Centre is valid; one road edge leaves the terrain.
        reject([](const Vector3F &,float &y){y=std::numeric_limits<float>::quiet_NaN();return true;});
        reject([](const Vector3F &p,float &y){y=.5f*p.x+.5f*p.z;return true;});
        reject([](const Vector3F &p,float &y){y=p.z< -20?2.f:0.f;return true;}); // Terrain step/hole boundary.
        auto ridge=source(true);ridge.roads[0].width=12;
        ridge.roads[0].knots[0].outHandle={0,0,-10};ridge.roads[0].knots[1].position={0,0,-30};ridge.roads[0].knots[1].inHandle={0,0,10};
        require(!sampleCityRoadSplines(ridge,options,[](const Vector3F &p,float &y){y=std::abs(p.x)<.1f?1.f:0.f;return true;},bake,error),
            "Equal-height road edges concealed excessive centre-to-edge crossfall");sameBake(old,bake);
        auto fixed=source();auto &r=fixed.roads[0];r.maxGrade=.12f;r.heightOffset=.2f;
        r.knots[0].position={0,0,0};r.knots[0].outHandle={10,1,0};r.knots[1].position={30,3,0};r.knots[1].inHandle={-10,-1,0};
        check(sampleCityRoadSplines(fixed,options,[](const Vector3F &,float &){return false;},bake,error),error);
        for(const auto &p:bake.roads[0].points)require(std::abs(p.position.y-.1f*p.position.x-.2f)<.0001f,"Fixed road ignored its authored vertical profile");
    }
    void budgetDenials()
    {
        const auto scheduler=WorldStreamingScheduler::shared();constexpr auto sourceClass=size_t(StreamMemoryClass::SourceCpu);
        struct Restore
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler;u64 limit;
            ~Restore(){String error;scheduler->setBudget(StreamMemoryClass::SourceCpu,limit,error);}
        } restore{scheduler,scheduler->stats().limits[sourceClass]};
        auto loaded=source(true);const auto original=encode(loaded);String error;
        const auto baseline=scheduler->stats().used[sourceClass];unsigned failures=0;bool admitted=false;
        for(u64 additional=0;additional<=262144;additional+=64)
        {
            check(scheduler->setBudget(StreamMemoryClass::SourceCpu,baseline+additional,error),error);
            const bool ok=deserializeCityRoadSplineSource(std::string_view(original.data(),original.size()),loaded,error);
            const auto resident=scheduler->stats().used[sourceClass];
            check(scheduler->setBudget(StreamMemoryClass::SourceCpu,restore.limit,error),error);
            require(resident==baseline,"RapidJSON allocation-denial path leaked SourceCpu storage");
            require(encode(loaded)==original,"Allocation-denied import lost last-good source");
            if(ok){admitted=true;break;}++failures;
        }
        require(admitted && failures>10,"SourceCpu denial sweep did not exercise staged parser allocation failures");
    }
    void fixture( const char *path )
    {
        std::ifstream file(path,std::ios::binary);require(bool(file),"Version 1 authoring fixture could not be opened");
        const std::string bytes((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
        CityRoadSplineSource s;String error;check(deserializeCityRoadSplineSource(bytes,s,error),error);
        require(s.roads.size()==1 && s.roads[0].width==12 && s.roads[0].heightMode==CityRoadHeightMode::Drape &&
            s.roads[0].knots.size()==2 && s.roads[0].knots[1].inHandle.x==-8,"Version 1 editable fixture lost authored controls");
        const auto retained=encode(s);CityRoadSplineSource reopened;
        check(deserializeCityRoadSplineSource(std::string_view(retained.data(),retained.size()),reopened,error),error);
        require(encode(reopened)==retained,"Version 1 fixture failed save/reopen roundtrip");
    }
}
int main( int argc, char **argv )
{
    int result=0;
    try {codec();versionTwoCodec();sampling();terrain();budgetDenials();if(argc>1)fixture(argv[1]);std::cout<<"City road spline source: v1/v2 codec, roundabout GUID references, Bezier, terrain, allocation denials and transactional failures PASS\n";}
    catch(const std::exception &e){std::cerr<<"City road spline source: "<<e.what()<<'\n';result=1;}
    WorldStreamingScheduler::shutdownShared();return result;
}
