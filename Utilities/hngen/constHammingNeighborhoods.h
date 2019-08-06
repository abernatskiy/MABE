// use it as vector<vector<map<char,vector<char>>>>
// first field is the number of flippable bits
// second is the number of bits that are to be flipped
// third is the character the neighborhood of which is sought

#pragma once

#define NEIGHBORHOODS1OF1 {\
{'0',{'1'}},\
{'1',{'0'}}}

#define NEIGHBORHOODS1OF2 {\
{'0',{'1','2'}},\
{'1',{'0','3'}},\
{'2',{'3','0'}},\
{'3',{'2','1'}}}

#define NEIGHBORHOODS2OF2 {\
{'0',{'3'}},\
{'1',{'2'}},\
{'2',{'1'}},\
{'3',{'0'}}}

#define NEIGHBORHOODS1OF3 {\
{'0',{'1','2','4'}},\
{'1',{'0','3','5'}},\
{'2',{'3','0','6'}},\
{'3',{'2','1','7'}},\
{'4',{'5','6','0'}},\
{'5',{'4','7','1'}},\
{'6',{'7','4','2'}},\
{'7',{'6','5','3'}}}

#define NEIGHBORHOODS2OF3 {\
{'0',{'3','5','6'}},\
{'1',{'2','4','7'}},\
{'2',{'1','7','4'}},\
{'3',{'0','6','5'}},\
{'4',{'7','1','2'}},\
{'5',{'6','0','3'}},\
{'6',{'5','3','0'}},\
{'7',{'4','2','1'}}}

#define NEIGHBORHOODS3OF3 {\
{'0',{'7'}},\
{'1',{'6'}},\
{'2',{'5'}},\
{'3',{'4'}},\
{'4',{'3'}},\
{'5',{'2'}},\
{'6',{'1'}},\
{'7',{'0'}}}

#define NEIGHBORHOODS1OF4 {\
{'0',{'1','2','4','8'}},\
{'1',{'0','3','5','9'}},\
{'2',{'3','0','6','a'}},\
{'3',{'2','1','7','b'}},\
{'4',{'5','6','0','c'}},\
{'5',{'4','7','1','d'}},\
{'6',{'7','4','2','e'}},\
{'7',{'6','5','3','f'}},\
{'8',{'9','a','c','0'}},\
{'9',{'8','b','d','1'}},\
{'a',{'b','8','e','2'}},\
{'b',{'a','9','f','3'}},\
{'c',{'d','e','8','4'}},\
{'d',{'c','f','9','5'}},\
{'e',{'f','c','a','6'}},\
{'f',{'e','d','b','7'}}}

#define NEIGHBORHOODS2OF4 {\
{'0',{'3','5','6','9','a','c'}},\
{'1',{'2','4','7','8','b','d'}},\
{'2',{'1','7','4','b','8','e'}},\
{'3',{'0','6','5','a','9','f'}},\
{'4',{'7','1','2','d','e','8'}},\
{'5',{'6','0','3','c','f','9'}},\
{'6',{'5','3','0','f','c','a'}},\
{'7',{'4','2','1','e','d','b'}},\
{'8',{'b','d','e','1','2','4'}},\
{'9',{'a','c','f','0','3','5'}},\
{'a',{'9','f','c','3','0','6'}},\
{'b',{'8','e','d','2','1','7'}},\
{'c',{'f','9','a','5','6','0'}},\
{'d',{'e','8','b','4','7','1'}},\
{'e',{'d','b','8','7','4','2'}},\
{'f',{'c','a','9','6','5','3'}}}

#define NEIGHBORHOODS3OF4 {\
{'0',{'e','d','b','7'}},\
{'1',{'f','c','a','6'}},\
{'2',{'c','f','9','5'}},\
{'3',{'d','e','8','4'}},\
{'4',{'a','9','f','3'}},\
{'5',{'b','8','e','2'}},\
{'6',{'8','b','d','1'}},\
{'7',{'9','a','c','0'}},\
{'8',{'6','5','3','f'}},\
{'9',{'7','4','2','e'}},\
{'a',{'4','7','1','d'}},\
{'b',{'5','6','0','c'}},\
{'c',{'2','1','7','b'}},\
{'d',{'3','0','6','a'}},\
{'e',{'0','3','5','9'}},\
{'f',{'1','2','4','8'}}}

#define NEIGHBORHOODS4OF4 {\
{'0',{'f'}},\
{'1',{'e'}},\
{'2',{'d'}},\
{'3',{'c'}},\
{'4',{'b'}},\
{'5',{'a'}},\
{'6',{'9'}},\
{'7',{'8'}},\
{'8',{'7'}},\
{'9',{'6'}},\
{'a',{'5'}},\
{'b',{'4'}},\
{'c',{'3'}},\
{'d',{'2'}},\
{'e',{'1'}},\
{'f',{'0'}}}

#define ALLNEIGBORS {\
{NEIGHBORHOODS1OF1},\
{NEIGHBORHOODS1OF2, NEIGHBORHOODS2OF2},\
{NEIGHBORHOODS1OF3, NEIGHBORHOODS2OF3, NEIGHBORHOODS3OF3},\
{NEIGHBORHOODS1OF4, NEIGHBORHOODS2OF4, NEIGHBORHOODS3OF4, NEIGHBORHOODS4OF4}}
