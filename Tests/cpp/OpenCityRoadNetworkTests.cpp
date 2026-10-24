#include <Workphone/Interface/Procedural/OpenCityRoadNetwork.hpp>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <limits>
using workphone::procedural::OpenCityRoadNetwork;
static void require(bool value,const char *message)
{if(!value)throw std::runtime_error(message);}
static void sameGraph(const OpenCityRoadNetwork &a,const OpenCityRoadNetwork &b)
{
    require(a.blocks==b.blocks&&a.spacing==b.spacing&&a.extent==b.extent&&
        a.nodes.size()==b.nodes.size()&&a.edges.size()==b.edges.size(),"Natural graph changed on replay/route selection");
    for(size_t i=0;i<a.nodes.size();++i)require(a.nodes[i].x==b.nodes[i].x&&a.nodes[i].z==b.nodes[i].z,
        "Natural node changed on replay/route selection");
    for(size_t i=0;i<a.edges.size();++i)require(a.edges[i].a==b.edges[i].a&&a.edges[i].b==b.edges[i].b,
        "Natural edge changed on replay/route selection");
}
static void graphContracts(const OpenCityRoadNetwork &graph)
{
    std::vector<unsigned char> visited(graph.nodes.size(),0);
    std::vector<size_t> pending{0};visited[0]=1;
    for(size_t head=0;head<pending.size();++head)for(const auto &edge:graph.edges)
    {
        size_t other=graph.nodes.size();
        if(edge.a==pending[head])other=edge.b;else if(edge.b==pending[head])other=edge.a;
        if(other<visited.size()&&!visited[other]){visited[other]=1;pending.push_back(other);}
    }
    require(pending.size()==graph.nodes.size(),"Natural road grammar created disconnected/isolated geometry");
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
            const bool shared=edge.a==other.a||edge.a==other.b||edge.b==other.a||edge.b==other.b;
            require(shared||t<-1.e-6||t>1+1.e-6||u<-1.e-6||u>1+1.e-6,
                "Natural roads cross without a split shared junction");
        }
    }
    for(auto value:degree)if(value>=3)++junctions;
    require(arbitrary>12&&junctions>5,"Natural network lost branching/arbitrary-heading topology");
}
int main()
{
    for(unsigned seed:{0u,7u,42u,4294967295u}) for(int blocks:{6,8,10})
    {
        const auto graph=OpenCityRoadNetwork::generate(seed,blocks,0);
        require(!graph.nodes.empty()&&!graph.edges.empty(),"Natural city graph is empty");
        require(graph.nodes.size()<=OpenCityRoadNetwork::maximumNodes&&
            graph.edges.size()<=OpenCityRoadNetwork::maximumEdges,"Natural graph exceeded bounded geometry budget");
        require(graph.route.size()>2&&graph.route[0].x==0&&graph.route[0].z==0&&
            graph.route[1].x==0&&graph.route[1].z<0,"Natural route lost the origin/-Z start");
        for(const auto &edge:graph.edges)
        {
            require(edge.a<graph.nodes.size()&&edge.b<graph.nodes.size()&&edge.a!=edge.b,"Natural edge has invalid endpoints");
            const auto a=graph.nodes[edge.a],b=graph.nodes[edge.b];
            require(std::hypot(b.x-a.x,b.z-a.z)>=OpenCityRoadNetwork::minimumEdgeLength-.002f,
                "Natural graph generated a tiny unsafe junction edge");
        }
        graphContracts(graph);
        const auto repeated=OpenCityRoadNetwork::generate(seed,blocks,0);
        sameGraph(graph,repeated);
        float previousLength=0;
        for(int route=0;route<3;++route)
        {
            const auto candidate=OpenCityRoadNetwork::generate(seed,blocks,route);
            sameGraph(graph,candidate);
            require(candidate.route.front().x==0&&candidate.route.front().z==0&&
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
        for(size_t i=0;i<graph.route.size();++i)
        {
            const auto a=graph.route[i],b=graph.route[(i+1)%graph.route.size()];
            require(graph.distance(a.x,a.z)<.003f,"Natural route left the connected road graph");
            const float step=std::hypot(b.x-a.x,b.z-a.z);
            require(step>0&&step<=2.002f,"Natural route is not a closed densely sampled loop");
        }
        std::cout<<"Natural city seed "<<seed<<", blocks "<<blocks<<": "<<graph.nodes.size()
            <<" nodes, "<<graph.edges.size()<<" edges\n";
    }
    const auto a=OpenCityRoadNetwork::generate(0,8),b=OpenCityRoadNetwork::generate(1,8);
    require(a.nodes[1].z!=b.nodes[1].z,"Natural road seed does not change geometry");
    require(OpenCityRoadNetwork{}.distance(0,0)==1000&&
        a.distance(std::numeric_limits<float>::quiet_NaN(),0)==1000&&
        a.distance(0,std::numeric_limits<float>::infinity())==1000,"Natural road query accepted invalid geometry");
    std::cout<<"Bounded seeded L-system city road network: PASS\n";
}
