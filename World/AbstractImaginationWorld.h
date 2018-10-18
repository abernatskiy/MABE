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
	std::shared_ptr<AbstractMentalImage> image;
	std::shared_ptr<DataMap> runningScores;
	std::shared_ptr<DataMap> sampleScores;

private:
	void resetWorld(int visualize) override { image->reset(visualize); };
	void updateExtraneousWorld(unsigned long timeStep, int visualize) override {}; // nothing happens in the world besides image updates which are made by the actuator

	// Mental image class knows better how to compare itself to the ground truth and what data can be provided to the Organism
	void recordRunningScores(unsigned long evalTime, int visualize) override {
		image->recordRunningScores(runningScores, evalTime, visualize);
	};
	void recordSampleScores(unsigned long evalTime, int visualize) override {
		image->recordSampleScores(sampleScores, runningScores, evalTime, visualize);
		runningScores->clearMap();
	};
	void evaluateOrganism(std::shared_ptr<Organism> currentOrganism, int visualize) override {
		image->evaluateOrganism(currentOrganism, sampleScores, visualize);
		sampleScores->clearMap();
	};

	// Stuff from AbstractIsolatedEmbodiedWorld that stays virtual: evaluation termination condition
	// virtual bool endEvaluation(unsigned long timeStep) = 0;

	// New virtual stuff: functions constructing appropriate objects for sensors and mental image
	virtual std::shared_ptr<AbstractMentalImage> makeMentalImage() = 0;
	virtual std::shared_ptr<AbstractSensors> makeSensors() = 0;

public:
	AbstractImaginationWorld(std::shared_ptr<ParametersTable> PT_) : AbstractIsolatedEmbodiedWorld(PT_) {
		runningScores = std::make_shared<DataMap>();
		sampleScores = std::make_shared<DataMap>();
		image = makeMentalImage();
		motors = std::make_shared<ImaginationMotors>(image);
		sensors = makeSensors();
	};
	virtual ~AbstractImaginationWorld() = default;
};
