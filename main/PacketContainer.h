#ifndef PACKETCONTAINER_H_INCLUDED
#define PACKETCONTAINER_H_INCLUDED
#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "Packet.h"

#define PACKET_QUEUE_MAX_SIZE 512

class PacketContainer {
    std::queue<Packet> q;
    int max_size = PACKET_QUEUE_MAX_SIZE;

    // sync utils
    std::mutex m;
    std::condition_variable empty_queue;
    std::condition_variable full_queue;

    // logging tag
    static constexpr char const* tag = "wwb-PacketContainer";

public:
    PacketContainer() : max_size(PACKET_QUEUE_MAX_SIZE){};
    ~PacketContainer(){};

    void push(Packet& p);
    bool timed_push(Packet& p, const int max_delay);

    void pop(Packet& store);
    bool timed_pop(Packet& store, const int max_delay);
};

#endif // !PACKETCONTAINER_H_INCLUDED