#pragma once

#include <vector>
#include <numeric>
#include <cmath>
#include <map>

inline long unsigned positionalBinaryToDecimal(const std::vector<bool>& thebinary) {
	return std::accumulate(thebinary.cbegin(), thebinary.cend(), 0, [](int result, bool bv) { return result*2 + (bv ? 1 : 0); });
}


template<typename T>
inline void incrementMapFieldRobustly(T key, std::map<T,double>& map) {
	if(map.find(key)==map.end())
		map[key] = 1.;
	else
		map[key]++;
}

/*
long unsigned pow2(unsigned i) {
	long unsigned answ = 1;
	for(unsigned j=0; j<i; j++)
		answ <<= 1;
	return answ;
}
*/
