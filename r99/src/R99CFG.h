/*
 * R99 build-size configuration.
 *
 * This package enables the 54K-oriented profile by default.  Comment out
 * R99_TARGET_54K to restore the original desktop capacities for regression
 * comparisons.
 */

#define R99_TARGET_54K

#ifdef R99_TARGET_54K

/* ED2 uses 834 symbols.  1123 is prime and gives 289 spare slots. */
#define LINLEN          192
#define BUFSIZE         256
#define SYMBOLS        1123
#define INCLUDE_DEPTH     4
#define MAXXREF          512

#else

#define LINLEN          512
#define BUFSIZE         513
#define SYMBOLS        2081
#define INCLUDE_DEPTH     8
#define MAXXREF         4096

#endif
