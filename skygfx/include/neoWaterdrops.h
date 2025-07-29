#include <externs.h>

struct WaterDrop
{
    float x, y, time;
    float size, uvsize, ttl;
    RwRGBA color;
    bool active;
    bool fades;

    void Fade(void);
};

struct WaterDropMoving
{
    WaterDrop *drop;
    float dist;
};

class WaterDrops
{
public:
    enum {
        MAXDROPS = 2000,
        MAXDROPSMOVING = 700
    };

    // Logic

    static inline float ms_xOff, ms_yOff; // not quite sure what these are
    static inline WaterDrop ms_drops[MAXDROPS] {};
    static inline int ms_numDrops;
    static inline WaterDropMoving ms_dropsMoving[MAXDROPSMOVING] {};
    static inline int ms_numDropsMoving;

    static inline bool ms_enabled;
    static inline bool ms_movingEnabled;

    static inline float ms_distMoved, ms_vecLen, ms_rainStrength;
    static inline RwV3d ms_vec;
    static inline RwV3d ms_lastAt;
    static inline RwV3d ms_lastPos;
    static inline RwV3d ms_posDelta;

    static inline int ms_splashDuration = -1;

    // debugging
    static inline bool sprayWater = false;
    static inline bool sprayBlood = false;

    static void Process(void);
    static void CalculateMovement(void);
    static void SprayDrops(void);
    static void MoveDrop(WaterDropMoving *moving);
    static void ProcessMoving(void);
    static void Fade(void);

    static WaterDrop *PlaceNew(float x, float y, float size, float time, bool fades, int R, int G, int B);
    static void NewTrace(WaterDropMoving*);
    static void NewDropMoving(WaterDrop*);
    // this has one more argument in VC: ttl, but it's always 2000.0
    static void FillScreenMoving(float amount, bool isBlood);
    static void FillScreen(int n);
    static void Clear(void);
    static void Reset(void);

    static void RegisterSplash(RwV3d* point, float distance = 20.0f, int duration = 14);
    static bool NoDrops(void);
    static bool NoRain(void);

    // Rendering

    static inline RwTexture *ms_maskTex;
    static inline RwTexture *ms_tex;
    // static inline RwRaster *ms_maskRaster;
    static inline RwRaster *ms_raster;
    static inline int ms_fbWidth, ms_fbHeight;
    static inline void *ms_vertexBuf;
    static inline void *ms_indexBuf;
    static inline RwOpenGLVertex *ms_vertPtr;
    static inline int ms_numBatchedDrops;
    static inline int ms_initialised;

    static void InitialiseRender(RwCamera *cam);
    static void AddToRenderList(WaterDrop *drop);
    static void Render(void);
};