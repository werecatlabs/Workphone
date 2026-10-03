#ifndef TypeManagerFixture_h__
#define TypeManagerFixture_h__

#include <Workphone/Memory/TypeManager.hpp>
#include <memory>

class TypeManagerFixture
{
public:
    TypeManagerFixture();

    ~TypeManagerFixture();

    std::unique_ptr<workphone::TypeManager> m_typeManager;
};

#endif  // TypeManagerFixture_h__
