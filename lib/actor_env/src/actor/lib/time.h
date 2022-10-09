#ifndef INC_LIB_TIME
#define INC_LIB_TIME

#include <actor/types.h>

// Find difference between two timestamps, where first is always preceeding the the second
static inline uint32_t time_since(uint32_t last, uint32_t now) {
  return (last > now ? ((uint32_t)-1) - last + now : now - last);
}

// Find difference between two timestamps any of which can be larger than the other
// Limitation: Difference can't be more than 31 bit 
static inline int32_t time_difference(uint32_t first, uint32_t second) {
  return ((int32_t)first - (int32_t)second);
}

// Find if second time is greater or equal than the first, taking overflows into account
// handles difference up to 31bit
#define time_gte(now, then) (time_difference(now, then) >= 0)
// Find if second time is greater than the first, taking overflows into account
// handles difference up to 31bit
#define time_gt(now, then) (time_difference(now, then) > 0)

static inline uint32_t time_not_zero(uint32_t time) {
  if (time == 0) {
    return 1;
  } else {
    return time;
  }
}

#endif