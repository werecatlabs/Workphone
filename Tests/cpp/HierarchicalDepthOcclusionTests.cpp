#include <Workphone/Graphics/HierarchicalDepthOcclusion.hpp>
#include <cstdio>
#include <limits>
#include <stdexcept>

using workphone::render::HierarchicalDepthOcclusion;
#define CHECK(value) do { if(!(value)) throw std::runtime_error(#value); } while(0)

int main()
{
    try
    {
        const HierarchicalDepthOcclusion::Matrix identity{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
        const float behindMin[] = { -.1f,-.1f,.6f }, behindMax[] = { .1f,.1f,.8f };
        const float frontMin[] = { -.1f,-.1f,-.8f }, frontMax[] = { .1f,.1f,-.6f };
        std::vector<float> depth(64*64,.2f);
        HierarchicalDepthOcclusion history;
        CHECK(history.publish(depth.data(),64,64,identity));
        CHECK(history.occluded(behindMin,behindMax,identity,identity));
        CHECK(!history.occluded(frontMin,frontMax,identity,identity));
        depth[32*64+32]=1;
        CHECK(history.publish(depth.data(),64,64,identity));
        CHECK(!history.occluded(behindMin,behindMax,identity,identity));
        depth[32*64+32]=std::numeric_limits<float>::quiet_NaN();
        CHECK(history.publish(depth.data(),64,64,identity));
        CHECK(!history.occluded(behindMin,behindMax,identity,identity));
        depth.assign(64*64,.2f);
        CHECK(history.publish(depth.data(),64,64,identity));
        auto movedCamera=identity; movedCamera[3]=.001f;
        CHECK(!history.occluded(behindMin,behindMax,identity,movedCamera));
        auto movedObject=identity; movedObject[11]=-1.4f;
        CHECK(!history.occluded(behindMin,behindMax,movedObject,identity));
        const float crossingNear[] = {-.1f,-.1f,-2};
        CHECK(!history.occluded(crossingNear,behindMax,identity,identity));
        const float crossingViewport[] = {-1.1f,-.1f,.6f};
        CHECK(!history.occluded(crossingViewport,behindMax,identity,identity));
        auto singular=identity; singular[15]=0;
        CHECK(!history.occluded(behindMin,behindMax,singular,identity));
        depth.assign(63*47,.2f);
        CHECK(history.publish(depth.data(),63,47,identity));
        CHECK(history.occluded(behindMin,behindMax,identity,identity));
        depth.assign(35*27,1);
        CHECK(history.publish(depth.data(),35,27,identity));
        CHECK(!history.occluded(behindMin,behindMax,identity,identity));
        CHECK(!history.publish(nullptr,35,27,identity));
        CHECK(!history.occluded(behindMin,behindMax,identity,identity));
        std::puts("Hierarchical depth: hidden/front/holes/invalid depth, projection, camera, resize and near-plane tests passed.");
        return 0;
    }
    catch(const std::exception &error)
    {
        std::fprintf(stderr,"%s\n",error.what()); return 1;
    }
}
