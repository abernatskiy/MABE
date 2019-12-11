//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include "DeterministicGate.h"

shared_ptr<ParameterLink<string>> DeterministicGate::IO_RangesPL = Parameters::register_parameter("BRAIN_MARKOV_GATES_DETERMINISTIC-IO_Ranges", (string)"1-4,1-4", "range of number of inputs and outputs (min inputs-max inputs,min outputs-max outputs)");

DeterministicGate::DeterministicGate(pair<vector<int>,vector<int>> addresses, vector<vector<int>> _table, int _ID, shared_ptr<ParametersTable> _PT) :
	AbstractGate(_PT) {
	ID = _ID;
	inputs = addresses.first;
	outputs = addresses.second;
	table = _table;
}

//void DeterministicGate::setupForBits(int* Ins, int nrOfIns, int Out, int logic) {
//	inputs.resize(nrOfIns);
//	for (int i = 0; i < nrOfIns; i++)
//		inputs[i] = Ins[i];
//	outputs.resize(1);
//	outputs[0] = Out;
//	table.resize(1 << nrOfIns);
//	for (int i = 0; i < (1 << nrOfIns); i++) {
//		table[i].resize(1);
//		table[i][0] = (logic >> i) & 1;
//	}
//}

void DeterministicGate::update(vector<double> & nodes, vector<double> & nextNodes) {
	int input = vectorToBitToInt(nodes,inputs,true); // converts the input values into an index (true indicates to reverse order)
	for (size_t i = 0; i < outputs.size(); i++) {
		nextNodes[outputs[i]] += table[input][i];
	}
}

shared_ptr<AbstractGate> DeterministicGate::makeCopy(shared_ptr<ParametersTable> _PT) {
	if (_PT == nullptr) {
		_PT = PT;
	}
	auto newGate = make_shared<DeterministicGate>(_PT);
	newGate->table = table;
	newGate->ID = ID;
	newGate->inputs = inputs;
	newGate->outputs = outputs;
	return newGate;
}

void DeterministicGate::mutateInternalStructure() {
	int inPatIdx = Random::getInt(0, table.size()-1);

	// True point mutation
	// int outIdx = Random::getInt(0, table[0].size()-1);
	// table[inPatIdx][outIdx] = table[inPatIdx][outIdx] ? 0 : 1;

	// Randomize output pattern mutation
	// It is supposed to make info landscape convex with an optimum at 1, but its empirical performance is questionable
	size_t outPatternWidth = table[inPatIdx].size();
	vector<int> newOutPattern(outPatternWidth);
	while(1) {
		for(size_t i=0; i<outPatternWidth; i++)
			newOutPattern[i] = Random::getInt(0, 1);
		if(table[inPatIdx]!=newOutPattern) {
			table[inPatIdx] = newOutPattern;
			break;
		}
	}
}
