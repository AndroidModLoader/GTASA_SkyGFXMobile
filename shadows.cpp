#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <gtasa_things.h>
#include <shadows.h>
#include <stdlib.h>

extern ConfigEntry* pMaxRTShadows;
extern ConfigEntry* pDropRTShadowsQuality;
extern ConfigEntry* pDynamicObjectsShadows;
extern ConfigEntry* pAllowPlayerClassicShadow;

CRegisteredShadow* asShadowsStored_NEW;
void (*EntityPreRender)(CEntity* entity);

uintptr_t ObjectPreRender_jumpto;

void PatchShadows()
{
    // Static shadows?
    asShadowsStored_NEW = new CRegisteredShadow[0xFF];
    aml->Write(pGTASA + 0x677BEC, (uintptr_t)&asShadowsStored_NEW, sizeof(void*));
    // CShadows::StoreShadowToBeRendered
    aml->Write(pGTASA + 0x5B929A, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASA + 0x5B92C0, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASA + 0x5B92E6, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASA + 0x5B930A, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASA + 0x5B932E, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASA + 0x5B9358, (uintptr_t)"\xFE", 1);
    // CShadows::StoreShadowToBeRendered (2nd arg is RwTexture*)
    aml->Write(pGTASA + 0x5B9444, (uintptr_t)"\xFE", 1);
    // CShadows::StoreShadowForVehicle
    aml->Write(pGTASA + 0x5B9BD4, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASA + 0x5B9B2A, (uintptr_t)"\xFE", 1);
    // CShadows::StoreShadowForPedObject
    aml->Write(pGTASA + 0x5B9F62, (uintptr_t)"\xFE", 1);
    // CShadows::StoreRealTimeShadow
    aml->Write(pGTASA + 0x5BA29E, (uintptr_t)"\xFE", 1);
    // CShadows::RenderExtraPlayerShadows
    aml->Write(pGTASA + 0x5BDDBA, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASA + 0x5BDD5A, (uintptr_t)"\xFE", 1);

    logger->Info("Static shadows storage has been bumped!");
}

extern int *RTShadowsQuality;
extern "C" void ObjectPreRenderPatch(CObject* object)
{
    EntityPreRender(object);
    if(!object->m_nObjectFlags.bIsPickup && *RTShadowsQuality == 2) DoShadowThisFrame(g_realTimeShadowMan, object);
}
__attribute__((optnone)) __attribute__((naked)) void ObjectPreRender_stub(void)
{
    asm volatile(
        "push {r0}\n"
        "mov r0, r4\n"
        "bl ObjectPreRenderPatch\n"
    );
    asm volatile("mov r12, %0\n" :: "r"(ObjectPreRender_jumpto));
    asm("pop {r0}\n"
        "vmov.f32 s2, #1.0\n"
        "bx r12\n");
}

DECL_HOOKv(RTShadowUpdate, CRealTimeShadow* shadow)
{
    if(shadow->m_pOwner->m_pRwObject != NULL) RTShadowUpdate(shadow);
}

void RTShadows()
{
    // RT Shadows? RealTime, not RayTraced!
    PatchRTShadowMan();
    
    if(pDynamicObjectsShadows->GetBool())
    {
        SET_TO(ObjectPreRender_jumpto,        pGTASA + 0x454E60 + 0x1);
        SET_TO(EntityPreRender,               aml->GetSym(hGTASA, "_ZN7CEntity9PreRenderEv"));
        Redirect(pGTASA + 0x454E56 + 0x1, (uintptr_t)ObjectPreRender_stub); // Add shadows for OBJECTs
        aml->PlaceRET(aml->GetSym(hGTASA,     "_ZN8CShadows18StoreShadowForPoleEP7CEntityfffffj") & ~1); // Disable static pole shadows
        // Fixing weird crashes...
        HOOKPLT(RTShadowUpdate,               pGTASA + 0x6759E4);
        aml->PlaceRET(aml->GetSym(hGTASA,     "_ZN22CRealTimeShadowManager20ReturnRealTimeShadowEP15CRealTimeShadow") & ~1);
        Redirect(pGTASA + 0x454D58 + 0x1, pGTASA + 0x454DD4 + 0x1); // Removed StoreShadowToBeRendered from Object::PreRender
    }
}