
#ifndef INC_ACTOR_CM3_PLATFORM
#define INC_ACTOR_CM3_PLATFORM
void actor_platform_init(void);
/*
 * To implement the STDIO functions you need to create
 * the _read and _write functions and hook them to the
 * USART you are using. This example also has a buffered
 * read function for basic line editing.
 */
int _write(int fd, char *ptr, int len);
int _read(int fd, char *ptr, int len);
void get_buffered_line(void);
#endif