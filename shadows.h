struct RwTexture;
class CShadowCamera;
class RpLight;
class CPhysical;

#pragma pack(push, 1)

struct ShadowCameraStorage
{
    CShadowCamera* m_pShadowCamera;
    RwTexture*     m_pShadowTexture;
};
class CRealTimeShadowManager_NEW
{
public:
    CRealTimeShadowManager_NEW(int shadowsCount)
    {
        this->m_bInitialized = false;
        this->m_nMaxShadows = shadowsCount;
        this->m_pShadows = new CRealTimeShadow*[shadowsCount];
        this->m_pBlurCamera.m_pShadowCamera = NULL;
        this->m_pBlurCamera.m_pShadowTexture = NULL;
        this->m_pGradientCamera.m_pShadowCamera = NULL;
        this->m_pGradientCamera.m_pShadowTexture = NULL;
        for(int i = 0; i < shadowsCount; ++i)
        {
            this->m_pShadows[i] = NULL;
        }
    }
public:
    bool m_bInitialized; // 0
    int m_nMaxShadows; // 5
    CRealTimeShadow** m_pShadows; // 9
    char filler[155]; // 13
    bool m_bSomethingForVehicleRenderPipe; // 164
    ShadowCameraStorage m_pBlurCamera; // 168
    ShadowCameraStorage m_pGradientCamera; // 176
};
extern CRealTimeShadowManager_NEW* g_realTimeShadowMan;

#pragma pack(pop)

void PatchShadows();
void RTShadows();
void PatchRTShadowMan();
void DoShadowThisFrame(CRealTimeShadowManager_NEW* self, CPhysical* physical);