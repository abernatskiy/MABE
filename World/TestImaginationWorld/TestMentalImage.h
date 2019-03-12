#pragma once

#include "../AbstractMentalImage.h"

class TestMentalImage : public AbstractMentalImage {

private:
	std::shared_ptr<int> worldState;
	int state;

public:
	TestMentalImage(std::shared_ptr<int> wState) : state(0), worldState(wState) {};
  void reset(int visualize) override { state=0; };
  void updateWithInputs(std::vector<double> inputs) override {
		state = 0;
		for(double v : inputs) {
			int curval = v==0. ? 0 : 1;
			state += curval;
		}
	};
  void recordRunningScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override {
		if(visualize) std::cout << "World state is " << (*worldState) << " and the network output is " << state << std::endl;
		double curScore = 1. / ( 1. + (state>(*worldState) ? state-(*worldState) : (*worldState)-state) );
		runningScoresMap->append("score", curScore);
	};
  void recordSampleScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override {
		sampleScoresMap->append("score", runningScoresMap->getAverage("score"));
	};
  void evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) override {
		if(visualize) std::cout << "Assigning score of " << sampleScoresMap->getAverage("score") << std::endl;
		org->dataMap.append("score", sampleScoresMap->getAverage("score"));
	};

  int numInputs() override {return 10;};
};
