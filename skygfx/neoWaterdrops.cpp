#include "include/neoWaterdrops.h"
#include "include/renderqueue.h"

extern RwRaster *pSkyGFXPostFXRaster1, *pSkyGFXPostFXRaster2, *pDarkRaster;
float scaling, frameTimeDelta, frameTimeDeltaInv, deltaMs;
float fRX, fRY, fRXInv, fRYInv;
int nRX, nRY;
int minScaled = 0, maxScaled = 0, maxminDiff = 0, oneScaled = 0;
RwOpenGLVertex DropletsBuffer[2 * 4 * WaterDrops::MAXDROPS] {};

static const RwRGBA dropColor(255, 255, 255, 255);
static const RwRGBA dropMaskColor(0, 0, 0, 255);
static const RwRGBA dropBloodColor(255, 40, 40, 255);
static const RwRGBA dropDirtColor(182, 159, 102, 255);

#define MAXSIZE 18
#define MINSIZE 5
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

// Hooks

DECL_HOOKv(CAEFireAudioEntity_AddAudioEvent, void* self, eAudioEvents event, RwV3d* position)
{
    CAEFireAudioEntity_AddAudioEvent(self, event, position);
    if(WaterDrops::neoWaterDrops && event == AE_FIRE_HYDRANT)
    {
        WaterDrops::RegisterSplash(position, 13.0f, 20);
    }
}
DECL_HOOKv(TriggerWaterSplash, void* self, RwV3d* position)
{
    TriggerWaterSplash(self, position);
    if(WaterDrops::neoWaterDrops)
    {
        WaterDrops::RegisterSplash(position, 10.0f, 1);
    }
}
DECL_HOOKv(TriggerBulletSplash, void* self, RwV3d* position)
{
    TriggerBulletSplash(self, position);
    if(WaterDrops::neoWaterDrops)
    {
        WaterDrops::RegisterSplash(position, 10.0f, 1);
    }
}
DECL_HOOKv(TriggerFootSplash, void* self, RwV3d* position)
{
    TriggerFootSplash(self, position);
    if(WaterDrops::neoWaterDrops)
    {
        WaterDrops::RegisterSplash(position, 10.0f, 1);
    }
}
DECL_HOOKv(ProcessHarvester_AddBodyPart, CObject* entity)
{
    ProcessHarvester_AddBodyPart(entity);
    if(WaterDrops::neoWaterDrops)
    {
        RwV3d dist;
        RwV3dSub(&dist, (RwV3d*)&entity->GetPosition(), &WaterDrops::ms_lastPos);
        if(RwV3dLength(&dist) <= 20.0f)
        {
            WaterDrops::FillScreenMoving(0.5f, true);
        }
    }
}
DECL_HOOKv(Chainsaw_AddAudioEvent, uintptr_t aeentity, eAudioEvents event)
{
    Chainsaw_AddAudioEvent(aeentity, event);
    if(WaterDrops::neoWaterDrops)
    {
        /* FIX_BUGS */
        if((aeentity - BYBIT(0x398, 0x470)) == (uintptr_t)FindPlayerPed(-1))
        {
            WaterDrops::FillScreenMoving(frameTimeDelta * 1.0f, true);
        }
    }
}
DECL_HOOKv(AddFxParticle, void* self, RwV3d *pos, RwV3d *vel, float timeSince, void *fxMults, float zRot, float lightMult, float lightMultLimit, uint8_t createLocal)
{
    AddFxParticle(self, pos, vel, timeSince, fxMults, zRot, lightMult, lightMultLimit, createLocal);

    if(!WaterDrops::neoWaterDrops) return;

    RwV3d dist;
    RwV3dSub(&dist, pos, &WaterDrops::ms_lastPos);
    float pd = 20.0f;
    bool isBlood = false;
    bool isDirt = false;
    /*if(self == g_fx->prt_blood) { pd = 5.0; isBlood = true; }
    else*/ if(self == g_fx->prt_boatsplash) { pd = 40.0; }
    //else if(self == g_fx->prt_splash) { pd = 15.0; }
    else if(self == g_fx->prt_wake) { pd = 10.0; }
    else if(self == g_fx->prt_watersplash) { pd = 30.0; }
    else if(self == g_fx->prt_wheeldirt && *pfWeatherRain != 0.0f) { pd = 15.0; isDirt = true; }
    else return;

    float len = RwV3dLength(&dist);
    if(len <= pd)
    {
        WaterDrops::FillScreenMoving(frameTimeDelta * 1.0f / (len / 2.0f), isBlood, isDirt);
    }
}
DECL_HOOKv(AddBloodFx, void* self, RwV3d *pos, RwV3d *dir, int32 num, float lightMult)
{
    AddBloodFx(self, pos, dir, num, lightMult);
    if(WaterDrops::neoWaterDrops)
    {
        RwV3d dist;
        RwV3dSub(&dist, pos, &WaterDrops::ms_lastPos);
        float len = RwV3dLength(&dist);
        if(len <= 7.0f) // org. 5.0f
        {
            for(int i = 0; i < num; ++i)
            {
                WaterDrops::FillScreenMoving(1.0f / (len / 2.0f), true);
            }
        }
    }
}

// Main Init

void WaterDrops::InitialiseHooks()
{
    HOOKPLT(CAEFireAudioEntity_AddAudioEvent, pGTASA + BYBIT(0x673B9C, 0x8464F0));
    HOOK(TriggerWaterSplash, aml->GetSym(hGTASA, "_ZN4Fx_c18TriggerWaterSplashER7CVector"));
    HOOK(TriggerBulletSplash, aml->GetSym(hGTASA, "_ZN4Fx_c19TriggerBulletSplashER7CVector"));
    HOOK(TriggerFootSplash, aml->GetSym(hGTASA, "_ZN4Fx_c17TriggerFootSplashER7CVector"));
    HOOKBLX(ProcessHarvester_AddBodyPart, pGTASA + BYBIT(0x55797A + 0x1, 0x6781A4));
    HOOKBLX(Chainsaw_AddAudioEvent, pGTASA + BYBIT(0x4DA62E + 0x1, 0x5DBEBC));
    HOOKPLT(AddFxParticle, pGTASA + BYBIT(0x67358C, 0x845AD8));
    HOOKPLT(AddBloodFx, pGTASA + BYBIT(0x66E82C, 0x83DD68)); // A little fix for bleeding wood
}

// Single Drop

void WaterDrop::Fade(void)
{
    this->time += deltaMs;
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
    nRX = RsGlobal->maximumWidth; fRX = nRX; fRXInv = 1.0f / fRX;
    nRY = RsGlobal->maximumHeight; fRY = nRY; fRYInv = 1.0f / fRY;
    
    // In case resolution changes
    scaling = fRY / 480.0f;
    frameTimeDelta = *ms_fTimeStep / (50.0f / 30.0f);
    frameTimeDeltaInv = 1.0f / frameTimeDelta;
    deltaMs = *ms_fTimeStep * 1000.0f / 50.0f;
    minScaled = SC(MINSIZE);
    maxScaled = SC(MAXSIZE);
    maxminDiff = maxScaled - minScaled;
    oneScaled = SC(1);

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

    uint16_t mode = GetCamMode();
    bool istopdown = (mode == MODE_TOPDOWN || mode == MODE_TWOPLAYER_SEPARATE_CARS_TOPDOWN);
    bool carlookdirection = 0;
    if(mode == MODE_1STPERSON && FindPlayerVehicle(-1, false))
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

void WaterDrops::SprayDrops(void)
{
    float delta = frameTimeDelta;
    if(!NoRain() && *pfWeatherRain != 0.0f && ms_enabled)
    {
        float tmp = 180.0f - ms_rainStrength;
        if(tmp < 40.0f) tmp = 40.0f;
        FillScreenMoving((tmp - 40.0f) * *pfWeatherRain * delta / 280.0f);
    }
    if(sprayWater) FillScreenMoving(0.5f * delta, false);
    if(sprayBlood) FillScreenMoving(0.5f * delta, true);
    
    if(ms_splashDuration >= 0)
    {
        FillScreenMoving(1.0f * delta); // VC does STRANGE things here
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
        dx = drop->x - fRX * 0.5f + ms_vec.x;
        if(mode == MODE_1STPERSON)
        {
            dy = drop->y - fRY * 1.2f - ms_vec.y;
        }
        else
        {
            dy = drop->y - fRY * 0.5f - ms_vec.y;
        }
        sum = fabsf(dx) + fabsf(dy);
        if(sum >= 0.001f)
        {
            dx *= (1.0 / sum);
            dy *= (1.0 / sum);
        }
        moving->dist += d;
        if(moving->dist > 25.0f) NewTrace(moving);
        drop->x += dx * d;
        drop->y += dy * d;
    }
    else
    {
        // movement when camera turns
        moving->dist += ms_vecLen;
        if(moving->dist > 25.0f) NewTrace(moving);
        drop->x -= ms_vec.x;
        drop->y += ms_vec.y;
    }

    if(drop->x < 0.0f || drop->y < 0.0f || drop->x > fRX || drop->y > fRY)
    {
        moving->drop = NULL;
        ms_numDropsMoving--;
    }
}

void WaterDrops::ProcessMoving(void)
{
    if(!ms_movingEnabled) return;
    for(WaterDropMoving* moving = &ms_dropsMoving[0]; moving < &ms_dropsMoving[MAXDROPSMOVING]; ++moving)
    {
        if(moving->drop) MoveDrop(moving);
    }
}

void WaterDrops::Fade(void)
{
    for(WaterDrop* drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; ++drop)
    {
        if(drop->active) drop->Fade();
    }
}

WaterDrop* WaterDrops::PlaceNew(float x, float y, float size, float ttl, bool fades, int R, int G, int B)
{
    if(NoDrops()) return NULL;
    for(WaterDrop* drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; ++drop)
    {
        if(!drop->active)
        {
            drop->x = x;
            drop->y = y;
            drop->size = size;
            drop->uvsize = (maxScaled - size + 1.0f) / (maxminDiff + 1.0f);
            drop->fades = fades;
            drop->active = 1;
            drop->color.red = R;
            drop->color.green = G;
            drop->color.blue = B;
            drop->color.alpha = 0xFF;
            drop->time = 0.0f;
            drop->ttl = ttl;
            ms_numDrops++;
            return drop;
        }
    }
    return NULL;
}

void WaterDrops::NewTrace(WaterDropMoving *moving)
{
    if(ms_numDrops < MAXDROPS)
    {
        moving->dist = 0.0f;
    
        WaterDrop* drop = moving->drop;
        if(drop->size <= minScaled) return;
    
        moving->drop -= oneScaled;
        PlaceNew(drop->x, drop->y, minScaled, 500.0f, 1, drop->color.red, drop->color.green, drop->color.blue);
    }
}

void WaterDrops::NewDropMoving(WaterDrop *drop)
{
    for(WaterDropMoving* moving = &ms_dropsMoving[0]; moving < &ms_dropsMoving[MAXDROPSMOVING]; moving++)
    {
        if(moving->drop == NULL)
        {
            moving->drop = drop;
            moving->dist = 0.0f;
            ms_numDropsMoving++;
            return;
        }
    }
}

void WaterDrops::FillScreenMoving(float amount, bool isBlood, bool isDirt)
{
    if((isBlood && !neoBloodDrops) || (isDirt && !neoDirtyDrops)) return;

    int n = (ms_vec.z <= 5.0f ? 1.0f : 1.5f) * amount * 20.0f;
    float x, y, size;
    WaterDrop *drop;

    while(n--)
    {
        if(ms_numDrops < MAXDROPS && ms_numDropsMoving < MAXDROPSMOVING)
        {
            x = rand() % nRX;
            y = rand() % nRY;
            size = rand() % (maxminDiff) + minScaled;
            
            if(isDirt)
            {
                drop = PlaceNew(x, y, size, 2000.0f, 1, dropDirtColor.red, dropDirtColor.green, dropDirtColor.blue);
            }
            else if(isBlood)
            {
                drop = PlaceNew(x, y, size, 2000.0f, 1, dropBloodColor.red, dropBloodColor.green, dropBloodColor.blue);
            }
            else
            {
                drop = PlaceNew(x, y, size, 2000.0f, 1, dropColor.red, dropColor.green, dropColor.blue);
            }
            if(drop) NewDropMoving(drop);
        }
    }
}

void WaterDrops::FillScreen(int n)
{
    if(!ms_initialised) return;
    
    float x, y, size;
    ms_numDrops = 0;
    WaterDrop* dropEnd = &ms_drops[n];
    for(WaterDrop* drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
    {
        drop->active = 0;
        if(drop < dropEnd)
        {
            x = rand() % nRX;
            y = rand() % nRY;
            size = rand() % (maxminDiff) + minScaled;
            PlaceNew(x, y, size, 2000.0f, 1, dropColor.red, dropColor.green, dropColor.blue);
        }
    }
}

void WaterDrops::Clear(void)
{
    for(WaterDrop* drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
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
    for(int i = 0; i < 2 * 4 * WaterDrops::MAXDROPS; ++i)
    {
        DropletsBuffer[i].rhw = 1.0f;
        DropletsBuffer[i].pos.z = 0.0f;
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
void WaterDrops::AddToRenderList(WaterDrop *drop)
{
    int i;
    float scale;

    float u1_1, u1_2;
    float v1_1, v1_2;
    float tmp = drop->uvsize * (300.0f - 40.0f) + 40.0f;
    
    u1_1 = drop->x - tmp;
    v1_1 = drop->y - tmp;
    u1_2 = drop->x + tmp;
    v1_2 = drop->y + tmp;
    u1_1 = (u1_1 <= 0.0f ? 0.0f : u1_1) * fRXInv;
    v1_1 = (v1_1 <= 0.0f ? 0.0f : v1_1) * fRYInv;
    u1_2 = (u1_2 >= fRX ? fRX : u1_2) * fRXInv;
    v1_2 = (v1_2 >= fRY ? fRY : v1_2) * fRYInv;

    scale = drop->size * 0.5f;

    // Mask
    for(i = 0; i < 4; i++)
    {
        ms_vertPtr->pos.x = drop->x + xy[i * 2] * scale;
        ms_vertPtr->pos.y = drop->y + xy[i * 2 + 1] * scale;
        ms_vertPtr->rgba = dropMaskColor;
        ms_vertPtr->rgba.alpha = drop->color.alpha;
        ms_vertPtr->texCoord.u = uv[i * 2];
        ms_vertPtr->texCoord.v = uv[i * 2 + 1];
        ms_vertPtr++;
    }
    // Drop
    for(i = 0; i < 4; i++)
    {
        ms_vertPtr->pos.x = drop->x + xy[i * 2] * scale;
        ms_vertPtr->pos.y = drop->y + xy[i * 2 + 1] * scale;
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
