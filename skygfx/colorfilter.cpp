#include <externs.h>

/* Others */
enum
{
    COLFIL_MOBILE = 0,
    COLFIL_PS2,
    COLFIL_PC,
    COLFIL_NONE,

    COLFIL_MAX
};

/* Variables */
float g_fPostFXAlphaDiv = 0.0f;
unsigned char g_nColorFilter = COLFIL_MOBILE;
bool g_bUsePCTimecyc = true;

const char* aColorFilterNames[COLFIL_MAX] = 
{
    "Default (Mobile)",
    "PS2 Style",
    "PC Style",
    "No Colorfilter",
};

/* Configs */
ConfigEntry* pCFGColorFilter;
ConfigEntry* pCFGUsePCTimecyc;

/* Functions */
void ColorfilterSettingChanged(int oldVal, int newVal, void* data)
{
    if(oldVal == newVal) return;

    pCFGColorFilter->SetInt(newVal);
    pCFGColorFilter->Clamp(0, COLFIL_MAX - 1);
    g_nColorFilter = pCFGColorFilter->GetInt();

    cfg->Save();
}
void PCTimecycSettingChanged(int oldVal, int newVal, void* data)
{
    if(oldVal == newVal) return;

    pCFGUsePCTimecyc->SetBool(newVal != 0);
    g_bUsePCTimecyc = pCFGUsePCTimecyc->GetBool();

    g_fPostFXAlphaDiv = 1.0f / (g_bUsePCTimecyc ? 256.0f : 128.0f);
    
    cfg->Save();
}

/* Hooks */
uintptr_t ColorFilter_BackTo;
extern "C" uintptr_t ColorFilter_Patch(char* sp)
{
    // Grading values (i.e. the color matrix)
    RwRGBAReal *red   = (RwRGBAReal *)(sp + 0x30);
    RwRGBAReal *green = (RwRGBAReal *)(sp + 0x20);
    RwRGBAReal *blue  = (RwRGBAReal *)(sp + 0x10);

    // PostFX values from Timecycle
    RwRGBA *postfx1 = (RwRGBA *)(sp + BYBIT(0xC, 0x58));
    RwRGBA *postfx2 = (RwRGBA *)(sp + 0x8);

    switch(g_nColorFilter)
    {
        case COLFIL_PS2:
        {
            float a = postfx2->alpha * g_fPostFXAlphaDiv;

            red->red     = ( postfx1->red   + a * postfx2->red   ) * 0.0078125f;
            green->green = ( postfx1->green + a * postfx2->green ) * 0.0078125f;
            blue->blue   = ( postfx1->blue  + a * postfx2->blue  ) * 0.0078125f;

            red->green = red->blue   = red->alpha   = 0.0f;
            green->red = green->blue = green->alpha = 0.0f;
            blue->red  = blue->green = blue->alpha  = 0.0f;

            break;
        }
        case COLFIL_PC:
        {
            float a1 = postfx1->alpha * g_fPostFXAlphaDiv;
            float a2 = postfx2->alpha * g_fPostFXAlphaDiv;

            red->red     = 1.0f + ( a1 * postfx1->red   + a2 * postfx2->red   ) * 0.00390625f;
            green->green = 1.0f + ( a1 * postfx1->green + a2 * postfx2->green ) * 0.00390625f;
            blue->blue   = 1.0f + ( a1 * postfx1->blue  + a2 * postfx2->blue  ) * 0.00390625f;

            red->green = red->blue   = red->alpha   = 0.0f;
            green->red = green->blue = green->alpha = 0.0f;
            blue->red  = blue->green = blue->alpha  = 0.0f;

            break;
        }
        case COLFIL_NONE:
        {
            red->red   = green->green = blue->blue   = 1.0f;
            red->green = red->blue    = red->alpha   = 0.0f;
            green->red = green->blue  = green->alpha = 0.0f;
            blue->red  = blue->green  = blue->alpha  = 0.0f;
            
            break;
        }
        default: // COLFIL_MOBILE
        {
            float r = postfx1->alpha * postfx1->red   + postfx2->alpha * postfx2->red;
            float g = postfx1->alpha * postfx1->green + postfx2->alpha * postfx2->green;
            float b = postfx1->alpha * postfx1->blue  + postfx2->alpha * postfx2->blue;
            float invsqrt = 1.0f / sqrtf(r*r + g*g + b*b);
            if(g_bUsePCTimecyc) invsqrt *= 0.5f;

            r *= invsqrt;
            g *= invsqrt;
            b *= invsqrt;

            red->red     *= (1.5f + r * 1.732f) * 0.4f;
            green->green *= (1.5f + g * 1.732f) * 0.4f;
            blue->blue   *= (1.5f + b * 1.732f) * 0.4f;

            break;
        }
    }
    return ColorFilter_BackTo;
}
NAKEDAT void ColorFilter_Inject(void)
{
  #ifdef AML32
    asm("PUSH {R0-R11}\n"
        "ADD R0, SP, #0x30\n"
        "BL ColorFilter_Patch\n"
        "VLDR S2, [SP, #(0x30 + 0x30)]\n" // red.r
        "VLDR S4, [SP, #(0x30 + 0x24)]\n" // green.g
        "VLDR S6, [SP, #(0x30 + 0x18)]\n" // blue.b
        "MOV R12, R0\n"
        "POP {R0-R11}\n"
        "BX R12"
    );
  #else
    asm("MOV X0, SP\n"
        "SUB SP, SP, #0x10\n"

        "STR S4, [SP, #0x0]\n" // save S4 (it is used bit later, 255.0f)
        "STR W9, [SP, #0x4]\n"
        "STR W10, [SP, #0x8]\n"

        "BL ColorFilter_Patch\n"

        "LDR S4, [SP, #0x0]\n" // restore S4 (it is used right after, 255.0f)
        "LDR W9, [SP, #0x4]\n"
        "LDR W10, [SP, #0x8]\n"
        "ADD SP, SP, #0x10\n"

        "LDR S3, [SP, #0x30]\n" // red.r
        "LDR S2, [SP, #0x24]\n" // green.g
        "LDR S1, [SP, #0x18]\n" // blue.b

        "BR X0"
    );
  #endif
}

/* Main */
void StartColorfilter()
{
  #ifdef AML32
    ColorFilter_BackTo = pGTASA + 0x5B6444 + 0x1;
    aml->Redirect(pGTASA + 0x5B643C + 0x1, (uintptr_t)ColorFilter_Inject);
    aml->Write32(pGTASA + 0x5B6444, 0x28007818);
  #else
    ColorFilter_BackTo = pGTASA + 0x6DA700;
    aml->Redirect(pGTASA + 0x6DA6F0, (uintptr_t)ColorFilter_Inject);
    aml->Write32(pGTASA + 0x6DA700, 0xD0000B88);
    aml->Write32(pGTASA + 0x6DA704, 0xF9452108);
    aml->Write32(pGTASA + 0x6DA708, 0x39400108);
  #endif

    pCFGColorFilter = cfg->Bind("Colorfilter", COLFIL_MOBILE, "Visuals");
    ColorfilterSettingChanged(COLFIL_MOBILE, pCFGColorFilter->GetInt(), NULL);
    AddSetting("Colorfilter", g_nColorFilter, 0, sizeofA(aColorFilterNames)-1, aColorFilterNames, ColorfilterSettingChanged, NULL);

    pCFGUsePCTimecyc = cfg->Bind("UsePCTimecyc", g_bUsePCTimecyc, "Visuals");
    PCTimecycSettingChanged(1, pCFGUsePCTimecyc->GetBool(), NULL);
    AddSetting("Using PC Timecyc", (g_bUsePCTimecyc != 0), 0, sizeofA(aYesNo)-1, aYesNo, PCTimecycSettingChanged, NULL);
}