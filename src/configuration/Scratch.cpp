#include "configuration/Scratch.h"


#include "utils/utils.h"

#include <iostream>
#include <cstdlib>

using namespace std;

/*****************BASIC EXPERIMENT SETTINGS*******************/
// The name of the experiment
string 						Scratch::name;
// The number of used cores
int 						Scratch::nCores;
// The duration of the experiment, in microsecond
unsigned long 				Scratch::duration;
// Whether to save the results
bool 						Scratch::isSave;
// Whether to use hardware temperature sensors
bool 						Scratch::useHardwareTempSensor;
// Whether to use soft temperature sensors
bool 						Scratch::useSoftTempSensor;
// This semaphore protects the access to internal members
sem_t 						Scratch::access_sem;
	
// This variable indicates if the clock frequency of the core is fixed
bool 						Scratch::fixedFrequency;
// This variable indicates if the cores will always be active
bool 						Scratch::fixedActive;
// This variable indicates if the tested approach is static
bool 						Scratch::is_staticApproach;

int 						Scratch::isAppendSaveFile;

user_defined_soft_temperature_sensor Scratch::softSensor;


vector<SoftTemperatureSensorConfig> Scratch::soft_sensors;

/*****************TASK SETTINGS*******************/
// This vector stores the periodicity of all tasks	
vector<_task_periodicity> 	Scratch::periodicities;
// This vector stores the types of all tasks
vector<_task_type> 			Scratch::task_types;
// This vector stores the auxiliary data of all tasks
vector<TaskArgument>  		Scratch::all_task_data;

	
/*****************AUXILIARY VARIABLES*******************/
// The tons and toffs for all the cores, if the user want to periodically switch
// the cores to idle state
vector<struct timespec> 	Scratch::PBOO_tons;
vector<struct timespec> 	Scratch::PBOO_toffs;
// the adaption period for an online approach
unsigned long 				Scratch::adaption_period;
// the name of the executed benchmark
string 						Scratch::benchmark;
// The worst-case execution times of the task on every core, if the
// pipelined architecture is adopted
vector<struct timespec> 	Scratch::WCETS;


// This function initialize all the members, it needs the name of the experiment,
// the number of used cores and experiment duration
void Scratch::initialize(int _nCores,
	unsigned long _duration,
	string _name, bool _useHardwareTempSensor, bool _useSoftTempSensor){
	// one second in microsecond
	unsigned long onesecond = 1000000;
	// we directly set the vairables and do not check whether they
	// are valid, because Scratch is only for storing data
	nCores                = _nCores;
	duration              = _duration;
	name                  = _name;
	useHardwareTempSensor = _useHardwareTempSensor;
	useSoftTempSensor     = _useSoftTempSensor;
	isAppendSaveFile	  = 0;


	// set to defaults
	adaption_period       = 1*onesecond;
	isSave                = true;
	benchmark             = "default";
	fixedFrequency        = false;
	fixedActive           = false;
	is_staticApproach     = true;
	WCETS                 = vector<struct timespec> (nCores, TimeUtil::Micros(50000));
	PBOO_tons             = vector<struct timespec> (nCores, TimeUtil::Micros(100000));
	PBOO_toffs            = vector<struct timespec> (nCores, TimeUtil::Micros(10000));

	// mutex semaphore
	sem_init(&access_sem, 0, 1);
}

// This function prints the content of all members
void Scratch::print(){
	cout << "nCores 			= " << nCores << endl;
	cout << "duration 			= " << duration << endl;
	cout << "name 				= " << name << endl;
	cout << "adaption_period 	= " << adaption_period << endl;
	cout << "fixedFrequency 	= " << fixedFrequency << endl;
	cout << "fixedActive 		= " << fixedActive << endl;
	displayvector(TimeUtil::convert_us(PBOO_tons) , "PBOO_tons");
	displayvector(TimeUtil::convert_us(PBOO_toffs), "PBOO_toffs");
	printAllTaskInfo();
}
// This function prints the information of all tasks
void Scratch::printAllTaskInfo(){
	for (int i = 0; i < (int)all_task_data.size(); ++i)
	{	
		all_task_data[i].print();
	}
}

/*********************SET FUNCTIONS****************************/

void Scratch::setName(string newname){
	sem_wait(&access_sem);
	name = newname;
	sem_post(&access_sem);
}

void Scratch::setSavingFile(bool f){
	sem_wait(&access_sem);
	isSave = f;
	sem_post(&access_sem);
}

void Scratch::setFixedFrequency(bool ff){
	sem_wait(&access_sem);
	fixedFrequency = ff;
	sem_post(&access_sem);	
}
void Scratch::setFixedActive(bool fa){
	sem_wait(&access_sem);
	fixedActive = fa;
	sem_post(&access_sem);
}
void Scratch::setStaticApproach(bool sa){
	sem_wait(&access_sem);
	is_staticApproach = sa;
	sem_post(&access_sem);
}

// Add a task with its information
void Scratch::addTask(_task_type type, _task_periodicity p, TaskArgument data){
	task_types.push_back(type);
	periodicities.push_back(p);
	all_task_data.push_back(data);
}

void Scratch::setPBOOTons(std::vector<struct timespec> tons){
	sem_wait(&access_sem);
	PBOO_tons = tons;
	sem_post(&access_sem);
}
void Scratch::setPBOOToffs(std::vector<struct timespec> toffs){
	sem_wait(&access_sem);
	PBOO_toffs = toffs;
	sem_post(&access_sem);
}

void Scratch::setAdaptionPeriod(unsigned long p){
	sem_wait(&access_sem);
	adaption_period = p;
	sem_post(&access_sem);
}

void Scratch::setBenchmark(const string& name){
	sem_wait(&access_sem);
	benchmark = name;
	sem_post(&access_sem);
}

void Scratch::setWCETs(vector<struct timespec> wcets){
	sem_wait(&access_sem);
	WCETS = wcets;
	sem_post(&access_sem);
}


void Scratch::addSoftTempSensor(string name, double _coefA, double _coefB){
	SoftTemperatureSensorConfig temp;
	temp.counterName = name;
	temp.coefA = _coefA;
	temp.coefB = _coefB;
	soft_sensors.push_back(temp);
}

/*********************GET FUNCTIONS****************************/

string Scratch::getName(){
	sem_wait(&access_sem);
	string ret = name;
	sem_post(&access_sem);
	return ret;
}
int Scratch::getNcores(){
	sem_wait(&access_sem);
	int ret = nCores;
	sem_post(&access_sem);
	return ret;
}

unsigned long Scratch::getDuration(){
	sem_wait(&access_sem);
	unsigned long ret = duration;
	sem_post(&access_sem);
	return ret;
}

bool Scratch::isUsingHardwareTempSensor(){
	return useHardwareTempSensor;
}
bool Scratch::isUsingSoftTempSensor(){
	return useSoftTempSensor;
}

bool Scratch::isSaveFile(){
	sem_wait(&access_sem);
	bool ret = isSave;
	sem_post(&access_sem);
	return ret;
}

bool Scratch::isFixedFrequency(){
	sem_wait(&access_sem);
	bool ret = fixedFrequency;
	sem_post(&access_sem);
	return ret;
}
bool Scratch::isFixedActive(){
	sem_wait(&access_sem);
	bool ret = fixedActive;
	sem_post(&access_sem);
	return ret;
}
bool Scratch::isStaticApproach(){
	sem_wait(&access_sem);
	bool ret = is_staticApproach;
	sem_post(&access_sem);
	return ret;
}


vector<_task_periodicity> 	Scratch::getAllTaskPeriodicity(){
	// vector<_task_periodicity> ret;
	sem_wait(&access_sem);
	vector<_task_periodicity> ret = periodicities;
	sem_post(&access_sem);
	return ret;
}

vector<_task_type>	Scratch::getAllTaskTypes(){
	sem_wait(&access_sem);
	vector<_task_type> ret = task_types;
	sem_post(&access_sem);
	return ret;
}

vector<TaskArgument>  Scratch::getTaskData(){
	sem_wait(&access_sem);
	vector<TaskArgument> ret = all_task_data;
	sem_post(&access_sem);
	return ret;
}

// Get the TaskArgument of the task whose id is taskid
TaskArgument  Scratch::getTaskData(int taskid){
	int id = 0;
	bool found = false;
	for (int i = 0; i < (int) all_task_data.size(); ++i)
	{
		if(all_task_data[i].taskId == taskid){
			id = i;
			found = true;
			break;
		}
	}
	if(!found){
		cout << "Scratch::getTaskData: cannot found task data for given task type" << endl;
		exit(1);
	}
	return all_task_data[id];
}


std::vector<struct timespec> Scratch::getPBOOTons(){
	sem_wait(&access_sem);
	vector<struct timespec> ret = PBOO_tons;
	sem_post(&access_sem);
	return ret;
}
std::vector<struct timespec> Scratch::getPBOOToffs(){
	sem_wait(&access_sem);
	vector<struct timespec> ret = PBOO_toffs;
	sem_post(&access_sem);
	return ret;
}

unsigned long Scratch::getAdaptionPeriod(){
	sem_wait(&access_sem);
	unsigned long ret = adaption_period;
	sem_post(&access_sem);
	return ret;

}

vector<struct timespec> Scratch::getWCETs(){
	sem_wait(&access_sem);
	vector<struct timespec> ret = WCETS;
	sem_post(&access_sem);
	return ret;
}

string Scratch::getBenchmarkName(){
	sem_wait(&access_sem);
	string ret = benchmark;
	sem_post(&access_sem);
	return ret;
}

std::string  Scratch::getApproachName(){
	return "test";
}


// std::vector<SoftTemperatureSensorConfig> Scratch::getSoftTempSensorsConfig(){
// 	return soft_sensors;
// }