#pragma once

// Le MNIST

#define TEXTURE_BRAIN_COMPONENT_IN_TEXTURE_SHAPES { "28,28,1", "14,14,1", "10,10,1", "5,5,1", "1,1,1", "1,1,1" }
#define TEXTURE_BRAIN_COMPONENT_IN_BIT_DEPTHS { 1, 6, 16, 16, 120, 84 }
#define TEXTURE_BRAIN_COMPONENT_FILTER_SHAPES { "2,2,1", "5,5,1", "2,2,1", "5,5,1", "1,1,1", "1,1,1" }
#define TEXTURE_BRAIN_COMPONENT_STRIDE_SHAPES { "2,2,1", "1,1,1", "2,2,1", "5,5,1", "1,1,1", "1,1,1" }
#define TEXTURE_BRAIN_COMPONENT_OUT_BIT_DEPTHS { 6, 16, 16, 120, 84, 10 }
#define TEXTURE_BRAIN_COMPONENT_FILE_NAMES { "", "", "", "", "", "" } // empty file name tells the program that the corresponding component should be evolved; if undefined, all components are evolved
#define TEXTURE_BRAIN_COMPONENT_MUTATION_RATES { 0.11, 0.132, 0.154, 0.176, 0.198, 0.23 } // if undefined, the rate will be distributed uniformly across the evolvable components
