#pragma once

#include <fstream>
#include <algorithm>
#include <vector>
#include <iostream>
#include <cstdlib>

template<typename T>
class SerializeableArray {
public:
	size_t numElements;
	T* elements;

	SerializeableArray();
	SerializeableArray(size_t numElems);
	SerializeableArray(size_t numElems, T* elems);
	SerializeableArray(const std::vector<T>& elems);
	~SerializeableArray();
	void serialize(std::string filename) const;
	void deserialize(std::string filename);
	void reserve(size_t numElems); // will exit if applied to a non-empty array
	size_t size() const { return numElements; };
	T& operator[](size_t i) { return elements[i]; };
	bool empty() const { return elements==nullptr && numElements==0; }
};

template<typename T>
SerializeableArray<T>::SerializeableArray() :
	numElements(0), elements(nullptr) {};

template<typename T>
SerializeableArray<T>::SerializeableArray(size_t numElems) :
	numElements(numElems) {
	elements = new T[numElements];
}

template<typename T>
SerializeableArray<T>::SerializeableArray(size_t numElems, T* elems) :
	SerializeableArray(numElems) {
	std::copy(elems, elems+numElems, elements);
}

template<typename T>
SerializeableArray<T>::SerializeableArray(const std::vector<T>& elems) :
	SerializeableArray(elems.size()) {
	std::copy(elems.begin(), elems.end(), elements);
}

template<typename T>
SerializeableArray<T>::~SerializeableArray() {
	if(elements)
		delete [] elements;
}

template<typename T>
void SerializeableArray<T>::serialize(std::string filename) const {
	std::ofstream outfile(filename, std::ios::out | std::ios::binary);
	outfile.write((char*) &numElements, sizeof(size_t));
	outfile.write((char*) elements, numElements*sizeof(T));
}

template<typename T>
void SerializeableArray<T>::deserialize(std::string filename) {
	std::ifstream infile(filename, std::ios::in | std::ios::binary);
	infile.read((char*) &numElements, sizeof(size_t));
	elements = new T[numElements];
	infile.read((char*) elements, numElements*sizeof(T));
}

template<typename T>
void SerializeableArray<T>::reserve(size_t numElems) {
	if(numElements!=0 || elements!=nullptr) {
		std::cerr << "ServiceableArray: reserve() called on an array that already uses some memory" << std::endl;
		exit(EXIT_FAILURE);
	}
	numElements = numElems;
	elements = new T[numElems];
}
