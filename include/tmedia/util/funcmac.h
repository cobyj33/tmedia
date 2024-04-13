#ifndef FUNCMAC_H
#define FUNCMAC_H

/**
 * Note that these function macros are should be treated as only defined within
 * a function, and expand out to string literals, not macros. Additionally,
 * they should not be used with direct string concatenation (two string literals
 * next to each other).
 * 
 * "These identifiers are variables, not preprocessor macros, and may not be
 * used to initialize char arrays or be concatenated with string literals."
 * - 6.51 Function Names as Strings
 *   https://gcc.gnu.org/onlinedocs/gcc/Function-Names.html
*/

/**
 * READ THE ABOVE COMMENT BEFORE PROCEEDING!!!
 * 
 * FUNCVINFO:
 * If available and used within a function, FUNCDINFO expands out to a string
 * literal containing the function name and it's call signature.
 * If not available, FUNCDINFO will just expand out to an empty string literal
 *  V for Verbose.
 * 
 * FUNCDINFO:
 * If available and used within a function, FUNCDINFO expands out to a string
 * literal containing the function name.
 * If not available, FUNCDINFO will just expand out to an empty string literal
 *  D for Debug. (the availability of FUNCDINFO does not depend on the current
 *  build being a debug build or not)
 * 
 * FUNCINFO:
 * If available and used within a function, FUNCINFO expands out to the current
 * function's name as a string literal. If not available, FUNCDINFO
 * will just expand out to an empty string literal.
*/

#if !defined(FUNCDINFO)
  #if defined(__GNUC__) && __GNUC__ >= 3
    #define FUNCDINFO __FUNCTION__
  #elif defined(_MSC_VER) // Visual Studio
    #define FUNCDINFO __FUNCNAME__
  #elif defined(__cplusplus) && __cplusplus >= 201103L
    #define FUNCDINFO __func__
  #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 19901L
    #define FUNCDINFO __func__
  #else
    #define FUNCDINFO ""
  #endif
#endif

#if !defined(FUNCVINFO)
  #if defined(__GNUC__) && __GNUC__ >= 3
    #define FUNCVINFO __PRETTY_FUNCTION__
  #elif defined(_MSC_VER) // Visual Studio
    #define FUNCVINFO __FUNCDNAME__
  #elif defined(__cplusplus) && __cplusplus >= 201103L
    #define FUNCVINFO __func__
  #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 19901L
    #define FUNCVINFO __func__
  #else
    #define FUNCVINFO ""
  #endif
#endif

#if !defined(FUNCINFO)
  #if (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
    #define FUNCINFO __func__
  #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 19901L
    #define FUNCINFO __func__
  #else
    #define FUNCINFO ""
  #endif
#endif


#endif