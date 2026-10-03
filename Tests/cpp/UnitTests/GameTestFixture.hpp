#ifndef GameTestFixture_h__
#define GameTestFixture_h__

#include "UnitTestsFixture.hpp"
#include <Workphone/Interface/Memory/ISharedObject.hpp>

class GameTestFixture : public workphone::ISharedObject
{
public:
    GameTestFixture();

    ~GameTestFixture();

    void createTasks();
};

#endif  // GameTestFixture_h__
