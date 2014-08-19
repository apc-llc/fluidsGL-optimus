#ifndef DEFINES_H
#define DEFINES_H

#define SCALING  4                 // Extra scaling for performing larger simulations
#define DIM     (512*SCALING)      // Square size of solver domain
#define DS      (DIM*DIM)          // Total domain size
#define CPADW   (DIM/2+1)          // Padded width for real->complex in-place FFT
#define RPADW   (2*(DIM/2+1))      // Padded width for real->complex in-place FFT
#define PDS     (DIM*CPADW)        // Padded total domain size

#define DT       0.09f             // Delta T for interative solver
#define VIS     (0.0025f*SCALING)  // Viscosity constant
#define FORCE   (5.8f*DIM*SCALING) // Force scale factor 
#define FR      (4*SCALING)        // Force update radius

#define TILEX    64                // Tile width
#define TILEY    64                // Tile height
#define TIDSX    64                // Tids in X
#define TIDSY    4                 // Tids in Y

#endif
