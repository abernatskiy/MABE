#pragma once

#include <string>
#include <cstdlib>
#include <errno.h>
#include <limits.h>

// Regards to Jonathan Boccara
// https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/

class WordDelimitedByComma : public std::string{};
class WordDelimitedBySemicolon : public std::string{};
class WordDelimitedByPipe : public std::string{};

inline std::istream& operator>>(std::istream& is, WordDelimitedByComma& output) { std::getline(is, output, ','); return is; }
inline std::istream& operator>>(std::istream& is, WordDelimitedBySemicolon& output) { std::getline(is, output, ';'); return is; }
inline std::istream& operator>>(std::istream& is, WordDelimitedByPipe& output) { std::getline(is, output, '|'); return is; }

// Regards to James Kanze
// https://stackoverflow.com/questions/14893264

inline bool isLegalUInt(const std::string& input) {
	char* end;
	errno = 0;
	unsigned long v = strtoul( input.c_str(), &end, 10 );
	return errno == 0 && *end == '\0' && end != input.c_str() && v <= UINT_MAX;
}

