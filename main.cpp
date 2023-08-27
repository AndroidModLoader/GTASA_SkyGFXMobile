#include <externs.h>

MYMOD(net.rusjj.skygfxmobile, SkyGFX Mobile Beta, 0.3, aap & TheOfficialFloW & RusJJ)
NEEDGAME(com.rockstargames.gtasa)
BEGIN_DEPLIST()
    ADD_DEPENDENCY_VER(net.rusjj.aml, 1.1)
END_DEPLIST()

/* Saves */
void* hGTASA;
uintptr_t pGTASA;

/* SkyGFX Externs */
void StartColorfilter();

/* Main */
extern "C" void OnModLoad()
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
    cfg->Bind("Author", "", "About")->SetString("[-=KILL MAN=-]"); cfg->ClearLast();
    cfg->Bind("IdeasFrom", "", "About")->SetString("aap, TheOfficialFloW"); cfg->ClearLast();
    cfg->Bind("Discord", "", "About")->SetString("https://discord.gg/2MY7W39kBg"); cfg->ClearLast();
    cfg->Bind("GitHub", "", "About")->SetString("https://github.com/AndroidModLoader/GTASA_SkyGFXMobile"); cfg->ClearLast();
    cfg->Save();

    /* Uhm... Modules? */
    StartColorfilter();
}

static Config cfgLocal("SkyGFXMobile");
Config* cfg = &cfgLocal;