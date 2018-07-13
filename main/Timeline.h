/*
 * Timeline.h
 */
#pragma once

#include <cstdlib>
#include <cstdio>
#include <time.h>
#include <iostream>

class Timeline{
	clock_t start; 
	//this is an offset that has to be added to the server's clock to generate a valid timestamp for the packet

public:

	Timeline();
	double now();
	void reset(); //reset the start;

};