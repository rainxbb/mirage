#pragma once
#include <atomic>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>

namespace Mirage
{

class FileWatcher
{
public:
    FileWatcher();
    ~FileWatcher();

    void WatchDirectory(const std::string& path, std::function<void(const std::string&)> onModified);
    void Stop();

private:
    void Poll();

    std::string m_watchPath;
    std::function<void(const std::string&)> m_callback;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_lastWriteTimes;

    std::thread m_thread;
    std::atomic<bool> m_running{false};
};

} // namespace Mirage
