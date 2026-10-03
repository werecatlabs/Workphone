#ifndef CBlockGenerator_H
#define CBlockGenerator_H

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IBlockGenerator.hpp>
#include <WPProcedural/CProceduralGenerator.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/Set.hpp>

namespace workphone
{
    namespace procedural
    {

        class WPProcedural_API CBlockGenerator : public CProceduralGenerator<IBlockGenerator>
        {
        public:
            CBlockGenerator();
            ~CBlockGenerator() override;

            void load( SmartPtr<ISharedObject> object ) override;
            void unload( SmartPtr<ISharedObject> object ) override;

            void generate() override;

            SmartPtr<IProceduralCity> getCity() const override;
            void setCity( SmartPtr<IProceduralCity> city ) override;

            void generateBlocks( SmartPtr<scene::IGameActor> cityLayer );

            void testMemoryUsage();

        private:
            struct BlockInfo
            {
                BlockInfo( bool isClosedBlock = false ) : IsClosedBlock( isClosedBlock )
                {
                }

                bool IsClosedBlock;
            };

            void generateBlocks();
            void generateBlocks2();

            bool isBuilding( SmartPtr<IData> data, String id );

            void calculateBlock2( SmartPtr<scene::IGameActor> originalNode, Set<u32> &visitedNodes );

            BlockInfo findNodesInBlock( SmartPtr<ICityBlock> block,
                                        SmartPtr<scene::IGameActor> originalNode,
                                        SmartPtr<scene::IGameActor> connectedNode,
                                        Set<u32> &visitedNodes );

            Properties m_blockPropGrp;
            Properties m_blockNodePropGrp;

            SmartPtr<scene::IGameActor> m_selectedLayer;

            SmartPtr<IProceduralCity> m_city;
        };
    }  // namespace procedural
}  // namespace workphone

#endif
