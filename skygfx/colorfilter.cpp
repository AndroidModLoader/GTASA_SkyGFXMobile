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
unsigned char g_nColorFilter = COLFIL_MOBILE;
ConfigEntry* pCFGColorFilter;
const char* aColorFilterNames[COLFIL_MAX] = 
{
    "Default (Mobile)",
    "PS2 Style",
    "PC Style",
    "No Colorfilter",
};

/* Functions */
void ColorfilterSettingChanged(int oldVal, int newVal, void* data)
{
    pCFGColorFilter->SetInt(newVal);
    pCFGColorFilter->Clamp(0, COLFIL_MAX - 1);
    g_nColorFilter = pCFGColorFilter->GetInt();

    cfg->Save();
}

/* Hooks */
uintptr_t ColorFilter_BackTo;
extern "C" void ColorFilter_Patch(char* sp)
{
    // Grading values (i.e. the color matrix)
    RwRGBAReal *red   = (RwRGBAReal *)(sp + 0x30);
    RwRGBAReal *green = (RwRGBAReal *)(sp + 0x20);
    RwRGBAReal *blue  = (RwRGBAReal *)(sp + 0x10);

    // PostFX values from Timecycle
    RwRGBA *postfx1 = (RwRGBA *)(sp + 0xC);
    RwRGBA *postfx2 = (RwRGBA *)(sp + 0x8);

    switch(g_nColorFilter)
    {
        case COLFIL_PS2:
        {
            float a = postfx2->alpha / 128.0f;

            red->red     = postfx1->red   / 128.0f + a * postfx2->red   / 128.0f;
            green->green = postfx1->green / 128.0f + a * postfx2->green / 128.0f;
            blue->blue   = postfx1->blue  / 128.0f + a * postfx2->blue  / 128.0f;

            red->green = red->blue   = red->alpha   = 0.0f;
            green->red = green->blue = green->alpha = 0.0f;
            blue->red  = blue->green = blue->alpha  = 0.0f;

            break;
        }
        case COLFIL_PC:
        {
            float a1 = postfx1->alpha / 128.0f;
            float a2 = postfx2->alpha / 128.0f;

            red->red     = 1.0f + a1 * postfx1->red   / 255.0f + a2 * postfx2->red   / 255.0f;
            green->green = 1.0f + a1 * postfx1->green / 255.0f + a2 * postfx2->green / 255.0f;
            blue->blue   = 1.0f + a1 * postfx1->blue  / 255.0f + a2 * postfx2->blue  / 255.0f;

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

            r *= invsqrt;
            g *= invsqrt;
            b *= invsqrt;

            red->red     *= (1.5f + r * 1.732f) * 0.4f;
            green->green *= (1.5f + g * 1.732f) * 0.4f;
            blue->blue   *= (1.5f + b * 1.732f) * 0.4f;

            break;
        }
    }
}
NAKEDAT void ColorFilter_Inject(void)
{
    asm("PUSH {R0-R11}\n"
        "ADD R0, SP, 0x30\n"
        "BL ColorFilter_Patch\n"
        "VLDR S2, [SP, #(0x30 + 0x30)]\n" // red.r
        "VLDR S4, [SP, #(0x30 + 0x24)]\n" // green.g
        "VLDR S6, [SP, #(0x30 + 0x18)]\n" // blue.b
    );

    asm("MOV R12, %0\n"
        "POP {R0-R11}\n"
        "BX R12\n"
      :: "r" (ColorFilter_BackTo)
    );
}

/* Main */
void StartColorfilter()
{
    ColorFilter_BackTo = pGTASA + 0x5B6444 + 0x1;
    aml->Redirect(pGTASA + 0x5B643C + 0x1, (uintptr_t)ColorFilter_Inject);
    aml->Write(pGTASA + 0x5B6444, pGTASA + 0x5B63DC, 2);
    aml->Write(pGTASA + 0x5B6446, pGTASA + 0x5B63EA, 2);

    pCFGColorFilter = cfg->Bind("Colorfilter", COLFIL_MOBILE, "Visuals");
    pCFGColorFilter->Clamp(0, COLFIL_MAX - 1);
    g_nColorFilter = pCFGColorFilter->GetInt();
    if(g_nColorFilter != COLFIL_MOBILE) ColorfilterSettingChanged(COLFIL_MOBILE, g_nColorFilter, NULL);
    AddSetting("Colorfilter", g_nColorFilter, 0, sizeofA(aColorFilterNames)-1, aColorFilterNames, ColorfilterSettingChanged, NULL);
}
