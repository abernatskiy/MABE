#include "DEMarkovBrain.h"

using namespace std;

DEMarkovBrain::DEMarkovBrain(int ins, int outs, shared_ptr<ParametersTable> PT_) :
	AbstractBrain(ins, outs, PT_),
	mb(make_shared<ClassicGateListBuilder>(PT_), ins, outs, PT_) {}

void DEMarkovBrain::update() {
	for(unsigned i=0; i<nrInputValues; i++)
		mb.inputValues[i] = inputValues[i];
	mb.update();
	for(unsigned i=0; i<nrOutputValues; i++)
		outputValues[i] = mb.outputValues[nrInputValues+i];
}

DataMap DEMarkovBrain::getStats(string& prefix) {
	return mb.getStats(prefix);
}

string DEMarkovBrain::description() {
	return mb.description();
}

void DEMarkovBrain::resetBrain() {
	AbstractBrain::resetBrain();
	mb.resetBrain();
}

shared_ptr<AbstractBrain> DEMarkovBrain::makeBrain(unordered_map<string,shared_ptr<AbstractGenome>>& _genomes) {
//	return make_shared<DEMarkovBrain>(nrInputValues, nrOutputValues, PT);
	return mb.makeBrain(_genomes);
}

shared_ptr<AbstractBrain> DEMarkovBrain::makeCopy(shared_ptr<ParametersTable> PT_) {
	if(PT_==nullptr) {
		cerr << "DEMB makeCopy caught a nullptr" << endl;
		exit(1);
	}
	return make_shared<DEMarkovBrain>(*this);
}

void DEMarkovBrain::initializeGenomes(std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) {
	mb.initializeGenomes(_genomes);
}

/********** Private definitions **********/

DEMarkovBrain::DEMarkovBrain(const DEMarkovBrain& original) :
	AbstractBrain(original.nrInputValues, original.nrOutputValues, original.PT),
	mb(original.getCopiesOfAllGates(), original.nrInputValues, original.nrOutputValues, original.PT) {}

vector<shared_ptr<AbstractGate>> DEMarkovBrain::getCopiesOfAllGates() const {
	vector<shared_ptr<AbstractGate>> _gates;
	for (auto gate : mb.gates)
		_gates.push_back(gate->makeCopy());
	return _gates;
}

