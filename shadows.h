struct RwTexture;
class CShadowCamera;
class RpLight;
class CPhysical;



#define MAX_RT_SHADOWS 40



#pragma pack(push, 1)

struct ShadowCameraStorage
{
    CShadowCamera* m_pShadowCamera;
    RwTexture*     m_pShadowTexture;
};
class CRealTimeShadowManager_NEW
{
public:
    CRealTimeShadowManager_NEW()
    {
        this->m_bInitialized = false;
        this->m_pBlurCamera.m_pShadowCamera = NULL;
        this->m_pBlurCamera.m_pShadowTexture = NULL;
        this->m_pGradientCamera.m_pShadowCamera = NULL;
        this->m_pGradientCamera.m_pShadowTexture = NULL;
        for(int i = 0; i < MAX_RT_SHADOWS; ++i)
        {
            this->m_pShadows[i] = NULL;
        }
    }
public:
    bool m_bInitialized; // 0
    char filler[163]; // 1
    bool m_bSomethingForVehicleRenderPipe; // 164
    ShadowCameraStorage m_pBlurCamera; // 168
    ShadowCameraStorage m_pGradientCamera; // 176
    CRealTimeShadow* m_pShadows[MAX_RT_SHADOWS];
};
extern CRealTimeShadowManager_NEW* g_realTimeShadowMan;

#pragma pack(pop)

void PatchShadows();
void RTShadows();
void PatchRTShadowMan();