#include <externs.h>

struct WaterDrop
{
    float x, y, time;
    float size, uvsize, ttl;
    RwRGBA color;
    bool active = false;
    bool fades;

    void Fade(void);
};

struct WaterDropMoving
{
    WaterDrop *drop = NULL;
    float dist = 0.0f;
};

class WaterDrops
{
public:
    enum
    {
        MAXDROPS = 1500,
        MAXDROPSMOVING = 700
    };

    // Config

    static inline bool neoWaterDrops = true;
    static inline bool neoBloodDrops = true;
    static inline bool neoDirtyDrops = true;

    // Debugging

    static inline bool sprayWater = false;
    static inline bool sprayBlood = false;

    // Logic

    static inline WaterDrop ms_drops[MAXDROPS] {};
    static inline int ms_numDrops = 0;
    static inline WaterDropMoving ms_dropsMoving[MAXDROPSMOVING] {};
    static inline int ms_numDropsMoving = 0;

    static inline bool ms_enabled;
    static inline bool ms_movingEnabled;

    static inline float ms_distMoved = 0.0f, ms_vecLen = 0.0f, ms_rainStrength = 0.0f;
    static inline RwV3d ms_vec;
    static inline RwV3d ms_lastAt;
    static inline RwV3d ms_lastPos;
    static inline RwV3d ms_posDelta;

    static inline int ms_splashDuration = -1;
    static inline RwTexture* m_pMask = NULL;

    static void InitialiseHooks();
    static void Process(void);
    static void CalculateMovement(void);
    static void SprayDrops(void);
    static void MoveDrop(WaterDropMoving *moving);
    static void ProcessMoving(void);
    static void Fade(void);

    static WaterDrop *PlaceNew(float x, float y, float size, float time, bool fades, int R, int G, int B);
    static void NewTrace(WaterDropMoving*);
    static void NewDropMoving(WaterDrop*);
    static void FillScreenMoving(float amount, bool isBlood = false, bool isDirt = false);
    static void FillScreen(int n);
    static void Clear(void);
    static void Reset(void);

    static void RegisterSplash(RwV3d* point, float distance = 20.0f, int duration = 14);
    static bool NoDrops(void);
    static bool NoRain(void);

    // Rendering

    //static inline int ms_fbWidth, ms_fbHeight;
    static inline RwOpenGLVertex *ms_vertPtr;
    static inline int ms_numBatchedDrops;
    static inline int ms_initialised;

    static void InitialiseRender(RwCamera *cam);
    static void AddToRenderList(WaterDrop *drop);
    static void Render(void);
};
