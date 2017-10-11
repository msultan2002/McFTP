#include "core/TaskLoad.h"

#include <iostream>

volatile double flop_a = 0.5, flop_b = 2.2;

TaskLoad::TaskLoad(){
	sem_init(&suspend_sem, 0, 0);
	sem_init(&resume_sem, 0, 0);
	sem_init(&stop_sem, 0, 0);

	wcet_us = 0;
	
	sleepEnd.tv_sec = 0;
    sleepEnd.tv_nsec = 0;

    TASKLOADCOUNTER = 0;
    TASKLOADSTOPCOUNTER = 0;

    // isInterrupted = false;
}

TaskLoad::~TaskLoad(){}

bool TaskLoad::LoadsInterface(unsigned long _wcet_us){
	initCheckCounter();
	runLoads(_wcet_us);
	return true;
}


bool TaskLoad::runLoads(unsigned long _wcet_us){

	std::cout << "TaskLoad::runLoads: This should not print!\n";
	return false;
}

void TaskLoad::suspend(const struct timespec& _sleepEndTime){
	sleepEnd = _sleepEndTime;
	sem_post(&suspend_sem);
}

void TaskLoad::resume(){
	sem_post(&resume_sem);
}


void TaskLoad::stop(){
	sem_post(&stop_sem);
}

void TaskLoad::dummy(void* array){
	(void) array;
}

void TaskLoad::do_flops( int n )
{
	int i;
	double c = 0.11;

	for ( i = 0; i < n; i++ ) {
		c += flop_a * flop_b;
	}
	dummy( ( void * ) &c );
}