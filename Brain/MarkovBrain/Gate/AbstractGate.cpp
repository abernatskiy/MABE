//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include "AbstractGate.h"

#include <iostream>

// *** General tools for All Gates ***

// converts values attained from genome for inputs and outputs to vaild brain state ids
// uses a pair of lookup tables - inputNodeMap and outputNodeMap
void AbstractGate::applyNodeMaps(const vector<int>& inputNodeMap, const vector<int>& outputNodeMap) {
	for (size_t i = 0; i < inputs.size(); i++)
		inputs[i] = inputNodeMap[inputs[i]];
	for (size_t i = 0; i < outputs.size(); i++)
		outputs[i] = outputNodeMap[outputs[i]];
}

shared_ptr<AbstractGate> AbstractGate::makeCopy(shared_ptr<ParametersTable> _PT) {
	cout << "ERROR IN AbstractGate::makeCopy() - You are using the abstract copy constructor for gates. You must define your own" << endl;
	exit(1);
}

void AbstractGate::resetGate() {
	//nothing to reset here!
}

vector<int> AbstractGate::getIns() {
	return inputs;
}

vector<int> AbstractGate::getOuts() {
	return outputs;
}

string AbstractGate::descriptionIO() {
	string S = "IN:";
	for (size_t i = 0; i < inputs.size(); i++)
		S = S + " " + to_string(inputs[i]);
	S = S + "\n";
	S = S + "OUT:";
	for (size_t i = 0; i < outputs.size(); i++)
		S = S + " " + to_string(outputs[i]);
	S = S + "\n";
	//S = S + getCodingRegions();
	return S;
}

string AbstractGate::description() {
	return "Gate " + to_string(ID) + " is a " + gateType() + " gate\n" + descriptionIO();
}

void AbstractGate::mutateConnections(int ista, int iend, int osta, int oend) {
	int chosenConn = Random::getInt(0, inputs.size()+outputs.size()-1);
	if(chosenConn < inputs.size())
		inputs[chosenConn] = Random::getInt(ista, iend);
	else
		outputs[chosenConn-inputs.size()] = Random::getInt(osta, oend);
}
