#include <Workphone/Interface/Procedural/OpenCityRoadNetwork.hpp>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <vector>
using workphone::procedural::OpenCityRoadNetwork;
static void require(bool value,const char *message)
{if(!value)throw std::runtime_error(message);}
static void sameGraph(const OpenCityRoadNetwork &a,const OpenCityRoadNetwork &b)
{
    require(a.blocks==b.blocks&&a.spacing==b.spacing&&a.extent==b.extent&&
        a.nodes.size()==b.nodes.size()&&a.edges.size()==b.edges.size(),"Natural graph changed on replay/route selection");
    for(size_t i=0;i<a.nodes.size();++i)require(a.nodes[i].x==b.nodes[i].x&&a.nodes[i].z==b.nodes[i].z,
        "Natural node changed on replay/route selection");
    for(size_t i=0;i<a.edges.size();++i)require(a.edges[i].a==b.edges[i].a&&a.edges[i].b==b.edges[i].b&&
        a.edges[i].roadClass==b.edges[i].roadClass,
        "Natural edge or road hierarchy changed on replay/route selection");
}
static void hierarchyContracts(const OpenCityRoadNetwork &graph)
{
    using RoadClass=OpenCityRoadNetwork::RoadClass;
    require(OpenCityRoadNetwork::roadWidth(RoadClass::Arterial)>
        OpenCityRoadNetwork::roadWidth(RoadClass::Collector)&&
        OpenCityRoadNetwork::roadWidth(RoadClass::Collector)>
        OpenCityRoadNetwork::roadWidth(RoadClass::Local),
        "Main roads, collectors and local streets lost their width hierarchy");
    require(OpenCityRoadNetwork::roadWidth(RoadClass::Local)>=4&&
        OpenCityRoadNetwork::roadWidth(RoadClass::Arterial)<=12,
        "Road hierarchy escaped the supported editable-road width range");
    std::vector<unsigned> arterialDegree(graph.nodes.size(),0);
    unsigned locals=0,collectors=0,arterials=0;
    for(const auto &edge:graph.edges)
    {
        switch(edge.roadClass)
        {
        case RoadClass::Local: ++locals; break;
        case RoadClass::Collector: ++collectors; break;
        case RoadClass::Arterial:
            ++arterials; ++arterialDegree[edge.a]; ++arterialDegree[edge.b]; break;
        default: require(false,"Natural road contains an invalid road class");
        }
    }
    require(locals>0&&collectors>0&&arterials>0,"City lost its primary or neighbourhood street network");
    size_t centre=graph.nodes.size();
    for(size_t i=0;i<graph.nodes.size();++i)
        if(graph.nodes[i].x==0&&graph.nodes[i].z==0)centre=i;
    require(centre<graph.nodes.size()&&arterialDegree[centre]==4,
        "City centre does not connect to all four main-road approaches");

    // Shortest paths on arterial edges alone prove that the fast approaches
    // really reach the centre, rather than acquiring their class from a nearby
    // street or depending on a detour through smaller neighbourhood roads.
    std::vector<double> cost(graph.nodes.size(),std::numeric_limits<double>::infinity());
    std::vector<unsigned char> settled(graph.nodes.size(),0); cost[centre]=0;
    for(size_t iteration=0;iteration<graph.nodes.size();++iteration)
    {
        size_t at=graph.nodes.size();
        for(size_t i=0;i<graph.nodes.size();++i)
            if(!settled[i]&&std::isfinite(cost[i])&&(at==graph.nodes.size()||cost[i]<cost[at]))at=i;
        if(at==graph.nodes.size())break;
        settled[at]=1;
        for(const auto &edge:graph.edges)
        {
            if(edge.roadClass!=RoadClass::Arterial||(edge.a!=at&&edge.b!=at))continue;
            const auto next=edge.a==at?edge.b:edge.a;
            const auto a=graph.nodes[at],b=graph.nodes[next];
            cost[next]=std::min(cost[next],cost[at]+std::hypot(double(b.x)-a.x,double(b.z)-a.z));
        }
    }
    for(size_t i=0;i<graph.nodes.size();++i)if(arterialDegree[i])
    {
        require(std::isfinite(cost[i]),"A main road is disconnected from the city centre");
        const auto point=graph.nodes[i];
        const double direct=std::hypot(point.x,point.z);
        require(cost[i]<=direct*1.05+.01,"A main-road approach takes an indirect detour into the centre");
        if(arterialDegree[i]==1)
            require(std::max(std::abs(point.x),std::abs(point.z))>=graph.extent*.8f,
                "A main-road approach terminates inside a neighbourhood");
    }
    for(const auto direction:{OpenCityRoadNetwork::Point{1,0},{-1,0},{0,1},{0,-1}})
    {
        double reach=0;
        for(size_t i=0;i<graph.nodes.size();++i)if(arterialDegree[i])
            reach=std::max(reach,double(graph.nodes[i].x)*direction.x+double(graph.nodes[i].z)*direction.z);
        require(reach>=graph.extent*.8,"A cardinal city entrance has no direct main road into the centre");
    }
}
static void graphContracts(const OpenCityRoadNetwork &graph)
{
    require(!graph.nodes.empty()&&!graph.edges.empty(),"Natural city graph is empty");
    require(graph.nodes.size()<=OpenCityRoadNetwork::maximumNodes&&
        graph.edges.size()<=OpenCityRoadNetwork::maximumEdges,"Natural graph exceeded bounded geometry budget");
    // Every edge may need a separate editable spline after junction trimming;
    // leave room inside the canonical source's 128-road limit without relying
    // on an incidental collinear merge to rescue a particular seed.
    require(graph.edges.size()<=120,"Natural graph exceeded its editable-road headroom");
    for(const auto &edge:graph.edges)
    {
        require(edge.a<graph.nodes.size()&&edge.b<graph.nodes.size()&&edge.a!=edge.b,
            "Natural edge has invalid endpoints");
        const auto a=graph.nodes[edge.a],b=graph.nodes[edge.b];
        require(std::hypot(b.x-a.x,b.z-a.z)>=OpenCityRoadNetwork::minimumEdgeLength-.002f,
            "Natural graph generated a tiny unsafe junction edge");
        require(std::hypot(b.x-a.x,b.z-a.z)>=24.f-.002f,
            "Natural road junctions leave less than 24 m of usable street");
    }
    std::vector<unsigned char> visited(graph.nodes.size(),0);
    std::vector<size_t> pending{0};visited[0]=1;
    for(size_t head=0;head<pending.size();++head)for(const auto &edge:graph.edges)
    {
        size_t other=graph.nodes.size();
        if(edge.a==pending[head])other=edge.b;else if(edge.b==pending[head])other=edge.a;
        if(other<visited.size()&&!visited[other]){visited[other]=1;pending.push_back(other);}
    }
    require(pending.size()==graph.nodes.size(),"Natural road grammar created disconnected/isolated geometry");
    require(graph.edges.size()>=graph.nodes.size()+2,"Natural city lost its three connected circuit choices");
    size_t arbitrary=0,junctions=0;
    std::vector<unsigned> degree(graph.nodes.size(),0);
    for(const auto &point:graph.nodes) require(std::isfinite(point.x)&&std::isfinite(point.z)&&
        std::abs(point.x)<=graph.extent-12+.002f&&std::abs(point.z)<=graph.extent-12+.002f,
        "Natural road point escaped finite bounded city footprint");
    for(size_t i=0;i<graph.edges.size();++i)
    {
        const auto &edge=graph.edges[i];++degree[edge.a];++degree[edge.b];
        const auto a=graph.nodes[edge.a],b=graph.nodes[edge.b];
        const double rx=b.x-a.x,rz=b.z-a.z,length=std::hypot(rx,rz);
        if(std::abs(rx)>.1&&std::abs(rz)>.1)++arbitrary;
        for(size_t j=0;j<i;++j)
        {
            const auto &other=graph.edges[j];const auto c=graph.nodes[other.a],d=graph.nodes[other.b];
            require(!(edge.a==other.a&&edge.b==other.b)&&!(edge.a==other.b&&edge.b==other.a),
                "Natural road graph emitted duplicate edges");
            const bool shared=edge.a==other.a||edge.a==other.b||edge.b==other.a||edge.b==other.b;
            if(!shared)
            {
                const auto pointSegment=[](OpenCityRoadNetwork::Point p,OpenCityRoadNetwork::Point from,
                    OpenCityRoadNetwork::Point to) {
                    const double dx=double(to.x)-from.x,dz=double(to.z)-from.z;
                    const double t=std::clamp(((double(p.x)-from.x)*dx+(double(p.z)-from.z)*dz)/(dx*dx+dz*dz),0.,1.);
                    return std::hypot(double(p.x)-from.x-t*dx,double(p.z)-from.z-t*dz);
                };
                const double clearance=std::min({pointSegment(a,c,d),pointSegment(b,c,d),
                    pointSegment(c,a,b),pointSegment(d,a,b)});
                require(clearance>=18.-.005,"Unrelated natural streets overlap their carriageway and footway corridors");
            }
            const double sx=d.x-c.x,sz=d.z-c.z,denominator=rx*sz-rz*sx;
            if(std::abs(denominator)<1.e-7)
            {
                if(std::abs((c.x-a.x)*rz-(c.z-a.z)*rx)>.001*length)continue;
                const double tc=((c.x-a.x)*rx+(c.z-a.z)*rz)/(length*length);
                const double td=((d.x-a.x)*rx+(d.z-a.z)*rz)/(length*length);
                const double overlap=std::min(1.,std::max(tc,td))-std::max(0.,std::min(tc,td));
                require(overlap*length<.005,"Natural road graph has collinear overlapping streets");
                continue;
            }
            const double t=((c.x-a.x)*sz-(c.z-a.z)*sx)/denominator;
            const double u=((c.x-a.x)*rz-(c.z-a.z)*rx)/denominator;
            require(shared||t<-1.e-6||t>1+1.e-6||u<-1.e-6||u>1+1.e-6,
                "Natural roads cross without a split shared junction");
        }
    }
    for(size_t node=0;node<graph.nodes.size();++node)
    {
        require(degree[node]<=4,"Natural road growth created an implausible multiway junction");
        if(degree[node]>=3)++junctions;
        std::vector<OpenCityRoadNetwork::Point> incident;
        const auto center=graph.nodes[node];
        for(const auto &edge:graph.edges)
        {
            if(edge.a!=node&&edge.b!=node)continue;
            const auto other=graph.nodes[edge.a==node?edge.b:edge.a];
            const OpenCityRoadNetwork::Point direction{other.x-center.x,other.z-center.z};
            for(const auto previous:incident)
            {
                const double dot=(double(direction.x)*previous.x+double(direction.z)*previous.z)/
                    (std::hypot(direction.x,direction.z)*std::hypot(previous.x,previous.z));
                // Opposite continuations are valid; acute adjacent arms create
                // asphalt wedges and leave no room for sidewalks/frontages.
                require(dot<=.7071067812+.00001,"Natural road junction has an angle below 45 degrees");
            }
            incident.push_back(direction);
        }
    }
    require(arbitrary>12&&junctions>5,"Natural network lost branching/arbitrary-heading topology");
}
static void seedContracts(unsigned seed,int blocks,bool report)
{
    try
    {
        const auto graph=OpenCityRoadNetwork::generate(seed,blocks,0);
        graphContracts(graph);
        hierarchyContracts(graph);
        require(graph.route.size()>2&&graph.route[0].x==0&&graph.route[0].z==0&&
            graph.route[1].x==0&&graph.route[1].z<0,"Natural route lost the origin/-Z start");
        const auto repeated=OpenCityRoadNetwork::generate(seed,blocks,0);
        sameGraph(graph,repeated);
        float previousLength=0;
        for(int route=0;route<3;++route)
        {
            const auto candidate=OpenCityRoadNetwork::generate(seed,blocks,route);
            sameGraph(graph,candidate);
            require(candidate.route.size()>2&&candidate.route.front().x==0&&candidate.route.front().z==0&&
                candidate.route[1].x==0&&candidate.route[1].z<0,"Natural route choice changed spawn heading");
            float length=0;
            for(size_t i=0;i<candidate.route.size();++i)
            {
                const auto a=candidate.route[i],b=candidate.route[(i+1)%candidate.route.size()];
                require(candidate.distance(a.x,a.z)<.003f,"Natural selected route left graph edges");
                const auto step=std::hypot(b.x-a.x,b.z-a.z);length+=step;
                require(step>0&&step<=2.002f,"Natural route choice broke closed sampling");
            }
            require(length>previousLength,"Natural route choices lost increasing closed circuits");
            previousLength=length;
        }
        if(report) std::cout<<"Natural city seed "<<seed<<", blocks "<<blocks<<": "<<graph.nodes.size()
            <<" nodes, "<<graph.edges.size()<<" edges\n";
    }
    catch(const std::exception &error)
    {
        std::cerr<<"Natural city regression: seed="<<seed<<", blocks="<<blocks<<": "<<error.what()<<'\n';
        throw;
    }
}
int main()
{
    for(unsigned seed:{0u,7u,42u,4294967295u}) for(int blocks:{6,8,10})
        seedContracts(seed,blocks,true);
    // Consecutive seeds cover nearby authored settings; a full-width sequence
    // exercises different grammar branches and snapping decisions, including
    // layouts whose street spacing is identical but whose seed is different.
    for(unsigned i=0;i<32;++i) for(int blocks:{6,8,10})
    {
        seedContracts(i+1,blocks,false);
        seedContracts(0x9e3779b9u*(i+1),blocks,false);
    }
    std::cout<<"Natural topology corpus: 204 seed/size cases, all three route choices: PASS\n";
    const auto a=OpenCityRoadNetwork::generate(0,8),b=OpenCityRoadNetwork::generate(17,8);
    require(a.spacing==b.spacing,"Seed-variety fixture no longer isolates topology from city scale");
    bool changed=a.nodes.size()!=b.nodes.size()||a.edges.size()!=b.edges.size();
    for(size_t i=0;i<std::min(a.nodes.size(),b.nodes.size());++i)
        changed=changed||a.nodes[i].x!=b.nodes[i].x||a.nodes[i].z!=b.nodes[i].z;
    for(size_t i=0;i<std::min(a.edges.size(),b.edges.size());++i)
        changed=changed||a.edges[i].a!=b.edges[i].a||a.edges[i].b!=b.edges[i].b;
    require(changed,"Natural road seed only scales an otherwise identical layout");
    require(OpenCityRoadNetwork{}.distance(0,0)==1000&&
        a.distance(std::numeric_limits<float>::quiet_NaN(),0)==1000&&
        a.distance(0,std::numeric_limits<float>::infinity())==1000,"Natural road query accepted invalid geometry");
    std::cout<<"Bounded seeded L-system city road network: PASS\n";
}
