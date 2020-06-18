#pragma once

#define TEXTURE_BRAIN_COMPONENT_IN_TEXTURE_SHAPES { "28,28,1", "7,7,1", "4,4,1" }
#define TEXTURE_BRAIN_COMPONENT_IN_BIT_DEPTHS { 1, 2, 2 }
#define TEXTURE_BRAIN_COMPONENT_FILTER_SHAPES { "4,4,1", "4,4,1", "4,4,1" }
#define TEXTURE_BRAIN_COMPONENT_STRIDE_SHAPES { "4,4,1", "1,1,1", "4,4,1" }
#define TEXTURE_BRAIN_COMPONENT_OUT_BIT_DEPTHS { 2, 2, 4 }
#define TEXTURE_BRAIN_COMPONENT_FILE_NAMES { "", "", "" } // empty file name tells the program that the corresponding component should be evolved; if undefined, all components are evolved
#define TEXTURE_BRAIN_COMPONENT_MUTATION_RATES { 0.333, 0.333, 0.333 } // if undefined, the rate will be distributed uniformly across the evolvable components
