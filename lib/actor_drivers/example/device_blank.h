#ifndef INC_DEV_BLANK
#define INC_DEV_BLANK

#ifdef __cplusplus
extern "C" {
#endif

#include <actor/actor.h>



struct actor_blank {
    actor_t *actor;
    actor_blank_properties_t *properties;
};


extern actor_class_t actor_blank_class;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif