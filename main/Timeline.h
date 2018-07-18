/*
 * Timeline.h
 */
#pragma once

#include <cstdlib>
#include <cstdio>
#include <time.h>
#include <iostream>
#include "lwip/err.h" 
#include "apps/sntp/sntp.h"
#include <mutex>

#define TIMESTAMP_VALIDATION 1514764801 //timestamp is valid if it's greater than 2018-01-01 

using namespace std;

class Timeline{
	clock_t start_offset; 
	//this is an offset that has to be added to the server's clock to generate a valid timestamp for the packet
	time_t central_clock;
    struct tm timeinfo;
    mutex m; //concurrent access to now variable

public:

	Timeline(){};
	//double now();
	//void reset(); //reset the start;
	void sync_time();
	time_t get_time(){ 
		lock_guard<mutex> lg(m);
		return central_clock + ((clock() - start_offset)/((time_t)CLOCKS_PER_SEC)); 
	}
	void initialize_sntp();
	void print_time();

};

extern Timeline *system_timeline;