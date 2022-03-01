#ifndef INCLUDE_PORT_FILE
#define QUOTEME(x) QUOTEME_1(x)
#define QUOTEME_1(x) #x
#define INCLUDE_PORT_FILE(port, file) QUOTEME(../ports/port/file)
#endif

#ifndef FREERTOS_PORT
#error "Define FREERTOS_PORT=[cm3,posix] env variable"
#endif

#include INCLUDE_PORT_FILE(FREERTOS_PORT, portmacro.h)