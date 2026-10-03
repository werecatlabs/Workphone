#ifndef GraphicsTestFixture_h__
#define GraphicsTestFixture_h__

#include "UnitTestsFixture.hpp"
#include <Workphone/Interface/Memory/ISharedObject.hpp>

class GraphicsTestFixture : public workphone::ISharedObject
{
public:
    GraphicsTestFixture();

    ~GraphicsTestFixture();

    void createTasks();
};

#endif  // GraphicsTestFixture_h__
