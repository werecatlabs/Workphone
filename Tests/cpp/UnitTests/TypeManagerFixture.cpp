#include "GameTestFixture.hpp"
#include "TypeManagerFixture.hpp"
#include "UnitTests.hpp"
#include "Workphone/Workphone.hpp"

using namespace workphone;

TypeManagerFixture::TypeManagerFixture()
{
    m_typeManager = std::make_unique<TypeManager>();
    TypeManager::setInstance( m_typeManager.get() );
    UnitTests::sTypeManager = m_typeManager.get();
}

TypeManagerFixture::~TypeManagerFixture()
{
    m_typeManager->unload();

    UnitTests::sTypeManager = nullptr;
    TypeManager::setInstance( nullptr );
    m_typeManager.reset();
}
