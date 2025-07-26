#include <externs.h>

MYMOD(net.rusjj.skygfxmobile, SkyGFX Mobile Beta, 0.5, aap & TheOfficialFloW & RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.3)
END_DEPLIST()

/* Saves */
void* hGTASA;
uintptr_t pGTASA;
uint32_t PS2_R; // PS2 Randomizer (thanks aap)

/* SkyGFX Externs */
void StartInterface();
void StartShaders();
void StartColorfilter();
void StartBuildingPipeline();
void StartShading();
void StartVehiclePipeline();
void StartMiscStuff();
void StartEnvMapStuff();
void StartEffectsStuff();
void StartRenderQueue();

/* Main */
extern "C" void OnModPreLoad()
{
    StartInterface();
}
extern "C" void OnAllModsLoaded()
{
    /* Logging */
    logger->SetTag("SkyGFX Mobile");

    /* Libraries */
    hGTASA = aml->GetLibHandle("libGTASA.so");
    pGTASA = aml->GetLib("libGTASA.so");

  #ifdef _SAUTILS_INTERFACE
    sautils = (ISAUtils*)GetInterface("SAUtils");
    if(sautils)
    {
        skygfxSettingsTab = sautils->AddSettingsTab("SkyGFX", "menu_maindisplay");
    }
  #endif
    
    /* Info in a config */
    cfg->Bind("MobileAuthor", "", "About")->SetString("[-=KILL MAN=-]"); cfg->ClearLast();
    cfg->Bind("MainStuffDevelopedBy", "", "About")->SetString("aap, TheOfficialFloW"); cfg->ClearLast();
    cfg->Bind("Discord", "", "About")->SetString("https://discord.gg/2MY7W39kBg"); cfg->ClearLast();
    cfg->Bind("GitHub", "", "About")->SetString("https://github.com/AndroidModLoader/GTASA_SkyGFXMobile"); cfg->ClearLast();
    cfg->Save();

    /* Get game functions and variables */
    ResolveExternals();

    /* Uhm... Modules? */
    StartColorfilter();
    StartVehiclePipeline();
    StartMiscStuff();
    StartShading();
    StartEnvMapStuff();
    StartShaders();
    StartBuildingPipeline();
    StartEffectsStuff();
    StartRenderQueue();
}

static Config cfgLocal("SkyGFXMobile");
Config* cfg = &cfgLocal;