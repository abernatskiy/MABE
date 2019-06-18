#pragma once

#include "../Brain/AbstractBrain.h"
#include "../Utilities/nlohmann/json.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <iomanip>

class AbstractSensors {

protected:
	std::shared_ptr<AbstractBrain> brain;
	unsigned long clock;

public:
	void attachToBrain(std::shared_ptr<AbstractBrain> br) { brain=br; };

	// Overloading of all four private methods below is encouraged, but call the prototypes withing the extensions for the void ones
	virtual void reset(int visualize) { clock=0; };
	virtual void update(int visualize) { clock++; };
	virtual int numOutputs() = 0;
	virtual int numInputs() { return 0; }; // reload for active perception
	virtual void* logTimeSeries(const std::string& label) { return nullptr; }; // optionally returns a pointer to an arbitrary data structure for global processing
	virtual unsigned numSaccades() { return 0; }; // reload for active perception
	virtual const std::vector<bool>& getLastPercept() { return std::vector<bool>(); };
	virtual nlohmann::json getSensorStats() {
		nlohmann::json sensorStatsJSON = nlohmann::json::object();
		sensorStatsJSON["numInputs"] = numInputs();
		sensorStatsJSON["numOutputs"] = numOutputs();
		return sensorStatsJSON;
	}
	void writeSensorStats() {
		std::ofstream ssf(Global::outputPrefixPL->get() + "sensorStats.json");
		ssf << std::setw(4) << getSensorStats() << std::endl;
		ssf.close();
	};
	virtual unsigned numActiveStatesInRecording() { return 0; };
	virtual unsigned numStatesInRecording() { return 1; };
};
