#include "PacketContainer.h"

#include <chrono>

using namespace std::chrono;
using std::condition_variable;
using std::mutex;
using std::queue;
using std::unique_lock;

void 
PacketContainer::push(Packet& p)
{
    unique_lock<mutex> ul(m);
    full_queue.wait(ul, [this]() { return (q.size() < max_size); });
    q.push(p);
    empty_queue.notify_all();
}

bool 
PacketContainer::timed_push(Packet& p, const int max_delay)
{
    unique_lock<mutex> ul(m);
    auto status =
        full_queue.wait_for(ul, seconds(max_delay),
                            [this]() { return (q.size() < max_size); });
    if (status == true) {
        q.push(p);
        empty_queue.notify_all();
        return true;
    }

    ESP_LOGI(tag, "Full queue timeout");
    return false;
}

void 
PacketContainer::pop(Packet& store)
{
    unique_lock<mutex> ul(m);
    empty_queue.wait(ul, [this]() { return (q.size() > 0); });
    store = q.front();
    q.pop();
    full_queue.notify_all();
}

bool 
PacketContainer::timed_pop(Packet& store, const int max_delay)
{
    unique_lock<mutex> ul(m);
    auto status = empty_queue.wait_for(ul, seconds(max_delay),
                                       [this]() { return (q.size() > 0); });
    if (status == true) {
        store = q.front();
        q.pop();
        full_queue.notify_all();
        return true;
    }

    ESP_LOGI(tag, "Full queue timed out");
    return false;
}