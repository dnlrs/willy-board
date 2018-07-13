#include "Timeline.h"

Timeline::Timeline(){
	this->start = clock();
}

double
Timeline::now(){
	if(this->start == -1){
		std::cout<<"Timeline not started."<<std::endl;
		return -1;
	}

	return (clock() - (this->start))/((double)CLOCKS_PER_SEC);
}

void
Timeline::reset(){
	this->start = clock();
}