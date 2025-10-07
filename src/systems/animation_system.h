#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include "components/animation.h"
#include "components/sprite.h"
#include "components/transform.h"
#include "systems/render_system.h"
#include "common.h"

#include <flecs.h>

void UpdateDirectionSystem(ecs_iter_t *it);
void AnimationGraphSystem(ecs_iter_t *it);
void update_animations(AppState *state, float dt);

#endif