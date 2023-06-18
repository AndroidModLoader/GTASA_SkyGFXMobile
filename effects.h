#pragma once

#include <mod/amlmod.h>

#define GRAIN_EFFECT_SCALE            0.75f
#define GRAIN_NIGHTVISION_STRENGTH    48
#define GRAIN_INFRAREDVISION_STRENGTH 64


// Functions
void SetUpWaterFog_stub(void);
void MobileEffectsRender_stub(void);
void EffectsRender_stub(void);

// Grain
extern unsigned char grainIntensity;