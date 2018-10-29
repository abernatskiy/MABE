#pragma once

#include "AbstractImaginationWorld.h"
#include "AbstractStateSchedule.h"

class AbstractSlideshowWorld : public AbstractImaginationWorld {

protected:
	// List of pointers that must be assigned to proper downcasts in the World's constructor:
	// std::shared_ptr<AbstractSensors> sensors
	// std::shared_ptr<AbstractMentalImage> mentalImage
	// and a new one:
	std::shared_ptr<AbstractStateSchedule> stateSchedule; // must be kinda like a motor: takes a pointer to some subset of World state upon construction, modifies it when advance() is called

private:
	// New function to override. These two are the only ones.
	virtual bool resetAgentBetweenStates() = 0;
	virtual int brainUpdatesPerWorldState() = 0;

	// Do not mind these following ones in the extensions
	virtual void resetWorld(int visualize) override {
		AbstractImaginationWorld::resetWorld(visualize);
		stateSchedule->reset(visualize);
	};

	void postEvaluationOuterWorldUpdate(unsigned long timeStep, int visualize) override {
		if(timeStep>0 && (timeStep+1)%brainUpdatesPerWorldState() == 0) {
			stateSchedule->advance(visualize);
			if(resetAgentBetweenStates()) {
				sensors->reset(visualize);
				brain->resetBrain();
				motors->reset(visualize);
				// no need to reattach brain to sensors and motors since it doesn't come off in the reset
			}
		}
	};

	bool endEvaluation(unsigned long timeStep) override {
		return stateSchedule->stateIsFinal();
	};

public:
	AbstractSlideshowWorld(std::shared_ptr<ParametersTable> PT_) : AbstractImaginationWorld(PT_) {};
	virtual ~AbstractSlideshowWorld() = default;

};
