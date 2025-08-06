#include "include/neoWaterdrops.h"
#include "include/renderqueue.h"

extern RwRaster *pSkyGFXPostFXRaster1, *pSkyGFXPostFXRaster2, *pDarkRaster;
float scaling;
RwOpenGLVertex DropletsBuffer[2 * 4 * WaterDrops::MAXDROPS] {};

#define MAXSIZE 15
#define MINSIZE 4
#define SC(x) ((int)((x)*scaling))
#define RAD2DEG(x) (180.0f*(x)/M_PI)
#define RwCameraGetFrame(_camera) ((RwFrame *)((_camera)->object.object.parent))
#define RwV3dAdd(o, a, b)                                       \
{                                                               \
    (o)->x = (((a)->x) + ( (b)->x));                            \
    (o)->y = (((a)->y) + ( (b)->y));                            \
    (o)->z = (((a)->z) + ( (b)->z));                            \
}

#define RwV3dSub(o, a, b)                                       \
{                                                               \
    (o)->x = (((a)->x) - ( (b)->x));                            \
    (o)->y = (((a)->y) - ( (b)->y));                            \
    (o)->z = (((a)->z) - ( (b)->z));                            \
}

#define RwV3dScale(o, a, s)                                     \
{                                                               \
    (o)->x = (((a)->x) * ( (s)));                               \
    (o)->y = (((a)->y) * ( (s)));                               \
    (o)->z = (((a)->z) * ( (s)));                               \
}

// Other Funcs

inline uint16_t GetCamMode()
{
#ifdef AML32
    return (uint16_t)( TheCamera->m_apCams[TheCamera->m_nCurrentActiveCam].Mode );
#else
    return (uint16_t)( TheCamera->Cams[TheCamera->ActiveCam].Mode );
#endif
}
inline float RwV3dLength(RwV3d* v)
{
    return ((CVector*)v)->Magnitude();
}
inline float RwV3dDotProduct(RwV3d* a, RwV3d* b)
{
    return DotProduct((CVector&)*a, (CVector&)*b);
}

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

    if(!ms_initialised) InitialiseRender(Scene->camera);
    WaterDrops::CalculateMovement();
    WaterDrops::SprayDrops();
    WaterDrops::ProcessMoving();
    WaterDrops::Fade();
}

void WaterDrops::CalculateMovement(void)
{
    RwMatrix *modelMatrix = &RwCameraGetFrame(Scene->camera)->modelling;
    RwV3dSub(&ms_posDelta, &modelMatrix->pos, &ms_lastPos);
    ms_distMoved = RwV3dLength(&ms_posDelta);
    
    ms_lastAt = modelMatrix->at;
    ms_lastPos = modelMatrix->pos;

    ms_vec.x = -RwV3dDotProduct(&modelMatrix->right, &ms_posDelta);
    ms_vec.y = RwV3dDotProduct(&modelMatrix->up, &ms_posDelta);
    ms_vec.z = RwV3dDotProduct(&modelMatrix->at, &ms_posDelta);
    RwV3dScale(&ms_vec, &ms_vec, 10.0f);
    ms_vecLen = sqrtf(ms_vec.y*ms_vec.y + ms_vec.x*ms_vec.x);

    short mode = GetCamMode();
    bool istopdown = (mode == MODE_TOPDOWN || mode == MODE_TWOPLAYER_SEPARATE_CARS_TOPDOWN);
    bool carlookdirection = 0;
    if(mode == MODE_1STPERSON && FindPlayerVehicle(-1, 0))
    {
        CPad *p = GetPad(0);
        if(GetLookBehindForCar(p) || GetLookLeft(p, true) || GetLookRight(p, true))
        {
            carlookdirection = 1;
        }
    }
    ms_enabled = !istopdown && !carlookdirection;
    ms_movingEnabled = !istopdown && !carlookdirection;

    float c = modelMatrix->at.z;
    if(c > 1.0f) c = 1.0f;
    if(c < -1.0f) c = -1.0f;
    ms_rainStrength = RAD2DEG(acos(c));
}

static int ndrops[] = {
    125, 250, 500, 1000, 1000,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
void WaterDrops::SprayDrops(void)
{
    if(!NoRain() && *pfWeatherRain != 0.0f && ms_enabled)
    {
        int tmp = 180.0f - ms_rainStrength;
        if(tmp < 40) tmp = 40;
        FillScreenMoving((tmp - 40.0f) / 150.0f * *pfWeatherRain * 0.5f);
    }
    if(sprayWater) FillScreenMoving(0.5f, false);
    if(sprayBlood) FillScreenMoving(0.5f, true);
    if(ms_splashDuration >= 0)
    {
        if(ms_numDrops < MAXDROPS)
        {
            FillScreenMoving(1.0f); // VC does STRANGE things here
        }
        ms_splashDuration--;
    }
}

void WaterDrops::MoveDrop(WaterDropMoving *moving)
{
    WaterDrop *drop = moving->drop;
    if(!ms_movingEnabled) return;
    if(!drop->active)
    {
        moving->drop = NULL;
        ms_numDropsMoving--;
        return;
    }
    if(ms_vec.z <= 0.0f || ms_distMoved <= 0.3f) return;

    uint16_t mode = GetCamMode();
    if(ms_vecLen <= 0.5f || mode == MODE_1STPERSON)
    {
        // movement out of center 
        float d = ms_vec.z * 0.2f;
        float dx, dy, sum;
        dx = drop->x - ms_fbWidth * 0.5f + ms_vec.x;
        if(mode == MODE_1STPERSON)
        {
            dy = drop->y - ms_fbHeight * 1.2f - ms_vec.y;
        }
        else
        {
            dy = drop->y - ms_fbHeight * 0.5f - ms_vec.y;
        }
        sum = fabsf(dx) + fabsf(dy);
        if(sum >= 0.001f)
        {
            dx *= (1.0 / sum);
            dy *= (1.0 / sum);
        }
        moving->dist += d;
        if(moving->dist > 20.0f) NewTrace(moving);
        drop->x += dx * d;
        drop->y += dy * d;
    }
    else
    {
        // movement when camera turns
        moving->dist += ms_vecLen;
        if(moving->dist > 20.0f) NewTrace(moving);
        drop->x -= ms_vec.x;
        drop->y += ms_vec.y;
    }

    if(drop->x < 0.0f || drop->y < 0.0f || drop->x > ms_fbWidth || drop->y > ms_fbHeight)
    {
        moving->drop = NULL;
        ms_numDropsMoving--;
    }
}

void WaterDrops::ProcessMoving(void)
{
    WaterDropMoving *moving;
    if(!ms_movingEnabled) return;
    for(moving = ms_dropsMoving; moving < &ms_dropsMoving[MAXDROPSMOVING]; ++moving)
    {
        if(moving->drop) MoveDrop(moving);
    }
}

void WaterDrops::Fade(void)
{
    WaterDrop *drop;
    for(drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; ++drop)
    {
        if(drop->active) drop->Fade();
    }
}

WaterDrop* WaterDrops::PlaceNew(float x, float y, float size, float ttl, bool fades, int R = 0xFF, int G = 0xFF, int B = 0xFF)
{
    WaterDrop *drop;
    int i;

    if(NoDrops()) return NULL;

    for(i = 0, drop = ms_drops; i < MAXDROPS; ++i, ++drop)
    {
        if(ms_drops[i].active == 0) goto found;
    }
    return NULL;

found:
    ms_numDrops++;
    drop->x = x;
    drop->y = y;
    drop->size = size;
    drop->uvsize = (SC(MAXSIZE) - size + 1.0f) / (SC(MAXSIZE) - SC(MINSIZE) + 1.0f);
    drop->fades = fades;
    drop->active = 1;
    drop->color.red = R;
    drop->color.green = G;
    drop->color.blue = B;
    drop->color.alpha = 0xFF;
    drop->time = 0.0f;
    drop->ttl = ttl;
    return drop;
}

void WaterDrops::NewTrace(WaterDropMoving *moving)
{
    if(ms_numDrops < MAXDROPS)
    {
        moving->dist = 0.0f;
        PlaceNew(moving->drop->x, moving->drop->y, SC(MINSIZE), 500.0f, 1, moving->drop->color.red, moving->drop->color.green, moving->drop->color.blue);
    }
}

void WaterDrops::NewDropMoving(WaterDrop *drop)
{
    WaterDropMoving *moving;
    for(moving = ms_dropsMoving; moving < &ms_dropsMoving[MAXDROPSMOVING]; moving++)
    {
        if(moving->drop == NULL) goto found;
    }
    return;

found:
    ms_numDropsMoving++;
    moving->drop = drop;
    moving->dist = 0.0f;
}

void WaterDrops::FillScreenMoving(float amount, bool isBlood)
{
    if(isBlood && !neoBloodDrops) return;

    int n = (ms_vec.z <= 5.0f ? 1.0f : 1.5f) * amount * 15.0f;
    float x, y, time;
    WaterDrop *drop;

    while(n--)
    {
        if(ms_numDrops < MAXDROPS && ms_numDropsMoving < MAXDROPSMOVING)
        {
            x = rand() % ms_fbWidth;
            y = rand() % ms_fbHeight;
            time = rand() % (SC(MAXSIZE) - SC(MINSIZE)) + SC(MINSIZE);
            if(!isBlood)
            {
                drop = PlaceNew(x, y, time, 2000.0f, 1);
            }
            else
            {
                drop = PlaceNew(x, y, time, 2000.0f, 1, 0xFF, 0x00, 0x00);
            }
            if(drop) NewDropMoving(drop);
        }
    }
}

void WaterDrops::FillScreen(int n)
{
    float x, y, time;
    WaterDrop *drop;

    if(!ms_initialised) return;
    ms_numDrops = 0;
    for(drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
    {
        drop->active = 0;
        if(drop < &ms_drops[n])
        {
            x = rand() % ms_fbWidth;
            y = rand() % ms_fbHeight;
            time = rand() % (SC(MAXSIZE) - SC(MINSIZE)) + SC(MINSIZE);
            PlaceNew(x, y, time, 2000.0f, 1);
        }
    }
}

void WaterDrops::Clear(void)
{
    WaterDrop *drop;
    for(drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
    {
        drop->active = 0;
    }
    ms_numDrops = 0;
}

void WaterDrops::Reset(void)
{
    Clear();
    ms_splashDuration = -1;
}

void WaterDrops::RegisterSplash(RwV3d* point, float distance, int duration)
{
    RwV3d dist;
    RwV3dSub(&dist, point, &ms_lastPos);

    if(RwV3dLength(&dist) <= distance)
    {
        ms_splashDuration = duration;
    }
}

bool WaterDrops::NoDrops(void)
{
    return ( *pfWeatherUnderwaterness > 0.339731634f || *ms_exitEnterState != 0 );
}

bool WaterDrops::NoRain(void)
{
    return CamNoRain() || PlayerNoRain() || *currArea != 0 || NoDrops();
}

// Initialise part

void WaterDrops::InitialiseRender(RwCamera *cam)
{
    // TODO: Check about indices, immediate mode is heavy af
    m_pMask = (RwTexture*)sautils->LoadRwTextureFromPNG("texdb/dropmask.png");
    if(!m_pMask)
    {
        uintptr_t txdDB = GetTextureDB("txd");
        if(txdDB)
        {
            RegisterTextureDB(txdDB);
            m_pMask = GetTextureFromTextureDB("sphere_CJ");
            UnregisterTextureDB(txdDB);
        }
    }
    ms_initialised = 1;
}

static const float xy[] =
{
    -1.0f,  1.0f, 1.0f,  1.0f,
    -1.0f, -1.0f, 1.0f, -1.0f
};
static const float uv[] =
{
    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 0.0f
};
static const RwRGBA dropMaskColor(0, 0, 0, 255);
void WaterDrops::AddToRenderList(WaterDrop *drop)
{
    int i;
    float scale;

    float u1_1, u1_2;
    float v1_1, v1_2;
    float tmp;

    tmp = drop->uvsize * (300.0f - 40.0f) + 40.0f;
    u1_1 = drop->x + ms_xOff - tmp;
    v1_1 = drop->y + ms_yOff - tmp;
    u1_2 = drop->x + ms_xOff + tmp;
    v1_2 = drop->y + ms_yOff + tmp;
    u1_1 = (u1_1 <= 0.0f ? 0.0f : u1_1) / pSkyGFXPostFXRaster1->width;
    v1_1 = (v1_1 <= 0.0f ? 0.0f : v1_1) / pSkyGFXPostFXRaster1->height;
    u1_2 = (u1_2 >= ms_fbWidth ? ms_fbWidth : u1_2) / pSkyGFXPostFXRaster1->width;
    v1_2 = (v1_2 >= ms_fbHeight ? ms_fbHeight : v1_2) / pSkyGFXPostFXRaster1->height;

    scale = drop->size * 0.5f;

    // Mask
    for(i = 0; i < 4; i++)
    {
        ms_vertPtr->pos.x = drop->x + xy[i * 2] * scale + ms_xOff;
        ms_vertPtr->pos.y = drop->y + xy[i * 2 + 1] * scale + ms_yOff;
        ms_vertPtr->pos.z = 0.0f;
        ms_vertPtr->rhw = 1.0f;
        ms_vertPtr->rgba = dropMaskColor;
        ms_vertPtr->rgba.alpha = drop->color.alpha;
        ms_vertPtr->texCoord.u = uv[i * 2];
        ms_vertPtr->texCoord.v = uv[i * 2 + 1];
        ms_vertPtr++;
    }
    // Drop
    for(i = 0; i < 4; i++)
    {
        ms_vertPtr->pos.x = drop->x + xy[i * 2] * scale + ms_xOff;
        ms_vertPtr->pos.y = drop->y + xy[i * 2 + 1] * scale + ms_yOff;
        ms_vertPtr->pos.z = 0.0f;
        ms_vertPtr->rhw = 1.0f;
        ms_vertPtr->rgba = drop->color;
        ms_vertPtr->texCoord.u = (i % 2 == 0 ? u1_1 : u1_2);
        ms_vertPtr->texCoord.v = 1.0f - (i < 2 ? v1_1 : v1_2);
        ms_vertPtr++;
    }

    ms_numBatchedDrops++;
}

void WaterDrops::Render(void)
{
    bool nofirstperson = ( FindPlayerVehicle(-1, 0) == 0 && GetCamMode() == MODE_1STPERSON );
    if(!ms_enabled || ms_numDrops <= 0 || nofirstperson || *ms_running) return;

    // When you put camera underwater, droplets disappear instantly instead of fading out
    if(NoDrops())
    {
        if(ms_numDrops > 0) Clear();
        return;
    }

    ms_vertPtr = &DropletsBuffer[0];
    ms_numBatchedDrops = 0;
    for(WaterDrop *drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
    {
        if(drop->active) AddToRenderList(drop);
    }

    if(ms_numBatchedDrops > 0)
    {
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)false);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)false);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)true);

        int buf = 0;
        RwRaster* moonmaskRaster = m_pMask ? m_pMask->raster : (*gpMoonMask)->raster;
        for(int i = 0; i < ms_numBatchedDrops; ++i)
        {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)moonmaskRaster);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            ERQ_BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ZERO);
            RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, &DropletsBuffer[buf], 4);
            ERQ_BlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO); // initial values

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)pSkyGFXPostFXRaster1);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDDESTALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVDESTALPHA);
            RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, &DropletsBuffer[buf + 4], 4);

            buf += 8;
        }
        
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)false);
    }
}