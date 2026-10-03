#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/StringPoolAllocator.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>

namespace workphone
{
    template <>
    StringPoolAllocator<c8>::StringPoolAllocator() noexcept : m_pool( nullptr )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        m_pool = applicationManager->getStringPool();
    }

    template <>
    StringPoolAllocator<wchar_t>::StringPoolAllocator() noexcept : m_pool( nullptr )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        m_pool = applicationManager->getStringPoolW();
    }
}  // namespace workphone
