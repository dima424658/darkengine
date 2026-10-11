#include <tmdecl.h>
#include <tmftab.h>

#pragma pack(push, 8)
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#pragma pack(pop)

namespace
{
struct TimerProcess
{
    pfv callback = nullptr;
    std::atomic<bool> running{false};
    unsigned periodMs = 1;
    std::thread worker;
};

std::array<TimerProcess, 16> g_processes;
std::mutex g_mutex;

void Stop(TimerProcess &process)
{
    process.running = false;
    if (process.worker.joinable())
        process.worker.join();
}

int AddProcess(pfv callback, int, long denominator)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    for (size_t i = 0; i < g_processes.size(); ++i)
        if (!g_processes[i].callback)
        {
            g_processes[i].callback = callback;
            g_processes[i].periodMs = std::max(1l, 1000l * denominator / 1193181l);
            return static_cast<int>(i);
        }
    return -1;
}

int RemoveProcess(int id)
{
    if (id < 0 || id >= static_cast<int>(g_processes.size())) return -1;
    Stop(g_processes[id]);
    g_processes[id].callback = nullptr;
    return 0;
}

int ActivateProcess(int id)
{
    if (id < 0 || id >= static_cast<int>(g_processes.size()) || !g_processes[id].callback) return -1;
    TimerProcess &process = g_processes[id];
    Stop(process);
    process.running = true;
    process.worker = std::thread([&process] {
        while (process.running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(process.periodMs));
            if (process.running && process.callback) process.callback();
        }
    });
    return 0;
}

int DeactivateProcess(int id)
{
    if (id < 0 || id >= static_cast<int>(g_processes.size())) return -1;
    Stop(g_processes[id]);
    return 0;
}

void SetPeriod(int id, int period)
{
    if (id < 0 || id >= static_cast<int>(g_processes.size())) return;
    const bool active = g_processes[id].running;
    Stop(g_processes[id]);
    g_processes[id].periodMs = std::max(1, period);
    if (active) ActivateProcess(id);
}

void SetDenominator(int id, int denominator) { SetPeriod(id, 1000l * denominator / 1193181l); }
void SetFrequency(int id, int frequency) { if (frequency > 0) SetPeriod(id, 1000 / frequency); }

pfv g_timerFunctions[TMC_TIMER_FNS] = {
    reinterpret_cast<pfv>(AddProcess), reinterpret_cast<pfv>(RemoveProcess),
    reinterpret_cast<pfv>(ActivateProcess), reinterpret_cast<pfv>(DeactivateProcess),
    reinterpret_cast<pfv>(SetDenominator), reinterpret_cast<pfv>(SetFrequency),
    reinterpret_cast<pfv>(SetPeriod)
};
}

extern "C" int tm_init_libt()
{
    tm_ftab = g_timerFunctions;
    return 0;
}

extern "C" int tm_close_libt()
{
    for (auto &process : g_processes)
    {
        Stop(process);
        process.callback = nullptr;
    }
    return 0;
}
