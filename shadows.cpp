#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <gtasa_things.h>
#include "GTASA_STRUCTS.h"
#include <shadows.h>
#include <stdlib.h>

CRegisteredShadow* asShadowsStored_NEW;
void (*EntityPreRender)(CEntity* entity);
void (*DoShadowThisFrame)(CRealTimeShadowManager* realtimeShadowManager, CPhysical* entity);

uintptr_t ObjectPreRender_jumpto;

void PatchShadows()
{
    // Static shadows?
    asShadowsStored_NEW = new CRegisteredShadow[0xFF];
    aml->Write(pGTASAAddr + 0x677BEC, (uintptr_t)&asShadowsStored_NEW, sizeof(void*));
    // CShadows::StoreShadowToBeRendered
    aml->Write(pGTASAAddr + 0x5B929A, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASAAddr + 0x5B92C0, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASAAddr + 0x5B92E6, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASAAddr + 0x5B930A, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASAAddr + 0x5B932E, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASAAddr + 0x5B9358, (uintptr_t)"\xFE", 1);
    // CShadows::StoreShadowToBeRendered (2nd arg is RwTexture*)
    aml->Write(pGTASAAddr + 0x5B9444, (uintptr_t)"\xFE", 1);
    // CShadows::StoreShadowForVehicle
    aml->Write(pGTASAAddr + 0x5B9BD4, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASAAddr + 0x5B9B2A, (uintptr_t)"\xFE", 1);
    // CShadows::StoreShadowForPedObject
    aml->Write(pGTASAAddr + 0x5B9F62, (uintptr_t)"\xFE", 1);
    // CShadows::StoreRealTimeShadow
    aml->Write(pGTASAAddr + 0x5BA29E, (uintptr_t)"\xFE", 1);
    // CShadows::RenderExtraPlayerShadows
    aml->Write(pGTASAAddr + 0x5BDDBA, (uintptr_t)"\xFE", 1);
    aml->Write(pGTASAAddr + 0x5BDD5A, (uintptr_t)"\xFE", 1);

    logger->Info("Static shadows storage has been bumped!");
}

inline bool CanRenderShadowForObjectId(short id)
{
    switch(id)
    {
        case 954:
        case 1210:
        case 1212:
        case 1213:
        case 1239:
        case 1240:
        case 1241:
        case 1242:
        case 1247:
        case 1248:
        case 1252:
        case 1253:
        case 1254:
        case 1272:
        case 1273:
        case 1274:
        case 1275:
        case 1276:
        case 1277:
        case 1279:
        case 1313:
        case 1314:
        case 1318:
            return false;
    }
    return true;
}

extern "C" void ObjectPreRenderPatch(CObject* object)
{
    EntityPreRender(object);
    // Flags ARE NOT WORKING (wtf?) so im using my own fn // if((*(int*)(*(int*)entity + 324) & (1 << 8)) != 0) 
    if(CanRenderShadowForObjectId(object->m_nModelIndex)) DoShadowThisFrame((CRealTimeShadowManager*)g_realTimeShadowMan, object);
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

DECL_HOOKv(RTShadowUpdate, uintptr_t shadow)
{
    int v2 = *(int*)(shadow + 24);
    if((v2 == 1 || v2 == 2) && *(int*)(*(int*)shadow + 24) == 0) return;
    RTShadowUpdate(shadow);
}

void RTShadows()
{
    // RT Shadows? RealTime, not RayTraced!
    PatchRTShadowMan();
    
    SET_TO(ObjectPreRender_jumpto,        pGTASAAddr + 0x454E60 + 0x1);
    SET_TO(DoShadowThisFrame,             aml->GetSym(pGTASA, "_ZN22CRealTimeShadowManager17DoShadowThisFrameEP9CPhysical"));
    SET_TO(EntityPreRender,               aml->GetSym(pGTASA, "_ZN7CEntity9PreRenderEv"));
    Redirect(pGTASAAddr + 0x454E56 + 0x1, (uintptr_t)ObjectPreRender_stub); // Add shadows for OBJECTs
    aml->PlaceRET(aml->GetSym(pGTASA,     "_ZN8CShadows18StoreShadowForPoleEP7CEntityfffffj") & ~1); // Disable static pole shadows
    // Fixing weird crashes...
    HOOKPLT(RTShadowUpdate,               pGTASAAddr + 0x6759E4);
    aml->PlaceRET(aml->GetSym(pGTASA,     "_ZN22CRealTimeShadowManager20ReturnRealTimeShadowEP15CRealTimeShadow") & ~1);
    Redirect(pGTASAAddr + 0x454D58 + 0x1, pGTASAAddr + 0x454DD4 + 0x1); // Removed StoreShadowToBeRendered from Object::PreRender

    logger->Info("Shadows for dynamic objects are enabled...");
}