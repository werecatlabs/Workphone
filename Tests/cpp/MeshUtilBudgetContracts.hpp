#pragma once
#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/VertexBuffer.hpp>
#include <Workphone/Mesh/VertexDeclaration.hpp>
#include <Workphone/Mesh/IndexBuffer.hpp>
#include <Workphone/Mesh/Vertex.hpp>
#include <Workphone/Mesh/VertexBoneAssignment.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <cstring>
#include <array>
#include <stdexcept>

namespace mesh_util_budget_contracts
{
    using namespace workphone;
    inline void require(bool value, const char *message)
    {
        if(!value) throw std::runtime_error(message);
    }
    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr size_t source = size_t(StreamMemoryClass::SourceCpu);
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        void set(u64 value)
        {
            String error;
            require(scheduler->setBudget(StreamMemoryClass::SourceCpu, value, error),
                    "Mesh utility capacity applies");
        }
        void restore() { set(original); }
        ~Capacity() { restore(); }
    };
    template<class T> u64 physical(const Array<T> &values)
    {
        return values.capacity() ? sourceBudgetStorageBytes(values.capacity() * sizeof(T), alignof(T)) : 0;
    }
    inline SmartPtr<IVertexBuffer> vertices()
    {
        auto declaration = make_ptr<VertexDeclaration>();
        declaration->addElement(0, 0, VertexElementSemantic::VES_POSITION, VertexElementType::VET_FLOAT3);
        declaration->addElement(1, 0, VertexElementSemantic::VES_TEXTURE_COORDINATES, VertexElementType::VET_FLOAT2);
        auto result = make_ptr<VertexBuffer>();
        result->setVertexDeclaration(declaration);
        result->setNumVertices(4);
        const float positions[] = {0, 2, 0, 1, 3, 0, 0, 4, 1, 0, 2, 0};
        const float uv[] = {10, 20, 11, 21, 12, 22, 13, 23};
        std::memcpy(result->createVertexData(0), positions, sizeof(positions));
        std::memcpy(result->createVertexData(1), uv, sizeof(uv));
        return result;
    }
    struct SharedFixture
    {
        SmartPtr<IMesh> mesh;
        SmartPtr<IVertexBuffer> shared;
        SmartPtr<ISubMesh> first, second;
        SmartPtr<IIndexBuffer> firstIndices, secondIndices;
    };
    inline SharedFixture sharedFixture()
    {
        SharedFixture result;
        result.mesh = make_ptr<Mesh>();
        result.shared = vertices();
        result.firstIndices = MeshUtil::createIndexBuffer(Array<u32>{3, 1, 3});
        result.secondIndices = MeshUtil::createIndexBuffer(Array<u32>{2, 0, 1});
        result.first = MeshUtil::createSubMesh(nullptr, result.firstIndices);
        result.second = MeshUtil::createSubMesh(nullptr, result.secondIndices);
        result.first->setUseSharedVertices(true);
        result.second->setUseSharedVertices(true);
        result.mesh->setSharedVertexBuffer(result.shared);
        result.mesh->setHasSharedVertexData(true);
        result.mesh->addSubMesh(result.first);
        result.mesh->addSubMesh(result.second);
        return result;
    }
    inline SmartPtr<ISubMesh> ownedFixture()
    {
        auto result = MeshUtil::createSubMesh(vertices(), MeshUtil::createIndexBuffer(Array<u32>{3, 1, 2}));
        result->setMaterialName("stone");
        return result;
    }
    inline SmartPtr<IMesh> groupedFixture()
    {
        auto mesh = make_ptr<Mesh>();
        mesh->addSubMesh(ownedFixture());
        mesh->addSubMesh(ownedFixture());
        return mesh;
    }
    inline void requireShared(const SharedFixture &fixture)
    {
        require(fixture.mesh->getHasSharedVertexData() && fixture.mesh->getSharedVertexBuffer() == fixture.shared &&
                    fixture.first->getUseSharedVertices() && fixture.second->getUseSharedVertices() &&
                    !fixture.first->getVertexBuffer() && !fixture.second->getVertexBuffer() &&
                    fixture.first->getIndexBuffer() == fixture.firstIndices &&
                    fixture.second->getIndexBuffer() == fixture.secondIndices &&
                    static_cast<u16 *>(fixture.firstIndices->getIndexData())[0] == 3 &&
                    static_cast<u16 *>(fixture.secondIndices->getIndexData())[0] == 2,
                "Denied unsharing preserves all submeshes, flags and original index aliases");
    }
    inline void requireUnshared(const SharedFixture &fixture)
    {
        require(!fixture.mesh->getHasSharedVertexData() && !fixture.mesh->getSharedVertexBuffer() &&
                    !fixture.first->getUseSharedVertices() && !fixture.second->getUseSharedVertices() &&
                    fixture.first->getVertexBuffer()->getNumVertices() == 2 &&
                    fixture.second->getVertexBuffer()->getNumVertices() == 3 &&
                    fixture.first->getIndexBuffer() != fixture.firstIndices &&
                    fixture.second->getIndexBuffer() != fixture.secondIndices,
                "Complete unsharing publishes both independent compacted buffer generations");
        const auto firstUv = static_cast<float *>(fixture.first->getVertexBuffer()->getVertexData(1));
        const auto secondUv = static_cast<float *>(fixture.second->getVertexBuffer()->getVertexData(1));
        require(firstUv[0] == 13 && firstUv[2] == 11 && secondUv[0] == 12 && secondUv[2] == 10 &&
                    static_cast<u16 *>(fixture.first->getIndexBuffer()->getIndexData())[2] == 0 &&
                    static_cast<u16 *>(fixture.second->getIndexBuffer()->getIndexData())[2] == 2 &&
                    static_cast<u16 *>(fixture.firstIndices->getIndexData())[0] == 3 &&
                    static_cast<u16 *>(fixture.secondIndices->getIndexData())[0] == 2,
                "Unsharing compacts every vertex stream and leaves retained source index aliases unchanged");
    }
    inline void run()
    {
        Capacity budget;
#if WP_USE_CUSTOM_NEW_DELETE
        {
            alignas(VertexDeclaration) std::array<unsigned char, sizeof(VertexDeclaration)> slot{};
            const auto resident = budget.used();
            budget.set(resident);
            bool denied = false;
            try { new(slot.data()) VertexDeclaration; }
            catch(const std::bad_alloc &) { denied = true; }
            require(denied && budget.used() == resident,
                    "Denied plain placement construction leaves borrowed stack storage with its owner");
            denied = false;
            try { new(slot.data(), __FILE__, __LINE__, __FUNCTION__) VertexDeclaration; }
            catch(const std::bad_alloc &) { denied = true; }
            require(denied && budget.used() == resident,
                    "Denied diagnostic placement construction does not free its borrowed stack slot");
            budget.restore();
            auto declaration = new(slot.data()) VertexDeclaration;
            require(declaration->getElements().capacity() == 4,
                    "Borrowed placement slot remains reusable after denied constructor admission");
            declaration->~VertexDeclaration();
            require(budget.used() == resident, "Successful placement object retires only its owned declaration metadata");
        }
#endif
        // Warm factory inventories at the largest number of simultaneous
        // utility candidates; the following measurements isolate owned storage.
        {
            auto source = ownedFixture();
            auto welded = MeshUtil::clean(source, real_Num(0.01));
            auto grouped = groupedFixture();
            auto merged = MeshUtil::mergeSubMeshesByMaterial(grouped);
            auto shared = sharedFixture();
            MeshUtil::unshareVertices(shared.mesh);
            requireUnshared(shared);
            auto box = MeshUtil::createBox();
            auto cylinder = MeshUtil::createCylinder(1, 1, 3, 1, false);
        }
        {
            // Vertex's constructor is DLL-internal. Check that overload's
            // borrowing signature without constructing private-ABI records,
            // and exercise the exported attribute-array path with three points.
            using BorrowedVertexInput = SmartPtr<ISubMesh> (*)(const Array<Vertex> &, const Array<u32> &);
            static_assert(std::is_same<decltype(static_cast<BorrowedVertexInput>(&MeshUtil::createSubMesh)),
                                       BorrowedVertexInput>::value,
                          "Interleaved utility inputs are borrowed by const reference");
            Array<Vector3<real_Num>> input;
            input.reserve(47);
            input.push_back(Vector3<real_Num>(0, 0, 0));
            input.push_back(Vector3<real_Num>(1, 0, 0));
            input.push_back(Vector3<real_Num>(0, 0, 1));
            Array<Vector3<real_Num>> normals(3, Vector3<real_Num>::up());
            Array<Vector2<real_Num>> uv(3, Vector2<real_Num>::zero());
            Array<u32> inputIndices{0, 1, 2};
            inputIndices.reserve(31);
            const auto data = input.data();
            const auto indices = inputIndices.data();
            auto mesh = MeshUtil::createMesh(input, normals, uv, inputIndices);
            auto submeshes = mesh->getSubMeshes();
            require(submeshes.size() == 1 && submeshes[0]->getVertexBuffer()->getNumVertices() == 3 &&
                        input.data() == data && inputIndices.data() == indices &&
                        !input.get_allocator().isSourceBudgeted() && !inputIndices.get_allocator().isSourceBudgeted(),
                    "Borrowed caller arrays keep external ownership while utility buffers use the actual vertex count");
        }
        for(bool cylinder : {false, true})
        {
            const auto resident = budget.used();
            unsigned rejected = 0, accepted = 0;
            for(u64 headroom : {0ULL, 128ULL, 512ULL, 1024ULL, 2048ULL, 4096ULL, 15000ULL, 30000ULL, 65536ULL})
            {
                const auto denials = budget.scheduler->stats().rejectedReservations[Capacity::source];
                budget.set(resident + headroom);
                try
                {
                    auto mesh = cylinder ? MeshUtil::createCylinder(1, 1, 3, 1, false) : MeshUtil::createBox();
                    require(mesh != nullptr, "Admitted procedural utility creates complete mesh ownership");
                    ++accepted;
                }
                catch(const std::exception &)
                {
                    require(budget.scheduler->stats().rejectedReservations[Capacity::source] != denials,
                            "Procedural utility threshold failure is a real admission rejection");
                    ++rejected;
                }
                budget.restore();
                require(budget.used() == resident,
                        "Procedural utility positions/normals/UV/index scratch and partial buffers unwind completely");
            }
            require(rejected >= 3 && accepted > 0,
                    "Procedural utility sweep rejects partial scratch populations and retries complete construction");
        }
        {
            auto source = ownedFixture();
            const auto resident = budget.used();
            auto points = MeshUtil::getPoints(source);
            auto indices = MeshUtil::getIndices(source);
            require(points.get_allocator().isSourceBudgeted() && indices.get_allocator().isSourceBudgeted() &&
                        points.size() == 4 && indices.size() == 3 &&
                        budget.used() == resident + physical(points) + physical(indices),
                    "Detached point/index outputs admit exact physical caller buffers");
            budget.set(budget.used());
            bool denied = false;
            try { auto copy = points; } catch(const std::bad_alloc &) { denied = true; }
            require(denied && points[1].y == 3, "Caller point copies preserve shared admission and existing data");
            const auto denials = sourceBudgetDenialCount();
            source = nullptr;
            require(sourceBudgetDenialCount() == denials && points[2].y == 4 && indices[0] == 3,
                    "Detached utility outputs survive full-budget producer destruction");
            const auto held = budget.used();
            const auto bytes = physical(points) + physical(indices);
            points = {}; indices = {};
            require(budget.used() == held - bytes, "Final utility output release retires both physical buffers");
            budget.restore();
        }
        {
            const auto resident = budget.used();
            auto map = MeshUtil::meshToHeightMap(nullptr, Vector2I(3, 5));
            u64 bytes = physical(map);
            for(const auto &row : map)
            {
                require(row.get_allocator().isSourceBudgeted() && row.size() == 5,
                        "Heightmap rows retain their own admitted allocator policy");
                bytes += physical(row);
            }
            require(map.get_allocator().isSourceBudgeted() && map.size() == 3 && budget.used() == resident + bytes,
                    "Heightmap outer metadata and every row charge actual retained spare capacity");
            budget.set(budget.used());
            bool denied = false;
            try { auto copy = map; } catch(const std::bad_alloc &) { denied = true; }
            require(denied && map[0].size() == 5, "Nested caller heightmap copies cannot bypass the shared cap");
            map = {};
            require(budget.used() == resident, "Nested heightmap teardown releases all levels at the cap");
            unsigned rejected = 0, accepted = 0;
            for(u64 headroom : {0ULL, 64ULL, 128ULL, 256ULL, 512ULL, 1024ULL, 2048ULL, 4096ULL})
            {
                budget.set(resident + headroom);
                try { auto candidate = MeshUtil::meshToHeightMap(nullptr, Vector2I(3, 5)); ++accepted; }
                catch(const std::bad_alloc &) { ++rejected; }
                require(budget.used() == resident, "Denied nested heightmap stages unwind every admitted earlier row");
            }
            require(rejected >= 3 && accepted > 0, "Heightmap sweep covers partial row admission and complete publication");
            budget.restore();
        }
        {
            auto source = ownedFixture();
            const auto original = source->getVertexBuffer();
            const auto pointer = original->getVertexData();
            unsigned rejected = 0, accepted = 0;
            for(u64 headroom : {0ULL, 128ULL, 256ULL, 512ULL, 1024ULL, 2048ULL, 4096ULL, 8192ULL, 65536ULL})
            {
                const auto resident = budget.used();
                const auto denials = budget.scheduler->stats().rejectedReservations[Capacity::source];
                budget.set(resident + headroom);
                try
                {
                    auto welded = MeshUtil::clean(source, real_Num(0.01));
                    ++accepted;
                    require(welded->getVertexBuffer()->getNumVertices() == 3 &&
                                welded->getVertexBuffer() != original && welded->getMaterialName() == "stone" &&
                                static_cast<float *>(welded->getVertexBuffer()->getVertexData(1))[4] == 12 &&
                                static_cast<u16 *>(welded->getIndexBuffer()->getIndexData())[0] == 0,
                            "Welding publishes compacted independent streams and remapped 16-bit indices");
                }
                catch(const std::exception &)
                {
                    require(budget.scheduler->stats().rejectedReservations[Capacity::source] != denials,
                            "Welding threshold failure is a real admission rejection");
                    ++rejected;
                }
                budget.restore();
                require(budget.used() == resident && original->getVertexData() == pointer &&
                            original->getNumVertices() == 4 && static_cast<float *>(pointer)[4] == 3 &&
                            static_cast<u16 *>(source->getIndexBuffer()->getIndexData())[0] == 3,
                        "Welding denial and success preserve source geometry while candidate storage retires");
            }
            require(rejected >= 4 && accepted > 0, "Welding sweep covers scratch, clone and stream staging denial plus retry");
        }
        {
            unsigned rejected = 0, accepted = 0;
            for(u64 headroom : {0ULL, 128ULL, 256ULL, 512ULL, 1024ULL, 2048ULL, 4096ULL, 8192ULL, 65536ULL})
            {
                auto fixture = sharedFixture();
                const auto resident = budget.used();
                const auto denials = budget.scheduler->stats().rejectedReservations[Capacity::source];
                budget.set(resident + headroom);
                try { MeshUtil::unshareVertices(fixture.mesh); ++accepted; }
                catch(const std::exception &)
                {
                    require(budget.scheduler->stats().rejectedReservations[Capacity::source] != denials,
                            "Unsharing threshold failure is a real admission rejection");
                    ++rejected;
                    requireShared(fixture);
                    require(budget.used() == resident, "Unsharing denial releases scratch and every staged submesh candidate");
                }
                budget.restore();
                if(!fixture.mesh->getHasSharedVertexData()) requireUnshared(fixture);
            }
            require(rejected >= 4 && accepted > 0, "Unsharing sweep covers partial candidates and complete two-submesh publication");
        }
        {
            auto source = groupedFixture();
            {
                auto points = MeshUtil::getPoints(source);
                auto indices = MeshUtil::getIndices(source);
                require(points.get_allocator().isSourceBudgeted() && indices.get_allocator().isSourceBudgeted() &&
                            points.size() == 8 && indices.size() == 6 && indices[3] == 7,
                        "Whole-mesh utility extraction retains admitted outputs and offsets detached submesh indices");
            }
            const auto resident = budget.used();
            unsigned rejected = 0, accepted = 0;
            for(u64 headroom : {0ULL, 64ULL, 128ULL, 256ULL, 512ULL, 1024ULL, 2048ULL, 4096ULL, 8192ULL, 65536ULL})
            {
                const auto denials = budget.scheduler->stats().rejectedReservations[Capacity::source];
                budget.set(resident + headroom);
                try
                {
                    auto merged = MeshUtil::mergeSubMeshesByMaterial(source);
                    auto submeshes = merged->getSubMeshes();
                    require(submeshes.size() == 1 && submeshes[0]->getVertexBuffer()->getNumVertices() == 8 &&
                                submeshes[0]->getVertexBuffer()->getVertexDeclaration()->getSize(1) == 8 &&
                                static_cast<float *>(submeshes[0]->getVertexBuffer()->getVertexData(1))[8] == 10 &&
                                static_cast<u16 *>(submeshes[0]->getIndexBuffer()->getIndexData())[3] == 7,
                            "Material merge preserves stream layout and offsets decoded source indices");
                    ++accepted;
                }
                catch(const std::exception &)
                {
                    require(budget.scheduler->stats().rejectedReservations[Capacity::source] != denials,
                            "Material grouping threshold failure is a real admission rejection");
                    ++rejected;
                }
                budget.restore();
                require(budget.used() == resident, "Material group map, nested lists and partial merged buffers unwind completely");
            }
            require(rejected >= 4 && accepted > 0, "Material grouping sweep covers map/list/geometry denial and retry");
        }
        {
            auto source = make_ptr<Mesh>();
            auto line = ownedFixture();
            line->setRenderOperationType(RenderOperationType::OT_LINE_LIST);
            line->setIndexBuffer(MeshUtil::createIndexBuffer(Array<u32>{3, 1}));
            auto emptyVertices = make_ptr<VertexBuffer>();
            emptyVertices->setVertexDeclaration(line->getVertexBuffer()->getVertexDeclaration()->clone());
            auto empty = MeshUtil::createSubMesh(emptyVertices, MeshUtil::createIndexBuffer(Array<u32>{}));
            empty->setMaterialName("stone");
            empty->setRenderOperationType(RenderOperationType::OT_LINE_LIST);
            source->addSubMesh(empty);
            source->addSubMesh(line);
            auto secondLine = line->clone();
            source->addSubMesh(secondLine);
            source->addSubMesh(ownedFixture());
            auto strip = ownedFixture();
            strip->setRenderOperationType(RenderOperationType::OT_LINE_STRIP);
            source->addSubMesh(strip);
            source->addSubMesh(strip->clone());
            const auto lineData = line->getVertexBuffer()->getVertexData(1);
            auto merged = MeshUtil::mergeSubMeshesByMaterial(source);
            auto submeshes = merged->getSubMeshes();
            unsigned lines = 0, triangles = 0, strips = 0;
            for(const auto &submesh : submeshes)
            {
                if(submesh->getRenderOperationType() == RenderOperationType::OT_LINE_LIST)
                {
                    ++lines;
                    require(submesh->getVertexBuffer()->getNumVertices() == 8 &&
                                submesh->getIndexBuffer()->getNumIndices() == 4 &&
                                static_cast<u16 *>(submesh->getIndexBuffer()->getIndexData())[2] == 7 &&
                                static_cast<float *>(submesh->getVertexBuffer()->getVertexData(1))[8] == 10,
                            "Empty-first line groups preserve declared streams and line index ordering");
                }
                else if(submesh->getRenderOperationType() == RenderOperationType::OT_TRIANGLE_LIST)
                {
                    ++triangles;
                    require(submesh->getIndexBuffer()->getNumIndices() == 3,
                            "The same material's triangle indices remain in a separate primitive group");
                }
                else if(submesh->getRenderOperationType() == RenderOperationType::OT_LINE_STRIP)
                {
                    ++strips;
                    require(submesh->getIndexBuffer()->getNumIndices() == 3,
                            "Separate strips retain their boundaries without added connecting primitives");
                }
            }
            require(lines == 1 && triangles == 1 && strips == 2 && submeshes.size() == 4 &&
                        !emptyVertices->getVertexData(0) && line->getVertexBuffer()->getVertexData(1) == lineData,
                    "Material grouping preserves render operations and all original stream aliases");

            auto missing = make_ptr<VertexBuffer>();
            missing->setVertexDeclaration(line->getVertexBuffer()->getVertexDeclaration()->clone());
            missing->setNumVertices(4);
            std::memcpy(missing->createVertexData(0), line->getVertexBuffer()->getVertexData(0), 12 * sizeof(float));
            auto invalid = make_ptr<Mesh>();
            auto incomplete = MeshUtil::createSubMesh(missing, MeshUtil::createIndexBuffer(Array<u32>{0, 1, 2}));
            incomplete->setMaterialName("stone");
            invalid->addSubMesh(incomplete);
            const auto resident = budget.used();
            bool rejected = false;
            try { MeshUtil::mergeSubMeshesByMaterial(invalid); }
            catch(const std::invalid_argument &) { rejected = true; }
            require(rejected, "A populated source missing a declared stream rejects material merging explicitly");
            require(budget.used() == resident,
                    "Malformed stream preflight avoids geometry candidates and retained factory inventory growth");
            require(!missing->getVertexData(1) &&
                        incomplete->getVertexBuffer() == missing,
                    "Missing-stream rejection preserves the complete original source ownership");
        }
        {
            auto source = make_ptr<Mesh>();
            auto first = ownedFixture();
            auto second = ownedFixture();
            first->setRenderOperationType(RenderOperationType::OT_LINE_LIST);
            second->setRenderOperationType(RenderOperationType::OT_LINE_LIST);
            source->addSubMesh(first);
            source->addSubMesh(second);
            auto merged = MeshUtil::mergeSubMeshesByMaterial(source);
            auto submeshes = merged->getSubMeshes();
            require(submeshes.size() == 2 &&
                        submeshes[0]->getRenderOperationType() == RenderOperationType::OT_LINE_LIST &&
                        submeshes[1]->getRenderOperationType() == RenderOperationType::OT_LINE_LIST &&
                        submeshes[0]->getIndexBuffer()->getNumIndices() == 3 &&
                        submeshes[1]->getIndexBuffer()->getNumIndices() == 3 &&
                        static_cast<u16 *>(submeshes[0]->getIndexBuffer()->getIndexData())[2] == 2 &&
                        static_cast<u16 *>(submeshes[1]->getIndexBuffer()->getIndexData())[0] == 3,
                    "Odd line lists retain separate submissions so trailing indices cannot form a cross-submesh line");
        }
        {
            auto expectUnsupported = [&](auto &&operation) {
                const auto resident = budget.used();
                bool rejected = false;
                try { operation(); } catch(const std::invalid_argument &) { rejected = true; }
                require(rejected && budget.used() == resident,
                        "Unsupported skeletal or vertex-animation transformation rejects before publication and unwinds scratch");
            };
            auto bone = make_ptr<VertexBoneAssignment>(3, 2, 0.75f);
            auto source = ownedFixture();
            source->addBoneAssignment(bone);
            auto original = source->getVertexBuffer();
            const auto vertexData = original->getVertexData(0);
            const auto indexData = source->getIndexBuffer()->getIndexData();
            expectUnsupported([&] { MeshUtil::clean(source, real_Num(0.01)); });
            auto mesh = make_ptr<Mesh>();
            mesh->addSubMesh(source);
            expectUnsupported([&] { MeshUtil::mergeSubMeshesByMaterial(mesh); });
            require(source->getBoneAssignments()[0] == bone && bone->getVertexIndex() == 3 &&
                        bone->getBoneIndex() == 2 && bone->getWeight() == 0.75f &&
                        original->getVertexData(0) == vertexData && source->getIndexBuffer()->getIndexData() == indexData &&
                        static_cast<u16 *>(indexData)[0] == 3,
                    "Rejected bone-bearing welding/merge preserve original assignment objects, indices and aliases");
            auto shared = sharedFixture();
            shared.first->addBoneAssignment(bone);
            expectUnsupported([&] { MeshUtil::unshareVertices(shared.mesh); });
            requireShared(shared);
            shared.first->removeBoneAssignment(bone);
            shared.mesh->addBoneAssignment(bone);
            expectUnsupported([&] { MeshUtil::unshareVertices(shared.mesh); });
            requireShared(shared);
            expectUnsupported([&] { MeshUtil::mergeSubMeshesByMaterial(shared.mesh); });
            require(shared.mesh->getBoneAssignments()[0] == bone && bone->getVertexIndex() == 3,
                    "Rejected shared transformations preserve mesh-level bone indices and assignment identity");
            shared.mesh->removeBoneAssignment(bone);
            static_cast<Mesh *>(shared.mesh.get())->setHasSkeleton(true);
            expectUnsupported([&] { MeshUtil::unshareVertices(shared.mesh); });
            expectUnsupported([&] { MeshUtil::mergeSubMeshesByMaterial(shared.mesh); });
            requireShared(shared);
            static_cast<Mesh *>(shared.mesh.get())->setHasSkeleton(false);
            auto pose = shared.mesh->createPose(0, "retained-pose");
            require(pose != nullptr, "Create retained pose metadata fixture");
            expectUnsupported([&] { MeshUtil::unshareVertices(shared.mesh); });
            expectUnsupported([&] { MeshUtil::mergeSubMeshesByMaterial(shared.mesh); });
            requireShared(shared);
            require(shared.mesh->getNumPoses() == 1 && shared.mesh->getPose(0) == pose,
                    "Rejected vertex transformations preserve original pose inventory and identities");
            shared.mesh->removeAllPoses();
            auto animation = shared.mesh->createAnimation("retained-animation", 1);
            require(animation != nullptr, "Create retained vertex-animation fixture");
            expectUnsupported([&] { MeshUtil::unshareVertices(shared.mesh); });
            expectUnsupported([&] { MeshUtil::mergeSubMeshesByMaterial(shared.mesh); });
            requireShared(shared);
            require(shared.mesh->getNumAnimations() == 1 && shared.mesh->getAnimation(0) == animation,
                    "Rejected vertex transformations preserve original animation inventory and identities");
            auto weighted = ownedFixture();
            weighted->getVertexBuffer()->getVertexDeclaration()->addElement(
                2, 0, VertexElementSemantic::VES_BLEND_WEIGHTS, VertexElementType::VET_FLOAT1);
            const auto weightedData = weighted->getVertexBuffer()->getVertexData(0);
            expectUnsupported([&] { MeshUtil::clean(weighted, real_Num(0.01)); });
            auto weightedMesh = make_ptr<Mesh>();
            weightedMesh->addSubMesh(weighted);
            expectUnsupported([&] { MeshUtil::mergeSubMeshesByMaterial(weightedMesh); });
            require(weighted->getVertexBuffer()->getVertexData(0) == weightedData &&
                        weighted->getVertexBuffer()->getNumVertices() == 4,
                    "Skinning attributes without explicit assignment lists reject without changing source geometry");
        }
    }
}
