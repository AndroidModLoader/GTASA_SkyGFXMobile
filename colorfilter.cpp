#include <mod/config.h>
#include <gtasa_things.h>
#include <renderware_things.h>

int nColorFilter = 0;
extern ConfigEntry* pColorfilter;
const char* pColorFilterSettings[4] = 
{
    "Default (Mobile)",
    "None",
    "PS2 Style",
    "PC Style",
};

void ColorfilterChanged(int oldVal, int newVal)
{
    nColorFilter = newVal;
    switch(newVal)
    {
        case 1:
            pColorfilter->SetString("none");
            break;
            
        case 2:
            pColorfilter->SetString("ps2");
            break;
            
        case 3:
            pColorfilter->SetString("pc");
            break;
            
        default:
            pColorfilter->SetString("default");
            break;
    }
    cfg->Save();
}

extern "C" void ColorFilter(char* sp)
{
    // Grading values (i.e. the color matrix)
    RwRGBAReal *red = (RwRGBAReal *)(sp + 0x30);
    RwRGBAReal *green = (RwRGBAReal *)(sp + 0x20);
    RwRGBAReal *blue = (RwRGBAReal *)(sp + 0x10);

    // PostFX values from Timecycle
    RwRGBA *postfx1 = (RwRGBA *)(sp + 0x0C);
    RwRGBA *postfx2 = (RwRGBA *)(sp + 0x08);

    switch(nColorFilter)
    {
        case 1: // No filter
        {
            red->red = 1.0f;
            green->green = 1.0f;
            blue->blue = 1.0f;
            red->green = red->blue = red->alpha = 0.0f;
            green->red = green->blue = green->alpha = 0.0f;
            blue->red = blue->green = blue->alpha = 0.0f;
            break;
        }

        case 2: // PS2
        {
            float a = postfx2->alpha/128.0f;
            red->red = postfx1->red/128.0f + a*postfx2->red/128.0f;
            green->green = postfx1->green/128.0f + a*postfx2->green/128.0f;
            blue->blue = postfx1->blue/128.0f + a*postfx2->blue/128.0f;
            red->green = red->blue = red->alpha = 0.0f;
            green->red = green->blue = green->alpha = 0.0f;
            blue->red = blue->green = blue->alpha = 0.0f;
            break;
        }

        case 3: // PC
        {
            float a1 = postfx1->alpha/128.0f;
            float a2 = postfx2->alpha/128.0f;
            red->red = 1.0f + a1*postfx1->red/255.0f + a2*postfx2->red/255.0f;
            green->green = 1.0f + a1*postfx1->green/255.0f + a2*postfx2->green/255.0f;
            blue->blue = 1.0f + a1*postfx1->blue/255.0f + a2*postfx2->blue/255.0f;
            red->green = red->blue = red->alpha = 0.0f;
            green->red = green->blue = green->alpha = 0.0f;
            blue->red = blue->green = blue->alpha = 0.0f;
            break;
        }

        default: // Mobile (default)
        {
            float r = postfx1->alpha*postfx1->red   + postfx2->alpha*postfx2->red;
            float g = postfx1->alpha*postfx1->green + postfx2->alpha*postfx2->green;
            float b = postfx1->alpha*postfx1->blue  + postfx2->alpha*postfx2->blue;
            float invsqrt = 1.0f/sqrtf(r*r + g*g + b*b);
            r *= invsqrt;
            g *= invsqrt;
            b *= invsqrt;
            red->red *= (1.5f + r*1.732f)*0.4f;
            green->green *= (1.5f + g*1.732f)*0.4f;
            blue->blue *= (1.5f + b*1.732f)*0.4f;
            break;
        }
    }
}

extern uintptr_t pGTASAAddr_Colorfilter;
// "optnone" will be ignored on GCC but this is required for CLang to stop doing VERY DUMB OPTIMIZATIONS
__attribute__((optnone)) __attribute__((naked)) void ColorFilter_stub(void)
{
    // !!! Do not type here anything !!!
    // !!! Adding ANY function WILL BREAK IT !!!

    asm volatile(
        "push {r0-r11}\n" // 12*4=0x30
        "add r0, sp, 0x30\n"
        "bl ColorFilter\n"
        "vldr s2, [sp, #(0x30+0x30)]\n" // red.r
        "vldr s4, [sp, #(0x24+0x30)]\n" // green.g
        "vldr s6, [sp, #(0x18+0x30)]\n" // blue.b
    );

    // End the function
    asm volatile(
        "mov r12, %0\n" // Should be there! BEFORE POP
                        // Anyway Clang uses R0 and then R12, which is not correct and completely dumb way
        "pop {r0-r11}\n"// We're popping r0-r11 BUT NOT r12 !!!
        "bx r12\n"
      :: "r" (pGTASAAddr_Colorfilter)
    );
}