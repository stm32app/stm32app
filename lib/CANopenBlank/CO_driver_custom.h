#ifndef CO_CUSTOM_CONFIG
#define CO_CUSTOM_CONFIG

#ifdef CANOPEN_CONFIG
# define INC_STRINGIFY_(f) #f
# define INC_STRINGIFY(f) INC_STRINGIFY_(f)
# include INC_STRINGIFY(CANOPEN_CONFIG)
#endif


#endif