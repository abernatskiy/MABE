#include "DerivedMarkovBrain.h"

using namespace std;

DerivedMarkovBrain::DerivedMarkovBrain(int ins, int outs, shared_ptr<ParametersTable> PT_) :
	AbstractBrain(ins, outs, PT_),
	mb(make_shared<ClassicGateListBuilder>(PT_), ins, outs, PT_) {}

void DerivedMarkovBrain::update() {
	for(unsigned i=0; i<nrInputValues; i++)
		mb.inputValues[i] = inputValues[i];
	mb.update();
	for(unsigned i=0; i<nrOutputValues; i++)
		outputValues[i] = mb.outputValues[nrInputValues+i];
}

DataMap DerivedMarkovBrain::getStats(string& prefix) {
	return mb.getStats(prefix);
}

string DerivedMarkovBrain::description() {
	return mb.description();
}

void DerivedMarkovBrain::resetBrain() {
	AbstractBrain::resetBrain();
	mb.resetBrain();
}

shared_ptr<AbstractBrain> DerivedMarkovBrain::makeBrain(unordered_map<string,shared_ptr<AbstractGenome>>& _genomes) {
//	return make_shared<DerivedMarkovBrain>(nrInputValues, nrOutputValues, PT);
	return mb.makeBrain(_genomes);
}

shared_ptr<AbstractBrain> DerivedMarkovBrain::makeCopy(shared_ptr<ParametersTable> PT_) {
	if(PT_==nullptr) {
		cerr << "DEMB makeCopy caught a nullptr" << endl;
		exit(1);
	}
	return make_shared<DerivedMarkovBrain>(*this);
}

void DerivedMarkovBrain::initializeGenomes(std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) {
	mb.initializeGenomes(_genomes);
}

/********** Private definitions **********/

DerivedMarkovBrain::DerivedMarkovBrain(const DerivedMarkovBrain& original) :
	AbstractBrain(original.nrInputValues, original.nrOutputValues, original.PT),
	mb(original.getCopiesOfAllGates(), original.nrInputValues, original.nrOutputValues, original.PT) {}

vector<shared_ptr<AbstractGate>> DerivedMarkovBrain::getCopiesOfAllGates() const {
	vector<shared_ptr<AbstractGate>> _gates;
	for (auto gate : mb.gates)
		_gates.push_back(gate->makeCopy());
	return _gates;
}

