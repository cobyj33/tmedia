#ifndef TMEDIA_OPTIM_H
#define TMEDIA_OPTIM_H

/* Branch prediction */
#if (defined(__GNUC__) && __GNUC__ >= 3) || defined(__clang__)
# define likely(p)     __builtin_expect(!!(p), 1)
# define unlikely(p)   __builtin_expect(!!(p), 0)
#elif defined(_MSC_VER)
# define likely(p)     (!!(p))
# define unlikely(p)   (!!(p))
#else
# define likely(p)     (!!(p))
# define unlikely(p)   (!!(p))
#endif

#endif