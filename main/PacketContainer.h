#ifndef PACKETCONTAINER_H_INCLUDED
#define PACKETCONTAINER_H_INCLUDED
#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "Packet.h"

#define PACKET_QUEUE_MAX_SIZE 512

using std::condition_variable;
using std::mutex;
using std::queue;
using std::unique_lock;

class PacketContainer {
    queue<Packet> q;
    int max_size = PACKET_QUEUE_MAX_SIZE;

    // sync utils
    mutex m;
    condition_variable empty_queue;
    condition_variable full_queue;

    // logging tag
    static constexpr char const* tag = "wwb-PacketContainer";

public:
    PacketContainer() : max_size(PACKET_QUEUE_MAX_SIZE){};
    ~PacketContainer(){};

    void push(Packet& p)
    {
        unique_lock<mutex> ul(m);
        full_queue.wait(ul, [this]() { return (q.size() < max_size); });
        q.push(p);
        empty_queue.notify_all();
    };

    bool timed_push(Packet& p, const int max_delay)
    {
        unique_lock<mutex> ul(m);
        auto status =
            full_queue.wait_for(ul, chrono::seconds(max_delay),
                                [this]() { return (q.size() < max_size); });
        if (status == true) {
            q.push(p);
            empty_queue.notify_all();
            return true;
        }

        ESP_LOGI(tag, "Full queue timeout");
        return false;
    }

    void pop(Packet& store)
    {
        unique_lock<mutex> ul(m);
        empty_queue.wait(ul, [this]() { return (q.size() > 0); });
        store = q.front();
        q.pop();
        full_queue.notify_all();
    }

    bool timed_pop(Packet& store, const int max_delay)
    {
        unique_lock<mutex> ul(m);
        auto status = empty_queue.wait_for(ul, chrono::seconds(max_delay),
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
};

#endif // !PACKETCONTAINER_H_INCLUDED