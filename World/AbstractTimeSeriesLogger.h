#pragma once

#include <string>

class AbstractTimeSeriesLogger {

public:
	virtual void logData(std::string stateLabel, void* sensorTS, void* brainTS, void* motorTS, void* mentalImageTS, int visualize) {};
};
