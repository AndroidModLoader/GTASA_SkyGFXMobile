#pragma once

extern uintptr_t pGTASA_Colorfilter;

extern const char* pColorFilterSettings[4];
extern int nColorFilter;
void ColorFilter_stub(void);
void ColorfilterChanged(int oldVal, int newVal, void* data);
