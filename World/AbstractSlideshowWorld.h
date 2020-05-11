#pragma once

#include "AbstractImaginationWorld.h"
#include "AbstractStateSchedule.h"
#include "AbstractTimeSeriesLogger.h"

class AbstractSlideshowWorld : public AbstractImaginationWorld {

protected:
	// List of pointers that must be assigned to proper downcasts in the World's constructor:
	// std::shared_ptr<AbstractSensors> sensors
	// std::shared_ptr<AbstractMentalImage> mentalImage
	// and a new one:
	std::shared_ptr<AbstractStateSchedule> stateSchedule; // must be kinda like a motor: takes a pointer to some subset of World state upon construction, modifies it when advance() is called
	std::shared_ptr<AbstractTimeSeriesLogger> timeSeriesLogger;

	// New functions to override. These two are the only ones.
	virtual bool resetAgentBetweenStates() = 0;
	virtual int brainUpdatesPerWorldState() = 0;

	// Do not mind these following ones in the extensions
	virtual void resetWorld(int visualize) override {
		AbstractImaginationWorld::resetWorld(visualize);
		stateSchedule->reset(visualize);
	};

	void postEvaluationOuterWorldUpdate(std::shared_ptr<Organism> org, unsigned long timeStep, int visualize) override {
		mentalImage->recordRunningScoresWithinState(org, timeStep%brainUpdatesPerWorldState(), brainUpdatesPerWorldState());
		if((timeStep+1)%brainUpdatesPerWorldState() == 0) {
			if(resetAgentBetweenStates()) {
//				std::cout << "Resetting everything" << std::endl << std::flush;
				if(visualize) {
					std::string stateLabel = std::string("state_") + stateSchedule->currentStateDescription();
					void* sts; void* bts; void* mts; void* mits;
					sts = sensors->logTimeSeries(stateLabel);
					bts = brain->logTimeSeries(stateLabel);
					mts = motors->logTimeSeries(stateLabel);
					mits = mentalImage->logTimeSeries(stateLabel);
					timeSeriesLogger->logData(stateLabel, sts, bts, mts, mits, visualize);
				}
				sensors->reset(visualize);
				brain->resetBrain();
				motors->reset(visualize);
				// no need to reattach brain to sensors and motors since it doesn't come off in the reset
				mentalImage->resetAfterWorldStateChange(visualize);
//				std::cout << "Optional reset stage passed" << std::endl << std::flush;
			}
//			std::cout << "Advancing the schedule: period " << brainUpdatesPerWorldState() << ", timeStep " << timeStep << std::endl << std::flush;
//			std::cout << "State schedule pointer: " << stateSchedule << std::endl << std::flush;
			stateSchedule->advance(visualize);
//			std::cout << "State schedule has been advanced" << std::endl << std::flush;
		}
	};

	bool endEvaluation(unsigned long timeStep) override {
		return stateSchedule->stateIsFinal();
	};

public:
	AbstractSlideshowWorld(std::shared_ptr<ParametersTable> PT_) :
		AbstractImaginationWorld(PT_),
		timeSeriesLogger(std::make_shared<AbstractTimeSeriesLogger>()) {};
	virtual ~AbstractSlideshowWorld() = default;
};
