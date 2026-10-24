#pragma once
#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/MeshPose.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Animation/AnimationVertexTrack.hpp>
#include <Workphone/Animation/AnimationMorphKeyFrame.hpp>
#include <Workphone/Animation/AnimationPoseKeyFrame.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <optional>
#include <stdexcept>

namespace pose_vertex_payload_budget_contracts
{
    using namespace workphone;
    inline void require(bool condition, const char *message)
    { if(!condition) throw std::runtime_error(message); }
    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr size_t source = size_t(StreamMemoryClass::SourceCpu);
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set(u64 bytes)
        {
            String error;
            require(scheduler->setBudget(StreamMemoryClass::SourceCpu, bytes, error), "Pose/vertex payload capacity applies");
        }
        void restore() { set(original); }
        ~Capacity() { restore(); }
    };
    template<class F> void denied(F &&function, const char *message)
    {
        bool rejected = false;
        try { function(); } catch(const std::bad_alloc &) { rejected = true; }
        require(rejected, message);
    }
    template<class T> u64 arrayBytes(size_t capacity)
    { return sourceBudgetStorageBytes(capacity * sizeof(T), alignof(T)); }
    inline SmartPtr<IVertexBuffer> geometry()
    {
        auto declaration = make_ptr<VertexDeclaration>();
        declaration->addElement(0, 0, VertexElementSemantic::VES_POSITION, VertexElementType::VET_FLOAT3);
        auto vertices = make_ptr<VertexBuffer>();
        vertices->setVertexDeclaration(declaration);
        vertices->setNumVertices(3);
        auto data = static_cast<float *>(vertices->createVertexData());
        for(unsigned i = 0; i < 9; ++i) data[i] = float(i);
        return vertices;
    }
    inline void mapPayloads(Capacity &budget)
    {
        const auto baseline = budget.used();
        auto pose = make_ptr<MeshPose>(2, "pose-budget");
        const auto empty = budget.used();
        require(empty > baseline && pose->getVertexOffsets().empty() && pose->getNormalOffsets().empty(),
                "Empty pose body and Source unordered-map sentinel/bucket metadata are admitted");
        pose->addVertex(7, Vector3F(1, 2, 3));
        const auto resident = budget.used();
        const auto positions = &pose->getVertexOffsets().at(7);
        const auto buckets = pose->getVertexOffsets().bucket_count();
        budget.set(resident);
        denied([&] { pose->addVertex(7, Vector3F(4, 5, 6), Vector3F(0, 1, 0)); },
               "Paired insertion admits missing normal metadata before changing existing position values");
        require(budget.used() == resident && !pose->getIncludesNormals() && pose->getNormalOffsets().empty() &&
                    &pose->getVertexOffsets().at(7) == positions && pose->getVertexOffsets().at(7).x == 1 &&
                    pose->getVertexOffsets().bucket_count() == buckets,
                "Denied paired map update preserves values, flags, nodes and bucket capacity");
        denied([&] { auto snapshot = pose->getVertexOffsets(); }, "Escaped pose map copy cannot bypass shared admission");
        budget.restore();
        pose->addVertex(7, Vector3F(4, 5, 6), Vector3F(0, 1, 0));
        const auto paired = budget.used();
        budget.set(paired);
        const auto denials = sourceBudgetDenialCount();
        pose->setIncludesNormals(false);
        pose->addVertex(7, Vector3F(8, 9, 10), Vector3F(1, 0, 0));
        require(sourceBudgetDenialCount() == denials && budget.used() == paired && pose->getIncludesNormals() &&
                    pose->getVertexOffsets().at(7).y == 9 && pose->getNormalOffsets().at(7).x == 1,
                "Existing paired map values update without admission at the cap");
        budget.restore();
        std::optional<MeshPose::VertexOffsetMap> retained;
        retained.emplace(pose->getVertexOffsets());
        retained->reserve(127);
        const auto spareBuckets = retained->bucket_count();
        const auto beforeUnload = budget.used();
        budget.set(beforeUnload);
        const auto noAdmission = sourceBudgetDenialCount();
        pose->unload(nullptr);
        require(sourceBudgetDenialCount() == noAdmission && pose->getNumVertexOffsets() == 0 &&
                    pose->getNormalOffsets().empty() && !pose->getIncludesNormals() &&
                    pose->getVertexOffsets().bucket_count() == buckets && budget.used() < beforeUnload &&
                    budget.used() > baseline && retained->at(7).y == 9 && retained->bucket_count() == spareBuckets,
                "Pose unload frees nodes at capacity while live map buckets and detached caller spare remain charged");
        const auto liveEmpty = budget.used();
        pose = nullptr;
        require(sourceBudgetDenialCount() == noAdmission && budget.used() < liveEmpty && retained->at(7).z == 10,
                "Pose destruction releases retained map skeletons while detached caller data survives");
        retained.reset();
        require(budget.used() == baseline, "Final detached pose map release retires its nodes and spare buckets");
        budget.restore();

        {
            auto independent = make_ptr<MeshPose>();
            const auto emptyMaps = budget.used();
            independent->addVertex(7, Vector3F(1, 2, 3));
            const auto nodeBytes = budget.used() - emptyMaps;
            const auto used = budget.used();
            const auto oldNode = &independent->getVertexOffsets().at(7);
            const auto positionBuckets = independent->getVertexOffsets().bucket_count();
            const auto normalBuckets = independent->getNormalOffsets().bucket_count();
            budget.set(used + nodeBytes * 2 - 1);
            denied([&] { independent->addVertex(11, Vector3F(4, 5, 6), Vector3F(0, 1, 0)); },
                   "One-byte-short second independent node admission rolls back the first new position node");
            require(budget.used() == used && independent->getNumVertexOffsets() == 1 &&
                        independent->getNormalOffsets().empty() && !independent->getIncludesNormals() &&
                        &independent->getVertexOffsets().at(7) == oldNode,
                    "Independent-node rollback preserves old nodes, values and exact physical residency");
            budget.set(used + nodeBytes * 2);
            independent->addVertex(11, Vector3F(4, 5, 6), Vector3F(0, 1, 0));
            require(budget.used() == used + nodeBytes * 2 &&
                        &independent->getVertexOffsets().at(7) == oldNode &&
                        independent->getVertexOffsets().bucket_count() == positionBuckets &&
                        independent->getNormalOffsets().bucket_count() == normalBuckets,
                    "No-rehash paired insertion admits exactly two nodes without copying old maps or buckets");
            budget.restore();
        }
        unsigned rejected = 0, accepted = 0;
        for(u64 headroom : {0ULL, 32ULL, 64ULL, 128ULL, 256ULL, 512ULL, 1024ULL, 2048ULL, 4096ULL, 8192ULL})
        {
            auto candidate = make_ptr<MeshPose>();
            // Fill the existing bucket allowance, forcing transactional table
            // staging instead of the independent-node insertion fast path.
            const auto count = candidate->getVertexOffsets().bucket_count();
            for(u32 i = 0; i < count; ++i) candidate->addVertex(i, Vector3F(float(i), 2, 3));
            const auto used = budget.used();
            const auto pointer = &candidate->getVertexOffsets().at(0);
            budget.set(used + headroom);
            try
            {
                candidate->addVertex(100000, Vector3F(4, 5, 6), Vector3F(0, 1, 0));
                ++accepted;
                require(candidate->getNumVertexOffsets() == count + 1 && candidate->getNormalOffsets().size() == 1 &&
                            candidate->getIncludesNormals(), "Admitted paired pose maps publish complete values");
            }
            catch(const std::bad_alloc &)
            {
                ++rejected;
                require(budget.used() == used && candidate->getNumVertexOffsets() == count &&
                            candidate->getNormalOffsets().empty() && !candidate->getIncludesNormals() &&
                            &candidate->getVertexOffsets().at(0) == pointer,
                        "Every denied map copy/node/bucket stage unwinds without partial paired publication");
            }
            budget.restore();
        }
        require(rejected >= 4 && accepted > 0, "Full-bucket paired pose sweep covers staged metadata denial and complete retry");
        auto full = make_ptr<MeshPose>();
        for(u32 i = 0; i < 20; ++i) full->addVertex(i, Vector3F(float(i), 0, 0));
        const auto used = budget.used();
        budget.set(used);
        denied([&] { full->clone(); }, "Pose clone body/map staging cannot bypass the budget");
        require(full->getNumVertexOffsets() == 20 && budget.used() == used,
                "Denied pose cloning preserves the complete original node population");
        budget.restore();
        auto clone = dynamic_pointer_cast<MeshPose>(full->clone());
        require(clone && clone->getVertexOffsets().at(19).x == 19, "Pose clone retries complete admitted map payloads");
    }
    inline void poseReferences(Capacity &budget)
    {
        auto key = make_ptr<AnimationPoseKeyFrame>();
        const auto resident = budget.used();
        struct Reference { u16 pose; f32 influence; };
        key->setNumPoses(3);
        const auto bytes = arrayBytes<Reference>(4);
        require(budget.used() == resident + bytes && key->getNumPoses() == 3,
                "Pose-reference payload charges its physical four-slot growth including spare capacity");
        key->setPoseReference(0, 7, 1.5f);
        key->setPoseReference(1, 8, -1);
        key->setNumPoses(2);
        budget.set(budget.used());
        denied([&] { key->setPoseReference(8, 9, 0.5f); }, "Denied pose-reference growth leaves existing payload intact");
        u16 pose = 0; f32 influence = 0;
        key->getPoseReference(0, pose, influence);
        require(key->getNumPoses() == 2 && pose == 7 && influence == 1 && budget.used() == resident + bytes,
                "Pose reference denial preserves count, clamped values and retained spare charge");
        key->getPoseReference(1, pose, influence);
        require(pose == 8 && influence == 0, "Pose influence lower clamp is preserved");
        const auto denials = sourceBudgetDenialCount();
        key->setNumPoses(0);
        require(sourceBudgetDenialCount() == denials && budget.used() == resident && key->getNumPoses() == 0,
                "Zero pose references physically release capacity without admission");
        budget.restore();
        key->setNumPoses(3);
        budget.set(budget.used());
        key->unload(nullptr);
        require(sourceBudgetDenialCount() == denials && budget.used() == resident,
                "Pose-keyframe unload retires the actual reference array at capacity");
        budget.restore();
    }
    inline void vertexTracks(Capacity &budget)
    {
        auto track = make_ptr<AnimationVertexTrack>();
        auto first = make_ptr<AnimationPoseKeyFrame>(); first->setTime(2);
        auto second = make_ptr<AnimationPoseKeyFrame>(); second->setTime(1);
        Array<SmartPtr<IAnimationPoseKeyFrame>> poses;
        poses.reserve(17); poses.push_back(first); poses.push_back(second);
        const auto resident = budget.used();
        track->setPoseKeyFrames(poses);
        require(budget.used() == resident + arrayBytes<SmartPtr<IAnimationPoseKeyFrame>>(17) &&
                    track->getAnimationType() == VertexAnimationType::VAT_POSE &&
                    track->getKeyFrame(0) == second && track->getKeyFrame(1) == first,
                "Vertex pose imports admit actual caller spare capacity and publish sorted ownership");
        auto retainedPoses = track->getPoseKeyFrames();
        require(retainedPoses.capacity() == 17 && retainedPoses.get_allocator().isSourceBudgeted(),
                "Escaped typed keyframe snapshots retain Source policy and their own spare capacity");
        Array<SmartPtr<IAnimationPoseKeyFrame>> replacement;
        replacement.reserve(33); replacement.push_back(first);
        auto morph = make_ptr<AnimationMorphKeyFrame>(); morph->setTime(3);
        Array<SmartPtr<IAnimationMorphKeyFrame>> morphs;
        morphs.reserve(11); morphs.push_back(morph);
        budget.set(budget.used());
        denied([&] { track->setPoseKeyFrames(replacement); }, "Denied typed replacement preserves the prior keyframe generation");
        denied([&] { track->setMorphKeyFrames(morphs); }, "Denied type-changing import preserves the previous animation type");
        require(track->getAnimationType() == VertexAnimationType::VAT_POSE && track->getNumKeyFrames() == 2 &&
                    track->getKeyFrame(0) == second && retainedPoses[1] == first,
                "Typed payload denial publishes neither partial keyframes nor a changed type");
        budget.restore();
        track->setMorphKeyFrames(morphs);
        auto retainedMorphs = track->getMorphKeyFrames();
        require(retainedMorphs.capacity() == 11 && retainedMorphs.get_allocator().isSourceBudgeted() &&
                    track->getAnimationType() == VertexAnimationType::VAT_MORPH && track->getNumKeyFrames() == 1,
                "Type-changing import retries with complete admitted morph snapshots");
        Array<f32> times{-7}; times.reserve(19);
        const auto pointer = times.data();
        budget.set(budget.used());
        denied([&] { track->_collectKeyFrameTimes(times); }, "Denied key-time collection cannot partially change caller storage");
        require(times.data() == pointer && times.size() == 1 && times[0] == -7 &&
                    !times.get_allocator().isSourceBudgeted(), "Denied key-time collection preserves external caller ownership");
        budget.restore();
        track->_collectKeyFrameTimes(times);
        require(times.get_allocator().isSourceBudgeted() && times.capacity() == 19 && times.size() == 4 &&
                    times[0] == -7 && times[1] == 3 && times[2] == 1 && times[3] == 2,
                "Collected typed timestamps preserve existing append semantics and transfer admitted caller capacity");
        auto vertices = geometry();
        auto alias = vertices;
        const auto held = budget.used();
        budget.set(held);
        const auto denials = sourceBudgetDenialCount();
        track->setAssociatedVertexData(vertices.get());
        require(sourceBudgetDenialCount() == denials && budget.used() == held &&
                    morph->getVertexBuffer() == vertices && track->getAssociatedVertexData() == vertices.get(),
                "Morph geometry aliases share the original physical reservations without duplicate admission");
        vertices = nullptr;
        track->removeKeyFrame(0);
        require(sourceBudgetDenialCount() == denials && retainedMorphs[0] == morph &&
                    morph->getVertexBuffer() == alias, "At-capacity typed removal preserves retained keyframe geometry owners");
        track->unload(nullptr);
        track = nullptr;
        require(sourceBudgetDenialCount() == denials && retainedPoses[0]->getTime() == 1 &&
                    retainedMorphs[0]->getVertexBuffer() == alias,
                "Vertex-track unload releases arrays without invalidating detached typed snapshots or morph pins");
        const auto beforeRelease = budget.used();
        morph->unload(nullptr);
        require(sourceBudgetDenialCount() == denials && !morph->getVertexBuffer() && budget.used() == beforeRelease &&
                    static_cast<float *>(alias->getVertexData())[8] == 8,
                "Morph unload releases its geometry pin while external aliases keep data and charge alive");
        const auto beforeAlias = budget.used();
        alias = nullptr;
        require(sourceBudgetDenialCount() == denials && budget.used() < beforeAlias,
                "Last geometry alias physically retires its original mesh bodies, metadata and stream leases");
        budget.restore();
    }
    inline void run()
    {
        Capacity budget;
        { auto warm = geometry(); }
        mapPayloads(budget);
        poseReferences(budget);
        vertexTracks(budget);
    }
}
