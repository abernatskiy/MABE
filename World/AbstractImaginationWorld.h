#pragma once

#include "AbstractIsolatedEmbodiedWorld.h"
#include "AbstractMentalImage.h"
#include "ImaginationMotors.h"
#include "../Utilities/Data.h"

// Abstract class for Worlds in which agents do only two things: perceive and imagine

// BG:
// As morning comes the Observer will wake up,
// So that it could go to sleep...

class AbstractImaginationWorld : public AbstractIsolatedEmbodiedWorld {

protected:
	// Two pointers must be initialized to appropriate daughter classes' pointers downcasts in the daughter classes' constuctors:
	// 1. std::shared_ptr<AbstractSensors> sensors, inherited from AbstractIsolatedEmbodiedWorld, and
	// 2. this one:
	std::shared_ptr<AbstractMentalImage> mentalImage;
	// Also, make sure to call makeMotors() after the image pointer is set up.

	void makeMotors() { motors = std::make_shared<ImaginationMotors>(mentalImage); }; // conveniently encapsulates call to ImaginationMotors constructor
	                                                                            // so that there's no need to mind it in daughter classes

	virtual void resetWorld(int visualize) override {
		AbstractIsolatedEmbodiedWorld::resetWorld(visualize);
		mentalImage->reset(visualize);
	}; // keeping it virtual because state of the World is likely to grow

private:
	// Stuff from AbstractIsolatedEmbodiedWorld that stays virtual and must be defined in daughter classes
	// virtual void postEvaluationOuterWorldUpdate(unsigned long timeStep, int visualize) = 0; // likely, a schedule of world changes (a "slideshow"
	//                                                                                         // in the discrete case) will be implemented here
	// virtual bool endEvaluation(unsigned long timeStep) = 0; // evaluation termination conditions

	std::shared_ptr<DataMap> runningScores;
	std::shared_ptr<DataMap> sampleScores;

	// agent does not affect world except by modifying the mental image
	void preEvaluationOuterWorldUpdate(std::shared_ptr<Organism> org, unsigned long timeStep, int visualize) override {};

	// Mental image class knows better how to compare itself to the ground truth and what data can be provided to the Organism
	void recordRunningScores(std::shared_ptr<Organism> org, unsigned long evalTime, int visualize) override {
		mentalImage->recordRunningScores(org, runningScores, evalTime, visualize);
	};
	void recordSampleScores(std::shared_ptr<Organism> org, unsigned long evalTime, int visualize) override {
		mentalImage->recordSampleScores(org, sampleScores, runningScores, evalTime, visualize);
		runningScores->clearMap();
	};
	void evaluateOrganism(std::shared_ptr<Organism> currentOrganism, int visualize) override {
		mentalImage->evaluateOrganism(currentOrganism, sampleScores, visualize);
		sampleScores->clearMap();
	};

public:
	AbstractImaginationWorld(std::shared_ptr<ParametersTable> PT_) : AbstractIsolatedEmbodiedWorld(PT_) {
		runningScores = std::make_shared<DataMap>();
		sampleScores = std::make_shared<DataMap>();
	};
	virtual ~AbstractImaginationWorld() = default;
};
