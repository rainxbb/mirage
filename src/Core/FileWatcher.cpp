#include "FileWatcher.h"

namespace Mirage
{

FileWatcher::FileWatcher() {}

FileWatcher::~FileWatcher() { Stop(); }

void FileWatcher::WatchDirectory(const std::string& path, std::function<void(const std::string&)> onModified)
{
    m_watchPath = path;
    m_callback = onModified;
    m_running = true;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
    {
        if (entry.is_regular_file())
        {
            m_lastWriteTimes[entry.path().string()] = std::filesystem::last_write_time(entry.path());
        }
    }

    m_thread = std::thread(&FileWatcher::Poll, this);
}

void FileWatcher::Stop()
{
    if (m_running)
    {
        m_running = false;
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }
}

void FileWatcher::Poll()
{
    while (m_running)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(m_watchPath))
        {
            if (entry.is_regular_file())
            {
                std::string pathStr = entry.path().string();
                auto currentTime = std::filesystem::last_write_time(entry.path());

                if (m_lastWriteTimes.find(pathStr) == m_lastWriteTimes.end() ||
                    m_lastWriteTimes[pathStr] != currentTime)
                {
                    m_lastWriteTimes[pathStr] = currentTime;
                    if (m_callback)
                    {
                        m_callback(pathStr);
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

} // namespace Mirage
