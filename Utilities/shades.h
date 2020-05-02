#include <string>

inline std::string shade128(std::uint8_t value) {
	if(value<26)
		return " ";
	else if(value<51)
		return "░";
	else if(value<76)
		return "▒";
	else if(value<101)
		return "▓";
	else
		return "█";
}

inline std::string shadeBinary(bool value) {
	if(value)
		return "█";
	else
		return " ";
}
