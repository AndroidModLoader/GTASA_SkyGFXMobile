#pragma once

enum ePipelineDualpassWay : uint8_t
{
    DPWay_Default = 0,
    DPWay_Alpha,
    DPWay_Everything,
    DPWay_LastMesh,

    DPWay_MaxWays,
};
extern ePipelineDualpassWay pipelineWay;

RxPipeline* CCustomBuildingDNPipeline_CreateCustomObjPipe_SkyGfx();

extern const char* pPipelineSettings[4];
void PipelineChanged(int oldVal, int newVal, void* data);
// End emu_*
