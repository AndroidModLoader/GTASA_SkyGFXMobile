#include "include/neoWaterdrops.h"
#include "include/renderqueue.h"

extern RwRaster *pSkyGFXPostFXRaster1, *pSkyGFXPostFXRaster2, *pDarkRaster;
float scaling;
#define SC(x) ((int)((x)*scaling))

// Single Drop

void WaterDrop::Fade(void)
{
    int delta = *ms_fTimeStep * 1000.0f / 50.0f;
    this->time += delta;
    if(this->time >= this->ttl)
    {
        --WaterDrops::ms_numDrops;
        this->active = 0;
    }
    else if(this->fades)
    {
        this->color.alpha = 255 - time / ttl * 255;
    }
}

// Multiple Drops

void WaterDrops::Process()
{
    // In case resolution changes
    ms_fbWidth = Scene->camera->framebuf->width;
    ms_fbHeight = Scene->camera->framebuf->height;
    scaling = ms_fbHeight / 480.0f;

    /*if(!ms_initialised) InitialiseRender(Scene->camera);
    WaterDrops::CalculateMovement();
    WaterDrops::SprayDrops();
    WaterDrops::ProcessMoving();
    WaterDrops::Fade();*/
}