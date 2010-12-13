#ifndef DEBUG_H_
# define DEBUG_H_                       1

# ifdef NDEBUG
#  define DBG(x, s) /* */
# else
extern int debug_level;
#  define DBG(x, s) if(x <= debug_level) { s; }
# endif

#endif /*!DEBUG_H_ */

