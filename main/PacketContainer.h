#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include "Packet.h"

#define CONTAINER_MAX_SIZE 10

using namespace std;


class PacketContainer{
	mutex m;
	queue<Packet> q;
	int max_size;
	int size;

	condition_variable empty_queue;
	condition_variable full_queue;

public:

	PacketContainer(const int max_size): max_size(max_size), size(0) {};
	~PacketContainer(){};

	void push(Packet& p){
		unique_lock<mutex> ul(m);
		full_queue.wait(ul,[this](){ return q.empty()||(size < max_size); });
		if(size < max_size){
			q.push(p);
			size++;
		}
		empty_queue.notify_all();
	};

	bool timed_push(Packet& p, const int max_delay){
		unique_lock<mutex> ul(m);
		auto status = full_queue.wait_for(ul,chrono::seconds(max_delay),[this](){ return q.empty()||(size < max_size); });
		if(status == true){
			if(size < max_size){
				q.push(p);
				size++;
				empty_queue.notify_all();
				return true;
			}
		} else 
			cout<<"full queue timeout"<<endl;

		return false;
	}

	void pop(Packet& store){
		unique_lock<mutex> ul(m);
		empty_queue.wait(ul,[this](){ return !q.empty(); });
		store = q.front();
		q.pop();
		size--;
		full_queue.notify_all();
	}

	bool timed_pop(Packet& store, const int max_delay){
		unique_lock<mutex> ul(m);
		auto status = empty_queue.wait_for(ul,chrono::seconds(max_delay),[this](){ return !q.empty(); });
		if(status == true){
			store = q.front();
			q.pop();
			size--;
			full_queue.notify_all();
			return true;
		} else 
			cout<<"full queue timeout"<<endl;
		
		return false;
	}
};


extern PacketContainer *packet_queue;