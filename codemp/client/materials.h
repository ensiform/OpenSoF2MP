#pragma once

#include "qcommon/q_shared.h"

void Mat_Init(void);
void Mat_Reset(void);
sfxHandle_t Mat_GetSound(char *key, int material);
qhandle_t Mat_GetDecal(char *key, int material);
const float Mat_GetDecalScale(char *key, int material);
qhandle_t Mat_GetEffect(char *key, int material);
qhandle_t Mat_GetDebris(char *key, int material);
const float Mat_GetDebrisScale(char *key, int material);
