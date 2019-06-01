#pragma once

#include <algorithm>
#include <vector>
#include <string>
#include <sstream>

// Decoders based on "one-hot" encoding
// Quotes are due to the fact that only the leftmost one counts, if there are any more ones to the right of it the decoder does not care
// This makes all states valid

inline unsigned decodeOHUInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	return std::distance(begin, std::max_element(begin, end));
}

inline int decodeOHSInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	int offsetArgmax = std::distance(begin+1, std::max_element(begin+1, end));
	return (*begin)<0.5 ? offsetArgmax : -1*offsetArgmax;
}

inline double decodeOHDouble(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	return static_cast<double>(std::distance(begin, std::max_element(begin, end))) / static_cast<double>(std::distance(begin, end));
}

// Decoders based on the standard positional encoding

inline unsigned decodeSPUInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	unsigned retval = 0;
	while(begin != end) {
		retval <<= 1;
		retval += static_cast<unsigned>(*begin > 0.5);
		begin++;
	}
	return retval;
}

// Decoders based on "multiple-hot" encoding, sometimes with veto bits (Chapman, Hintze and co)

inline std::vector<unsigned> decodeMHUInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	// no veto bits, any positive distance between begin and end is acceptable
	std::vector<unsigned> outs;
	auto curpos = begin;
	unsigned curval = 0;
	while(curpos!=end) {
		if(*curpos!=0.)
			outs.push_back(curval);
		curval++;
		curpos += 1;
	}
	return outs;
}

inline std::vector<unsigned> decodeMHVUInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	// one veto bit per bit, distance between begin and end must be even
	std::vector<unsigned> outs;
	auto curpos = begin;
	unsigned curval = 0;
	while(curpos!=end) {
		if(*curpos==0. && *(curpos+1)!=0.) // curpos is a veto; the field immediately following indicates if the decision has been made
			outs.push_back(curval);
		curval++;
		curpos += 2;
	}
	return outs;
}

inline std::vector<unsigned> decodeMH2VUInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	// two veto bits per bit, distance between begin and end must be a multiple of three
	std::vector<unsigned> outs;
	auto curpos = begin;
	unsigned curval = 0;
	while(curpos!=end) {
		if(*curpos==0. && *(curpos+1)==0. && *(curpos+2)!=0.) // curpos and curpos+1 are vetos; the field immediately following them indicates if the decision has been made
			outs.push_back(curval);
		curval++;
		curpos += 3;
	}
	return outs;
}

// Miscellaneous decoders

inline bool decodeTriggerBits(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	if(begin==end)
		return false;
	for(auto it=begin; it!=end; it++) {
		if(*it==0)
			return false;
	}
//	std::cout << "Trigger pressed! Trigger bits were";
//	for(auto it=begin; it!=end; it++)
//		std::cout << " " << *it;
//	std::cout << std::endl;

	return true;
}

// Utilities

inline std::string bitRangeToStr(std::vector<double>::iterator startAt, unsigned bits) {
	std::ostringstream s;
	for(auto it=startAt; it!=startAt+bits; it++)
		s << ( *it==0. ? 0 : 1 );
	return s.str();
}
