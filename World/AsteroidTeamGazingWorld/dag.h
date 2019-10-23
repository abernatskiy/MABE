#pragma once

#include <list>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <set>

template<typename T>
struct DAGNode {
	T payload;
	std::vector<T> parents;
	std::vector<T> children;
};

template<typename T>
class DAGraph {
private:
	std::list<DAGNode<T>> nodes; // automatically maintained in topological order by the insertion procedure
	std::set<T> payloads;
public:
	void insertNode(T payload, std::vector<T> parentPayloads);
	void printGraph() const;
	std::vector<T> getOrderedPayloads(T payload) const; // doesn't actually do any sorting cause the graph is sorted by construction
	std::vector<T> getAllAncestors(T payload) const;
	std::vector<T> getParents(T payload) const;
	std::vector<T> getChildren(T payload) const;
	std::pair<std::vector<T>,std::vector<T>> getParentsAndChildren(T payload) const;
};

/***********************/
/***** Definitions *****/
/***********************/

template<typename T>
void DAGraph<T>::insertNode(T payload, std::vector<T> parentPayloads) {
	payloads.insert(payload);
	if(payloads.size()!=nodes.size()+1) {
		std::cerr << "Attempting to insert a duplicate node with payload " << payload << " into the following DAG:" << std::endl;
		printGraph();
		exit(EXIT_FAILURE);
	}

	size_t parentsFound = 0;
	for(auto& pp : parentPayloads) {
		for(auto& node : nodes) {
			if(node.payload==pp) {
				node.children.push_back(payload);
				parentsFound++;
				break;
			}
		}
	}

	if(parentsFound!=parentPayloads.size()) {
		std::cerr << "Some of the parents among";
		for(const auto& pp : parentPayloads)
			std::cerr << " " << pp;
		std::cerr << " have not been found while inserting node " << payload << " into the following DAG:" << std::endl;
		printGraph();
		exit(EXIT_FAILURE);
	}

	DAGNode<T> newNode {payload, parentPayloads, {}};
	nodes.push_back(newNode);
}

template<typename T>
void DAGraph<T>::printGraph() const {
	for(const auto& node : nodes) {
		std::cout << node.payload << ": parents";
		for(const auto& pit : node.parents)
			std::cout << " " << pit;
		std::cout << " children";
		for(const auto& cit : node.children)
			std::cout << " " << cit;
		std::cout << std::endl;
	}
}

template<typename T>
std::vector<T> DAGraph<T>::getOrderedPayloads(T payload) const {
	std::vector<T> allPayloads;
	for(const auto& node : nodes)
		allPayloads.push_back(node.payload);
	return allPayloads;
}

template<typename T>
std::vector<T> DAGraph<T>::getAllAncestors(T payload) const {
	auto immediateAncestors = getParents(payload);
	if(immediateAncestors.size()==0)
		return {};
	else {
		std::set<T> allAncestors;
		for(auto parent : immediateAncestors) {
			allAncestors.insert(parent);
			auto parentsAncestors = getAllAncestors(parent);
			allAncestors.insert(parentsAncestors.begin(), parentsAncestors.end());
		}
		return std::vector<T>(allAncestors.begin(), allAncestors.end());
	}
}

template<typename T>
std::vector<T> DAGraph<T>::getParents(T payload) const {
	for(const auto& node : nodes)
		if(node.payload == payload)
			return node.parents;
	std::cerr << "Couldn't find a node " << payload << " while searching for its parents in graph" << std::endl;
	printGraph();
	exit(EXIT_FAILURE);
}

template<typename T>
std::vector<T> DAGraph<T>::getChildren(T payload) const {
	for(const auto& node : nodes)
		if(node.payload == payload)
			return node.children;
	std::cerr << "Couldn't find a node " << payload << " while searching for its children in graph" << std::endl;
	printGraph();
	exit(EXIT_FAILURE);
}

template<typename T>
std::pair<std::vector<T>,std::vector<T>> DAGraph<T>::getParentsAndChildren(T payload) const {
	for(const auto& node : nodes)
		if(node.payload == payload)
			return std::make_pair(node.parents, node.children);
	std::cerr << "Couldn't find a node " << payload << " while searching for its parents and children in graph" << std::endl;
	printGraph();
	exit(EXIT_FAILURE);
}
