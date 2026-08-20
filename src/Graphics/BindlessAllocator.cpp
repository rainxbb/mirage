#include "BindlessAllocator.h"

#include <stdexcept>

namespace Mirage
{

BindlessAllocator::BindlessAllocator(uint32_t maxCapacity) : m_maxCapacity(maxCapacity), m_nextIndex(0) {}

BindlessAllocator::~BindlessAllocator() {}

uint32_t BindlessAllocator::Allocate()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_freeList.empty())
    {
        uint32_t index = m_freeList.back();
        m_freeList.pop_back();
        return index;
    }

    uint32_t index = m_nextIndex.fetch_add(1);
    if (index >= m_maxCapacity)
    {
        throw std::runtime_error("Bindless Descriptor Pool exhausted!");
    }
    return index;
}

void BindlessAllocator::Free(uint32_t index)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_freeList.push_back(index);
}

} // namespace Mirage
