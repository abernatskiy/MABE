#pragma once

#define BRAIN_COMPONENT_JUNCTION_SIZES { 64, 32, 16, 12, 10, 8, 6, 4 }
//#define BRAIN_COMPONENT_JUNCTION_SIZES { 64, 32, 16, 14, 12, 10, 8, 6, 5, 4} // the last one is the output
//#define BRAIN_COMPONENT_FILE_NAMES { "", "", "", "", "", "", "", "", "", "" } // empty file name tells the program that the corresponding component should be evolved; if undefined, all components are evolved
//#define BRAIN_COMPONENT_MUTATION_RATES { 0.2, 0.05, 0.05, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1 } // if undefined, the rate will be distributed uniformly across the evolvable components
//#define BRAIN_COMPONENT_HIDDEN_NODES { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } // all zeros will be assumed if undefined
