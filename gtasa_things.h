#include <stdint.h>
#include "GTASA_STRUCTS.h"

#define SET_TO(__a1, __a2) *(void**)&(__a1) = (void*)(__a2)

extern void* pGTASA;
extern int nColorFilter;
extern uintptr_t pGTASAAddr;

void Redirect(uintptr_t addr, uintptr_t to);