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

using namespace std;

class Timeline{
	clock_t start; 
	//this is an offset that has to be added to the server's clock to generate a valid timestamp for the packet
	time_t now;
    struct tm timeinfo;
    mutex m; //concurrent access to now variable

public:

	Timeline();
	//double now();
	//void reset(); //reset the start;
	void sync_time();
	time_t get_time(){ 
		lock_guard<mutex> lg(m);
		return now; 
	}
	void initialize_sntp();
	void print_time();

};

extern Timeline *system_timeline;