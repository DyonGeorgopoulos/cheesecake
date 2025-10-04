#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include "components/rendering.h"
#include "renderer.h"
#include <flecs.h>

void AnimationSystem(ecs_iter_t* it);
void AnimationControllerSystem(ecs_iter_t *it);
#endif