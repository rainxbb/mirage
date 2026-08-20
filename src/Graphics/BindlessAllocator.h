#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

namespace Mirage
{

class BindlessAllocator
{
public:
    BindlessAllocator(uint32_t maxCapacity);
    ~BindlessAllocator();

    uint32_t Allocate();
    void Free(uint32_t index);

private:
    uint32_t m_maxCapacity;
    std::atomic<uint32_t> m_nextIndex;
    std::vector<uint32_t> m_freeList;
    std::mutex m_mutex;
};

} // namespace Mirage
