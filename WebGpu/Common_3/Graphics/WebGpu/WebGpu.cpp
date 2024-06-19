/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This file is part of The-Forge
 * (see https://github.com/ConfettiFX/The-Forge).
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "../../../../Common_3/Graphics/GraphicsConfig.h"

#ifdef WEBGPU

/************************************************************************/
/************************************************************************/
#if defined(_WINDOWS)
#include <Windows.h>
#endif

#if defined(__linux__)
#define stricmp(a, b) strcasecmp(a, b)
#define vsprintf_s    vsnprintf
#define strncpy_s     strncpy
#endif

#include "../../../../Common_3/Resources/ResourceLoader/ThirdParty/OpenSource/tinyimageformat/tinyimageformat_apis.h"
#include "../../../../Common_3/Resources/ResourceLoader/ThirdParty/OpenSource/tinyimageformat/tinyimageformat_base.h"
#include "../../../../Common_3/Resources/ResourceLoader/ThirdParty/OpenSource/tinyimageformat/tinyimageformat_query.h"
#include "../../../../Common_3/Utilities/ThirdParty/OpenSource/Nothings/stb_ds.h"
#include "../../../../Common_3/Utilities/ThirdParty/OpenSource/bstrlib/bstrlib.h"

#include "../../../../Common_3/Graphics/Interfaces/IGraphics.h"
#include "../../../../Common_3/Utilities/Interfaces/ILog.h"

#include "../../../../Common_3/Graphics/GraphicsConfig.h"
#include "../../../../Common_3/Utilities/Math/AlgorithmsImpl.h"
#include "../../../../Common_3/Utilities/Math/MathTypes.h"
#include "../../../../Common_3/Utilities/Threading/Atomics.h"

#include "../../../../Common_3/Utilities/Interfaces/IMemory.h"

#if defined(AUTOMATED_TESTING)
#include "../../Application/Interfaces/IScreenshot.h"
#endif

//-V:SAFE_FREE:779
#define SAFE_FREE(p_var)       \
    if (p_var)                 \
    {                          \
        tf_free((void*)p_var); \
        p_var = NULL;          \
    }

#define DECL_CHAINED_STRUCT(type, name)                         \
    type               name = {};                               \
    WGPUChainedStruct* name##Chain = (WGPUChainedStruct*)&name; \
    UNREF_PARAM(name##Chain);

#define DECL_CHAINED_STRUCT_OUT(type, name)                           \
    type                  name = {};                                  \
    WGPUChainedStructOut* name##Chain = (WGPUChainedStructOut*)&name; \
    UNREF_PARAM(name##Chain);

#define ADD_TO_NEXT_CHAIN(parent, child)                     \
    parent##Chain->next = (const WGPUChainedStruct*)(child); \
    parent##Chain = (WGPUChainedStruct*)parent##Chain->next;

#define ADD_TO_NEXT_CHAIN_OUT(parent, child)              \
    parent##Chain->next = (WGPUChainedStructOut*)(child); \
    parent##Chain = (WGPUChainedStructOut*)parent##Chain->next;

#define ADD_TO_NEXT_CHAIN_COND(cond, parent, child)              \
    if (cond)                                                    \
    {                                                            \
        parent##Chain->next = (const WGPUChainedStruct*)(child); \
        parent##Chain = (WGPUChainedStruct*)parent##Chain->next; \
    }

#define ADD_TO_NEXT_CHAIN_OUT_COND(cond, parent, child)             \
    if (cond)                                                       \
    {                                                               \
        parent##Chain->next = (WGPUChainedStructOut*)(child);       \
        parent##Chain = (WGPUChainedStructOut*)parent##Chain->next; \
    }

DECLARE_RENDERER_FUNCTION(void, getBufferSizeAlign, Renderer* pRenderer, const BufferDesc* pDesc, ResourceSizeAlign* pOut);
DECLARE_RENDERER_FUNCTION(void, getTextureSizeAlign, Renderer* pRenderer, const TextureDesc* pDesc, ResourceSizeAlign* pOut);
DECLARE_RENDERER_FUNCTION(void, addBuffer, Renderer* pRenderer, const BufferDesc* pDesc, Buffer** pp_buffer)
DECLARE_RENDERER_FUNCTION(void, removeBuffer, Renderer* pRenderer, Buffer* pBuffer)
DECLARE_RENDERER_FUNCTION(void, mapBuffer, Renderer* pRenderer, Buffer* pBuffer, ReadRange* pRange)
DECLARE_RENDERER_FUNCTION(void, unmapBuffer, Renderer* pRenderer, Buffer* pBuffer)
DECLARE_RENDERER_FUNCTION(void, cmdUpdateBuffer, Cmd* pCmd, Buffer* pBuffer, uint64_t dstOffset, Buffer* pSrcBuffer, uint64_t srcOffset,
                          uint64_t size)
DECLARE_RENDERER_FUNCTION(void, cmdUpdateSubresource, Cmd* pCmd, Texture* pTexture, Buffer* pSrcBuffer,
                          const struct SubresourceDataDesc* pSubresourceDesc)
DECLARE_RENDERER_FUNCTION(void, cmdCopySubresource, Cmd* pCmd, Buffer* pDstBuffer, Texture* pTexture,
                          const struct SubresourceDataDesc* pSubresourceDesc)
DECLARE_RENDERER_FUNCTION(void, addTexture, Renderer* pRenderer, const TextureDesc* pDesc, Texture** ppTexture)
DECLARE_RENDERER_FUNCTION(void, removeTexture, Renderer* pRenderer, Texture* pTexture)
/************************************************************************/
// Descriptor Pool Functions
/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
// Descriptor Set Structure
/************************************************************************/
typedef struct DescriptorIndexMap
{
    char*    key;
    uint32_t value;
} DescriptorIndexMap;

static const DescriptorInfo* GetDescriptor(const RootSignature* pRootSignature, const char* pResName)
{
    const DescriptorIndexMap* pNode = shgetp_null(pRootSignature->pDescriptorNameToIndexMap, pResName);
    if (pNode)
    {
        return &pRootSignature->pDescriptors[pNode->value];
    }
    else
    {
        LOGF(LogLevel::eERROR, "Invalid descriptor param (%s)", pResName);
        return NULL;
    }
}
/************************************************************************/
// Render Pass Implementation
/************************************************************************/
/************************************************************************/
// Globals
/************************************************************************/
/************************************************************************/
// Internal utility functions
/************************************************************************/
static inline FORGE_CONSTEXPR WGPUAddressMode ToAddressMode(AddressMode mode)
{
    switch (mode)
    {
    case ADDRESS_MODE_MIRROR:
        return WGPUAddressMode_MirrorRepeat;
    case ADDRESS_MODE_REPEAT:
        return WGPUAddressMode_Repeat;
    case ADDRESS_MODE_CLAMP_TO_EDGE:
        return WGPUAddressMode_ClampToEdge;
    case ADDRESS_MODE_CLAMP_TO_BORDER:
        return WGPUAddressMode_ClampToEdge;
    default:
        return WGPUAddressMode_Force32;
    }
}

static inline FORGE_CONSTEXPR WGPUCompareFunction ToCompareFunction(CompareMode mode)
{
    switch (mode)
    {
    case CMP_NEVER:
        return WGPUCompareFunction_Never;
    case CMP_LESS:
        return WGPUCompareFunction_Less;
    case CMP_EQUAL:
        return WGPUCompareFunction_Equal;
    case CMP_LEQUAL:
        return WGPUCompareFunction_LessEqual;
    case CMP_GREATER:
        return WGPUCompareFunction_Greater;
    case CMP_NOTEQUAL:
        return WGPUCompareFunction_NotEqual;
    case CMP_GEQUAL:
        return WGPUCompareFunction_GreaterEqual;
    case CMP_ALWAYS:
        return WGPUCompareFunction_Always;
    default:
        return WGPUCompareFunction_Force32;
    }
}

static inline FORGE_CONSTEXPR WGPUStencilOperation ToStencilOp(StencilOp op)
{
    switch (op)
    {
    case STENCIL_OP_KEEP:
        return WGPUStencilOperation_Keep;
    case STENCIL_OP_SET_ZERO:
        return WGPUStencilOperation_Zero;
    case STENCIL_OP_REPLACE:
        return WGPUStencilOperation_Replace;
    case STENCIL_OP_INVERT:
        return WGPUStencilOperation_Invert;
    case STENCIL_OP_INCR:
        return WGPUStencilOperation_IncrementWrap;
    case STENCIL_OP_DECR:
        return WGPUStencilOperation_DecrementWrap;
    case STENCIL_OP_INCR_SAT:
        return WGPUStencilOperation_IncrementClamp;
    case STENCIL_OP_DECR_SAT:
        return WGPUStencilOperation_DecrementClamp;
    default:
        return WGPUStencilOperation_Force32;
    }
}

static inline FORGE_CONSTEXPR WGPUBlendOperation ToBlendOp(BlendMode mode)
{
    switch (mode)
    {
    case BM_ADD:
        return WGPUBlendOperation_Add;
    case BM_SUBTRACT:
        return WGPUBlendOperation_Subtract;
    case BM_REVERSE_SUBTRACT:
        return WGPUBlendOperation_ReverseSubtract;
    case BM_MIN:
        return WGPUBlendOperation_Min;
    case BM_MAX:
        return WGPUBlendOperation_Max;
    default:
        return WGPUBlendOperation_Force32;
    }
}

static inline FORGE_CONSTEXPR WGPUBlendFactor ToBlendFactor(BlendConstant constant)
{
    switch (constant)
    {
    case BC_ZERO:
        return WGPUBlendFactor_Zero;
    case BC_ONE:
        return WGPUBlendFactor_One;
    case BC_SRC_COLOR:
        return WGPUBlendFactor_Src;
    case BC_ONE_MINUS_SRC_COLOR:
        return WGPUBlendFactor_OneMinusSrc;
    case BC_DST_COLOR:
        return WGPUBlendFactor_Dst;
    case BC_ONE_MINUS_DST_COLOR:
        return WGPUBlendFactor_OneMinusDst;
    case BC_SRC_ALPHA:
        return WGPUBlendFactor_SrcAlpha;
    case BC_ONE_MINUS_SRC_ALPHA:
        return WGPUBlendFactor_OneMinusSrcAlpha;
    case BC_DST_ALPHA:
        return WGPUBlendFactor_DstAlpha;
    case BC_ONE_MINUS_DST_ALPHA:
        return WGPUBlendFactor_OneMinusDstAlpha;
    case BC_SRC_ALPHA_SATURATE:
        return WGPUBlendFactor_SrcAlphaSaturated;
    case BC_BLEND_FACTOR:
        return WGPUBlendFactor_Constant;
    case BC_ONE_MINUS_BLEND_FACTOR:
        return WGPUBlendFactor_OneMinusConstant;
    default:
        return WGPUBlendFactor_Force32;
    }
}

static inline FORGE_CONSTEXPR WGPUCullMode ToCullMode(CullMode mode)
{
    switch (mode)
    {
    case CULL_MODE_NONE:
        return WGPUCullMode_None;
    case CULL_MODE_BACK:
        return WGPUCullMode_Back;
    case CULL_MODE_FRONT:
        return WGPUCullMode_Front;
    default:
        return WGPUCullMode_Force32;
    }
}

static inline FORGE_CONSTEXPR WGPUFrontFace ToFrontFace(FrontFace face)
{
    switch (face)
    {
    case FRONT_FACE_CCW:
        return WGPUFrontFace_CCW;
    case FRONT_FACE_CW:
        return WGPUFrontFace_CW;
    default:
        return WGPUFrontFace_Force32;
    }
}

static inline FORGE_CONSTEXPR WGPUPrimitiveTopology ToPrimitiveTopo(PrimitiveTopology topo)
{
    switch (topo)
    {
    case PRIMITIVE_TOPO_POINT_LIST:
        return WGPUPrimitiveTopology_PointList;
    case PRIMITIVE_TOPO_LINE_LIST:
        return WGPUPrimitiveTopology_LineList;
    case PRIMITIVE_TOPO_LINE_STRIP:
        return WGPUPrimitiveTopology_LineStrip;
    case PRIMITIVE_TOPO_TRI_LIST:
        return WGPUPrimitiveTopology_TriangleList;
    case PRIMITIVE_TOPO_TRI_STRIP:
        return WGPUPrimitiveTopology_TriangleStrip;
    default:
        return WGPUPrimitiveTopology_Force32;
    }
}

static inline FORGE_CONSTEXPR WGPUIndexFormat ToIndexType(uint32_t index)
{
    switch (index)
    {
    case INDEX_TYPE_UINT16:
        return WGPUIndexFormat_Uint16;
    case INDEX_TYPE_UINT32:
        return WGPUIndexFormat_Uint32;
    default:
        return WGPUIndexFormat_Undefined;
    }
}

static inline FORGE_CONSTEXPR WGPUVertexStepMode ToStepMode(VertexBindingRate rate)
{
    switch (rate)
    {
    case VERTEX_BINDING_RATE_VERTEX:
        return WGPUVertexStepMode_Vertex;
    case VERTEX_BINDING_RATE_INSTANCE:
        return WGPUVertexStepMode_Instance;
    default:
        return WGPUVertexStepMode_Force32;
    }
}

static inline FORGE_CONSTEXPR WGPUVertexFormat ToVertexFormat(TinyImageFormat format)
{
    switch (format)
    {
    case TinyImageFormat_R8G8_UINT:
        return WGPUVertexFormat_Uint8x2;
    case TinyImageFormat_R8G8B8A8_UINT:
        return WGPUVertexFormat_Uint8x4;

    case TinyImageFormat_R8G8_SINT:
        return WGPUVertexFormat_Sint8x2;
    case TinyImageFormat_R8G8B8A8_SINT:
        return WGPUVertexFormat_Sint8x4;

    case TinyImageFormat_R8G8_UNORM:
        return WGPUVertexFormat_Unorm8x2;
    case TinyImageFormat_R8G8B8A8_UNORM:
        return WGPUVertexFormat_Unorm8x4;

    case TinyImageFormat_R8G8_SNORM:
        return WGPUVertexFormat_Snorm8x2;
    case TinyImageFormat_R8G8B8A8_SNORM:
        return WGPUVertexFormat_Snorm8x4;

    case TinyImageFormat_R16G16_UNORM:
        return WGPUVertexFormat_Unorm16x2;
    case TinyImageFormat_R16G16B16A16_UNORM:
        return WGPUVertexFormat_Unorm16x4;

    case TinyImageFormat_R16G16_SNORM:
        return WGPUVertexFormat_Snorm16x2;
    case TinyImageFormat_R16G16B16A16_SNORM:
        return WGPUVertexFormat_Snorm16x4;

    case TinyImageFormat_R16G16_SINT:
        return WGPUVertexFormat_Sint16x2;
    case TinyImageFormat_R16G16B16A16_SINT:
        return WGPUVertexFormat_Sint16x4;

    case TinyImageFormat_R16G16_UINT:
        return WGPUVertexFormat_Uint16x2;
    case TinyImageFormat_R16G16B16A16_UINT:
        return WGPUVertexFormat_Uint16x4;

    case TinyImageFormat_R16G16_SFLOAT:
        return WGPUVertexFormat_Float16x2;
    case TinyImageFormat_R16G16B16A16_SFLOAT:
        return WGPUVertexFormat_Float16x4;

    case TinyImageFormat_R32_SFLOAT:
        return WGPUVertexFormat_Float32;
    case TinyImageFormat_R32G32_SFLOAT:
        return WGPUVertexFormat_Float32x2;
    case TinyImageFormat_R32G32B32_SFLOAT:
        return WGPUVertexFormat_Float32x3;
    case TinyImageFormat_R32G32B32A32_SFLOAT:
        return WGPUVertexFormat_Float32x4;

    case TinyImageFormat_R32_SINT:
        return WGPUVertexFormat_Sint32;
    case TinyImageFormat_R32G32_SINT:
        return WGPUVertexFormat_Sint32x2;
    case TinyImageFormat_R32G32B32_SINT:
        return WGPUVertexFormat_Sint32x3;
    case TinyImageFormat_R32G32B32A32_SINT:
        return WGPUVertexFormat_Sint32x4;

    case TinyImageFormat_R32_UINT:
        return WGPUVertexFormat_Uint32;
    case TinyImageFormat_R32G32_UINT:
        return WGPUVertexFormat_Uint32x2;
    case TinyImageFormat_R32G32B32_UINT:
        return WGPUVertexFormat_Uint32x3;
    case TinyImageFormat_R32G32B32A32_UINT:
        return WGPUVertexFormat_Uint32x4;
    default:
        return WGPUVertexFormat_Force32;
    }
}

static inline FORGE_CONSTEXPR WGPUFilterMode ToFilterMode(FilterType type)
{
    switch (type)
    {
    case FILTER_NEAREST:
        return WGPUFilterMode_Nearest;
    case FILTER_LINEAR:
        return WGPUFilterMode_Linear;
    default:
        return WGPUFilterMode_Force32;
    }
}

static inline FORGE_CONSTEXPR WGPUMipmapFilterMode ToMipmapMode(MipMapMode type)
{
    switch (type)
    {
    case MIPMAP_MODE_NEAREST:
        return WGPUMipmapFilterMode_Nearest;
    case MIPMAP_MODE_LINEAR:
        return WGPUMipmapFilterMode_Linear;
    default:
        return WGPUMipmapFilterMode_Force32;
    }
}
/************************************************************************/
// Renderer Context Init Exit (multi GPU)
/************************************************************************/
static uint32_t gRendererCount = 0;

static inline FORGE_CONSTEXPR const char* ToBackTypeName(WGPUBackendType type)
{
    switch (type)
    {
    case WGPUBackendType_Null:
        return "WGPU NULL";
    case WGPUBackendType_WebGPU:
        return "WGPU WEBGPU";
    case WGPUBackendType_D3D11:
        return "WGPU DX11";
    case WGPUBackendType_D3D12:
        return "WGPU DX12";
    case WGPUBackendType_Metal:
        return "WGPU MTL";
    case WGPUBackendType_Vulkan:
        return "WGPU VK";
    case WGPUBackendType_OpenGL:
        return "WGPU GL";
    case WGPUBackendType_OpenGLES:
        return "WGPU GLES";
    default:
        return "WGPU UNDEFINED";
    }
}

#define WGPU_FORMAT_VERSION(version, outVersionString)                                                                             \
    ASSERT(VK_MAX_DESCRIPTION_SIZE == TF_ARRAY_COUNT(outVersionString));                                                           \
    snprintf(outVersionString, TF_ARRAY_COUNT(outVersionString), "%u.%u.%u", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), \
             VK_VERSION_PATCH(version));

void wgpu_initRendererContext(const char* appName, const RendererContextDesc* pDesc, RendererContext** ppContext)
{
    ASSERT(appName);
    ASSERT(pDesc);
    ASSERT(ppContext);
    ASSERT(gRendererCount == 0);

    RendererContext* pContext = (RendererContext*)tf_calloc_memalign(1, alignof(RendererContext), sizeof(RendererContext));

    for (uint32_t i = 0; i < TF_ARRAY_COUNT(pContext->mGpus); ++i)
    {
        setDefaultGPUSettings(&pContext->mGpus[i].mSettings);
    }

    DECL_CHAINED_STRUCT(WGPUInstanceDescriptor, desc);
#if defined(WEBGPU_DAWN)
    desc.features.timedWaitAnyEnable = true;
    desc.features.timedWaitAnyMaxCount = 1;
#endif
    pContext->mWgp.pInstance = wgpuCreateInstance(&desc);
    ASSERT(pContext->mWgp.pInstance);

    WGPUBackendType backends[] = {
        WGPUBackendType_Vulkan,
#if defined(_WINDOWS)
        WGPUBackendType_D3D11,
        WGPUBackendType_D3D12,
#endif
#if defined(ANDROID)
    // OpenGLES is not yet implemented in Dawn.
    // WGPUBackendType_OpenGLES,
#endif
    };
    for (uint32_t i = 0; i < TF_ARRAY_COUNT(backends); ++i)
    {
        WGPUAdapter               adapter = NULL;
        WGPURequestAdapterOptions adapterOptions = {};
        adapterOptions.backendType = backends[i];

#if defined(WEBGPU_DAWN)
        // OpenGLES is only available in Compatability mode..
        if (backends[i] == WGPUBackendType_OpenGLES)
        {
            adapterOptions.compatibilityMode = true;
        }
        adapterOptions.compatibilityMode = true;
#endif

        wgpuInstanceRequestAdapter(
            pContext->mWgp.pInstance, &adapterOptions,
            [](WGPURequestAdapterStatus, WGPUAdapter adapter, char const*, void* userdata) { *(WGPUAdapter*)userdata = adapter; },
            &adapter);
        if (!adapter)
        {
            continue;
        }

        GpuInfo* gpu = &pContext->mGpus[pContext->mGpuCount++];
        gpu->mWgp.pAdapter = adapter;
        gpu->mWgp.mCompatMode = adapterOptions.compatibilityMode;

        // Features
#if defined(WEBGPU_DAWN)
        bool featMemHeaps = false;
        bool featAdapterPropsVk = false;
#endif

        size_t           featureCount = wgpuAdapterEnumerateFeatures(gpu->mWgp.pAdapter, NULL);
        WGPUFeatureName* features = featureCount ? (WGPUFeatureName*)tf_malloc(featureCount * sizeof(WGPUFeatureName)) : NULL;
        wgpuAdapterEnumerateFeatures(gpu->mWgp.pAdapter, features);
        for (uint32_t featureIdx = 0; featureIdx < featureCount; ++featureIdx)
        {
            WGPUFeatureName feature = features[featureIdx];
            if (WGPUFeatureName_TimestampQuery == feature)
            {
                gpu->mSettings.mTimestampQueries = true;
            }
#if defined(WEBGPU_NATIVE)
            if (WGPUNativeFeature_MultiDrawIndirect == feature)
            {
                gpu->mSettings.mMultiDrawIndirect = true;
            }
            if (WGPUNativeFeature_MultiDrawIndirectCount == feature)
            {
                gpu->mSettings.mMultiDrawIndirectCount = true;
            }
            // #TODO
            // if (WGPUNativeFeature_PipelineStatisticsQuery == feature)
            //{
            //    gpu->mSettings.mPipelineStatsQueries = true;
            //}
#elif defined(WEBGPU_DAWN)
            if (WGPUFeatureName_AdapterPropertiesMemoryHeaps == feature)
            {
                featMemHeaps = true;
            }
            if (WGPUFeatureName_AdapterPropertiesVk == feature)
            {
                featAdapterPropsVk = true;
            }
            if (WGPUFeatureName_StaticSamplers == feature)
            {
                gpu->mWgp.mStaticSamplers = true;
            }
#endif
        }
        SAFE_FREE(features);

        // Properties
        DECL_CHAINED_STRUCT_OUT(WGPUAdapterProperties, props);
#if defined(WEBGPU_DAWN)
        WGPUAdapterPropertiesMemoryHeaps heapProps = {};
        heapProps.chain.sType = WGPUSType_AdapterPropertiesMemoryHeaps;
        ADD_TO_NEXT_CHAIN_OUT_COND(featMemHeaps, props, &heapProps);

        WGPUAdapterPropertiesVk vkProps = {};
        vkProps.chain.sType = WGPUSType_AdapterPropertiesVk;
        ADD_TO_NEXT_CHAIN_OUT_COND(featAdapterPropsVk, props, &vkProps);
#endif
        wgpuAdapterGetProperties(gpu->mWgp.pAdapter, &props);

        // Limits
        DECL_CHAINED_STRUCT_OUT(WGPUSupportedLimits, supported);
#if defined(WEBGPU_NATIVE)
        WGPUSupportedLimitsExtras supportedExt = {};
        supportedExt.chain.sType = (WGPUSType)WGPUSType_SupportedLimitsExtras;
        ADD_TO_NEXT_CHAIN_OUT(supported, &supportedExt);
#elif defined(WEBGPU_DAWN)
        WGPUDawnExperimentalSubgroupLimits subgroupExt = {};
        subgroupExt.chain.sType = WGPUSType_DawnExperimentalSubgroupLimits;
        ADD_TO_NEXT_CHAIN_OUT(supported, &subgroupExt);
#endif
        WGPULimits& limits = supported.limits;
        WGPUBool    ret = wgpuAdapterGetLimits(gpu->mWgp.pAdapter, &supported);
        ASSERT(ret);
        gpu->mWgp.mLimits = limits;

        static const uint32_t COPY_BYTES_PER_ROW_ALIGNMENT = 256;
#if defined(WEBGPU_NATIVE)
        static const uint32_t COPY_BUFFER_ALIGNMENT = 256;
#else
        static const uint32_t COPY_BUFFER_ALIGNMENT = 4;
#endif

        GPUSettings* settings = &gpu->mSettings;
        snprintf(settings->mGpuVendorPreset.mGpuName, MAX_GPU_VENDOR_STRING_LENGTH, "%s | %s", props.name,
                 ToBackTypeName(props.backendType));
        settings->mGpuVendorPreset.mModelId = props.deviceID;
        settings->mGpuVendorPreset.mRevisionId = 0;
        settings->mGpuVendorPreset.mVendorId = props.vendorID;
        strncpy(settings->mGpuVendorPreset.mVendorName, props.vendorName, MAX_GPU_VENDOR_STRING_LENGTH);

        // Driver version
#if defined(WEBGPU_DAWN)
        snprintf(settings->mGpuVendorPreset.mGpuDriverVersion, MAX_GPU_VENDOR_STRING_LENGTH, "%u.%u", 999999, 99);
        if (featAdapterPropsVk)
        {
            if (gpuVendorEquals(settings->mGpuVendorPreset.mVendorId, "nvidia"))
            {
                uint32_t major, minor, secondaryBranch, tertiaryBranch;
                major = (vkProps.driverVersion >> 22) & 0x3ff;
                minor = (vkProps.driverVersion >> 14) & 0x0ff;
                secondaryBranch = (vkProps.driverVersion >> 6) & 0x0ff;
                tertiaryBranch = (vkProps.driverVersion) & 0x003f;
                snprintf(settings->mGpuVendorPreset.mGpuDriverVersion, MAX_GPU_VENDOR_STRING_LENGTH, "%u.%u.%u.%u", major, minor,
                         secondaryBranch, tertiaryBranch);
            }
            else if (gpuVendorEquals(settings->mGpuVendorPreset.mVendorId, "intel"))
            {
                uint32_t major = vkProps.driverVersion >> 14;
                uint32_t minor = vkProps.driverVersion & 0x3fff;
                snprintf(settings->mGpuVendorPreset.mGpuDriverVersion, MAX_GPU_VENDOR_STRING_LENGTH, "%u.%u", major, minor);
            }
            else
            {
                WGPU_FORMAT_VERSION(vkProps.driverVersion, settings->mGpuVendorPreset.mGpuDriverVersion);
            }
        }
        else // D3D
        {
            // We do not get a driver version but we can look for it in the driver description..
            // reverse search for a space since there will be one to separate driver version from other text..
            char* firstNumChar = strrchr((char*)props.driverDescription, ' ');
            if (firstNumChar)
                snprintf(settings->mGpuVendorPreset.mGpuDriverVersion, MAX_GPU_VENDOR_STRING_LENGTH, "%s", firstNumChar);
        }
#endif
        settings->mGpuVendorPreset.mPresetLevel = getGPUPresetLevel(
            settings->mGpuVendorPreset.mVendorId, settings->mGpuVendorPreset.mModelId, settings->mGpuVendorPreset.mVendorName, props.name);

        // #NOTE: Set model id to backend type after we are done with preset selection
        // Since all the backends have same modelId, setting modelId to backend type lets us use the GPU selection UI correctly to switch
        // between different backends
        settings->mGpuVendorPreset.mModelId = backends[i];

        settings->mAllowBufferTextureInSameHeap = false;
        settings->mBuiltinDrawID = false;
        settings->mDynamicRenderingSupported = false;
        settings->mGeometryShaderSupported = false;
#if defined(WEBGPU_DAWN)
        settings->mGpuMarkers = true;
#else
        settings->mGpuMarkers = false;
#endif
        settings->mGraphicsQueueSupported = true;
        settings->mHDRSupported = false;
        settings->mIndirectCommandBuffer = false;
        settings->mIndirectRootConstant = false;
        settings->mMaxBoundTextures = limits.maxSampledTexturesPerShaderStage;
        settings->mMaxComputeThreads[0] = limits.maxComputeWorkgroupSizeX;
        settings->mMaxComputeThreads[1] = limits.maxComputeWorkgroupSizeY;
        settings->mMaxComputeThreads[2] = limits.maxComputeWorkgroupSizeZ;
        settings->mMaxTotalComputeThreads = limits.maxComputeInvocationsPerWorkgroup;
        settings->mMaxVertexInputBindings = limits.maxVertexBuffers;
        // #TODO
        settings->mOcclusionQueries = false;
        settings->mPrimitiveIdSupported = true;
        settings->mRayPipelineSupported = false;
        settings->mRayQuerySupported = false;
        settings->mRaytracingSupported = false;
        settings->mROVsSupported = false;
        settings->mSamplerAnisotropySupported = true;
        settings->mSoftwareVRSSupported = false;
        settings->mTessellationSupported = false;
        settings->mUniformBufferAlignment = limits.minUniformBufferOffsetAlignment;
        settings->mUploadBufferAlignment = COPY_BUFFER_ALIGNMENT;
        settings->mUploadBufferTextureAlignment = COPY_BYTES_PER_ROW_ALIGNMENT;
        settings->mUploadBufferTextureRowAlignment = COPY_BYTES_PER_ROW_ALIGNMENT;
        settings->mVRAM = 4ULL * TF_GB;
        settings->mWaveLaneCount = 32;
#if defined(WEBGPU_DAWN)
        uint64_t vram = 0;
        for (uint32_t h = 0; h < heapProps.heapCount; ++h)
        {
            if (heapProps.heapInfo[h].properties & WGPUHeapProperty_DeviceLocal)
            {
                vram += heapProps.heapInfo[h].size;
            }
        }
        wgpuAdapterPropertiesMemoryHeapsFreeMembers(heapProps);
        settings->mVRAM = vram ? vram : settings->mVRAM;
        settings->mWaveLaneCount = subgroupExt.minSubgroupSize ? subgroupExt.minSubgroupSize : settings->mWaveLaneCount;
#endif
        settings->mWaveOpsSupportFlags = WAVE_OPS_SUPPORT_FLAG_NONE;
        settings->mWaveOpsSupportedStageFlags = SHADER_STAGE_NONE;

#if defined(WEBGPU_DAWN)
        wgpuAdapterPropertiesFreeMembers(props);
#endif
    }

    ASSERT(pContext->mGpuCount);

    *ppContext = pContext;
}

void wgpu_exitRendererContext(RendererContext* pContext)
{
    ASSERT(gRendererCount == 0);
    ASSERT(pContext);
    ASSERT(pContext->mWgp.pInstance);

    for (uint32_t i = 0; i < pContext->mGpuCount; ++i)
    {
        wgpuAdapterRelease(pContext->mGpus[i].mWgp.pAdapter);
    }

    wgpuInstanceRelease(pContext->mWgp.pInstance);
    pContext->mWgp.pInstance = NULL;

    SAFE_FREE(pContext);
}

/************************************************************************/
// Renderer Init Remove
/************************************************************************/
typedef struct NullDescriptors
{
    Texture* pDefaultTextureSRV[TEXTURE_DIM_COUNT];
    Texture* pDefaultTextureUAV[TEXTURE_DIM_COUNT];
    Buffer*  pDefaultBufferSRV;
    Buffer*  pDefaultBufferUAV;
    Sampler* pDefaultSampler;

    WGPUDepthStencilState mDefaultDs;
    WGPUBlendState        mDefaultBs;
    WGPUPrimitiveState    mDefaultPs;
} NullDescriptors;

static inline WGPUDepthStencilState ToDepthStencilState(const DepthStateDesc* pDesc, const RasterizerStateDesc* pRast)
{
    ASSERT(pDesc->mDepthFunc < CompareMode::MAX_COMPARE_MODES);
    ASSERT(pDesc->mStencilFrontFunc < CompareMode::MAX_COMPARE_MODES);
    ASSERT(pDesc->mStencilFrontFail < StencilOp::MAX_STENCIL_OPS);
    ASSERT(pDesc->mDepthFrontFail < StencilOp::MAX_STENCIL_OPS);
    ASSERT(pDesc->mStencilFrontPass < StencilOp::MAX_STENCIL_OPS);
    ASSERT(pDesc->mStencilBackFunc < CompareMode::MAX_COMPARE_MODES);
    ASSERT(pDesc->mStencilBackFail < StencilOp::MAX_STENCIL_OPS);
    ASSERT(pDesc->mDepthBackFail < StencilOp::MAX_STENCIL_OPS);
    ASSERT(pDesc->mStencilBackPass < StencilOp::MAX_STENCIL_OPS);

    WGPUDepthStencilState ds = {};
    ds = {};
    ds.depthBias = pRast->mDepthBias;
    ds.depthBiasClamp = 0.0f;
    ds.depthBiasSlopeScale = pRast->mSlopeScaledDepthBias;
    ds.depthCompare = ToCompareFunction(pDesc->mDepthFunc);
    ds.depthWriteEnabled = pDesc->mDepthWrite;

    ds.stencilBack.compare = ToCompareFunction(pDesc->mStencilBackFunc);
    ds.stencilBack.depthFailOp = ToStencilOp(pDesc->mStencilBackFail);
    ds.stencilBack.failOp = ToStencilOp(pDesc->mDepthBackFail);
    ds.stencilBack.passOp = ToStencilOp(pDesc->mStencilBackPass);

    ds.stencilFront.compare = ToCompareFunction(pDesc->mStencilFrontFunc);
    ds.stencilFront.depthFailOp = ToStencilOp(pDesc->mStencilFrontFail);
    ds.stencilFront.failOp = ToStencilOp(pDesc->mDepthFrontFail);
    ds.stencilFront.passOp = ToStencilOp(pDesc->mStencilFrontPass);

    ds.stencilReadMask = pDesc->mStencilReadMask;
    ds.stencilWriteMask = pDesc->mStencilWriteMask;

    return ds;
}

static inline WGPUBlendState ToBlendState(const BlendStateDesc* pDesc, uint32_t index)
{
    WGPUBlendState bs = {};
    bs.alpha.dstFactor = ToBlendFactor(pDesc->mDstAlphaFactors[index]);
    bs.alpha.operation = ToBlendOp(pDesc->mBlendAlphaModes[index]);
    bs.alpha.srcFactor = ToBlendFactor(pDesc->mSrcAlphaFactors[index]);

    bs.color.dstFactor = ToBlendFactor(pDesc->mDstFactors[index]);
    bs.color.operation = ToBlendOp(pDesc->mBlendModes[index]);
    bs.color.srcFactor = ToBlendFactor(pDesc->mSrcFactors[index]);

    return bs;
}

static inline WGPUPrimitiveState ToPrimitiveState(const RasterizerStateDesc* pDesc)
{
    WGPUPrimitiveState ps = {};
    ps.cullMode = ToCullMode(pDesc->mCullMode);
    ps.frontFace = ToFrontFace(pDesc->mFrontFace);
    // #TODO
    // ps.stripIndexFormat = WGPUIndexFormat_Undefined;

    return ps;
}

static void AddDefaultResources(Renderer* pRenderer)
{
    // 1D texture
    TextureDesc textureDesc = {};
    textureDesc.mArraySize = 1;
    textureDesc.mDepth = 1;
    textureDesc.mFormat = TinyImageFormat_R8G8B8A8_UNORM;
    textureDesc.mHeight = 1;
    textureDesc.mMipLevels = 1;
    textureDesc.mSampleCount = SAMPLE_COUNT_1;
    textureDesc.mStartState = RESOURCE_STATE_COMMON;
    textureDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
    textureDesc.mWidth = 1;
    textureDesc.pName = "DefaultTextureSRV_1D";
    addTexture(pRenderer, &textureDesc, &pRenderer->pNullDescriptors->pDefaultTextureSRV[TEXTURE_DIM_1D]);
    textureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE;
    textureDesc.pName = "DefaultTextureUAV_1D";
    addTexture(pRenderer, &textureDesc, &pRenderer->pNullDescriptors->pDefaultTextureUAV[TEXTURE_DIM_1D]);

    // 2D texture
    textureDesc.mWidth = 2;
    textureDesc.mHeight = 2;
    textureDesc.mArraySize = 1;
    textureDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
    textureDesc.pName = "DefaultTextureSRV_2D";
    addTexture(pRenderer, &textureDesc, &pRenderer->pNullDescriptors->pDefaultTextureSRV[TEXTURE_DIM_2D]);
    textureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE;
    textureDesc.pName = "DefaultTextureUAV_2D";
    addTexture(pRenderer, &textureDesc, &pRenderer->pNullDescriptors->pDefaultTextureUAV[TEXTURE_DIM_2D]);

    // 2D MS texture
    textureDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
    textureDesc.mStartState = RESOURCE_STATE_RENDER_TARGET;
    textureDesc.mSampleCount = SAMPLE_COUNT_4;
    textureDesc.pName = "DefaultTextureSRV_2DMS";
    addTexture(pRenderer, &textureDesc, &pRenderer->pNullDescriptors->pDefaultTextureSRV[TEXTURE_DIM_2DMS]);
    textureDesc.mStartState = RESOURCE_STATE_COMMON;
    textureDesc.mSampleCount = SAMPLE_COUNT_1;

    // 2D texture array
    textureDesc.mArraySize = 2;
    textureDesc.pName = "DefaultTextureSRV_2D_ARRAY";
    addTexture(pRenderer, &textureDesc, &pRenderer->pNullDescriptors->pDefaultTextureSRV[TEXTURE_DIM_2D_ARRAY]);
    textureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE;
    textureDesc.pName = "DefaultTextureUAV_2D_ARRAY";
    addTexture(pRenderer, &textureDesc, &pRenderer->pNullDescriptors->pDefaultTextureUAV[TEXTURE_DIM_2D_ARRAY]);

    // 3D texture
    textureDesc.mDepth = 2;
    textureDesc.mArraySize = 1;
    textureDesc.pName = "DefaultTextureSRV_3D";
    addTexture(pRenderer, &textureDesc, &pRenderer->pNullDescriptors->pDefaultTextureSRV[TEXTURE_DIM_3D]);
    textureDesc.mDescriptors = DESCRIPTOR_TYPE_RW_TEXTURE;
    textureDesc.pName = "DefaultTextureUAV_3D";
    addTexture(pRenderer, &textureDesc, &pRenderer->pNullDescriptors->pDefaultTextureUAV[TEXTURE_DIM_3D]);

    // Cube texture
    textureDesc.mDepth = 1;
    textureDesc.mArraySize = 6;
    textureDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE_CUBE;
    textureDesc.pName = "DefaultTextureSRV_CUBE";
    addTexture(pRenderer, &textureDesc, &pRenderer->pNullDescriptors->pDefaultTextureSRV[TEXTURE_DIM_CUBE]);
    textureDesc.mArraySize = 6 * 2;
    textureDesc.pName = "DefaultTextureSRV_CUBE_ARRAY";
    if (!pRenderer->pGpu->mWgp.mCompatMode)
    {
        addTexture(pRenderer, &textureDesc, &pRenderer->pNullDescriptors->pDefaultTextureSRV[TEXTURE_DIM_CUBE_ARRAY]);
    }

    BufferDesc bufferDesc = {};
    bufferDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bufferDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    bufferDesc.mStartState = RESOURCE_STATE_COMMON;
    bufferDesc.mSize = sizeof(uint32_t);
    bufferDesc.mFirstElement = 0;
    bufferDesc.mElementCount = 1;
    bufferDesc.mStructStride = sizeof(uint32_t);
    bufferDesc.mFormat = TinyImageFormat_R32_UINT;
    bufferDesc.pName = "DefaultBufferSRV";
    addBuffer(pRenderer, &bufferDesc, &pRenderer->pNullDescriptors->pDefaultBufferSRV);
    bufferDesc.mDescriptors = DESCRIPTOR_TYPE_RW_BUFFER;
    bufferDesc.pName = "DefaultBufferUAV";
    addBuffer(pRenderer, &bufferDesc, &pRenderer->pNullDescriptors->pDefaultBufferUAV);

    SamplerDesc samplerDesc = {};
    samplerDesc.mAddressU = ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerDesc.mAddressV = ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerDesc.mAddressW = ADDRESS_MODE_CLAMP_TO_BORDER;
    addSampler(pRenderer, &samplerDesc, &pRenderer->pNullDescriptors->pDefaultSampler);

    BlendStateDesc blendStateDesc = {};
    blendStateDesc.mDstAlphaFactors[0] = BC_ZERO;
    blendStateDesc.mDstFactors[0] = BC_ZERO;
    blendStateDesc.mSrcAlphaFactors[0] = BC_ONE;
    blendStateDesc.mSrcFactors[0] = BC_ONE;
    blendStateDesc.mColorWriteMasks[0] = COLOR_MASK_ALL;
    blendStateDesc.mRenderTargetMask = BLEND_STATE_TARGET_ALL;
    blendStateDesc.mIndependentBlend = false;
    pRenderer->pNullDescriptors->mDefaultBs = ToBlendState(&blendStateDesc, 0);

    DepthStateDesc depthStateDesc = {};
    depthStateDesc.mDepthFunc = CMP_ALWAYS;
    depthStateDesc.mDepthTest = false;
    depthStateDesc.mDepthWrite = false;
    depthStateDesc.mStencilBackFunc = CMP_ALWAYS;
    depthStateDesc.mStencilFrontFunc = CMP_ALWAYS;
    depthStateDesc.mStencilReadMask = 0xFF;
    depthStateDesc.mStencilWriteMask = 0xFF;

    RasterizerStateDesc rasterizerStateDesc = {};
    rasterizerStateDesc.mCullMode = CULL_MODE_BACK;
    pRenderer->pNullDescriptors->mDefaultPs = ToPrimitiveState(&rasterizerStateDesc);

    pRenderer->pNullDescriptors->mDefaultDs = ToDepthStencilState(&depthStateDesc, &rasterizerStateDesc);

    WGPUBindGroupLayoutDescriptor layoutDesc = {};
    pRenderer->mWgp.pEmptyDescriptorSetLayout = wgpuDeviceCreateBindGroupLayout(pRenderer->mWgp.pDevice, &layoutDesc);

    WGPUBindGroupDescriptor emptyDesc = {};
    emptyDesc.layout = pRenderer->mWgp.pEmptyDescriptorSetLayout;
    pRenderer->mWgp.pEmptyDescriptorSet = wgpuDeviceCreateBindGroup(pRenderer->mWgp.pDevice, &emptyDesc);
}

static void RemoveDefaultResources(Renderer* pRenderer)
{
    for (uint32_t dim = 0; dim < TEXTURE_DIM_COUNT; ++dim)
    {
        if (pRenderer->pNullDescriptors->pDefaultTextureSRV[dim])
        {
            removeTexture(pRenderer, pRenderer->pNullDescriptors->pDefaultTextureSRV[dim]);
        }
        if (pRenderer->pNullDescriptors->pDefaultTextureUAV[dim])
        {
            removeTexture(pRenderer, pRenderer->pNullDescriptors->pDefaultTextureUAV[dim]);
        }
    }

    removeBuffer(pRenderer, pRenderer->pNullDescriptors->pDefaultBufferSRV);
    removeBuffer(pRenderer, pRenderer->pNullDescriptors->pDefaultBufferUAV);

    removeSampler(pRenderer, pRenderer->pNullDescriptors->pDefaultSampler);

    wgpuBindGroupRelease(pRenderer->mWgp.pEmptyDescriptorSet);
    wgpuBindGroupLayoutRelease(pRenderer->mWgp.pEmptyDescriptorSetLayout);
}

static void ValidationCallback(WGPUErrorType type, char const* pMessage, void* /* pUserData */)
{
    LOGF(LogLevel::eERROR, "[%s] : %s (%i)", "wgpu", pMessage, type);
    ASSERTFAIL("[%s] : %s (%i)", "wgpu", pMessage, type);
};

void wgpu_initRenderer(const char* appName, const RendererDesc* pDesc, Renderer** ppRenderer)
{
    ASSERT(appName);
    ASSERT(pDesc);
    ASSERT(ppRenderer);

    uint8_t* mem = (uint8_t*)tf_calloc_memalign(1, alignof(Renderer), sizeof(Renderer) + sizeof(NullDescriptors));
    ASSERT(mem);

    Renderer* pRenderer = (Renderer*)mem;
    pRenderer->mRendererApi = RENDERER_API_WEBGPU;
    pRenderer->mGpuMode = pDesc->mGpuMode;
    pRenderer->mShaderTarget = pDesc->mShaderTarget;
    pRenderer->pNullDescriptors = (NullDescriptors*)(mem + sizeof(Renderer));
    pRenderer->pName = appName;
    pRenderer->mLinkedNodeCount = 1;

    ASSERT(pDesc->mGpuMode != GPU_MODE_UNLINKED || pDesc->pContext); // context required in unlinked mode
    if (pDesc->pContext)
    {
        ASSERT(pDesc->mGpuIndex < pDesc->pContext->mGpuCount);
        pRenderer->mOwnsContext = false;
        pRenderer->pContext = pDesc->pContext;
        pRenderer->mUnlinkedRendererIndex = gRendererCount;
    }
    else
    {
        RendererContextDesc contextDesc = {};
        contextDesc.mEnableGpuBasedValidation = pDesc->mEnableGpuBasedValidation;
        wgpu_initRendererContext(appName, &contextDesc, &pRenderer->pContext);
        pRenderer->mOwnsContext = true;
        if (!pRenderer->pContext)
        {
            SAFE_FREE(pRenderer);
            return;
        }
    }

    GPUSettings gpuSettings[MAX_MULTIPLE_GPUS] = {};
    for (uint32_t i = 0; i < pRenderer->pContext->mGpuCount; ++i)
    {
        gpuSettings[i] = pRenderer->pContext->mGpus[i].mSettings;
    }
    uint32_t gpuIndex = util_select_best_gpu(gpuSettings, pRenderer->pContext->mGpuCount);
    pRenderer->pGpu = &pRenderer->pContext->mGpus[gpuIndex];

    WGPUFeatureName wantedFeatures[] = {
        WGPUFeatureName_DepthClipControl,
        WGPUFeatureName_Depth32FloatStencil8,
        WGPUFeatureName_TimestampQuery,
        WGPUFeatureName_TextureCompressionBC,
        WGPUFeatureName_TextureCompressionETC2,
        WGPUFeatureName_TextureCompressionASTC,
        WGPUFeatureName_IndirectFirstInstance,
        WGPUFeatureName_ShaderF16,
        WGPUFeatureName_RG11B10UfloatRenderable,
        WGPUFeatureName_BGRA8UnormStorage,
        WGPUFeatureName_Float32Filterable,
#if defined(WEBGPU_NATIVE)
        (WGPUFeatureName)WGPUNativeFeature_PushConstants,
        (WGPUFeatureName)WGPUNativeFeature_MultiDrawIndirect,
        (WGPUFeatureName)WGPUNativeFeature_MultiDrawIndirectCount,
        (WGPUFeatureName)WGPUNativeFeature_VertexWritableStorage,
        (WGPUFeatureName)WGPUNativeFeature_TextureBindingArray,
        (WGPUFeatureName)WGPUNativeFeature_SampledTextureAndStorageBufferArrayNonUniformIndexing,
        (WGPUFeatureName)WGPUNativeFeature_PipelineStatisticsQuery,
        (WGPUFeatureName)WGPUNativeFeature_StorageResourceBindingArray,
#endif
#if defined(WEBGPU_DAWN)
        WGPUFeatureName_StaticSamplers,
        WGPUFeatureName_Unorm16TextureFormats,
        WGPUFeatureName_Snorm16TextureFormats,
#endif
    };
    WGPUFeatureName finalFeatures[TF_ARRAY_COUNT(wantedFeatures)] = {};
    uint32_t        finalFeatureCount = 0;

    size_t           featureCount = wgpuAdapterEnumerateFeatures(pRenderer->pGpu->mWgp.pAdapter, NULL);
    WGPUFeatureName* features = featureCount ? (WGPUFeatureName*)tf_malloc(featureCount * sizeof(WGPUFeatureName)) : NULL;
    wgpuAdapterEnumerateFeatures(pRenderer->pGpu->mWgp.pAdapter, features);
    for (uint32_t feature = 0; feature < featureCount; ++feature)
    {
        for (uint32_t wanted = 0; wanted < TF_ARRAY_COUNT(wantedFeatures); ++wanted)
        {
            if (features[feature] == wantedFeatures[wanted])
            {
                finalFeatures[finalFeatureCount++] = wantedFeatures[wanted];
                break;
            }
        }
    }
    SAFE_FREE(features);

    DECL_CHAINED_STRUCT_OUT(WGPUSupportedLimits, supported);
#if defined(WEBGPU_NATIVE)
    WGPUSupportedLimitsExtras supportedExt = {};
    supportedExt.chain.sType = (WGPUSType)WGPUSType_SupportedLimitsExtras;
    ADD_TO_NEXT_CHAIN_OUT(supported, &supportedExt);
#endif
    WGPULimits& limits = supported.limits;
    WGPUBool    ret = wgpuAdapterGetLimits(pRenderer->pGpu->mWgp.pAdapter, &supported);
    ASSERT(ret);

    DECL_CHAINED_STRUCT(WGPURequiredLimits, required);
#if defined(WEBGPU_NATIVE)
    WGPURequiredLimitsExtras requiredExt = {};
    requiredExt.chain.sType = (WGPUSType)WGPUSType_RequiredLimitsExtras;
    requiredExt.limits = supportedExt.limits;
    ADD_TO_NEXT_CHAIN(required, &requiredExt);
#endif
    required.limits = limits;

    DECL_CHAINED_STRUCT(WGPUDeviceDescriptor, deviceDesc);
    deviceDesc.requiredFeatureCount = finalFeatureCount;
    deviceDesc.requiredFeatures = finalFeatures;
    deviceDesc.requiredLimits = &required;
#if defined(WEBGPU_DAWN)
    const char* enableToggles[] = {
        "allow_unsafe_apis",
    };
    const char* disableToggles[] = {
        "lazy_clear_resource_on_first_use",
        "nonzero_clear_resources_on_creation_for_testing",
    };
    WGPUDawnTogglesDescriptor toggles = {};
    toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
    toggles.enabledToggleCount = TF_ARRAY_COUNT(enableToggles);
    toggles.enabledToggles = enableToggles;
    toggles.disabledToggleCount = TF_ARRAY_COUNT(disableToggles);
    toggles.disabledToggles = disableToggles;
    ADD_TO_NEXT_CHAIN(deviceDesc, &toggles);
#endif
    wgpuAdapterRequestDevice(
        pRenderer->pGpu->mWgp.pAdapter, &deviceDesc,
        [](WGPURequestDeviceStatus, WGPUDevice device, char const*, void* userdata) { *(WGPUDevice*)userdata = device; },
        &pRenderer->mWgp.pDevice);
    ASSERT(pRenderer->mWgp.pDevice);

    wgpuDeviceSetUncapturedErrorCallback(pRenderer->mWgp.pDevice, ValidationCallback, nullptr /* pUserData */);

    AddDefaultResources(pRenderer);

    ++gRendererCount;
    ASSERT(gRendererCount <= MAX_UNLINKED_GPUS);

    // Renderer is good!
    *ppRenderer = pRenderer;
}

void wgpu_exitRenderer(Renderer* pRenderer)
{
    ASSERT(pRenderer);
    --gRendererCount;

    RemoveDefaultResources(pRenderer);

    wgpuDeviceRelease(pRenderer->mWgp.pDevice);

    if (pRenderer->mOwnsContext)
    {
        wgpu_exitRendererContext(pRenderer->pContext);
    }

    SAFE_FREE(pRenderer);
}
/************************************************************************/
// Resource Creation Functions
/************************************************************************/
void wgpu_addFence(Renderer* pRenderer, Fence** ppFence)
{
    ASSERT(pRenderer);
    ASSERT(ppFence);

    Fence* pFence = (Fence*)tf_calloc(1, sizeof(Fence));
    ASSERT(pFence);

    *ppFence = pFence;
}

void wgpu_removeFence(Renderer* pRenderer, Fence* pFence)
{
    ASSERT(pRenderer);
    ASSERT(pFence);

    SAFE_FREE(pFence);
}

void wgpu_addSemaphore(Renderer* pRenderer, Semaphore** ppSemaphore)
{
    ASSERT(pRenderer);
    ASSERT(ppSemaphore);

    Semaphore* pSemaphore = (Semaphore*)tf_calloc(1, sizeof(Semaphore));
    ASSERT(pSemaphore);

    *ppSemaphore = pSemaphore;
}

void wgpu_removeSemaphore(Renderer* pRenderer, Semaphore* pSemaphore)
{
    ASSERT(pRenderer);
    ASSERT(pSemaphore);

    SAFE_FREE(pSemaphore);
}

void wgpu_addQueue(Renderer* pRenderer, QueueDesc* pDesc, Queue** ppQueue)
{
    ASSERT(pDesc != NULL);

    Queue* pQueue = (Queue*)tf_calloc(1, sizeof(Queue));
    ASSERT(pQueue);

    pQueue->mWgp.pRenderer = pRenderer;
    pQueue->mWgp.pQueue = wgpuDeviceGetQueue(pRenderer->mWgp.pDevice);
    ASSERT(pQueue->mWgp.pQueue);

    *ppQueue = pQueue;
}

void wgpu_removeQueue(Renderer* pRenderer, Queue* pQueue)
{
    ASSERT(pRenderer);
    ASSERT(pQueue);
    ASSERT(pQueue->mWgp.pQueue);

    wgpuQueueRelease(pQueue->mWgp.pQueue);

    SAFE_FREE(pQueue);
}

void wgpu_addCmdPool(Renderer* pRenderer, const CmdPoolDesc* pDesc, CmdPool** ppCmdPool)
{
    ASSERT(pRenderer);
    ASSERT(ppCmdPool);

    CmdPool* pCmdPool = (CmdPool*)tf_calloc(1, sizeof(CmdPool));
    ASSERT(pCmdPool);

    pCmdPool->pQueue = pDesc->pQueue;

    *ppCmdPool = pCmdPool;
}

void wgpu_removeCmdPool(Renderer* pRenderer, CmdPool* pCmdPool)
{
    ASSERT(pRenderer);
    ASSERT(pCmdPool);

    SAFE_FREE(pCmdPool);
}

void wgpu_addCmd(Renderer* pRenderer, const CmdDesc* pDesc, Cmd** ppCmd)
{
    ASSERT(pRenderer);
    ASSERT(ppCmd);

    Cmd* pCmd = (Cmd*)tf_calloc_memalign(1, alignof(Cmd), sizeof(Cmd));
    ASSERT(pCmd);

    pCmd->pRenderer = pRenderer;
    pCmd->pQueue = pDesc->pPool->pQueue;

    *ppCmd = pCmd;
}

void wgpu_removeCmd(Renderer* pRenderer, Cmd* pCmd)
{
    ASSERT(pRenderer);
    ASSERT(pCmd);

    arrfree(pCmd->mWgp.pRenderEncoderArray);
    arrfree(pCmd->mWgp.pComputeEncoderArray);
    SAFE_FREE(pCmd);
}

void wgpu_addCmd_n(Renderer* pRenderer, const CmdDesc* pDesc, uint32_t cmdCount, Cmd*** pppCmd)
{
    // verify that ***cmd is valid
    ASSERT(pRenderer);
    ASSERT(pDesc);
    ASSERT(cmdCount);
    ASSERT(pppCmd);

    Cmd** ppCmds = (Cmd**)tf_calloc(cmdCount, sizeof(Cmd*));
    ASSERT(ppCmds);

    // add n new cmds to given pool
    for (uint32_t i = 0; i < cmdCount; ++i)
    {
        ::addCmd(pRenderer, pDesc, &ppCmds[i]);
    }

    *pppCmd = ppCmds;
}

void wgpu_removeCmd_n(Renderer* pRenderer, uint32_t cmdCount, Cmd** ppCmds)
{
    // verify that given command list is valid
    ASSERT(ppCmds);

    // remove every given cmd in array
    for (uint32_t i = 0; i < cmdCount; ++i)
    {
        removeCmd(pRenderer, ppCmds[i]);
    }

    SAFE_FREE(ppCmds);
}

static void CreateSurface(Renderer* pRenderer, WindowHandle hwnd, WGPUSurface* outSurface)
{
    // Create a WSI surface for the window:
    DECL_CHAINED_STRUCT(WGPUSurfaceDescriptor, surfaceDesc);
    switch (hwnd.type)
    {
#if defined(_WINDOWS)
    case WINDOW_HANDLE_TYPE_WIN32:
    {
        WGPUSurfaceDescriptorFromWindowsHWND hwndDesc = {};
        hwndDesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
        hwndDesc.hinstance = ::GetModuleHandle(NULL);
        hwndDesc.hwnd = hwnd.window;
        ADD_TO_NEXT_CHAIN(surfaceDesc, &hwndDesc);
        *outSurface = wgpuInstanceCreateSurface(pRenderer->pContext->mWgp.pInstance, &surfaceDesc);
        break;
    }
#endif
#if defined(ANDROID)
    case WINDOW_HANDLE_TYPE_ANDROID:
    {
        WGPUSurfaceDescriptorFromAndroidNativeWindow hwndDesc = {};
        hwndDesc.chain.sType = WGPUSType_SurfaceDescriptorFromAndroidNativeWindow;
        hwndDesc.window = hwnd.window;
        ADD_TO_NEXT_CHAIN(surfaceDesc, &hwndDesc);
        *outSurface = wgpuInstanceCreateSurface(pRenderer->pContext->mWgp.pInstance, &surfaceDesc);
        break;
    }
#endif
    default:
        LOGF(eERROR, "Unsupported window handle type %d", (int)hwnd.type);
        ASSERT(false);
    }
}

static inline WGPUPresentMode GetPreferredPresentMode(const WGPUSurfaceCapabilities& caps, bool vsync)
{
    WGPUPresentMode presentMode = WGPUPresentMode_Force32;

    WGPUPresentMode preferredModeList[] =
    {
        WGPUPresentMode_Immediate,
#if !defined(ANDROID) && !defined(NX64)
        // Bad for thermal
        WGPUPresentMode_Mailbox,
#endif
#if defined(WEBGPU_NATIVE)
        WGPUPresentMode_FifoRelaxed,
#endif
        WGPUPresentMode_Fifo
    };
    const uint32_t preferredModeCount = TF_ARRAY_COUNT(preferredModeList);
    const uint32_t preferredModeStartIndex = vsync ? (preferredModeCount - 2) : 0;

    for (uint32_t j = preferredModeStartIndex; j < preferredModeCount; ++j)
    {
        WGPUPresentMode mode = preferredModeList[j];
        uint32_t        i = 0;
        for (; i < caps.presentModeCount; ++i)
        {
            if (caps.presentModes[i] == mode)
            {
                break;
            }
        }
        if (i < caps.presentModeCount)
        {
            presentMode = mode;
            break;
        }
    }

    if (WGPUPresentMode_Force32 == presentMode)
    {
        presentMode = caps.presentModes[0];
    }

    return presentMode;
}

void wgpu_toggleVSync(Renderer* pRenderer, SwapChain** ppSwapChain)
{
    SwapChain* pSwapChain = *ppSwapChain;
    pSwapChain->mEnableVsync = !pSwapChain->mEnableVsync;

    WGPUSurfaceCapabilities caps = {};
    wgpuSurfaceGetCapabilities(pSwapChain->mWgp.pSurface, pRenderer->pGpu->mWgp.pAdapter, &caps);

    WGPUSurfaceConfiguration surfaceConfig = pSwapChain->mWgp.mConfig;
    surfaceConfig.presentMode = GetPreferredPresentMode(caps, pSwapChain->mEnableVsync);
    wgpuSurfaceCapabilitiesFreeMembers(caps);

    wgpuSurfaceUnconfigure(pSwapChain->mWgp.pSurface);
    wgpuSurfaceConfigure(pSwapChain->mWgp.pSurface, &surfaceConfig);
}

void wgpu_addSwapChain(Renderer* pRenderer, const SwapChainDesc* pDesc, SwapChain** ppSwapChain)
{
    ASSERT(pRenderer);
    ASSERT(pDesc);
    ASSERT(ppSwapChain);
    ASSERT(pDesc->mImageCount <= MAX_SWAPCHAIN_IMAGES);
    ASSERT(pDesc->ppPresentQueues);

    LOGF(LogLevel::eINFO, "Adding WebGpu swapchain @ %ux%u", pDesc->mWidth, pDesc->mHeight);

    SwapChain* pSwapChain = (SwapChain*)tf_calloc(1, sizeof(SwapChain) + sizeof(RenderTarget));
    ASSERT(pSwapChain);
    pSwapChain->ppRenderTargets = (RenderTarget**)(pSwapChain + 1);

    CreateSurface(pRenderer, pDesc->mWindowHandle, &pSwapChain->mWgp.pSurface);

    WGPUSurfaceCapabilities caps = {};
    wgpuSurfaceGetCapabilities(pSwapChain->mWgp.pSurface, pRenderer->pGpu->mWgp.pAdapter, &caps);

    WGPUSurfaceConfiguration surfaceConfig = {};
    surfaceConfig.alphaMode = caps.alphaModes[0];
    surfaceConfig.device = pRenderer->mWgp.pDevice;
    surfaceConfig.format = (WGPUTextureFormat)TinyImageFormat_ToWGPUTextureFormat(pDesc->mColorFormat);
    surfaceConfig.height = pDesc->mHeight;
    surfaceConfig.presentMode = GetPreferredPresentMode(caps, pDesc->mEnableVsync);
    surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
    surfaceConfig.width = pDesc->mWidth;

    // Doesnt seem to work - Error View format (TextureFormat::RGBA8UnormSrgb) is not supported
    // static WGPUTextureFormat srgbFormat;
    // srgbFormat = (WGPUTextureFormat)TinyImageFormat_ToWGPUTextureFormat(TinyImageFormat_ToSRGB(pDesc->mColorFormat));
    // if (COLOR_SPACE_SDR_SRGB == pDesc->mColorSpace || COLOR_SPACE_EXTENDED_SRGB == pDesc->mColorSpace)
    //{
    //    surfaceConfig.viewFormats = &srgbFormat;
    //    surfaceConfig.viewFormatCount = 1;
    //}
    wgpuSurfaceConfigure(pSwapChain->mWgp.pSurface, &surfaceConfig);

    // Create the swapchain RT descriptor.
    RenderTargetDesc descColor = {};
    descColor.mWidth = pDesc->mWidth;
    descColor.mHeight = pDesc->mHeight;
    descColor.mDepth = 1;
    descColor.mArraySize = 1;
    descColor.mFormat = pDesc->mColorFormat;
    descColor.mClearValue = pDesc->mColorClearValue;
    descColor.mSampleCount = SAMPLE_COUNT_1;
    descColor.mSampleQuality = 0;
    descColor.mFlags |= TEXTURE_CREATION_FLAG_ALLOW_DISPLAY_TARGET;

    for (uint32_t i = 0; i < pDesc->mImageCount; ++i)
    {
        addRenderTarget(pRenderer, &descColor, &pSwapChain->ppRenderTargets[i]);
    }

    pSwapChain->mWgp.mConfig = surfaceConfig;
    pSwapChain->mImageCount = 1;
    pSwapChain->mEnableVsync = pDesc->mEnableVsync;
    pSwapChain->mFormat = pDesc->mColorFormat;
    pSwapChain->mColorSpace = pDesc->mColorSpace;

    wgpuSurfaceCapabilitiesFreeMembers(caps);

    *ppSwapChain = pSwapChain;
}

void wgpu_removeSwapChain(Renderer* pRenderer, SwapChain* pSwapChain)
{
    ASSERT(pRenderer);
    ASSERT(pSwapChain);
    ASSERT(pSwapChain->mWgp.pSurface);

    for (uint32_t i = 0; i < pSwapChain->mImageCount; ++i)
    {
        removeRenderTarget(pRenderer, pSwapChain->ppRenderTargets[i]);
    }

    wgpuSurfaceUnconfigure(pSwapChain->mWgp.pSurface);
    wgpuSurfaceRelease(pSwapChain->mWgp.pSurface);

    SAFE_FREE(pSwapChain);
}

void wgpu_addResourceHeap(Renderer*, const ResourceHeapDesc*, ResourceHeap**) { ASSERTFAIL("Not supported"); }

void wgpu_removeResourceHeap(Renderer*, ResourceHeap*) { ASSERTFAIL("Not supported"); }

void wgpu_getBufferSizeAlign(Renderer*, const BufferDesc*, ResourceSizeAlign*) { ASSERTFAIL("Not supported"); }

void wgpu_getTextureSizeAlign(Renderer*, const TextureDesc*, ResourceSizeAlign*) { ASSERTFAIL("Not supported"); }

static WGPUBufferUsageFlags ToBufferUsage(BufferCreationFlags, DescriptorType usage)
{
    WGPUBufferUsageFlags result = WGPUBufferUsage_None;
    if (usage & DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    {
        result |= WGPUBufferUsage_Uniform;
    }
    if (usage & DESCRIPTOR_TYPE_RW_BUFFER)
    {
        result |= WGPUBufferUsage_Storage;
    }
    if (usage & DESCRIPTOR_TYPE_BUFFER)
    {
        result |= WGPUBufferUsage_Storage;
    }
    if (usage & DESCRIPTOR_TYPE_INDEX_BUFFER)
    {
        result |= WGPUBufferUsage_Index;
    }
    if (usage & DESCRIPTOR_TYPE_VERTEX_BUFFER)
    {
        result |= WGPUBufferUsage_Vertex;
    }
    if (usage & DESCRIPTOR_TYPE_INDIRECT_BUFFER)
    {
        result |= WGPUBufferUsage_Indirect;
    }
    return result;
}

void wgpu_addBuffer(Renderer* pRenderer, const BufferDesc* pDesc, Buffer** ppBuffer)
{
    ASSERT(pRenderer);
    ASSERT(pDesc);
    ASSERT(pDesc->mSize > 0);
    ASSERT(pRenderer->mGpuMode != GPU_MODE_UNLINKED || pDesc->mNodeIndex == pRenderer->mUnlinkedRendererIndex);

    Buffer* pBuffer = (Buffer*)tf_calloc_memalign(1, alignof(Buffer), sizeof(Buffer));
    ASSERT(ppBuffer);

    uint64_t size = round_up_64(pDesc->mSize, pRenderer->pGpu->mSettings.mUploadBufferAlignment);

    ResourceMemoryUsage  memUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size = size;
    bufferDesc.usage = ToBufferUsage(pDesc->mFlags, pDesc->mDescriptors);
    bufferDesc.usage |= WGPUBufferUsage_CopyDst;
    if (RESOURCE_MEMORY_USAGE_CPU_ONLY == pDesc->mMemoryUsage)
    {
        bufferDesc.usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_MapWrite;
        memUsage = pDesc->mMemoryUsage;
    }
    else if (RESOURCE_MEMORY_USAGE_GPU_TO_CPU == pDesc->mMemoryUsage)
    {
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        memUsage = pDesc->mMemoryUsage;
    }
#if defined(ENABLE_GRAPHICS_DEBUG)
    bufferDesc.label = pDesc->pName;
#endif
    pBuffer->mWgp.pBuffer = wgpuDeviceCreateBuffer(pRenderer->mWgp.pDevice, &bufferDesc);
    /************************************************************************/
    /************************************************************************/
    pBuffer->mSize = (uint32_t)size;
    pBuffer->mMemoryUsage = memUsage;
    pBuffer->mNodeIndex = pDesc->mNodeIndex;
    pBuffer->mDescriptors = pDesc->mDescriptors;

    if ((pDesc->mFlags & BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT) &&
        (bufferDesc.usage & (WGPUBufferUsage_MapWrite | WGPUBufferUsage_MapRead)))
    {
        mapBuffer(pRenderer, pBuffer, NULL);
    }

    *ppBuffer = pBuffer;
}

void wgpu_removeBuffer(Renderer* pRenderer, Buffer* pBuffer)
{
    ASSERT(pRenderer);
    ASSERT(pBuffer);

    wgpuBufferDestroy(pBuffer->mWgp.pBuffer);

    SAFE_FREE(pBuffer);
}

static inline WGPUTextureDimension ToTextureDimension(const TextureDesc* pDesc)
{
    WGPUTextureDimension dim = WGPUTextureDimension_Force32;
    if (pDesc->mFlags & TEXTURE_CREATION_FLAG_FORCE_2D)
    {
        ASSERT(pDesc->mDepth == 1);
        dim = WGPUTextureDimension_2D;
    }
    else if (pDesc->mFlags & TEXTURE_CREATION_FLAG_FORCE_3D)
    {
        dim = WGPUTextureDimension_3D;
    }
    else
    {
        if (pDesc->mDepth > 1)
        {
            dim = WGPUTextureDimension_3D;
        }
        else if (pDesc->mHeight > 1)
        {
            dim = WGPUTextureDimension_2D;
        }
        else
        {
            dim = WGPUTextureDimension_1D;
        }
    }

    return dim;
}

static inline WGPUTextureUsageFlags ToTextureUsage(DescriptorType usage, ResourceState startState)
{
    WGPUTextureUsageFlags result = WGPUTextureUsage_None;
    if (DESCRIPTOR_TYPE_TEXTURE == (usage & DESCRIPTOR_TYPE_TEXTURE))
    {
        result |= WGPUTextureUsage_TextureBinding;
    }
    if (DESCRIPTOR_TYPE_RW_TEXTURE == (usage & DESCRIPTOR_TYPE_RW_TEXTURE))
    {
        result |= WGPUTextureUsage_StorageBinding;
    }
    if (startState & (RESOURCE_STATE_RENDER_TARGET | RESOURCE_STATE_DEPTH_WRITE))
    {
        result |= WGPUTextureUsage_RenderAttachment;
    }
    return result;
}

static inline WGPUTextureAspect ToTextureAspect(TinyImageFormat format, bool includeStencilBit)
{
    if (TinyImageFormat_HasDepthOrStencil(format))
    {
        if (!TinyImageFormat_HasDepth(format))
        {
            ASSERT(includeStencilBit);
            return WGPUTextureAspect_StencilOnly;
        }
        if (TinyImageFormat_HasStencil(format) && includeStencilBit)
        {
            return WGPUTextureAspect_All;
        }
        else
        {
            return WGPUTextureAspect_DepthOnly;
        }
    }
    else
    {
        return WGPUTextureAspect_All;
    }
}

void wgpu_addTexture(Renderer* pRenderer, const TextureDesc* pDesc, Texture** ppTexture)
{
    ASSERT(pRenderer);
    ASSERT(pDesc && pDesc->mWidth && pDesc->mHeight && (pDesc->mDepth || pDesc->mArraySize));
    ASSERT(pRenderer->mGpuMode != GPU_MODE_UNLINKED || pDesc->mNodeIndex == pRenderer->mUnlinkedRendererIndex);
    if (pDesc->mSampleCount > SAMPLE_COUNT_1 && pDesc->mMipLevels > 1)
    {
        LOGF(LogLevel::eERROR, "Multi-Sampled textures cannot have mip maps");
        ASSERT(false);
        return;
    }

    size_t totalSize = sizeof(Texture);
    totalSize += (pDesc->mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE ? (pDesc->mMipLevels * sizeof(VkImageView)) : 0);
    Texture* pTexture = (Texture*)tf_calloc_memalign(1, alignof(Texture), totalSize);
    ASSERT(pTexture);

    if (pDesc->mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE)
    {
        pTexture->mWgp.pUavs = (WGPUTextureView*)(pTexture + 1);
    }

    if (pDesc->mFlags & TEXTURE_CREATION_FLAG_ALLOW_DISPLAY_TARGET)
    {
        pTexture->mOwnsImage = false;
    }
    else if (pDesc->pNativeHandle && !(pDesc->mFlags & TEXTURE_CREATION_FLAG_IMPORT_BIT))
    {
        pTexture->mOwnsImage = false;
    }
    else
    {
        DECL_CHAINED_STRUCT(WGPUTextureDescriptor, textureDesc);
        textureDesc.dimension = ToTextureDimension(pDesc);
        textureDesc.format = (WGPUTextureFormat)TinyImageFormat_ToWGPUTextureFormat(pDesc->mFormat);
        textureDesc.mipLevelCount = pDesc->mMipLevels;
        textureDesc.sampleCount = pDesc->mSampleCount;
        textureDesc.size.depthOrArrayLayers = (pDesc->mArraySize != 1 ? pDesc->mArraySize : pDesc->mDepth);
        textureDesc.size.height = pDesc->mHeight;
        textureDesc.size.width = pDesc->mWidth;
        textureDesc.usage = ToTextureUsage(pDesc->mDescriptors, pDesc->mStartState);
        textureDesc.usage |= WGPUTextureUsage_CopySrc | WGPUTextureUsage_CopyDst;
#if defined(ENABLE_GRAPHICS_DEBUG)
        textureDesc.label = pDesc->pName;
#endif
#if defined(WEBGPU_DAWN)
        WGPUTextureBindingViewDimensionDescriptor desc = {};
        if (pRenderer->pGpu->mWgp.mCompatMode && (DESCRIPTOR_TYPE_TEXTURE_CUBE == (pDesc->mDescriptors & DESCRIPTOR_TYPE_TEXTURE_CUBE)))
        {
            desc.chain.sType = WGPUSType_TextureBindingViewDimensionDescriptor;
            desc.textureBindingViewDimension = WGPUTextureViewDimension_Cube;
            ADD_TO_NEXT_CHAIN(textureDesc, &desc);
        }
#endif

        pTexture->mWgp.pTexture = wgpuDeviceCreateTexture(pRenderer->mWgp.pDevice, &textureDesc);
        pTexture->mOwnsImage = true;
    }

    if (!(pDesc->mFlags & TEXTURE_CREATION_FLAG_ALLOW_DISPLAY_TARGET))
    {
        WGPUTextureDimension     dim = wgpuTextureGetDimension(pTexture->mWgp.pTexture);
        const uint32_t           arraySize = pDesc->mArraySize;
        const DescriptorType     descriptors = pDesc->mDescriptors;
        const bool               cubemapRequired = (DESCRIPTOR_TYPE_TEXTURE_CUBE == (descriptors & DESCRIPTOR_TYPE_TEXTURE_CUBE));
        /************************************************************************/
        // Create image view
        /************************************************************************/
        WGPUTextureViewDimension viewDim = WGPUTextureViewDimension_Undefined;
        switch (dim)
        {
        case WGPUTextureDimension_1D:
            if (arraySize > 1)
            {
                ASSERTFAIL("Cannot support 1D Texture Array in WebGpu");
            }
            viewDim = WGPUTextureViewDimension_1D;
            break;
        case WGPUTextureDimension_2D:
            if (cubemapRequired)
            {
                viewDim = (arraySize > 6) ? WGPUTextureViewDimension_CubeArray : WGPUTextureViewDimension_Cube;
            }
            else
            {
                viewDim = arraySize > 1 ? WGPUTextureViewDimension_2DArray : WGPUTextureViewDimension_2D;
            }
            break;
        case WGPUTextureDimension_3D:
            if (arraySize > 1)
            {
                ASSERTFAIL("Cannot support 3D Texture Array in WebGpu");
            }
            viewDim = WGPUTextureViewDimension_3D;
            break;
        default:
            ASSERTFAIL("Image dimension not supported!");
            break;
        }

        ASSERT(viewDim != WGPUTextureViewDimension_Undefined && "Invalid Image View");

        WGPUTextureViewDescriptor srvDesc = {};
#if defined(ENABLE_GRAPHICS_DEBUG)
        srvDesc.label = pDesc->pName;
#endif
        // SRV
        srvDesc.arrayLayerCount = arraySize;
        srvDesc.aspect = ToTextureAspect(pDesc->mFormat, false);
        srvDesc.baseArrayLayer = 0;
        srvDesc.baseMipLevel = 0;
        srvDesc.dimension = viewDim;
        srvDesc.format = wgpuTextureGetFormat(pTexture->mWgp.pTexture);
        srvDesc.mipLevelCount = pDesc->mMipLevels;

        if (descriptors & DESCRIPTOR_TYPE_TEXTURE)
        {
            pTexture->mWgp.pSrv = wgpuTextureCreateView(pTexture->mWgp.pTexture, &srvDesc);
            ASSERT(pTexture->mWgp.pSrv);

            // SRV stencil
            if (TinyImageFormat_HasStencil(pDesc->mFormat))
            {
                srvDesc.aspect = WGPUTextureAspect_StencilOnly;
                pTexture->mWgp.pSrvStencil = wgpuTextureCreateView(pTexture->mWgp.pTexture, &srvDesc);
                ASSERT(pTexture->mWgp.pSrvStencil);
            }
        }

        // UAV
        if (descriptors & DESCRIPTOR_TYPE_RW_TEXTURE)
        {
            WGPUTextureViewDescriptor uavDesc = srvDesc;
            // #NOTE : We dont support imageCube, imageCubeArray for consistency with other APIs
            // All cubemaps will be used as image2DArray for Image Load / Store ops
            if (uavDesc.dimension == WGPUTextureViewDimension_CubeArray || uavDesc.dimension == WGPUTextureViewDimension_Cube)
            {
                uavDesc.dimension = WGPUTextureViewDimension_2DArray;
            }
            uavDesc.mipLevelCount = 1;
            for (uint32_t i = 0; i < pDesc->mMipLevels; ++i)
            {
                uavDesc.baseMipLevel = i;
                pTexture->mWgp.pUavs[i] = wgpuTextureCreateView(pTexture->mWgp.pTexture, &uavDesc);
                ASSERT(pTexture->mWgp.pUavs[i]);
            }
        }
    }

    pTexture->mNodeIndex = pDesc->mNodeIndex;
    pTexture->mWidth = pDesc->mWidth;
    pTexture->mHeight = pDesc->mHeight;
    pTexture->mDepth = pDesc->mDepth;
    pTexture->mMipLevels = pDesc->mMipLevels;
    pTexture->mUav = pDesc->mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE;
    pTexture->mArraySizeMinusOne = pDesc->mArraySize - 1;
    pTexture->mFormat = pDesc->mFormat;
    pTexture->mSampleCount = pDesc->mSampleCount;

    *ppTexture = pTexture;
}

void wgpu_removeTexture(Renderer* pRenderer, Texture* pTexture)
{
    ASSERT(pRenderer);
    ASSERT(pTexture);

    if (pTexture->mWgp.pSrv)
    {
        wgpuTextureViewRelease(pTexture->mWgp.pSrv);
    }

    if (pTexture->mWgp.pSrvStencil)
    {
        wgpuTextureViewRelease(pTexture->mWgp.pSrvStencil);
    }

    if (pTexture->mWgp.pUavs)
    {
        for (uint32_t i = 0; i < pTexture->mMipLevels; ++i)
        {
            wgpuTextureViewRelease(pTexture->mWgp.pUavs[i]);
        }
    }

    if (pTexture->mOwnsImage)
    {
        wgpuTextureDestroy(pTexture->mWgp.pTexture);
    }

    SAFE_FREE(pTexture);
}

void wgpu_addRenderTarget(Renderer* pRenderer, const RenderTargetDesc* pDesc, RenderTarget** ppRenderTarget)
{
    ASSERT(pRenderer);
    ASSERT(pDesc);
    ASSERT(ppRenderTarget);
    ASSERT(pRenderer->mGpuMode != GPU_MODE_UNLINKED || pDesc->mNodeIndex == pRenderer->mUnlinkedRendererIndex);

    bool const isDepth = TinyImageFormat_IsDepthOnly(pDesc->mFormat) || TinyImageFormat_IsDepthAndStencil(pDesc->mFormat);

    ASSERT(!((isDepth) && (pDesc->mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE)) && "Cannot use depth stencil as UAV");

    ((RenderTargetDesc*)pDesc)->mMipLevels = max(1U, pDesc->mMipLevels);

    uint32_t depthOrArraySize = pDesc->mArraySize * pDesc->mDepth;
    uint32_t numRTVs = pDesc->mMipLevels;
    if ((pDesc->mDescriptors & DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES) ||
        (pDesc->mDescriptors & DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES))
        numRTVs *= depthOrArraySize;
    size_t totalSize = sizeof(RenderTarget);
    totalSize += numRTVs * sizeof(WGPUTextureView);
    RenderTarget* pRenderTarget = (RenderTarget*)tf_calloc_memalign(1, alignof(RenderTarget), totalSize);
    ASSERT(pRenderTarget);

    pRenderTarget->mWgp.pSlices = (WGPUTextureView*)(pRenderTarget + 1);

    TextureDesc textureDesc = {};
    textureDesc.mArraySize = pDesc->mArraySize;
    textureDesc.mClearValue = pDesc->mClearValue;
    textureDesc.mDepth = pDesc->mDepth;
    textureDesc.mFlags = pDesc->mFlags;
    textureDesc.mFormat = pDesc->mFormat;
    textureDesc.mHeight = pDesc->mHeight;
    textureDesc.mMipLevels = pDesc->mMipLevels;
    textureDesc.mSampleCount = pDesc->mSampleCount;
    textureDesc.mSampleQuality = pDesc->mSampleQuality;
    textureDesc.mWidth = pDesc->mWidth;
    textureDesc.pNativeHandle = pDesc->pNativeHandle;
    textureDesc.mNodeIndex = pDesc->mNodeIndex;
    textureDesc.pSharedNodeIndices = pDesc->pSharedNodeIndices;
    textureDesc.mSharedNodeIndexCount = pDesc->mSharedNodeIndexCount;

    if (!isDepth)
    {
        textureDesc.mStartState |= RESOURCE_STATE_RENDER_TARGET;
    }
    else
    {
        textureDesc.mStartState |= RESOURCE_STATE_DEPTH_WRITE;
    }

    // Set this by default to be able to sample the rendertarget in shader
    textureDesc.mDescriptors = pDesc->mDescriptors;
    // Create SRV by default for a render target unless this is on tile texture where SRV is not supported
    if (!(pDesc->mFlags & TEXTURE_CREATION_FLAG_ON_TILE))
    {
        textureDesc.mDescriptors |= DESCRIPTOR_TYPE_TEXTURE;
    }
    else
    {
        if ((textureDesc.mDescriptors & DESCRIPTOR_TYPE_TEXTURE) || (textureDesc.mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE))
        {
            LOGF(eWARNING, "On tile textures do not support DESCRIPTOR_TYPE_TEXTURE or DESCRIPTOR_TYPE_RW_TEXTURE");
        }
        // On tile textures do not support SRV/UAV as there is no backing memory
        // You can only read these textures as input attachments inside same render pass
        textureDesc.mDescriptors &= (DescriptorType)(~(DESCRIPTOR_TYPE_TEXTURE | DESCRIPTOR_TYPE_RW_TEXTURE));
    }

    textureDesc.pName = pDesc->pName;
    textureDesc.pPlacement = pDesc->pPlacement;
    addTexture(pRenderer, &textureDesc, &pRenderTarget->pTexture);

    if (!(pDesc->mFlags & TEXTURE_CREATION_FLAG_ALLOW_DISPLAY_TARGET))
    {
        WGPUTextureDimension     dim = wgpuTextureGetDimension(pRenderTarget->pTexture->mWgp.pTexture);
        WGPUTextureViewDimension viewDim = WGPUTextureViewDimension_Undefined;
        switch (dim)
        {
        case WGPUTextureDimension_1D:
            ASSERTFAIL("1D RTV not supported");
            break;
        case WGPUTextureDimension_2D:
        {
            viewDim = pDesc->mArraySize > 1 ? WGPUTextureViewDimension_2DArray : WGPUTextureViewDimension_2D;
            break;
        }
        case WGPUTextureDimension_3D:
        {
            if (pDesc->mArraySize > 1)
            {
                ASSERTFAIL("3D Array RTV not supported");
            }
            viewDim = WGPUTextureViewDimension_3D;
            break;
        }
        default:
            ASSERTFAIL("Not supported");
            break;
        }

        WGPUTextureViewDescriptor viewDesc = {};
        viewDesc.arrayLayerCount = pDesc->mArraySize;
        viewDesc.aspect = WGPUTextureAspect_All;
        viewDesc.baseArrayLayer = 0;
        viewDesc.baseMipLevel = 0;
        viewDesc.dimension = viewDim;
        viewDesc.format = wgpuTextureGetFormat(pRenderTarget->pTexture->mWgp.pTexture);
        viewDesc.mipLevelCount = 1;
#if defined(ENABLE_GRAPHICS_DEBUG)
        viewDesc.label = pDesc->pName;
#endif
        pRenderTarget->mWgp.pDefault = wgpuTextureCreateView(pRenderTarget->pTexture->mWgp.pTexture, &viewDesc);
        ASSERT(pRenderTarget->mWgp.pDefault);

        depthOrArraySize = wgpuTextureGetDepthOrArrayLayers(pRenderTarget->pTexture->mWgp.pTexture);

        for (uint32_t i = 0; i < pDesc->mMipLevels; ++i)
        {
            viewDesc.baseMipLevel = i;
            if ((pDesc->mDescriptors & DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES) ||
                (pDesc->mDescriptors & DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES))
            {
                for (uint32_t j = 0; j < depthOrArraySize; ++j)
                {
                    const uint32_t index = i * depthOrArraySize + j;
                    viewDesc.arrayLayerCount = 1;
                    viewDesc.baseArrayLayer = j;
                    pRenderTarget->mWgp.pSlices[index] = wgpuTextureCreateView(pRenderTarget->pTexture->mWgp.pTexture, &viewDesc);
                    ASSERT(pRenderTarget->mWgp.pSlices[index]);
                }
            }
            else
            {
                pRenderTarget->mWgp.pSlices[i] = wgpuTextureCreateView(pRenderTarget->pTexture->mWgp.pTexture, &viewDesc);
                ASSERT(pRenderTarget->mWgp.pSlices[i]);
            }
        }
    }

    pRenderTarget->mWidth = pDesc->mWidth;
    pRenderTarget->mHeight = pDesc->mHeight;
    pRenderTarget->mArraySize = pDesc->mArraySize;
    pRenderTarget->mDepth = pDesc->mDepth;
    pRenderTarget->mMipLevels = pDesc->mMipLevels;
    pRenderTarget->mSampleCount = pDesc->mSampleCount;
    pRenderTarget->mSampleQuality = pDesc->mSampleQuality;
    pRenderTarget->mFormat = pDesc->mFormat;
    pRenderTarget->mClearValue = pDesc->mClearValue;
    pRenderTarget->mVRMultiview = (pDesc->mFlags & TEXTURE_CREATION_FLAG_VR_MULTIVIEW) != 0;
    pRenderTarget->mVRFoveatedRendering = (pDesc->mFlags & TEXTURE_CREATION_FLAG_VR_FOVEATED_RENDERING) != 0;
    pRenderTarget->mDescriptors = pDesc->mDescriptors;

    *ppRenderTarget = pRenderTarget;
}

void wgpu_removeRenderTarget(Renderer* pRenderer, RenderTarget* pRenderTarget)
{
    if (pRenderTarget->mWgp.pDefault)
    {
        wgpuTextureViewRelease(pRenderTarget->mWgp.pDefault);
    }

    const uint32_t depthOrArraySize = pRenderTarget->mArraySize * pRenderTarget->mDepth;
    if ((pRenderTarget->mDescriptors & DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES) ||
        (pRenderTarget->mDescriptors & DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES))
    {
        for (uint32_t i = 0; i < pRenderTarget->mMipLevels; ++i)
        {
            for (uint32_t j = 0; j < depthOrArraySize; ++j)
            {
                wgpuTextureViewRelease(pRenderTarget->mWgp.pSlices[i * depthOrArraySize + j]);
            }
        }
    }
    else if (pRenderTarget->mWgp.pSlices[0])
    {
        for (uint32_t i = 0; i < pRenderTarget->mMipLevels; ++i)
        {
            wgpuTextureViewRelease(pRenderTarget->mWgp.pSlices[i]);
        }
    }

    ::removeTexture(pRenderer, pRenderTarget->pTexture);

    SAFE_FREE(pRenderTarget);
}

void wgpu_addSampler(Renderer* pRenderer, const SamplerDesc* pDesc, Sampler** ppSampler)
{
    ASSERT(pRenderer);
    ASSERT(pDesc->mCompareFunc < MAX_COMPARE_MODES);
    ASSERT(ppSampler);

    Sampler* pSampler = (Sampler*)tf_calloc_memalign(1, alignof(Sampler), sizeof(Sampler));
    ASSERT(pSampler);

    // default sampler lod values
    // used if not overriden by mSetLodRange or not Linear mipmaps
    float minSamplerLod = 0;
    float maxSamplerLod = pDesc->mMipMapMode == MIPMAP_MODE_LINEAR ? VK_LOD_CLAMP_NONE : 0;
    // user provided lods
    if (pDesc->mSetLodRange)
    {
        minSamplerLod = pDesc->mMinLod;
        maxSamplerLod = pDesc->mMaxLod;
    }

    WGPUSamplerDescriptor samplerDesc = {};
    samplerDesc.addressModeU = ToAddressMode(pDesc->mAddressU);
    samplerDesc.addressModeV = ToAddressMode(pDesc->mAddressV);
    samplerDesc.addressModeW = ToAddressMode(pDesc->mAddressW);
    samplerDesc.compare = pDesc->mCompareFunc != CMP_NEVER ? ToCompareFunction(pDesc->mCompareFunc) : WGPUCompareFunction_Undefined;
    samplerDesc.lodMaxClamp = maxSamplerLod;
    samplerDesc.lodMinClamp = minSamplerLod;
    samplerDesc.magFilter = ToFilterMode(pDesc->mMagFilter);
    samplerDesc.maxAnisotropy = max((uint16_t)1u, (uint16_t)pDesc->mMaxAnisotropy);
    samplerDesc.minFilter = ToFilterMode(pDesc->mMinFilter);
    samplerDesc.mipmapFilter = ToMipmapMode(pDesc->mMipMapMode);
    pSampler->mWgp.pSampler = wgpuDeviceCreateSampler(pRenderer->mWgp.pDevice, &samplerDesc);
    ASSERT(pSampler->mWgp.pSampler);

    *ppSampler = pSampler;
}

void wgpu_removeSampler(Renderer* pRenderer, Sampler* pSampler)
{
    ASSERT(pRenderer);
    ASSERT(pSampler);

    wgpuSamplerRelease(pSampler->mWgp.pSampler);

    SAFE_FREE(pSampler);
}

/************************************************************************/
// Buffer Functions
/************************************************************************/
void wgpu_mapBuffer(Renderer* pRenderer, Buffer* pBuffer, ReadRange* pRange)
{
    ASSERT(pRenderer);
    ASSERT(pBuffer);

    const size_t      offset = pRange ? pRange->mOffset : 0;
    const size_t      size = pRange ? pRange->mSize : pBuffer->mSize;
    const WGPUMapMode mapMode = pBuffer->mMemoryUsage == RESOURCE_MEMORY_USAGE_GPU_TO_CPU ? WGPUMapMode_Read : WGPUMapMode_Write;
#if defined(WEBGPU_NATIVE)
    wgpuBufferMapAsync(
        pBuffer->mWgp.pBuffer, mapMode, offset, size, [](WGPUBufferMapAsyncStatus status, void* userdata) {}, NULL);
    wgpuDevicePoll(pRenderer->mWgp.pDevice, true, NULL);
#elif defined(WEBGPU_DAWN)
    WGPUFuture future = wgpuBufferMapAsyncF(pBuffer->mWgp.pBuffer, mapMode, offset, size,
                                            { NULL, WGPUCallbackMode_WaitAnyOnly, [](WGPUBufferMapAsyncStatus, void*) {}, NULL });
    WGPUFutureWaitInfo waitInfo = { future };
    wgpuInstanceWaitAny(pRenderer->pContext->mWgp.pInstance, 1, &waitInfo, UINT64_MAX);
#endif
    if (WGPUMapMode_Write == mapMode)
    {
        pBuffer->pCpuMappedAddress = wgpuBufferGetMappedRange(pBuffer->mWgp.pBuffer, offset, size);
    }
    else
    {
        pBuffer->pCpuMappedAddress = (void*)wgpuBufferGetConstMappedRange(pBuffer->mWgp.pBuffer, offset, size);
    }
}

void wgpu_unmapBuffer(Renderer* pRenderer, Buffer* pBuffer)
{
    ASSERT(pRenderer);
    ASSERT(pBuffer);

    wgpuBufferUnmap(pBuffer->mWgp.pBuffer);
    pBuffer->pCpuMappedAddress = NULL;
}
/************************************************************************/
// Descriptor Set Functions
/************************************************************************/
struct DynamicUniformData
{
    WGPUBuffer pBuffer;
    uint32_t   mOffset;
    uint32_t   mSize;
};

void wgpu_addDescriptorSet(Renderer* pRenderer, const DescriptorSetDesc* pDesc, DescriptorSet** ppDescriptorSet)
{
    ASSERT(pRenderer);
    ASSERT(pDesc);
    ASSERT(ppDescriptorSet);

    const RootSignature*            pRootSignature = pDesc->pRootSignature;
    const DescriptorUpdateFrequency updateFreq = pDesc->mUpdateFrequency;
    const uint32_t                  nodeIndex = pRenderer->mGpuMode == GPU_MODE_LINKED ? pDesc->mNodeIndex : 0;
    const uint32_t                  dynamicOffsetCount = pRootSignature->mWgp.mDynamicDescriptorCounts[updateFreq];

    if (!pRootSignature->mWgp.mDescriptorSetLayouts[updateFreq])
    {
        LOGF(LogLevel::eERROR, "NULL Descriptor Set Layout for update frequency %u. Cannot allocate descriptor set", (uint32_t)updateFreq);
        ASSERTFAIL("NULL Descriptor Set Layout for update frequency. Cannot allocate descriptor set");
        return;
    }

    uint32_t totalSize = sizeof(DescriptorSet);
    totalSize += pDesc->mMaxSets * sizeof(WGPUBindGroup);

    uint32_t descriptorCount = 0;
    for (uint32_t descIndex = 0; descIndex < pRootSignature->mDescriptorCount; ++descIndex)
    {
        const DescriptorInfo* descInfo = &pRootSignature->pDescriptors[descIndex];
        const bool            staticSampler = pRenderer->pGpu->mWgp.mStaticSamplers && descInfo->mStaticSampler;
        if (descInfo->mUpdateFrequency != (uint32_t)updateFreq || DESCRIPTOR_TYPE_ROOT_CONSTANT == descInfo->mType || staticSampler)
        {
            continue;
        }
        totalSize += pDesc->mMaxSets * sizeof(WGPUBindGroupEntry);
        if (descInfo->mSize > 1)
        {
#if defined(WEBGPU_NATIVE)
            totalSize += pDesc->mMaxSets * sizeof(WGPUBindGroupEntryExtras);
#endif
            totalSize += pDesc->mMaxSets * sizeof(WGPUBuffer) * descInfo->mSize;
        }
        ++descriptorCount;
    }
    totalSize += pDesc->mMaxSets * dynamicOffsetCount * sizeof(DynamicUniformData);

    DescriptorSet* pDescriptorSet = (DescriptorSet*)tf_calloc_memalign(1, alignof(DescriptorSet), totalSize);

    pDescriptorSet->mWgp.pRootSignature = pRootSignature;
    pDescriptorSet->mWgp.mUpdateFrequency = (uint8_t)updateFreq;
    pDescriptorSet->mWgp.mDynamicOffsetCount = (uint8_t)dynamicOffsetCount;
    pDescriptorSet->mWgp.mDynamicOffsetHandleIndex = pRootSignature->mWgp.mDynamicDescriptorStartIndex[updateFreq];
    pDescriptorSet->mWgp.mNodeIndex = (uint8_t)nodeIndex;
    pDescriptorSet->mWgp.mMaxSets = pDesc->mMaxSets;
    pDescriptorSet->mWgp.mEntryCount = descriptorCount;

    uint8_t* pMem = (uint8_t*)(pDescriptorSet + 1);
    pDescriptorSet->mWgp.pHandles = (WGPUBindGroup*)pMem;
    pMem += pDesc->mMaxSets * sizeof(WGPUBindGroup);
    pDescriptorSet->mWgp.pEntries = (WGPUBindGroupEntry*)pMem;
    pMem += pDesc->mMaxSets * descriptorCount * sizeof(WGPUBindGroupEntry);

    if (pDescriptorSet->mWgp.mDynamicOffsetCount)
    {
        pDescriptorSet->mWgp.pDynamicUniformData = (DynamicUniformData*)pMem;
        pMem += pDescriptorSet->mWgp.mMaxSets * pDescriptorSet->mWgp.mDynamicOffsetCount * sizeof(DynamicUniformData);
    }

    uint32_t staticSamplerCount = 0;

    for (uint32_t descIndex = 0; descIndex < pRootSignature->mDescriptorCount; ++descIndex)
    {
        const DescriptorInfo* descInfo = &pRootSignature->pDescriptors[descIndex];
        const bool            staticSampler = pRenderer->pGpu->mWgp.mStaticSamplers && descInfo->mStaticSampler;
        if (descInfo->mUpdateFrequency != (uint32_t)updateFreq || DESCRIPTOR_TYPE_ROOT_CONSTANT == descInfo->mType || staticSampler)
        {
            continue;
        }

        DescriptorType type = (DescriptorType)descInfo->mType;

        for (uint32_t index = 0; index < pDesc->mMaxSets; ++index)
        {
            DECL_CHAINED_STRUCT(WGPUBindGroupEntry, entry);
            entry.binding = descInfo->mWgp.mReg;

            switch (type)
            {
            case DESCRIPTOR_TYPE_SAMPLER:
            {
                if (descInfo->mStaticSampler)
                {
                    entry.sampler = pRootSignature->mWgp.pStaticSamplers[staticSamplerCount];
                }
                else
                {
                    if (descInfo->mSize > 1)
                    {
#if defined(WEBGPU_NATIVE)
                        WGPUBindGroupEntryExtras* entryExt = (WGPUBindGroupEntryExtras*)pMem;
                        entryExt->chain.sType = (WGPUSType)WGPUSType_BindGroupEntryExtras;
                        ADD_TO_NEXT_CHAIN(entry, entryExt);
                        pMem += sizeof(WGPUBindGroupEntryExtras);
                        entryExt->samplers = (WGPUSampler*)pMem;
                        entryExt->samplerCount = descInfo->mSize;
                        pMem += sizeof(WGPUSampler) * descInfo->mSize;

                        for (uint32_t arr = 0; arr < descInfo->mSize; ++arr)
                        {
                            ((WGPUSampler*)entryExt->samplers)[arr] = pRenderer->pNullDescriptors->pDefaultSampler->mWgp.pSampler;
                        }
#else
                        ASSERTFAIL("WebGpu Dawn - Arrays Not supported");
#endif
                    }
                    else
                    {
                        entry.sampler = pRenderer->pNullDescriptors->pDefaultSampler->mWgp.pSampler;
                    }
                }
                break;
            }
            case DESCRIPTOR_TYPE_TEXTURE:
            case DESCRIPTOR_TYPE_RW_TEXTURE:
            {
                WGPUTextureView view = (type == DESCRIPTOR_TYPE_RW_TEXTURE)
                                           ? pRenderer->pNullDescriptors->pDefaultTextureUAV[descInfo->mDim]->mWgp.pUavs[0]
                                           : pRenderer->pNullDescriptors->pDefaultTextureSRV[descInfo->mDim]->mWgp.pSrv;
                if (descInfo->mSize > 1)
                {
#if defined(WEBGPU_NATIVE)
                    WGPUBindGroupEntryExtras* entryExt = (WGPUBindGroupEntryExtras*)pMem;
                    entryExt->chain.sType = (WGPUSType)WGPUSType_BindGroupEntryExtras;
                    ADD_TO_NEXT_CHAIN(entry, entryExt);
                    pMem += sizeof(WGPUBindGroupEntryExtras);
                    entryExt->textureViews = (WGPUTextureView*)pMem;
                    entryExt->textureViewCount = descInfo->mSize;
                    pMem += sizeof(WGPUTextureView) * descInfo->mSize;

                    for (uint32_t arr = 0; arr < descInfo->mSize; ++arr)
                    {
                        ((WGPUTextureView*)entryExt->textureViews)[arr] = view;
                    }
#else
                    ASSERTFAIL("WebGpu Dawn - Arrays Not supported");
#endif
                }
                else
                {
                    entry.textureView = view;
                }

                break;
            }
            case DESCRIPTOR_TYPE_BUFFER:
            case DESCRIPTOR_TYPE_BUFFER_RAW:
            case DESCRIPTOR_TYPE_RW_BUFFER:
            case DESCRIPTOR_TYPE_RW_BUFFER_RAW:
            case DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            {
                Buffer* buffer = pRenderer->pNullDescriptors->pDefaultBufferSRV;
                if (descInfo->mSize > 1)
                {
#if defined(WEBGPU_NATIVE)
                    WGPUBindGroupEntryExtras* entryExt = (WGPUBindGroupEntryExtras*)pMem;
                    entryExt->chain.sType = (WGPUSType)WGPUSType_BindGroupEntryExtras;
                    ADD_TO_NEXT_CHAIN(entry, entryExt);
                    pMem += sizeof(WGPUBindGroupEntryExtras);
                    entryExt->buffers = (WGPUBuffer*)pMem;
                    entryExt->bufferCount = descInfo->mSize;
                    pMem += sizeof(WGPUBuffer) * descInfo->mSize;

                    for (uint32_t arr = 0; arr < descInfo->mSize; ++arr)
                    {
                        ((WGPUBuffer*)entryExt->buffers)[arr] = buffer->mWgp.pBuffer;
                    }
                    entry.size = buffer->mSize;
#else
                    ASSERTFAIL("WebGpu Dawn - Arrays Not supported");
#endif
                }
                else
                {
                    entry.buffer = buffer->mWgp.pBuffer;
                    entry.size = buffer->mSize;
                }
                break;
            }
            default:
                break;
            }

            WGPUBindGroupEntry* entryP = pDescriptorSet->mWgp.pEntries + (index * descriptorCount) + descInfo->mHandleIndex;
            *entryP = entry;
        }

        if (descInfo->mStaticSampler)
        {
            ++staticSamplerCount;
        }
    }

    *ppDescriptorSet = pDescriptorSet;
}

void wgpu_removeDescriptorSet(Renderer* pRenderer, DescriptorSet* pDescriptorSet)
{
    ASSERT(pRenderer);
    ASSERT(pDescriptorSet);

    for (uint32_t i = 0; i < pDescriptorSet->mWgp.mMaxSets; ++i)
    {
        if (pDescriptorSet->mWgp.pHandles[i])
        {
            wgpuBindGroupRelease(pDescriptorSet->mWgp.pHandles[i]);
        }
    }

    SAFE_FREE(pDescriptorSet);
}

#if defined(ENABLE_GRAPHICS_DEBUG) || defined(PVS_STUDIO)
#define VALIDATE_DESCRIPTOR(descriptor, msgFmt, ...)                           \
    if (!VERIFYMSG((descriptor), "%s : " msgFmt, __FUNCTION__, ##__VA_ARGS__)) \
    {                                                                          \
        continue;                                                              \
    }
#else
#define VALIDATE_DESCRIPTOR(descriptor, ...)
#endif

void wgpu_updateDescriptorSet(Renderer* pRenderer, uint32_t index, DescriptorSet* pDescriptorSet, uint32_t count,
                              const DescriptorData* pParams)
{
    ASSERT(pRenderer);
    ASSERT(pDescriptorSet);
    ASSERT(pDescriptorSet->mWgp.pHandles);
    ASSERT(index < pDescriptorSet->mWgp.mMaxSets);

    const RootSignature* pRootSignature = pDescriptorSet->mWgp.pRootSignature;
    WGPUBindGroupEntry*  entries = pDescriptorSet->mWgp.pEntries + (index * pDescriptorSet->mWgp.mEntryCount);

    for (uint32_t i = 0; i < count; ++i)
    {
        const DescriptorData* pParam = pParams + i;
        uint32_t              paramIndex = pParam->mBindByIndex ? pParam->mIndex : UINT32_MAX;

        VALIDATE_DESCRIPTOR(pParam->pName || (paramIndex != UINT32_MAX), "DescriptorData has NULL name and invalid index");

        const DescriptorInfo* pDesc =
            (paramIndex != UINT32_MAX) ? (pRootSignature->pDescriptors + paramIndex) : GetDescriptor(pRootSignature, pParam->pName);
        if (paramIndex != UINT32_MAX)
        {
            VALIDATE_DESCRIPTOR(pDesc, "Invalid descriptor with param index (%u)", paramIndex);
        }
        else
        {
            VALIDATE_DESCRIPTOR(pDesc, "Invalid descriptor with param name (%s)", pParam->pName ? pParam->pName : "<NULL>");
        }

        const DescriptorType type = (DescriptorType)pDesc->mType; //-V522
        const uint32_t       arrayCount = max(1U, pParam->mCount);
#if defined(WEBGPU_NATIVE)
        const uint32_t arrayStart = pParam->mArrayOffset;
#endif

        WGPUBindGroupEntry* entry = &entries[pDesc->mHandleIndex];

        VALIDATE_DESCRIPTOR(pDesc->mUpdateFrequency == pDescriptorSet->mWgp.mUpdateFrequency,
                            "Descriptor (%s) - Mismatching update frequency and set index", pDesc->pName);

        switch (type)
        {
        case DESCRIPTOR_TYPE_SAMPLER:
        {
            // Index is invalid when descriptor is a static sampler
            VALIDATE_DESCRIPTOR(
                !pDesc->mStaticSampler,
                "Trying to update a static sampler (%s). All static samplers must be set in addRootSignature and cannot be updated "
                "later",
                pDesc->pName);

            VALIDATE_DESCRIPTOR(pParam->ppSamplers, "NULL Sampler (%s)", pDesc->pName);

            if (arrayCount > 1)
            {
#if defined(WEBGPU_NATIVE)
                WGPUBindGroupEntryExtras* entryExt = (WGPUBindGroupEntryExtras*)entry->nextInChain;
                for (uint32_t arr = 0; arr < arrayCount; ++arr)
                {
                    VALIDATE_DESCRIPTOR(pParam->ppSamplers[arr], "NULL Sampler (%s [%u] )", pDesc->pName, arr);
                    ((WGPUSampler*)entryExt->samplers)[arrayStart + arr] = pParam->ppSamplers[arr]->mWgp.pSampler;
                }
#else
                ASSERTFAIL("WebGpu Dawn - Arrays Not supported");
#endif
            }
            else
            {
                VALIDATE_DESCRIPTOR(pParam->ppSamplers[0], "NULL Sampler (%s [%u] )", pDesc->pName, 0);
                entry->sampler = pParam->ppSamplers[0]->mWgp.pSampler;
            }
            break;
        }
        case DESCRIPTOR_TYPE_TEXTURE:
        {
            VALIDATE_DESCRIPTOR(pParam->ppTextures, "NULL Texture (%s)", pDesc->pName);

            if (!pParam->mBindStencilResource)
            {
                if (arrayCount > 1)
                {
#if defined(WEBGPU_NATIVE)
                    WGPUBindGroupEntryExtras* entryExt = (WGPUBindGroupEntryExtras*)entry->nextInChain;
                    for (uint32_t arr = 0; arr < arrayCount; ++arr)
                    {
                        VALIDATE_DESCRIPTOR(pParam->ppTextures[arr], "NULL Texture (%s [%u] )", pDesc->pName, arr);
                        ((WGPUTextureView*)entryExt->textureViews)[arrayStart + arr] = pParam->ppTextures[arr]->mWgp.pSrv;
                    }
#else
                    ASSERTFAIL("WebGpu Dawn - Arrays Not supported");
#endif
                }
                else
                {
                    VALIDATE_DESCRIPTOR(pParam->ppTextures[0], "NULL Texture (%s [%u] )", pDesc->pName, 0);
                    entry->textureView = pParam->ppTextures[0]->mWgp.pSrv;
                }
            }
            else
            {
                if (arrayCount > 1)
                {
#if defined(WEBGPU_NATIVE)
                    WGPUBindGroupEntryExtras* entryExt = (WGPUBindGroupEntryExtras*)entry->nextInChain;
                    for (uint32_t arr = 0; arr < arrayCount; ++arr)
                    {
                        VALIDATE_DESCRIPTOR(pParam->ppTextures[arr], "NULL Texture (%s [%u] )", pDesc->pName, arr);
                        ((WGPUTextureView*)entryExt->textureViews)[arrayStart + arr] = pParam->ppTextures[arr]->mWgp.pSrvStencil;
                    }
#else
                    ASSERTFAIL("WebGpu Dawn - Arrays Not supported");
#endif
                }
                else
                {
                    VALIDATE_DESCRIPTOR(pParam->ppTextures[0], "NULL Texture (%s [%u] )", pDesc->pName, 0);
                    entry->textureView = pParam->ppTextures[0]->mWgp.pSrvStencil;
                }
            }
            break;
        }
        case DESCRIPTOR_TYPE_RW_TEXTURE:
        {
            VALIDATE_DESCRIPTOR(pParam->ppTextures, "NULL RW Texture (%s)", pDesc->pName);

            if (pParam->mBindMipChain)
            {
#if defined(WEBGPU_NATIVE)
                VALIDATE_DESCRIPTOR(pParam->ppTextures[0], "NULL RW Texture (%s)", pDesc->pName);
                VALIDATE_DESCRIPTOR(
                    (!arrayStart),
                    "Descriptor (%s) - mBindMipChain supports only updating the whole mip-chain. No partial updates supported",
                    pParam->pName ? pParam->pName : "<NULL>");
                const uint32_t mipCount = pParam->ppTextures[0]->mMipLevels;

                WGPUBindGroupEntryExtras* entryExt = (WGPUBindGroupEntryExtras*)entry->nextInChain;
                for (uint32_t arr = 0; arr < mipCount; ++arr)
                {
                    ((WGPUTextureView*)entryExt->textureViews)[arrayStart + arr] = pParam->ppTextures[0]->mWgp.pUavs[arr];
                }
#else
                ASSERTFAIL("WebGpu Dawn - Arrays Not supported");
#endif
            }
            else
            {
                const uint32_t mipSlice = pParam->mUAVMipSlice;

                if (arrayCount > 1)
                {
#if defined(WEBGPU_NATIVE)
                    WGPUBindGroupEntryExtras* entryExt = (WGPUBindGroupEntryExtras*)entry->nextInChain;
                    for (uint32_t arr = 0; arr < arrayCount; ++arr)
                    {
                        VALIDATE_DESCRIPTOR(pParam->ppTextures[arr], "NULL RW Texture (%s [%u] )", pDesc->pName, arr);
                        VALIDATE_DESCRIPTOR(mipSlice < pParam->ppTextures[arr]->mMipLevels,
                                            "Descriptor : (%s [%u] ) Mip Slice (%u) exceeds mip levels (%u)", pDesc->pName, arr, mipSlice,
                                            pParam->ppTextures[arr]->mMipLevels);
                        ((WGPUTextureView*)entryExt->textureViews)[arrayStart + arr] = pParam->ppTextures[arr]->mWgp.pUavs[mipSlice];
                    }
#else
                    ASSERTFAIL("WebGpu Dawn - Arrays Not supported");
#endif
                }
                else
                {
                    VALIDATE_DESCRIPTOR(pParam->ppTextures[0], "NULL RW Texture (%s [%u] )", pDesc->pName, 0);
                    VALIDATE_DESCRIPTOR(mipSlice < pParam->ppTextures[0]->mMipLevels,
                                        "Descriptor : (%s [%u] ) Mip Slice (%u) exceeds mip levels (%u)", pDesc->pName, 0, mipSlice,
                                        pParam->ppTextures[0]->mMipLevels);
                    entry->textureView = pParam->ppTextures[0]->mWgp.pUavs[mipSlice];
                }
            }
            break;
        }
        case DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        {
            if (pDesc->mRootDescriptor)
            {
                VALIDATE_DESCRIPTOR(false,
                                    "Descriptor (%s) - Trying to update a root cbv through updateDescriptorSet. All root cbvs must be "
                                    "updated through cmdBindDescriptorSetWithRootCbvs",
                                    pDesc->pName);

                break;
            }
        case DESCRIPTOR_TYPE_BUFFER:
        case DESCRIPTOR_TYPE_BUFFER_RAW:
        case DESCRIPTOR_TYPE_RW_BUFFER:
        case DESCRIPTOR_TYPE_RW_BUFFER_RAW:
        {
            VALIDATE_DESCRIPTOR(pParam->ppBuffers, "NULL Buffer (%s)", pDesc->pName);

            if (arrayCount > 1)
            {
#if defined(WEBGPU_NATIVE)
                WGPUBindGroupEntryExtras* entryExt = (WGPUBindGroupEntryExtras*)entry->nextInChain;
                for (uint32_t arr = 0; arr < arrayCount; ++arr)
                {
                    VALIDATE_DESCRIPTOR(pParam->ppBuffers[arr], "NULL Buffer (%s [%u] )", pDesc->pName, arr);

                    if (pParam->pRanges)
                    {
                        ASSERTFAIL("WebGpu - Buffer Array offsets Not supported");
                    }

                    ((WGPUBuffer*)entryExt->buffers)[arrayStart + arr] = pParam->ppBuffers[arr]->mWgp.pBuffer;
                }
#else
                ASSERTFAIL("WebGpu Dawn - Arrays Not supported");
#endif
            }
            else
            {
                VALIDATE_DESCRIPTOR(pParam->ppBuffers[0], "NULL Buffer (%s [%u] )", pDesc->pName, 0);

                if (pParam->pRanges)
                {
                    DescriptorDataRange range = pParam->pRanges[0];
#if defined(ENABLE_GRAPHICS_DEBUG)
                    uint64_t maxRange = DESCRIPTOR_TYPE_UNIFORM_BUFFER == type ? pRenderer->pGpu->mWgp.mLimits.maxUniformBufferBindingSize
                                                                               : pRenderer->pGpu->mWgp.mLimits.maxStorageBufferBindingSize;
                    VALIDATE_DESCRIPTOR(range.mSize <= maxRange, "Descriptor (%s) - pRanges[%u].mSize is %ull which exceeds max size %u",
                                        pDesc->pName, 0, range.mSize, maxRange);
#endif

                    VALIDATE_DESCRIPTOR(range.mSize, "Descriptor (%s) - pRanges[%u].mSize is zero", pDesc->pName, 0);

                    entry->buffer = pParam->ppBuffers[0]->mWgp.pBuffer;
                    entry->offset = range.mOffset;
                    entry->size = range.mSize;
                }
                else
                {
                    entry->buffer = pParam->ppBuffers[0]->mWgp.pBuffer;
                    entry->offset = 0;
                    entry->size = pParam->ppBuffers[0]->mSize;
                }
            }
        }
        break;
        }
        default:
            break;
        }
    }

    if (pDescriptorSet->mWgp.pHandles[index])
    {
        wgpuBindGroupRelease(pDescriptorSet->mWgp.pHandles[index]);
    }

    WGPUBindGroupDescriptor bindDesc = {};
    bindDesc.entries = entries;
    bindDesc.entryCount = pDescriptorSet->mWgp.mEntryCount;
    bindDesc.layout = pRootSignature->mWgp.mDescriptorSetLayouts[pDescriptorSet->mWgp.mUpdateFrequency];
    pDescriptorSet->mWgp.pHandles[index] = wgpuDeviceCreateBindGroup(pRenderer->mWgp.pDevice, &bindDesc);
    ASSERT(pDescriptorSet->mWgp.pHandles[index]);
}

static void SetBindGroup(Cmd* pCmd, uint32_t index, DescriptorSet* pDescriptorSet, uint32_t* offsets)
{
    const RootSignature* pRootSignature = pDescriptorSet->mWgp.pRootSignature;
    if (pCmd->mWgp.pBoundPipelineLayout != pRootSignature->mWgp.pPipelineLayout)
    {
        pCmd->mWgp.pBoundPipelineLayout = pRootSignature->mWgp.pPipelineLayout;

        // Vulkan requires to bind all descriptor sets upto the highest set number even if they are empty
        // Example: If shader uses only set 2, we still have to bind empty sets for set=0 and set=1
        for (uint32_t setIndex = 0; setIndex < DESCRIPTOR_UPDATE_FREQ_COUNT; ++setIndex)
        {
            if (pRootSignature->mWgp.mDescriptorSetLayouts[setIndex] == pCmd->pRenderer->mWgp.pEmptyDescriptorSetLayout)
            {
                if (pCmd->mWgp.mInsideComputePass)
                {
                    wgpuComputePassEncoderSetBindGroup(pCmd->mWgp.pComputeEncoder, setIndex, pCmd->pRenderer->mWgp.pEmptyDescriptorSet, 0,
                                                       NULL);
                }
                else if (pCmd->mWgp.mInsideRenderPass)
                {
                    wgpuRenderPassEncoderSetBindGroup(pCmd->mWgp.pRenderEncoder, setIndex, pCmd->pRenderer->mWgp.pEmptyDescriptorSet, 0,
                                                      NULL);
                }
            }
        }

        if (pRootSignature->mWgp.mStaticSamplersOnly)
        {
            if (pCmd->mWgp.mInsideComputePass)
            {
                wgpuComputePassEncoderSetBindGroup(pCmd->mWgp.pComputeEncoder, 0, pRootSignature->mWgp.pStaticSamplerSet, 0, NULL);
            }
            else if (pCmd->mWgp.mInsideRenderPass)
            {
                wgpuRenderPassEncoderSetBindGroup(pCmd->mWgp.pRenderEncoder, 0, pRootSignature->mWgp.pStaticSamplerSet, 0, NULL);
            }
        }
    }

    if (pCmd->mWgp.mInsideComputePass)
    {
        wgpuComputePassEncoderSetBindGroup(pCmd->mWgp.pComputeEncoder, pDescriptorSet->mWgp.mUpdateFrequency,
                                           pDescriptorSet->mWgp.pHandles[index], pDescriptorSet->mWgp.mDynamicOffsetCount, offsets);
    }
    else if (pCmd->mWgp.mInsideRenderPass)
    {
        wgpuRenderPassEncoderSetBindGroup(pCmd->mWgp.pRenderEncoder, pDescriptorSet->mWgp.mUpdateFrequency,
                                          pDescriptorSet->mWgp.pHandles[index], pDescriptorSet->mWgp.mDynamicOffsetCount, offsets);
    }
    else
    {
        ASSERTFAIL("Encoder needs to be set before calling cmdBindDescriptorSet");
    }
}

static const uint32_t WGP_MAX_ROOT_DESCRIPTORS = 32;

void wgpu_cmdBindDescriptorSet(Cmd* pCmd, uint32_t index, DescriptorSet* pDescriptorSet)
{
    ASSERT(pCmd);

    static uint32_t offsets[WGP_MAX_ROOT_DESCRIPTORS] = {};
    SetBindGroup(pCmd, index, pDescriptorSet, offsets);
}

void wgpu_cmdBindPushConstants(Cmd* pCmd, RootSignature* pRootSignature, uint32_t paramIndex, const void* pConstants)
{
    ASSERT(pCmd);
    ASSERT(pConstants);
    ASSERT(pRootSignature);
    ASSERT(paramIndex >= 0 && paramIndex < pRootSignature->mDescriptorCount);
    ASSERT(pCmd->mWgp.mInsideRenderPass);

#if defined(WEBGPU_NATIVE)
    const DescriptorInfo* pDesc = pRootSignature->pDescriptors + paramIndex;
    ASSERT(pDesc);
    ASSERT(DESCRIPTOR_TYPE_ROOT_CONSTANT == pDesc->mType);

    wgpuRenderPassEncoderSetPushConstants(pCmd->mWgp.pRenderEncoder, pDesc->mWgp.mStages, 0, pDesc->mSize, pConstants);
#else
    ASSERTFAIL("WebGpu Dawn - Push constants not supported");
#endif
}

void wgpu_cmdBindDescriptorSetWithRootCbvs(Cmd* pCmd, uint32_t index, DescriptorSet* pDescriptorSet, uint32_t count,
                                           const DescriptorData* pParams)
{
    ASSERT(pCmd);
    ASSERT(pDescriptorSet);
    ASSERT(pParams);

    const RootSignature* pRootSignature = pDescriptorSet->mWgp.pRootSignature;
    uint32_t             offsets[WGP_MAX_ROOT_DESCRIPTORS] = {};
    WGPUBindGroupEntry*  entries = pDescriptorSet->mWgp.pEntries + (index * pDescriptorSet->mWgp.mEntryCount);
    bool                 needUpdate = false;

    for (uint32_t i = 0; i < count; ++i)
    {
        const DescriptorData* pParam = pParams + i;
        uint32_t              paramIndex = pParam->mBindByIndex ? pParam->mIndex : UINT32_MAX;

        const DescriptorInfo* pDesc =
            (paramIndex != UINT32_MAX) ? (pRootSignature->pDescriptors + paramIndex) : GetDescriptor(pRootSignature, pParam->pName);
        if (paramIndex != UINT32_MAX)
        {
            VALIDATE_DESCRIPTOR(pDesc, "Invalid descriptor with param index (%u)", paramIndex);
        }
        else
        {
            VALIDATE_DESCRIPTOR(pDesc, "Invalid descriptor with param name (%s)", pParam->pName);
        }

#if defined(ENABLE_GRAPHICS_DEBUG)
        const uint64_t maxRange = DESCRIPTOR_TYPE_UNIFORM_BUFFER == pDesc->mType
                                      ? //-V522
                                      pCmd->pRenderer->pGpu->mWgp.mLimits.maxUniformBufferBindingSize
                                      : pCmd->pRenderer->pGpu->mWgp.mLimits.maxStorageBufferBindingSize;
#endif

        VALIDATE_DESCRIPTOR(pDesc->mRootDescriptor, "Descriptor (%s) - must be a root cbv", pDesc->pName);
        VALIDATE_DESCRIPTOR(pParam->mCount <= 1, "Descriptor (%s) - cmdBindDescriptorSetWithRootCbvs does not support arrays",
                            pDesc->pName);
        VALIDATE_DESCRIPTOR(pParam->pRanges, "Descriptor (%s) - pRanges must be provided for cmdBindDescriptorSetWithRootCbvs",
                            pDesc->pName);

        DescriptorDataRange range = pParam->pRanges[0];
        VALIDATE_DESCRIPTOR(range.mSize, "Descriptor (%s) - pRanges->mSize is zero", pDesc->pName);
#if defined(ENABLE_GRAPHICS_DEBUG)
        VALIDATE_DESCRIPTOR(range.mSize <= maxRange, "Descriptor (%s) - pRanges->mSize is %ull which exceeds max size %u", pDesc->pName,
                            range.mSize, maxRange);
#endif

        const uint32_t dynamicOffsetHandleIndex = pDesc->mHandleIndex - pDescriptorSet->mWgp.mDynamicOffsetHandleIndex;
        offsets[dynamicOffsetHandleIndex] = range.mOffset; //-V522
        DynamicUniformData* pData =
            &pDescriptorSet->mWgp.pDynamicUniformData[index * pDescriptorSet->mWgp.mDynamicOffsetCount + dynamicOffsetHandleIndex];
        if (pData->pBuffer != pParam->ppBuffers[0]->mWgp.pBuffer || range.mSize != pData->mSize)
        {
            *pData = { pParam->ppBuffers[0]->mWgp.pBuffer, 0, range.mSize };

            WGPUBindGroupEntry* entry = &entries[pDesc->mHandleIndex];
            entry->buffer = pData->pBuffer;
            entry->offset = 0;
            entry->size = pData->mSize;
            needUpdate = true;
        }
    }

    if (needUpdate)
    {
        if (pDescriptorSet->mWgp.pHandles[index])
        {
            wgpuBindGroupRelease(pDescriptorSet->mWgp.pHandles[index]);
        }

        WGPUBindGroupDescriptor bindDesc = {};
        bindDesc.entries = entries;
        bindDesc.entryCount = pDescriptorSet->mWgp.mEntryCount;
        bindDesc.layout = pRootSignature->mWgp.mDescriptorSetLayouts[pDescriptorSet->mWgp.mUpdateFrequency];
        pDescriptorSet->mWgp.pHandles[index] = wgpuDeviceCreateBindGroup(pCmd->pRenderer->mWgp.pDevice, &bindDesc);
        ASSERT(pDescriptorSet->mWgp.pHandles[index]);
    }

    SetBindGroup(pCmd, index, pDescriptorSet, offsets);
}
/************************************************************************/
// Shader Functions
/************************************************************************/
void wgpu_addShaderBinary(Renderer* pRenderer, const BinaryShaderDesc* pDesc, Shader** ppShaderProgram)
{
    ASSERT(pRenderer);
    ASSERT(pDesc);
    ASSERT(ppShaderProgram);

    uint32_t counter = 0;

    size_t totalSize = sizeof(Shader);
    totalSize += sizeof(PipelineReflection);

    for (uint32_t i = 0; i < SHADER_STAGE_COUNT; ++i)
    {
        ShaderStage stageMask = (ShaderStage)(1 << i);
        if (stageMask == (pDesc->mStages & stageMask))
        {
            ++counter;
        }
    }

    totalSize += counter * sizeof(WGPUShaderModule);
    totalSize += counter * sizeof(char*);
    Shader* pShaderProgram = (Shader*)tf_calloc(1, totalSize);
    pShaderProgram->mStages = pDesc->mStages;
    pShaderProgram->pReflection = (PipelineReflection*)(pShaderProgram + 1); //-V1027
    pShaderProgram->mWgp.pShaderModules = (WGPUShaderModule*)(pShaderProgram->pReflection + 1);

    ShaderReflection stageReflections[SHADER_STAGE_COUNT] = {};
    counter = 0;

    for (uint32_t i = 0; i < SHADER_STAGE_COUNT; ++i)
    {
        ShaderStage stageMask = (ShaderStage)(1 << i);
        if (stageMask == (pShaderProgram->mStages & stageMask))
        {
            const BinaryShaderStageDesc* stageDesc = NULL;
            WGPUShaderStage              stage = WGPUShaderStage_Force32;
            switch (stageMask)
            {
            case SHADER_STAGE_VERT:
            {
                stageDesc = &pDesc->mVert;
                stage = WGPUShaderStage_Vertex;
            }
            break;
            case SHADER_STAGE_FRAG:
            {
                stageDesc = &pDesc->mFrag;
                stage = WGPUShaderStage_Fragment;
            }
            break;
            case SHADER_STAGE_COMP:
            {
                stageDesc = &pDesc->mComp;
                stage = WGPUShaderStage_Compute;
            }
            break;
            default:
                ASSERT(false && "Shader Stage not supported!");
                break;
            }

            DECL_CHAINED_STRUCT(WGPUShaderModuleDescriptor, moduleDesc);
#if defined(WEBGPU_NATIVE)
            // #NOTE: Spirv doesnt seem to work - Crash inside wgpuDeviceCreateShaderModule
            WGPUShaderModuleGLSLDescriptor glslDesc = {};
            glslDesc.chain.sType = (WGPUSType)WGPUSType_ShaderModuleGLSLDescriptor;
            glslDesc.code = stageDesc->pGlsl;
            glslDesc.stage = stage;
            ADD_TO_NEXT_CHAIN(moduleDesc, &glslDesc);
#elif defined(WEBGPU_DAWN)
            WGPUShaderModuleSPIRVDescriptor spvDesc = {};
            spvDesc.chain.sType = (WGPUSType)WGPUSType_ShaderModuleSPIRVDescriptor;
            spvDesc.code = (uint32_t*)stageDesc->pByteCode;
            spvDesc.codeSize = stageDesc->mByteCodeSize / sizeof(uint32_t);
            ADD_TO_NEXT_CHAIN(moduleDesc, &spvDesc);
#endif
            pShaderProgram->mWgp.pShaderModules[counter] = wgpuDeviceCreateShaderModule(pRenderer->mWgp.pDevice, &moduleDesc);

            extern void vk_createShaderReflection(const uint8_t* shaderCode, uint32_t shaderSize, ShaderStage shaderStage,
                                                  ShaderReflection* pOutReflection);
            vk_createShaderReflection((const uint8_t*)stageDesc->pByteCode, (uint32_t)stageDesc->mByteCodeSize, stageMask,
                                      &stageReflections[counter]);

            ++counter;
        }
    }

    createPipelineReflection(stageReflections, counter, pShaderProgram->pReflection);

    *ppShaderProgram = pShaderProgram;
}

void wgpu_removeShader(Renderer* pRenderer, Shader* pShaderProgram)
{
    ASSERT(pRenderer);

    if (pShaderProgram->mStages & SHADER_STAGE_VERT)
    {
        wgpuShaderModuleRelease(pShaderProgram->mWgp.pShaderModules[pShaderProgram->pReflection->mVertexStageIndex]);
    }
    if (pShaderProgram->mStages & SHADER_STAGE_FRAG)
    {
        wgpuShaderModuleRelease(pShaderProgram->mWgp.pShaderModules[pShaderProgram->pReflection->mPixelStageIndex]);
    }
    if (pShaderProgram->mStages & SHADER_STAGE_COMP)
    {
        wgpuShaderModuleRelease(pShaderProgram->mWgp.pShaderModules[0]);
    }

    destroyPipelineReflection(pShaderProgram->pReflection);

    SAFE_FREE(pShaderProgram);
}
/************************************************************************/
// Root Signature Functions
/************************************************************************/
static inline WGPUShaderStageFlags ToShaderStageFlags(ShaderStage stages)
{
    WGPUShaderStageFlags res = WGPUShaderStage_None;
    if (SHADER_STAGE_ALL_GRAPHICS == (stages & SHADER_STAGE_ALL_GRAPHICS))
        return WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;

    if (stages & SHADER_STAGE_VERT)
    {
        res |= WGPUShaderStage_Vertex;
    }
    if (stages & SHADER_STAGE_FRAG)
    {
        res |= WGPUShaderStage_Fragment;
    }
    if (stages & SHADER_STAGE_COMP)
    {
        res |= WGPUShaderStage_Compute;
    }

    ASSERT(res != WGPUShaderStage_None);
    return res;
}

static inline WGPUTextureViewDimension ToTextureViewDim(TextureDimension dim)
{
    WGPUTextureViewDimension ret = WGPUTextureViewDimension_Force32;
    switch (dim)
    {
    case TEXTURE_DIM_1D:
        ret = WGPUTextureViewDimension_1D;
        break;
    case TEXTURE_DIM_2D:
        ret = WGPUTextureViewDimension_2D;
        break;
        break;
    case TEXTURE_DIM_2DMS:
        ret = WGPUTextureViewDimension_2D;
        break;
    case TEXTURE_DIM_3D:
        ret = WGPUTextureViewDimension_3D;
        break;
    case TEXTURE_DIM_CUBE:
        ret = WGPUTextureViewDimension_Cube;
        break;
    case TEXTURE_DIM_1D_ARRAY:
        ASSERTFAIL("TEXTURE_DIM_1D_ARRAY Not supported");
        break;
    case TEXTURE_DIM_2D_ARRAY:
        ret = WGPUTextureViewDimension_2DArray;
        break;
    case TEXTURE_DIM_2DMS_ARRAY:
        ret = WGPUTextureViewDimension_2DArray;
        break;
    case TEXTURE_DIM_CUBE_ARRAY:
        ret = WGPUTextureViewDimension_CubeArray;
        break;
    default:
        ret = WGPUTextureViewDimension_Force32;
        break;
    }

    return ret;
}

static inline bool IsMultisampled(TextureDimension dim)
{
    bool ret = false;
    if (dim == TEXTURE_DIM_2DMS || dim == TEXTURE_DIM_2DMS_ARRAY)
    {
        ret = true;
    }

    return ret;
}

static inline FORGE_CONSTEXPR WGPUStorageTextureAccess ToTextureAccess(TextureAccess access)
{
    switch (access)
    {
    case TEXTURE_ACCESS_READONLY:
        return WGPUStorageTextureAccess_ReadOnly;
    case TEXTURE_ACCESS_WRITEONLY:
        return WGPUStorageTextureAccess_WriteOnly;
    case TEXTURE_ACCESS_READWRITE:
        return WGPUStorageTextureAccess_ReadWrite;
    default:
        return WGPUStorageTextureAccess_Force32;
    }
}

static inline WGPUTextureSampleType ToTextureSampleType(TinyImageFormat fmt)
{
    if (TinyImageFormat_UNDEFINED == fmt || TinyImageFormat_IsFloat(fmt))
    {
        return WGPUTextureSampleType_Float;
    }
    else if (TinyImageFormat_IsSigned(fmt))
    {
        return WGPUTextureSampleType_Sint;
    }
    else
    {
        return WGPUTextureSampleType_Uint;
    }
}

typedef struct wgp_UpdateFrequencyLayoutInfo
{
    /// Array of all bindings in the descriptor set
    WGPUBindGroupLayoutEntry* mBindings = NULL;
    /// Array of all descriptors in this descriptor set
    DescriptorInfo**          mDescriptors = NULL;
    /// Array of all descriptors marked as dynamic in this descriptor set (applicable to DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    DescriptorInfo**          mDynamicDescriptors = NULL;
} wgp_UpdateFrequencyLayoutInfo;

static bool compareWGPUBindGroupLayoutEntry(const WGPUBindGroupLayoutEntry* pLhs, const WGPUBindGroupLayoutEntry* pRhs)
{
    if (pRhs->binding < pLhs->binding)
    {
        return true;
    }
    return false;
}

typedef DescriptorInfo* PDescriptorInfo;
static bool             compareWDescriptorInfo(const PDescriptorInfo* pLhs, const PDescriptorInfo* pRhs)
{
    DescriptorInfo* lhs = *pLhs;
    DescriptorInfo* rhs = *pRhs;
    if (lhs->mRootDescriptor != rhs->mRootDescriptor)
    {
        return lhs->mRootDescriptor ? false : true;
    }
    return lhs->mWgp.mReg < rhs->mWgp.mReg;
}

DEFINE_SIMPLE_SORT_FUNCTION(static, simpleSortWGPUBindGroupLayoutEntry, WGPUBindGroupLayoutEntry, compareWGPUBindGroupLayoutEntry)
DEFINE_INSERTION_SORT_FUNCTION(static, stableSortWGPUBindGroupLayoutEntry, WGPUBindGroupLayoutEntry, compareWGPUBindGroupLayoutEntry,
                               simpleSortWGPUBindGroupLayoutEntry)
DEFINE_PARTITION_IMPL_FUNCTION(static, partitionImplWGPUBindGroupLayoutEntry, WGPUBindGroupLayoutEntry, compareWGPUBindGroupLayoutEntry)
DEFINE_QUICK_SORT_IMPL_FUNCTION(static, quickSortImplWGPUBindGroupLayoutEntry, WGPUBindGroupLayoutEntry, compareWGPUBindGroupLayoutEntry,
                                stableSortWGPUBindGroupLayoutEntry, partitionImplWGPUBindGroupLayoutEntry)
DEFINE_QUICK_SORT_FUNCTION(static, sortWGPUBindGroupLayoutEntry, WGPUBindGroupLayoutEntry, quickSortImplWGPUBindGroupLayoutEntry)

DEFINE_SIMPLE_SORT_FUNCTION(static, simpleSortWDescriptorInfo, PDescriptorInfo, compareWDescriptorInfo)
DEFINE_INSERTION_SORT_FUNCTION(static, stableSortWDescriptorInfo, PDescriptorInfo, compareWDescriptorInfo, simpleSortWDescriptorInfo)

void wgpu_addRootSignature(Renderer* pRenderer, const RootSignatureDesc* pRootSignatureDesc, RootSignature** ppRootSignature)
{
    ASSERT(pRenderer);
    ASSERT(pRootSignatureDesc);
    ASSERT(ppRootSignature);

    typedef struct StaticSamplerNode
    {
        char*    key;
        Sampler* value;
    } StaticSamplerNode;

    static constexpr uint32_t     kMaxLayoutCount = DESCRIPTOR_UPDATE_FREQ_COUNT;
    wgp_UpdateFrequencyLayoutInfo layouts[kMaxLayoutCount] = {};
#if defined(WEBGPU_NATIVE)
    WGPUPushConstantRange pushConstants[SHADER_STAGE_COUNT] = {};
    uint32_t              pushConstantCount = 0;
#endif
    ShaderResource*    shaderResources = NULL;
    StaticSamplerNode* staticSamplerMap = NULL;
    sh_new_arena(staticSamplerMap);

    for (uint32_t i = 0; i < pRootSignatureDesc->mStaticSamplerCount; ++i)
    {
        ASSERT(pRootSignatureDesc->ppStaticSamplers[i]);
        shput(staticSamplerMap, pRootSignatureDesc->ppStaticSamplerNames[i], pRootSignatureDesc->ppStaticSamplers[i]);
    }

    PipelineType        pipelineType = PIPELINE_TYPE_UNDEFINED;
    DescriptorIndexMap* indexMap = NULL;
    sh_new_arena(indexMap);

    // Collect all unique shader resources in the given shaders
    // Resources are parsed by name (two resources named "XYZ" in two shaders will be considered the same resource)
    for (uint32_t sh = 0; sh < pRootSignatureDesc->mShaderCount; ++sh)
    {
        PipelineReflection const* pReflection = pRootSignatureDesc->ppShaders[sh]->pReflection;

        if (pReflection->mShaderStages & SHADER_STAGE_COMP)
            pipelineType = PIPELINE_TYPE_COMPUTE;
        else
            pipelineType = PIPELINE_TYPE_GRAPHICS;

        for (uint32_t i = 0; i < pReflection->mShaderResourceCount; ++i)
        {
            ShaderResource const* pRes = &pReflection->pShaderResources[i];
            DescriptorIndexMap*   pNode = shgetp_null(indexMap, pRes->name);
            if (!pNode)
            {
                ShaderResource* pResource = NULL;
                for (ptrdiff_t r = 0; r < arrlen(shaderResources); ++r)
                {
                    ShaderResource* pCurrent = &shaderResources[r];
                    if (pCurrent->type == pRes->type && (pCurrent->used_stages == pRes->used_stages) &&
                        (((pCurrent->reg ^ pRes->reg) | (pCurrent->set ^ pRes->set)) == 0))
                    {
                        pResource = pCurrent;
                        break;
                    }
                }
                if (!pResource)
                {
                    shput(indexMap, pRes->name, (uint32_t)arrlen(shaderResources));
                    arrpush(shaderResources, *pRes);
                }
                else
                {
                    ASSERT(pRes->type == pResource->type);
                    if (pRes->type != pResource->type)
                    {
                        LOGF(LogLevel::eERROR,
                             "\nFailed to create root signature\n"
                             "Shared shader resources %s and %s have mismatching types (%u) and (%u). All shader resources "
                             "sharing the same register and space addRootSignature "
                             "must have the same type",
                             pRes->name, pResource->name, (uint32_t)pRes->type, (uint32_t)pResource->type);
                        return;
                    }

                    uint32_t value = shget(indexMap, pResource->name);
                    shput(indexMap, pRes->name, value);
                    pResource->used_stages |= pRes->used_stages;
                }
            }
            else
            {
                if (shaderResources[pNode->value].reg != pRes->reg) //-V::522, 595
                {
                    LOGF(LogLevel::eERROR,
                         "\nFailed to create root signature\n"
                         "Shared shader resource %s has mismatching binding. All shader resources "
                         "shared by multiple shaders specified in addRootSignature "
                         "must have the same binding and set",
                         pRes->name);
                    return;
                }
                if (shaderResources[pNode->value].set != pRes->set) //-V::522, 595
                {
                    LOGF(LogLevel::eERROR,
                         "\nFailed to create root signature\n"
                         "Shared shader resource %s has mismatching set. All shader resources "
                         "shared by multiple shaders specified in addRootSignature "
                         "must have the same binding and set",
                         pRes->name);
                    return;
                }

                for (ptrdiff_t r = 0; r < arrlen(shaderResources); ++r)
                {
                    if (strcmp(shaderResources[r].name, pNode->key) == 0)
                    {
                        shaderResources[r].used_stages |= pRes->used_stages;
                        break;
                    }
                }
            }
        }
    }

    uint32_t staticSamplerCount = 0;

    // Fill the descriptor array to be stored in the root signature
    for (ptrdiff_t i = 0; i < arrlen(shaderResources); ++i)
    {
        StaticSamplerNode* staticSamplerMapIter = shgetp_null(staticSamplerMap, shaderResources[i].name);
        if (staticSamplerMapIter)
        {
            ++staticSamplerCount;
        }
    }

    size_t totalSize = sizeof(RootSignature);
    totalSize += arrlenu(shaderResources) * sizeof(DescriptorInfo);
    if (!pRenderer->pGpu->mWgp.mStaticSamplers)
    {
        totalSize += staticSamplerCount * (sizeof(WGPUSampler));
    }
    RootSignature* pRootSignature = (RootSignature*)tf_calloc_memalign(1, alignof(RootSignature), totalSize);
    ASSERT(pRootSignature);

    pRootSignature->pDescriptors = (DescriptorInfo*)(pRootSignature + 1);                                           //-V1027
    pRootSignature->mWgp.pStaticSamplers = (WGPUSampler*)(pRootSignature->pDescriptors + arrlenu(shaderResources)); //-V1027
    pRootSignature->pDescriptorNameToIndexMap = indexMap;
    pRootSignature->mPipelineType = pipelineType;

    if (arrlen(shaderResources))
    {
        pRootSignature->mDescriptorCount = (uint32_t)arrlen(shaderResources);
    }

    uint32_t perStageDescriptorSampledImages = 0;
    staticSamplerCount = 0;

#if defined(WEBGPU_NATIVE)
    WGPUBindGroupLayoutEntryExtras bindingExts[1024] = {};
    uint32_t                       bindingExtCount = 0;
#endif
#if defined(WEBGPU_DAWN)
    WGPUStaticSamplerBindingLayout staticSamplerBindings[1024] = {};
#endif

    // Fill the descriptor array to be stored in the root signature
    for (ptrdiff_t i = 0; i < arrlen(shaderResources); ++i)
    {
        DescriptorInfo*           pDesc = &pRootSignature->pDescriptors[i];
        ShaderResource const*     pRes = &shaderResources[i];
        uint32_t                  setIndex = pRes->set;
        DescriptorUpdateFrequency updateFreq = (DescriptorUpdateFrequency)setIndex;

        // Copy the binding information generated from the shader reflection into the descriptor
        pDesc->mWgp.mReg = pRes->reg;
        pDesc->mSize = pRes->size;
        pDesc->mType = pRes->type;
        pDesc->pName = pRes->name;
        pDesc->mDim = pRes->dim;

        // If descriptor is not a root constant create a new layout binding for this descriptor and add it to the binding array
        if (pDesc->mType != DESCRIPTOR_TYPE_ROOT_CONSTANT)
        {
            DECL_CHAINED_STRUCT(WGPUBindGroupLayoutEntry, binding);
#if defined(WEBGPU_NATIVE)
            WGPUBindGroupLayoutEntryExtras bindingExt = {};
            bindingExt.chain.sType = (WGPUSType)WGPUSType_BindGroupLayoutEntryExtras;
            bindingExt.count = pDesc->mSize;
            if (pDesc->mSize > 1)
            {
                ASSERT(bindingExtCount < TF_ARRAY_COUNT(bindingExts));
                bindingExts[bindingExtCount] = bindingExt;
                ADD_TO_NEXT_CHAIN(binding, &bindingExts[bindingExtCount++]);
            }
#endif
            binding.binding = pRes->reg;

            if (DESCRIPTOR_TYPE_SAMPLER == pDesc->mType)
            {
                // #TODO: Check
                binding.sampler.type = WGPUSamplerBindingType_Filtering;
#if defined(WEBGPU_DAWN)
                if (pRenderer->pGpu->mWgp.mStaticSamplers)
                {
                    // Find if the given descriptor is a static sampler
                    StaticSamplerNode* staticSamplerMapIter = shgetp_null(staticSamplerMap, pDesc->pName);
                    if (staticSamplerMapIter)
                    {
                        ASSERT(staticSamplerCount < TF_ARRAY_COUNT(staticSamplerBindings));
                        ASSERT(pDesc->mUpdateFrequency == DESCRIPTOR_UPDATE_FREQ_NONE);
                        LOGF(LogLevel::eINFO, "Descriptor (%s) : User specified Static Sampler", pDesc->pName);
                        pDesc->mStaticSampler = true;

                        WGPUStaticSamplerBindingLayout staticSamplerBinding = {};
                        staticSamplerBinding.chain.sType = WGPUSType_StaticSamplerBindingLayout;
                        staticSamplerBinding.sampler = staticSamplerMapIter->value->mWgp.pSampler;
                        staticSamplerBindings[staticSamplerCount] = staticSamplerBinding;
                        ADD_TO_NEXT_CHAIN(binding, &staticSamplerBindings[staticSamplerCount++]);

                        binding.sampler = WGPU_SAMPLER_BINDING_LAYOUT_INIT;
                    }
                }
#endif
            }
            else if (DESCRIPTOR_TYPE_UNIFORM_BUFFER == pDesc->mType)
            {
                binding.buffer.minBindingSize = 0;
                binding.buffer.type = WGPUBufferBindingType_Uniform;
            }
            else if (DESCRIPTOR_TYPE_BUFFER == pDesc->mType)
            {
                binding.buffer.minBindingSize = 0;
                binding.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
            }
            else if (DESCRIPTOR_TYPE_RW_BUFFER == pDesc->mType)
            {
                binding.buffer.minBindingSize = 0;
                binding.buffer.type = WGPUBufferBindingType_Storage;
            }
            else if (DESCRIPTOR_TYPE_TEXTURE == pDesc->mType)
            {
                // #TODO: Check
                binding.texture.sampleType = ToTextureSampleType(pRes->format);
                binding.texture.viewDimension = ToTextureViewDim(pRes->dim);
                binding.texture.multisampled = IsMultisampled(pRes->dim);
                if (binding.texture.multisampled)
                {
                    binding.texture.sampleType = WGPUTextureSampleType_UnfilterableFloat;
                }
            }
            else if (DESCRIPTOR_TYPE_RW_TEXTURE == pDesc->mType)
            {
                binding.storageTexture.access = ToTextureAccess(pRes->access);
                binding.storageTexture.format = (WGPUTextureFormat)TinyImageFormat_ToWGPUTextureFormat(pRes->format);
                binding.storageTexture.viewDimension = ToTextureViewDim(pRes->dim);
            }

            if (DESCRIPTOR_TYPE_TEXTURE == pDesc->mType)
            {
#if defined(WEBGPU_NATIVE)
                perStageDescriptorSampledImages += bindingExt.count;
#else
                ++perStageDescriptorSampledImages;
#endif
                ASSERT(perStageDescriptorSampledImages <= pRenderer->pGpu->mSettings.mMaxBoundTextures);
            }

            // If a user specified a uniform buffer to be used as a dynamic uniform buffer change its type to
            // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC Also log a message for debugging purpose
            if (isDescriptorRootCbv(pRes->name))
            {
                if (pDesc->mSize == 1)
                {
                    LOGF(LogLevel::eINFO, "Descriptor (%s) : User specified VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC", pDesc->pName);
                    binding.buffer.hasDynamicOffset = true;
                }
                else
                {
                    LOGF(LogLevel::eWARNING, "Descriptor (%s) : Cannot use VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC for arrays",
                         pDesc->pName);
                }
            }

            binding.visibility = ToShaderStageFlags(pRes->used_stages);

            // Store the vulkan related info in the descriptor to avoid constantly calling the util_to_vk mapping functions
            pDesc->mWgp.mStages = binding.visibility;
            pDesc->mUpdateFrequency = updateFreq;

            if (!pDesc->mStaticSampler)
            {
                arrpush(layouts[setIndex].mDescriptors, pDesc);
            }

            if (binding.buffer.hasDynamicOffset)
            {
                arrpush(layouts[setIndex].mDynamicDescriptors, pDesc);
                pDesc->mRootDescriptor = true;
            }

            arrpush(layouts[setIndex].mBindings, binding);
        }
        // If descriptor is a root constant, add it to the root constant array
        else
        {
#if defined(WEBGPU_NATIVE)
            LOGF(LogLevel::eINFO, "Descriptor (%s) : User specified Push Constant", pDesc->pName);

            pDesc->mRootDescriptor = true;
            setIndex = 0;
            pDesc->mHandleIndex = pushConstantCount++;
            pDesc->mWgp.mStages = ToShaderStageFlags(pRes->used_stages);

            pushConstants[pDesc->mHandleIndex] = {};
            pushConstants[pDesc->mHandleIndex].start = 0;
            pushConstants[pDesc->mHandleIndex].end = pDesc->mSize;
            pushConstants[pDesc->mHandleIndex].stages = pDesc->mWgp.mStages;
#else
            ASSERTFAIL("WebGpu Dawn - Push constants not supported");
#endif
        }
    }

    // Create descriptor layouts
    // Put least frequently changed params first
    for (uint32_t layoutIndex = kMaxLayoutCount; layoutIndex-- > 0U;)
    {
        wgp_UpdateFrequencyLayoutInfo& layout = layouts[layoutIndex];

        if (arrlen(layouts[layoutIndex].mBindings))
        {
            // sort table by type (CBV/SRV/UAV) by register
            sortWGPUBindGroupLayoutEntry(layout.mBindings, arrlenu(layout.mBindings));
        }

        bool createLayout = arrlen(layout.mBindings) != 0;
        // Check if we need to create an empty layout in case there is an empty set between two used sets
        // Example: set = 0 is used, set = 2 is used. In this case, set = 1 needs to exist even if it is empty
        if (!createLayout && layoutIndex < kMaxLayoutCount - 1)
        {
            createLayout = pRootSignature->mWgp.mDescriptorSetLayouts[layoutIndex + 1] != NULL;
        }

        if (createLayout)
        {
            if (arrlen(layout.mBindings))
            {
                WGPUBindGroupLayoutDescriptor layoutInfo = {};
                layoutInfo.entryCount = (uint32_t)arrlen(layout.mBindings);
                layoutInfo.entries = layout.mBindings;

                pRootSignature->mWgp.mDescriptorSetLayouts[layoutIndex] =
                    wgpuDeviceCreateBindGroupLayout(pRenderer->mWgp.pDevice, &layoutInfo);
                ASSERT(pRootSignature->mWgp.mDescriptorSetLayouts[layoutIndex]);
            }
            else
            {
                pRootSignature->mWgp.mDescriptorSetLayouts[layoutIndex] = pRenderer->mWgp.pEmptyDescriptorSetLayout;
            }
        }

        if (!arrlen(layout.mBindings))
        {
            continue;
        }

        pRootSignature->mWgp.mDynamicDescriptorStartIndex[layoutIndex] = UINT8_MAX;
        stableSortWDescriptorInfo(layout.mDescriptors, arrlenu(layout.mDescriptors));

        for (ptrdiff_t descIndex = 0; descIndex < arrlen(layout.mDescriptors); ++descIndex)
        {
            DescriptorInfo* pDesc = layout.mDescriptors[descIndex];
            pDesc->mHandleIndex = (uint32_t)descIndex;
            if (pDesc->mRootDescriptor && UINT8_MAX == pRootSignature->mWgp.mDynamicDescriptorStartIndex[layoutIndex])
            {
                pRootSignature->mWgp.mDynamicDescriptorStartIndex[layoutIndex] = (uint8_t)descIndex;
            }
            if (!pRenderer->pGpu->mWgp.mStaticSamplers && DESCRIPTOR_TYPE_SAMPLER == pDesc->mType)
            {
                // Find if the given descriptor is a static sampler
                StaticSamplerNode* staticSamplerMapIter = shgetp_null(staticSamplerMap, pDesc->pName);
                if (staticSamplerMapIter)
                {
                    ASSERT(pDesc->mUpdateFrequency == DESCRIPTOR_UPDATE_FREQ_NONE);
                    LOGF(LogLevel::eINFO, "Descriptor (%s) : User specified Static Sampler", pDesc->pName);
                    pDesc->mStaticSampler = true;
                    pRootSignature->mWgp.pStaticSamplers[staticSamplerCount] = staticSamplerMapIter->value->mWgp.pSampler;
                    ++staticSamplerCount;
                }
            }
        }

        if (!pRenderer->pGpu->mWgp.mStaticSamplers && staticSamplerCount == arrlenu(layout.mDescriptors))
        {
            pRootSignature->mWgp.mStaticSamplersOnly = true;
            WGPUBindGroupEntry* entries = (WGPUBindGroupEntry*)alloca(staticSamplerCount * sizeof(WGPUBindGroupEntry));
            for (ptrdiff_t descIndex = 0; descIndex < arrlen(layout.mDescriptors); ++descIndex)
            {
                DescriptorInfo* pDesc = layout.mDescriptors[descIndex];
                entries[descIndex].binding = pDesc->mWgp.mReg;
                entries[descIndex].sampler = pRootSignature->mWgp.pStaticSamplers[descIndex];
            }
            WGPUBindGroupDescriptor groupDesc = {};
            groupDesc.entries = entries;
            groupDesc.entryCount = staticSamplerCount;
            groupDesc.layout = pRootSignature->mWgp.mDescriptorSetLayouts[layoutIndex];
            pRootSignature->mWgp.pStaticSamplerSet = wgpuDeviceCreateBindGroup(pRenderer->mWgp.pDevice, &groupDesc);
            ASSERT(pRootSignature->mWgp.pStaticSamplerSet);
        }

        if (arrlen(layout.mDynamicDescriptors))
        {
            // vkCmdBindDescriptorSets - pDynamicOffsets - entries are ordered by the binding numbers in the descriptor set layouts
            stableSortWDescriptorInfo(layout.mDynamicDescriptors, arrlenu(layout.mDynamicDescriptors));

            pRootSignature->mWgp.mDynamicDescriptorCounts[layoutIndex] = (uint8_t)arrlen(layout.mDynamicDescriptors);
        }
    }

    // Rearrange static samplers to match descriptor order
    if (!pRenderer->pGpu->mWgp.mStaticSamplers && !pRootSignature->mWgp.mStaticSamplersOnly)
    {
        staticSamplerCount = 0;
        for (uint32_t d = 0; d < pRootSignature->mDescriptorCount; ++d)
        {
            const DescriptorInfo* pDesc = &pRootSignature->pDescriptors[d];
            if (!pDesc->mStaticSampler)
            {
                continue;
            }
            // Find if the given descriptor is a static sampler
            StaticSamplerNode* staticSamplerMapIter = shgetp_null(staticSamplerMap, pDesc->pName);
            ASSERT(staticSamplerMapIter);
            pRootSignature->mWgp.pStaticSamplers[staticSamplerCount++] = staticSamplerMapIter->value->mWgp.pSampler;
        }
    }
    /************************************************************************/
    // Pipeline layout
    /************************************************************************/
    WGPUBindGroupLayout descriptorSetLayouts[kMaxLayoutCount] = {};
    uint32_t            descriptorSetLayoutCount = 0;
    for (uint32_t i = 0; i < DESCRIPTOR_UPDATE_FREQ_COUNT; ++i)
    {
        if (pRootSignature->mWgp.mDescriptorSetLayouts[i])
        {
            descriptorSetLayouts[descriptorSetLayoutCount++] = pRootSignature->mWgp.mDescriptorSetLayouts[i];
        }
    }

    DECL_CHAINED_STRUCT(WGPUPipelineLayoutDescriptor, layoutDesc);
#if defined(WEBGPU_NATIVE)
    WGPUPipelineLayoutExtras layoutDescExt = {};
    layoutDescExt.chain.sType = (WGPUSType)WGPUSType_PipelineLayoutExtras;
    layoutDescExt.pushConstantRangeCount = pushConstantCount;
    layoutDescExt.pushConstantRanges = pushConstants;
    ADD_TO_NEXT_CHAIN(layoutDesc, &layoutDescExt);
#endif
    layoutDesc.bindGroupLayoutCount = descriptorSetLayoutCount;
    layoutDesc.bindGroupLayouts = descriptorSetLayouts;
    pRootSignature->mWgp.pPipelineLayout = wgpuDeviceCreatePipelineLayout(pRenderer->mWgp.pDevice, &layoutDesc);
    ASSERT(pRootSignature->mWgp.pPipelineLayout);
    /************************************************************************/
    // Cleanup
    /************************************************************************/
    for (uint32_t i = 0; i < kMaxLayoutCount; ++i)
    {
        arrfree(layouts[i].mBindings);
        arrfree(layouts[i].mDescriptors);
        arrfree(layouts[i].mDynamicDescriptors);
    }
    arrfree(shaderResources);
    shfree(staticSamplerMap);

    *ppRootSignature = pRootSignature;
}

void wgpu_removeRootSignature(Renderer* pRenderer, RootSignature* pRootSignature)
{
    ASSERT(pRenderer);

    for (uint32_t i = 0; i < DESCRIPTOR_UPDATE_FREQ_COUNT; ++i)
    {
        if (pRootSignature->mWgp.mDescriptorSetLayouts[i] &&
            pRootSignature->mWgp.mDescriptorSetLayouts[i] != pRenderer->mWgp.pEmptyDescriptorSetLayout)
        {
            wgpuBindGroupLayoutRelease(pRootSignature->mWgp.mDescriptorSetLayouts[i]);
        }
    }

    if (pRootSignature->mWgp.mStaticSamplersOnly)
    {
        wgpuBindGroupRelease(pRootSignature->mWgp.pStaticSamplerSet);
    }

    shfree(pRootSignature->pDescriptorNameToIndexMap);

    wgpuPipelineLayoutRelease(pRootSignature->mWgp.pPipelineLayout);

    SAFE_FREE(pRootSignature);
}

uint32_t wgpu_getDescriptorIndexFromName(const RootSignature* pRootSignature, const char* pName)
{
    for (uint32_t i = 0; i < pRootSignature->mDescriptorCount; ++i)
    {
        if (!strcmp(pName, pRootSignature->pDescriptors[i].pName))
        {
            return i;
        }
    }

    return UINT32_MAX;
}

/************************************************************************/
// Pipeline State Functions
/************************************************************************/
static void addGraphicsPipeline(Renderer* pRenderer, const PipelineDesc* pMainDesc, Pipeline** ppPipeline)
{
    ASSERT(pRenderer);
    ASSERT(ppPipeline);
    ASSERT(pMainDesc);

    const GraphicsPipelineDesc* pDesc = &pMainDesc->mGraphicsDesc;

    ASSERT(pDesc->pShaderProgram);
    ASSERT(pDesc->pRootSignature);

    Pipeline* pPipeline = (Pipeline*)tf_calloc_memalign(1, alignof(Pipeline), sizeof(Pipeline));
    ASSERT(pPipeline);

    pPipeline->mWgp.mType = PIPELINE_TYPE_GRAPHICS;

    static RasterizerStateDesc rast = {};
    const Shader*              pShaderProgram = pDesc->pShaderProgram;
    const VertexLayout*        pVertexLayout = pDesc->pVertexLayout;
    const RasterizerStateDesc* pRast = pDesc->pRasterizerState ? pDesc->pRasterizerState : &rast;

    WGPUDepthStencilState ds =
        pDesc->pDepthState ? ToDepthStencilState(pDesc->pDepthState, pRast) : pRenderer->pNullDescriptors->mDefaultDs;
    ds.format = (WGPUTextureFormat)TinyImageFormat_ToWGPUTextureFormat(pDesc->mDepthStencilFormat);
    if (!TinyImageFormat_HasStencil(pDesc->mDepthStencilFormat))
    {
        ds.stencilBack.compare = WGPUCompareFunction_Always;
        ds.stencilBack.depthFailOp = WGPUStencilOperation_Keep;
        ds.stencilBack.failOp = WGPUStencilOperation_Keep;
        ds.stencilBack.passOp = WGPUStencilOperation_Keep;
        ds.stencilFront = ds.stencilBack;
    }

    WGPUColorTargetState cts[MAX_RENDER_TARGET_ATTACHMENTS] = {};
    WGPUBlendState       bs[MAX_RENDER_TARGET_ATTACHMENTS] = {};
    for (uint32_t rt = 0; rt < pDesc->mRenderTargetCount; ++rt)
    {
        bs[rt] = pDesc->pBlendState ? ToBlendState(pDesc->pBlendState, rt) : pRenderer->pNullDescriptors->mDefaultBs;
        if (pDesc->pBlendState)
        {
            const BlendStateDesc* srcBlend = pDesc->pBlendState;
            const bool            blendEnable = (srcBlend->mSrcFactors[rt] != BC_ONE || srcBlend->mDstFactors[rt] != BC_ZERO ||
                                      srcBlend->mSrcAlphaFactors[rt] != BC_ONE || srcBlend->mDstAlphaFactors[rt] != BC_ZERO);
            cts[rt].blend = blendEnable ? &bs[rt] : NULL;
        }
        cts[rt].format = (WGPUTextureFormat)TinyImageFormat_ToWGPUTextureFormat(pDesc->pColorFormats[rt]);
        cts[rt].writeMask = pDesc->pBlendState ? pDesc->pBlendState->mColorWriteMasks[rt] : COLOR_MASK_ALL;
    }

    WGPUFragmentState fs = {};
    // #TODO
    fs.constantCount = 0;
    // #TODO
    fs.constants = NULL;
    // #TODO
    fs.entryPoint = pDesc->pShaderProgram->pReflection->mStageReflections[pShaderProgram->pReflection->mPixelStageIndex].pEntryPoint;
    fs.module = pShaderProgram->mWgp.pShaderModules[pShaderProgram->pReflection->mPixelStageIndex];
    fs.targetCount = pDesc->mRenderTargetCount;
    fs.targets = cts;

    WGPUVertexAttribute    vbAttribs[MAX_VERTEX_BINDINGS][MAX_VERTEX_ATTRIBS] = {};
    WGPUVertexBufferLayout vb[MAX_VERTEX_BINDINGS] = {};
    uint32_t               vbCount = 0;
    if (pVertexLayout)
    {
        vbCount = pVertexLayout->mBindingCount;
        for (uint32_t binding = 0; binding < pVertexLayout->mBindingCount; ++binding)
        {
            vb[binding].arrayStride = pVertexLayout->mBindings[binding].mStride;
            vb[binding].attributes = vbAttribs[binding];
            vb[binding].stepMode = ToStepMode(pVertexLayout->mBindings[binding].mRate);
        }

        for (uint32_t attr = 0; attr < pVertexLayout->mAttribCount; ++attr)
        {
            const VertexAttrib*     srcAttr = &pVertexLayout->mAttribs[attr];
            const VertexBinding*    srcBinding = &pVertexLayout->mBindings[srcAttr->mBinding];
            WGPUVertexBufferLayout* dstBinding = &vb[srcAttr->mBinding];
            WGPUVertexAttribute*    dstAttr = &vbAttribs[srcAttr->mBinding][dstBinding->attributeCount++];
            dstAttr->format = ToVertexFormat(srcAttr->mFormat);
            dstAttr->offset = srcAttr->mOffset;
            dstAttr->shaderLocation = srcAttr->mLocation;

            // update binding stride if necessary
            if (!srcBinding->mStride)
            {
                // guessing stride using attribute offset in case there are several attributes at the same binding
                dstBinding->arrayStride =
                    max((uint64_t)(srcAttr->mOffset + TinyImageFormat_BitSizeOfBlock(srcAttr->mFormat) / 8), dstBinding->arrayStride);
            }
        }
    }

    WGPUVertexState vs = {};
    vs.bufferCount = vbCount;
    vs.buffers = vb;
    // #TODO
    vs.constantCount = 0;
    // #TODO
    vs.constants = NULL;
    vs.entryPoint = pDesc->pShaderProgram->pReflection->mStageReflections[pShaderProgram->pReflection->mVertexStageIndex].pEntryPoint;
    vs.module = pShaderProgram->mWgp.pShaderModules[pShaderProgram->pReflection->mVertexStageIndex];

    WGPUMultisampleState ms = {};
    ms.alphaToCoverageEnabled = false;
    ms.count = pDesc->mSampleCount;
    ms.mask = UINT32_MAX;

    WGPUPrimitiveState ps = pDesc->pRasterizerState ? ToPrimitiveState(pDesc->pRasterizerState) : pRenderer->pNullDescriptors->mDefaultPs;
    ps.topology = ToPrimitiveTopo(pDesc->mPrimitiveTopo);
    ps.stripIndexFormat = WGPUIndexFormat_Undefined;
    if (ps.topology == WGPUPrimitiveTopology_LineStrip || ps.topology == WGPUPrimitiveTopology_TriangleStrip)
    {
        ps.stripIndexFormat = ToIndexType(pDesc->mIndexType);
    }

    WGPURenderPipelineDescriptor renderDesc = {};
    renderDesc.depthStencil = TinyImageFormat_HasDepthOrStencil(pDesc->mDepthStencilFormat) ? &ds : NULL;
    renderDesc.fragment = &fs;
    renderDesc.layout = pDesc->pRootSignature->mWgp.pPipelineLayout;
    renderDesc.multisample = ms;
    renderDesc.primitive = ps;
    renderDesc.vertex = vs;
#if defined(ENABLE_GRAPHICS_DEBUG)
    renderDesc.label = pMainDesc->pName;
#endif
    pPipeline->mWgp.pRender = wgpuDeviceCreateRenderPipeline(pRenderer->mWgp.pDevice, &renderDesc);
    ASSERT(pPipeline->mWgp.pRender);

    *ppPipeline = pPipeline;
}

static void addComputePipeline(Renderer* pRenderer, const PipelineDesc* pMainDesc, Pipeline** ppPipeline)
{
    ASSERT(pRenderer);
    ASSERT(ppPipeline);
    ASSERT(pMainDesc);

    const ComputePipelineDesc* pDesc = &pMainDesc->mComputeDesc;

    ASSERT(pDesc->pShaderProgram);
    ASSERT(pDesc->pRootSignature);
    ASSERT(pDesc->pShaderProgram->mWgp.pShaderModules[0]);

    Pipeline* pPipeline = (Pipeline*)tf_calloc_memalign(1, alignof(Pipeline), sizeof(Pipeline));
    ASSERT(pPipeline);

    pPipeline->mWgp.mType = PIPELINE_TYPE_COMPUTE;

    // Pipeline
    WGPUComputePipelineDescriptor computeDesc = {};
    // #TODO
    computeDesc.compute.constantCount = 0;
    // #TODO
    computeDesc.compute.constants = NULL;
    // #TODO: Check
    computeDesc.compute.entryPoint = pDesc->pShaderProgram->pReflection->mStageReflections[0].pEntryPoint;
    computeDesc.compute.module = pDesc->pShaderProgram->mWgp.pShaderModules[0];
    computeDesc.layout = pDesc->pRootSignature->mWgp.pPipelineLayout;
#if defined(ENABLE_GRAPHICS_DEBUG)
    computeDesc.label = pMainDesc->pName;
#endif
    pPipeline->mWgp.pCompute = wgpuDeviceCreateComputePipeline(pRenderer->mWgp.pDevice, &computeDesc);
    ASSERT(pPipeline->mWgp.pCompute);

    *ppPipeline = pPipeline;
}

void wgpu_addPipeline(Renderer* pRenderer, const PipelineDesc* pDesc, Pipeline** ppPipeline)
{
    switch (pDesc->mType)
    {
    case (PIPELINE_TYPE_COMPUTE):
    {
        addComputePipeline(pRenderer, pDesc, ppPipeline);
        break;
    }
    case (PIPELINE_TYPE_GRAPHICS):
    {
        addGraphicsPipeline(pRenderer, pDesc, ppPipeline);
        break;
    }
    default:
    {
        ASSERTFAIL("Unknown pipeline type %i", pDesc->mType);
        *ppPipeline = {};
        break;
    }
    }

    if (*ppPipeline && pDesc->pName)
    {
        setPipelineName(pRenderer, *ppPipeline, pDesc->pName);
    }
}

void wgpu_removePipeline(Renderer* pRenderer, Pipeline* pPipeline)
{
    ASSERT(pRenderer);
    ASSERT(pPipeline);

    if (PIPELINE_TYPE_COMPUTE == pPipeline->mWgp.mType)
    {
        wgpuComputePipelineRelease(pPipeline->mWgp.pCompute);
    }
    else if (PIPELINE_TYPE_GRAPHICS == pPipeline->mWgp.mType)
    {
        wgpuRenderPipelineRelease(pPipeline->mWgp.pRender);
    }
    else
    {
        ASSERTFAIL("removePipeline type not supported");
    }

    SAFE_FREE(pPipeline);
}

void wgpu_addPipelineCache(Renderer* pRenderer, const PipelineCacheDesc* pDesc, PipelineCache** ppPipelineCache)
{
    ASSERT(pRenderer);
    ASSERT(pDesc);
    ASSERT(ppPipelineCache);
}

void wgpu_removePipelineCache(Renderer* pRenderer, PipelineCache* pPipelineCache)
{
    ASSERT(pRenderer);
    ASSERT(pPipelineCache);
    SAFE_FREE(pPipelineCache);
}

void wgpu_getPipelineCacheData(Renderer*, PipelineCache*, size_t*, void*) {}
/************************************************************************/
// Command buffer functions
/************************************************************************/
void wgpu_resetCmdPool(Renderer*, CmdPool*) {}

static void BeginComputeEncoder(Cmd* pCmd, WGPUComputePassDescriptor& computeDesc)
{
    pCmd->mWgp.pComputeEncoder = wgpuCommandEncoderBeginComputePass(pCmd->mWgp.pEncoder, &computeDesc);
    pCmd->mWgp.mInsideComputePass = true;
}

void EndComputeEncoder(Cmd* pCmd)
{
    if (!pCmd->mWgp.mInsideComputePass)
    {
        return;
    }

    wgpuComputePassEncoderEnd(pCmd->mWgp.pComputeEncoder);
    pCmd->mWgp.mInsideComputePass = false;
    arrpush(pCmd->mWgp.pComputeEncoderArray, pCmd->mWgp.pComputeEncoder);
}

static void BeginRenderEncoder(Cmd* pCmd, WGPURenderPassDescriptor& renderDesc)
{
    pCmd->mWgp.pRenderEncoder = wgpuCommandEncoderBeginRenderPass(pCmd->mWgp.pEncoder, &renderDesc);
    pCmd->mWgp.mInsideRenderPass = true;
}

static void EndRenderEncoder(Cmd* pCmd)
{
    if (!pCmd->mWgp.mInsideRenderPass)
    {
        return;
    }

    wgpuRenderPassEncoderEnd(pCmd->mWgp.pRenderEncoder);
    pCmd->mWgp.mInsideRenderPass = false;
    arrpush(pCmd->mWgp.pRenderEncoderArray, pCmd->mWgp.pRenderEncoder);
}

void wgpu_beginCmd(Cmd* pCmd)
{
    ASSERT(pCmd);

    if (arrlenu(pCmd->mWgp.pRenderEncoderArray))
    {
        for (uint32_t i = 0; i < arrlenu(pCmd->mWgp.pRenderEncoderArray); ++i)
        {
            wgpuRenderPassEncoderRelease(pCmd->mWgp.pRenderEncoderArray[i]);
        }
        arrsetlen(pCmd->mWgp.pRenderEncoderArray, 0);
        pCmd->mWgp.pRenderEncoder = NULL;
    }
    if (arrlenu(pCmd->mWgp.pComputeEncoderArray))
    {
        for (uint32_t i = 0; i < arrlenu(pCmd->mWgp.pComputeEncoderArray); ++i)
        {
            wgpuComputePassEncoderRelease(pCmd->mWgp.pComputeEncoderArray[i]);
        }
        arrsetlen(pCmd->mWgp.pComputeEncoderArray, 0);
        pCmd->mWgp.pComputeEncoder = NULL;
    }
    if (pCmd->mWgp.pEncoder)
    {
        wgpuCommandEncoderRelease(pCmd->mWgp.pEncoder);
        pCmd->mWgp.pEncoder = NULL;
    }
    if (pCmd->mWgp.pCmdBuf)
    {
        wgpuCommandBufferRelease(pCmd->mWgp.pCmdBuf);
        pCmd->mWgp.pCmdBuf = NULL;
    }

    WGPUCommandEncoderDescriptor beginDesc = {};
    pCmd->mWgp.pEncoder = wgpuDeviceCreateCommandEncoder(pCmd->pRenderer->mWgp.pDevice, &beginDesc);

    pCmd->mWgp.pBoundPipelineLayout = NULL;
}

void wgpu_endCmd(Cmd* pCmd)
{
    ASSERT(pCmd);
    ASSERT(pCmd->mWgp.pEncoder);

    EndRenderEncoder(pCmd);
    EndComputeEncoder(pCmd);

    WGPUCommandBufferDescriptor cmdBufDesc = {};
    pCmd->mWgp.pCmdBuf = wgpuCommandEncoderFinish(pCmd->mWgp.pEncoder, &cmdBufDesc);
}

static inline FORGE_CONSTEXPR WGPULoadOp ToLoadOp(LoadActionType loadAction)
{
    switch (loadAction)
    {
    case LOAD_ACTION_DONTCARE:
        return WGPULoadOp_Undefined;
    case LOAD_ACTION_LOAD:
        return WGPULoadOp_Load;
    case LOAD_ACTION_CLEAR:
        return WGPULoadOp_Clear;
    default:
        return WGPULoadOp_Undefined;
    }
}

static inline FORGE_CONSTEXPR WGPUStoreOp ToStoreOp(StoreActionType storeAction)
{
    switch (storeAction)
    {
    case STORE_ACTION_STORE:
        return WGPUStoreOp_Store;
    case STORE_ACTION_DONTCARE:
        return WGPUStoreOp_Discard;
    case STORE_ACTION_NONE:
        return WGPUStoreOp_Store;
    default:
        return WGPUStoreOp_Discard;
    }
}

void wgpu_cmdBindRenderTargets(Cmd* pCmd, const BindRenderTargetsDesc* pDesc)
{
    ASSERT(pCmd);

    if (!pDesc)
    {
        EndRenderEncoder(pCmd);
        return;
    }

    WGPURenderPassDescriptor             renderPassDesc = {};
    WGPURenderPassColorAttachment        colorRts[MAX_RENDER_TARGET_ATTACHMENTS] = {};
    WGPURenderPassDepthStencilAttachment ds = {};
    renderPassDesc.colorAttachmentCount = pDesc->mRenderTargetCount;
    renderPassDesc.colorAttachments = colorRts;
    for (uint32_t i = 0; i < pDesc->mRenderTargetCount; ++i)
    {
        const BindRenderTargetDesc*    desc = &pDesc->mRenderTargets[i];
        const float*                   clearValue = desc->mOverrideClearValue ? &desc->mClearValue.r : &desc->pRenderTarget->mClearValue.r;
        WGPURenderPassColorAttachment* rt = &colorRts[i];
        rt->clearValue = { clearValue[0], clearValue[1], clearValue[2], clearValue[3] };
        rt->loadOp = ToLoadOp(desc->mLoadAction);
        rt->storeOp = ToStoreOp(desc->mStoreAction);
#if defined(WEBGPU_DAWN)
        rt->depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

        if (!desc->mUseMipSlice && !desc->mUseArraySlice)
        {
            rt->view = desc->pRenderTarget->mWgp.pDefault;
        }
        else
        {
            uint32_t handle = 0;
            if (desc->mUseMipSlice)
            {
                if (desc->mUseArraySlice)
                {
                    handle = desc->mMipSlice * desc->pRenderTarget->mArraySize + desc->mArraySlice;
                }
                else
                {
                    handle = desc->mMipSlice;
                }
            }
            else if (desc->mUseArraySlice)
            {
                handle = desc->mArraySlice;
            }
            rt->view = desc->pRenderTarget->mWgp.pSlices[handle];
        }
    }

    if (pDesc->mDepthStencil.pDepthStencil)
    {
        const BindDepthTargetDesc* desc = &pDesc->mDepthStencil;
        const ClearValue*          clearValue = desc->mOverrideClearValue ? &desc->mClearValue : &desc->pDepthStencil->mClearValue;
        if (TinyImageFormat_HasDepth(desc->pDepthStencil->mFormat))
        {
            ds.depthClearValue = clearValue->depth;
            ds.depthLoadOp = ToLoadOp(desc->mLoadAction);
            ds.depthReadOnly = false;
            ds.depthStoreOp = ToStoreOp(desc->mStoreAction);
        }
        if (TinyImageFormat_HasStencil(desc->pDepthStencil->mFormat))
        {
            ds.stencilClearValue = clearValue->stencil;
            ds.stencilLoadOp = ToLoadOp(desc->mLoadActionStencil);
            ds.stencilReadOnly = false;
            ds.stencilStoreOp = ToStoreOp(desc->mStoreActionStencil);
        }

        if (!desc->mUseMipSlice && !desc->mUseArraySlice)
        {
            ds.view = desc->pDepthStencil->mWgp.pDefault;
        }
        else
        {
            uint32_t handle = 0;
            if (desc->mUseMipSlice)
            {
                if (desc->mUseArraySlice)
                {
                    handle = desc->mMipSlice * desc->pDepthStencil->mArraySize + desc->mArraySlice;
                }
                else
                {
                    handle = desc->mMipSlice;
                }
            }
            else if (desc->mUseArraySlice)
            {
                handle = desc->mArraySlice;
            }
            ds.view = desc->pDepthStencil->mWgp.pSlices[handle];
        }

        renderPassDesc.depthStencilAttachment = &ds;
    }

    EndComputeEncoder(pCmd);
    ASSERT(!pCmd->mWgp.mInsideRenderPass);
    BeginRenderEncoder(pCmd, renderPassDesc);
}

void wgpu_cmdSetSampleLocations(Cmd*, SampleCount, uint32_t, uint32_t, SampleLocations*) {}

void wgpu_cmdSetViewport(Cmd* pCmd, float x, float y, float width, float height, float minDepth, float maxDepth)
{
    ASSERT(pCmd);
    ASSERT(pCmd->mWgp.mInsideRenderPass);
    wgpuRenderPassEncoderSetViewport(pCmd->mWgp.pRenderEncoder, x, y, width, height, minDepth, maxDepth);
}

void wgpu_cmdSetScissor(Cmd* pCmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    ASSERT(pCmd);
    ASSERT(pCmd->mWgp.mInsideRenderPass);
    wgpuRenderPassEncoderSetScissorRect(pCmd->mWgp.pRenderEncoder, x, y, width, height);
}

void wgpu_cmdSetStencilReferenceValue(Cmd* pCmd, uint32_t val)
{
    ASSERT(pCmd);
    ASSERT(pCmd->mWgp.mInsideRenderPass);
    wgpuRenderPassEncoderSetStencilReference(pCmd->mWgp.pRenderEncoder, val);
}

void wgpu_cmdBindPipeline(Cmd* pCmd, Pipeline* pPipeline)
{
    ASSERT(pCmd);
    ASSERT(pPipeline);

    if (PIPELINE_TYPE_COMPUTE == pPipeline->mWgp.mType)
    {
        if (!pCmd->mWgp.mInsideComputePass)
        {
            ASSERT(!pCmd->mWgp.mInsideRenderPass);
            WGPUComputePassDescriptor computeDesc = {};
            BeginComputeEncoder(pCmd, computeDesc);
        }

        wgpuComputePassEncoderSetPipeline(pCmd->mWgp.pComputeEncoder, pPipeline->mWgp.pCompute);
    }
    else if (PIPELINE_TYPE_GRAPHICS == pPipeline->mWgp.mType)
    {
        ASSERT(pCmd->mWgp.mInsideRenderPass);
        wgpuRenderPassEncoderSetPipeline(pCmd->mWgp.pRenderEncoder, pPipeline->mWgp.pRender);
    }
}

static inline FORGE_CONSTEXPR WGPUIndexFormat ToIndexFormat(IndexType type)
{
    switch (type)
    {
    case INDEX_TYPE_UINT32:
        return WGPUIndexFormat_Uint32;
    case INDEX_TYPE_UINT16:
        return WGPUIndexFormat_Uint16;
    default:
        return WGPUIndexFormat_Force32;
    }
}

void wgpu_cmdBindIndexBuffer(Cmd* pCmd, Buffer* pBuffer, uint32_t indexType, uint64_t offset)
{
    ASSERT(pCmd);
    ASSERT(pCmd->mWgp.mInsideRenderPass);
    ASSERT(pBuffer);

    WGPUIndexFormat fmt = ToIndexFormat((IndexType)indexType);
    wgpuRenderPassEncoderSetIndexBuffer(pCmd->mWgp.pRenderEncoder, pBuffer->mWgp.pBuffer, fmt, offset, pBuffer->mSize - offset);
}

void wgpu_cmdBindVertexBuffer(Cmd* pCmd, uint32_t bufferCount, Buffer** ppBuffers, const uint32_t* pStrides, const uint64_t* pOffsets)
{
    ASSERT(pCmd);
    ASSERT(pCmd->mWgp.mInsideRenderPass);
    ASSERT(bufferCount);
    ASSERT(ppBuffers);
    ASSERT(pStrides);

    for (uint32_t i = 0; i < bufferCount; ++i)
    {
        uint64_t offset = pOffsets ? pOffsets[i] : 0;
        wgpuRenderPassEncoderSetVertexBuffer(pCmd->mWgp.pRenderEncoder, i, ppBuffers[i]->mWgp.pBuffer, offset,
                                             ppBuffers[i]->mSize - offset);
    }
}

void wgpu_cmdDraw(Cmd* pCmd, uint32_t vertexCount, uint32_t firstVertex)
{
    ASSERT(pCmd);
    ASSERT(pCmd->mWgp.mInsideRenderPass);
    wgpuRenderPassEncoderDraw(pCmd->mWgp.pRenderEncoder, vertexCount, 1, firstVertex, 0);
}

void wgpu_cmdDrawInstanced(Cmd* pCmd, uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount, uint32_t firstInstance)
{
    ASSERT(pCmd);
    ASSERT(pCmd->mWgp.mInsideRenderPass);
    wgpuRenderPassEncoderDraw(pCmd->mWgp.pRenderEncoder, vertexCount, instanceCount, firstVertex, firstInstance);
}

void wgpu_cmdDrawIndexed(Cmd* pCmd, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
{
    ASSERT(pCmd);
    ASSERT(pCmd->mWgp.mInsideRenderPass);
    wgpuRenderPassEncoderDrawIndexed(pCmd->mWgp.pRenderEncoder, indexCount, 1, firstIndex, firstVertex, 0);
}

void wgpu_cmdDrawIndexedInstanced(Cmd* pCmd, uint32_t indexCount, uint32_t firstIndex, uint32_t instanceCount, uint32_t firstVertex,
                                  uint32_t firstInstance)
{
    ASSERT(pCmd);
    ASSERT(pCmd->mWgp.mInsideRenderPass);
    wgpuRenderPassEncoderDrawIndexed(pCmd->mWgp.pRenderEncoder, indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
}

void wgpu_cmdDispatch(Cmd* pCmd, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    ASSERT(pCmd);
    ASSERT(pCmd->mWgp.mInsideComputePass);
    wgpuComputePassEncoderDispatchWorkgroups(pCmd->mWgp.pComputeEncoder, groupCountX, groupCountY, groupCountZ);
}

void wgpu_cmdResourceBarrier(Cmd*, uint32_t, BufferBarrier*, uint32_t, TextureBarrier*, uint32_t, RenderTargetBarrier*) {}

void wgpu_cmdUpdateBuffer(Cmd* pCmd, Buffer* pBuffer, uint64_t dstOffset, Buffer* pSrcBuffer, uint64_t srcOffset, uint64_t size)
{
    ASSERT(pCmd);
    ASSERT(pSrcBuffer);
    ASSERT(pBuffer);
    ASSERT(srcOffset + size <= pSrcBuffer->mSize);
    ASSERT(dstOffset + size <= pBuffer->mSize);

    wgpuCommandEncoderCopyBufferToBuffer(pCmd->mWgp.pEncoder, pSrcBuffer->mWgp.pBuffer, srcOffset, pBuffer->mWgp.pBuffer, dstOffset, size);
}

typedef struct SubresourceDataDesc
{
    uint64_t mSrcOffset;
    uint32_t mMipLevel;
    uint32_t mArrayLayer;
    uint32_t mRowPitch;
    uint32_t mSlicePitch;
} SubresourceDataDesc;

void wgpu_cmdUpdateSubresource(Cmd* pCmd, Texture* pTexture, Buffer* pSrcBuffer, const SubresourceDataDesc* pSubresourceDesc)
{
    ASSERT(pCmd);

    const uint32_t width = max<uint32_t>(1, pTexture->mWidth >> pSubresourceDesc->mMipLevel);
    const uint32_t height = max<uint32_t>(1, pTexture->mHeight >> pSubresourceDesc->mMipLevel);
    const uint32_t depth = max<uint32_t>(1, pTexture->mDepth >> pSubresourceDesc->mMipLevel);

    WGPUTextureDataLayout layout = {};
    layout.bytesPerRow = pSubresourceDesc->mRowPitch;
    layout.offset = pSubresourceDesc->mSrcOffset;
    layout.rowsPerImage = pSubresourceDesc->mSlicePitch / pSubresourceDesc->mRowPitch;

    WGPUImageCopyBuffer src = {};
    src.buffer = pSrcBuffer->mWgp.pBuffer;
    src.layout = layout;

    WGPUImageCopyTexture dst = {};
    dst.aspect = WGPUTextureAspect_All;
    dst.mipLevel = pSubresourceDesc->mMipLevel;
    dst.origin = { 0, 0, pSubresourceDesc->mArrayLayer };
    dst.texture = pTexture->mWgp.pTexture;

    WGPUExtent3D extent = { width, height, depth };
    wgpuCommandEncoderCopyBufferToTexture(pCmd->mWgp.pEncoder, &src, &dst, &extent);
}

void wgpu_cmdCopySubresource(Cmd* pCmd, Buffer* pDstBuffer, Texture* pTexture, const SubresourceDataDesc* pSubresourceDesc)
{
    ASSERT(pCmd);

    const uint32_t width = max<uint32_t>(1, pTexture->mWidth >> pSubresourceDesc->mMipLevel);
    const uint32_t height = max<uint32_t>(1, pTexture->mHeight >> pSubresourceDesc->mMipLevel);
    const uint32_t depth = max<uint32_t>(1, pTexture->mDepth >> pSubresourceDesc->mMipLevel);

    WGPUTextureDataLayout layout = {};
    layout.bytesPerRow = pSubresourceDesc->mRowPitch;
    layout.offset = pSubresourceDesc->mSrcOffset;
    layout.rowsPerImage = pSubresourceDesc->mSlicePitch / pSubresourceDesc->mRowPitch;

    WGPUImageCopyBuffer dst = {};
    dst.buffer = pDstBuffer->mWgp.pBuffer;
    dst.layout = layout;

    WGPUImageCopyTexture src = {};
    src.aspect = WGPUTextureAspect_All;
    src.mipLevel = pSubresourceDesc->mMipLevel;
    src.origin = { 0, 0, pSubresourceDesc->mArrayLayer };
    src.texture = pTexture->mWgp.pTexture;

    WGPUExtent3D extent = { width, height, depth };
    wgpuCommandEncoderCopyTextureToBuffer(pCmd->mWgp.pEncoder, &src, &dst, &extent);
}
/************************************************************************/
// Queue Fence Semaphore Functions
/************************************************************************/
void wgpu_acquireNextImage(Renderer* pRenderer, SwapChain* pSwapChain, Semaphore*, Fence*, uint32_t* pImageIndex)
{
    ASSERT(pRenderer);
    ASSERT(pSwapChain);

    RenderTarget* rt = pSwapChain->ppRenderTargets[0];
    *pImageIndex = 0;
    WGPUSurfaceTexture surfaceTexture = {};
    wgpuSurfaceGetCurrentTexture(pSwapChain->mWgp.pSurface, &surfaceTexture);
    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success)
    {
        // Try reconfig once
        if (surfaceTexture.texture != NULL)
        {
            wgpuTextureRelease(surfaceTexture.texture);
        }
        wgpuSurfaceConfigure(pSwapChain->mWgp.pSurface, &pSwapChain->mWgp.mConfig);
        wgpuSurfaceGetCurrentTexture(pSwapChain->mWgp.pSurface, &surfaceTexture);
        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success)
        {
            ASSERT(false);
            *pImageIndex = UINT32_MAX;
            return;
        }
    }

    rt->pTexture->mWgp.pTexture = surfaceTexture.texture;
    // WGPUTextureViewDescriptor viewDesc = {};
    // viewDesc.arrayLayerCount = 1;
    // viewDesc.aspect = WGPUTextureAspect_All;
    // viewDesc.baseArrayLayer = 0;
    // viewDesc.baseMipLevel = 0;
    // viewDesc.dimension = WGPUTextureViewDimension_2D;
    // viewDesc.format = pSwapChain->mWgp.mConfig.viewFormats[0];
    // viewDesc.mipLevelCount = 1;
    rt->mWgp.pDefault = wgpuTextureCreateView(surfaceTexture.texture, NULL);
}

static void QueueWorkDoneCallback(WGPUQueueWorkDoneStatus, void*)
{
    // No-op
}

void wgpu_queueSubmit(Queue* pQueue, const QueueSubmitDesc* pDesc)
{
    ASSERT(pQueue);
    ASSERT(pDesc);

    const uint32_t cmdCount = pDesc->mCmdCount;

    WGPUCommandBuffer* cmds = (WGPUCommandBuffer*)alloca(cmdCount * sizeof(WGPUCommandBuffer));
    for (uint32_t i = 0; i < cmdCount; ++i)
    {
        cmds[i] = pDesc->ppCmds[i]->mWgp.pCmdBuf;
    }
    wgpuQueueSubmit(pQueue->mWgp.pQueue, cmdCount, cmds);

    // Crashes in WEBGPU_NATIVE
#if defined(WEBGPU_DAWN)
    WGPUQueueWorkDoneCallbackInfo callbackInfo = {};
    callbackInfo.callback = QueueWorkDoneCallback;
    pQueue->mWgp.mWorkDoneFuture = wgpuQueueOnSubmittedWorkDoneF(pQueue->mWgp.pQueue, callbackInfo);
    pQueue->mWgp.mFutureValid = true;

    if (pDesc->mSubmitDone)
    {
        wgpuInstanceProcessEvents(pQueue->mWgp.pRenderer->pContext->mWgp.pInstance);
    }
#endif
}

void wgpu_queuePresent(Queue* pQueue, const QueuePresentDesc* pDesc)
{
    ASSERT(pQueue);
    ASSERT(pDesc);
    ASSERT(pDesc->pSwapChain);

    wgpuSurfacePresent(pDesc->pSwapChain->mWgp.pSurface);

    wgpuTextureViewRelease(pDesc->pSwapChain->ppRenderTargets[0]->mWgp.pDefault);
    wgpuTextureRelease(pDesc->pSwapChain->ppRenderTargets[0]->pTexture->mWgp.pTexture);
    pDesc->pSwapChain->ppRenderTargets[0]->pTexture->mWgp.pTexture = NULL;
    pDesc->pSwapChain->ppRenderTargets[0]->mWgp.pDefault = NULL;

    // Crashes in WEBGPU_NATIVE
#if defined(WEBGPU_DAWN)
    if (pDesc->mSubmitDone)
    {
        wgpuInstanceProcessEvents(pQueue->mWgp.pRenderer->pContext->mWgp.pInstance);
    }
#endif
}

void wgpu_waitForFences(Renderer* pRenderer, uint32_t fenceCount, Fence** ppFences)
{
    ASSERT(pRenderer);
    ASSERT(fenceCount);
    ASSERT(ppFences);
}

void wgpu_waitQueueIdle(Queue* pQueue)
{
#if defined(WEBGPU_DAWN)
    WGPUInstance instance = pQueue->mWgp.pRenderer->pContext->mWgp.pInstance;
    wgpuInstanceProcessEvents(instance);
    if (pQueue->mWgp.mFutureValid)
    {
        WGPUFutureWaitInfo waitInfo = { pQueue->mWgp.mWorkDoneFuture };
        wgpuInstanceWaitAny(instance, 1, &waitInfo, UINT64_MAX);
        pQueue->mWgp.mFutureValid = false;
    }
#endif
}

void wgpu_getFenceStatus(Renderer*, Fence*, FenceStatus*) {}

/************************************************************************/
// Utility functions
/************************************************************************/
TinyImageFormat wgpu_getSupportedSwapchainFormat(Renderer* pRenderer, const SwapChainDesc* pDesc, ColorSpace colorSpace)
{
    const bool  srgb = COLOR_SPACE_SDR_SRGB == colorSpace || COLOR_SPACE_EXTENDED_SRGB == colorSpace;
    WGPUSurface surface = {};
    CreateSurface(pRenderer, pDesc->mWindowHandle, &surface);
    WGPUSurfaceCapabilities caps = {};
    wgpuSurfaceGetCapabilities(surface, pRenderer->pGpu->mWgp.pAdapter, &caps);
    TinyImageFormat fmt = TinyImageFormat_UNDEFINED;
    for (uint32_t i = 0; i < caps.formatCount; ++i)
    {
        TinyImageFormat capFormat = TinyImageFormat_FromWGPUTextureFormat((TinyImageFormat_WGPUTextureFormat)caps.formats[i]);
        if (srgb && TinyImageFormat_IsSRGB(capFormat))
        {
            fmt = capFormat;
            break;
        }
    }
    if (TinyImageFormat_UNDEFINED == fmt)
    {
        fmt = TinyImageFormat_FromWGPUTextureFormat((TinyImageFormat_WGPUTextureFormat)caps.formats[0]);
    }
    wgpuSurfaceCapabilitiesFreeMembers(caps);
    wgpuSurfaceRelease(surface);

    return fmt;
}

uint32_t wgpu_getRecommendedSwapchainImageCount(Renderer*, const WindowHandle*) { return 1; }
/************************************************************************/
// Indirect draw functions
/************************************************************************/
void     wgpu_addIndirectCommandSignature(Renderer* pRenderer, const CommandSignatureDesc* pDesc, CommandSignature** ppCommandSignature)
{
    ASSERT(pRenderer);
    ASSERT(pDesc);
    ASSERT(pDesc->mIndirectArgCount == 1);

    CommandSignature* pCommandSignature = (CommandSignature*)tf_calloc(1, sizeof(CommandSignature));
    ASSERT(pCommandSignature);

    pCommandSignature->mDrawType = pDesc->pArgDescs[0].mType;
    switch (pDesc->pArgDescs[0].mType)
    {
    case INDIRECT_DRAW:
        pCommandSignature->mStride += sizeof(IndirectDrawArguments);
        break;
    case INDIRECT_DRAW_INDEX:
        pCommandSignature->mStride += sizeof(IndirectDrawIndexArguments);
        break;
    case INDIRECT_DISPATCH:
        pCommandSignature->mStride += sizeof(IndirectDispatchArguments);
        break;
    default:
        ASSERT(false);
        break;
    }

    *ppCommandSignature = pCommandSignature;
}

void wgpu_removeIndirectCommandSignature(Renderer* pRenderer, CommandSignature* pCommandSignature)
{
    ASSERT(pRenderer);
    SAFE_FREE(pCommandSignature);
}

void wgpu_cmdExecuteIndirect(Cmd* pCmd, CommandSignature* pCommandSignature, uint maxCommandCount, Buffer* pIndirectBuffer,
                             uint64_t bufferOffset, Buffer* pCounterBuffer, uint64_t counterBufferOffset)
{
    if (pCommandSignature->mDrawType == INDIRECT_DRAW || pCommandSignature->mDrawType == INDIRECT_DRAW_INDEX)
    {
        typedef void (*PFN_wgpuRenderPassEncoderDrawIndirect)(WGPURenderPassEncoder renderPassEncoder, WGPUBuffer indirectBuffer,
                                                              uint64_t indirectOffset);
        PFN_wgpuRenderPassEncoderDrawIndirect drawIndirect =
            (pCommandSignature->mDrawType == INDIRECT_DRAW) ? wgpuRenderPassEncoderDrawIndirect : wgpuRenderPassEncoderDrawIndexedIndirect;
        typedef void (*PFN_wgpuRenderPassEncoderMultiDrawIndirect)(WGPURenderPassEncoder encoder, WGPUBuffer buffer, uint64_t offset,
                                                                   uint32_t count);
        PFN_wgpuRenderPassEncoderMultiDrawIndirect multiDrawIndirect = NULL;
        typedef void (*PFN_wgpuRenderPassEncoderMultiDrawIndirectCount)(WGPURenderPassEncoder encoder, WGPUBuffer buffer, uint64_t offset,
                                                                        WGPUBuffer count_buffer, uint64_t count_buffer_offset,
                                                                        uint32_t max_count);
        PFN_wgpuRenderPassEncoderMultiDrawIndirectCount multiDrawIndirectCount = NULL;

#if defined(WEBGPU_NATIVE)
        multiDrawIndirect = (pCommandSignature->mDrawType == INDIRECT_DRAW) ? wgpuRenderPassEncoderMultiDrawIndirect
                                                                            : wgpuRenderPassEncoderMultiDrawIndexedIndirect;
        multiDrawIndirectCount = (pCommandSignature->mDrawType == INDIRECT_DRAW) ? wgpuRenderPassEncoderMultiDrawIndirectCount
                                                                                 : wgpuRenderPassEncoderMultiDrawIndexedIndirectCount;
#endif
        ASSERT(pCmd->mWgp.mInsideRenderPass);

        if (pCmd->pRenderer->pGpu->mSettings.mMultiDrawIndirect)
        {
            if (pCounterBuffer && pCmd->pRenderer->pGpu->mSettings.mMultiDrawIndirectCount)
            {
                multiDrawIndirectCount(pCmd->mWgp.pRenderEncoder, pIndirectBuffer->mWgp.pBuffer, bufferOffset, pCounterBuffer->mWgp.pBuffer,
                                       counterBufferOffset, maxCommandCount);
            }
            else
            {
                multiDrawIndirect(pCmd->mWgp.pRenderEncoder, pIndirectBuffer->mWgp.pBuffer, bufferOffset, maxCommandCount);
            }
        }
        else
        {
            // Cannot use counter buffer when MDI is not supported. We will blindly loop through maxCommandCount
            for (uint32_t cmd = 0; cmd < maxCommandCount; ++cmd)
            {
                drawIndirect(pCmd->mWgp.pRenderEncoder, pIndirectBuffer->mWgp.pBuffer, bufferOffset + cmd * pCommandSignature->mStride);
            }
        }
    }
    else if (pCommandSignature->mDrawType == INDIRECT_DISPATCH)
    {
        ASSERT(pCmd->mWgp.mInsideComputePass);
        for (uint32_t i = 0; i < maxCommandCount; ++i)
        {
            wgpuComputePassEncoderDispatchWorkgroupsIndirect(pCmd->mWgp.pComputeEncoder, pIndirectBuffer->mWgp.pBuffer,
                                                             bufferOffset + i * pCommandSignature->mStride);
        }
    }
}
/************************************************************************/
// Query Heap Implementation
/************************************************************************/
void wgpu_getTimestampFrequency(Queue* pQueue, double* pFrequency)
{
    ASSERT(pQueue);
    ASSERT(pFrequency);

    // WebGpu already provides timestamp in nanoseconds. We only need to convert it to seconds (1 second = 1B nanoseconds)
    *pFrequency = 1000000000.0;
}

static inline FORGE_CONSTEXPR WGPUQueryType ToQueryType(QueryType type)
{
    switch (type)
    {
    case QUERY_TYPE_TIMESTAMP:
        return WGPUQueryType_Timestamp;
    case QUERY_TYPE_OCCLUSION:
        return WGPUQueryType_Occlusion;
#if defined(WEBGPU_NATIVE)
    case QUERY_TYPE_PIPELINE_STATISTICS:
        return (WGPUQueryType)WGPUNativeQueryType_PipelineStatistics;
#endif
    default:
        return WGPUQueryType_Force32;
    }
}

void wgpu_addQueryPool(Renderer* pRenderer, const QueryPoolDesc* pDesc, QueryPool** ppQueryPool)
{
    ASSERT(pRenderer);
    ASSERT(pDesc);
    ASSERT(ppQueryPool);

    QueryPool* pQueryPool = (QueryPool*)tf_calloc(1, sizeof(QueryPool));
    ASSERT(ppQueryPool);

    const uint32_t queryCount = pDesc->mQueryCount * (QUERY_TYPE_TIMESTAMP == pDesc->mType ? 2 : 1);
    uint32_t       queryStride = sizeof(uint64_t);

    DECL_CHAINED_STRUCT(WGPUQuerySetDescriptor, queryDesc);
    queryDesc.count = queryCount;
    queryDesc.type = ToQueryType(pDesc->mType);
#if defined(ENABLE_GRAPHICS_DEBUG)
    queryDesc.label = pDesc->pName;
#endif

#if defined(WEBGPU_NATIVE)
    WGPUPipelineStatisticName    pipelineStats[] = { WGPUPipelineStatisticName_VertexShaderInvocations,
                                                  WGPUPipelineStatisticName_ClipperInvocations,
                                                  WGPUPipelineStatisticName_ClipperPrimitivesOut,
                                                  WGPUPipelineStatisticName_FragmentShaderInvocations,
                                                  WGPUPipelineStatisticName_ComputeShaderInvocations };
    WGPUQuerySetDescriptorExtras queryDescExt = {};
    queryDescExt.chain.sType = (WGPUSType)WGPUSType_QuerySetDescriptorExtras;
    if (QUERY_TYPE_PIPELINE_STATISTICS == pDesc->mType)
    {
        queryDescExt.pipelineStatisticCount = TF_ARRAY_COUNT(pipelineStats);
        queryDescExt.pipelineStatistics = pipelineStats;
        ADD_TO_NEXT_CHAIN(queryDesc, &queryDescExt);
        queryStride = sizeof(uint64_t) * TF_ARRAY_COUNT(pipelineStats);
    }
#endif

    pQueryPool->mWgp.pQuerySet = wgpuDeviceCreateQuerySet(pRenderer->mWgp.pDevice, &queryDesc);
    ASSERT(pQueryPool->mWgp.pQuerySet);
    pQueryPool->mWgp.mType = queryDesc.type;
    pQueryPool->mStride = queryStride;

    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size = queryStride * queryCount;
    bufferDesc.usage = WGPUBufferUsage_QueryResolve | WGPUBufferUsage_CopySrc;
#if defined(ENABLE_GRAPHICS_DEBUG)
    bufferDesc.label = pDesc->pName;
#endif
    pQueryPool->mWgp.pResolveBuffer = wgpuDeviceCreateBuffer(pRenderer->mWgp.pDevice, &bufferDesc);
    bufferDesc.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    pQueryPool->mWgp.pReadbackBuffer = wgpuDeviceCreateBuffer(pRenderer->mWgp.pDevice, &bufferDesc);

    *ppQueryPool = pQueryPool;
}

void wgpu_removeQueryPool(Renderer* pRenderer, QueryPool* pQueryPool)
{
    ASSERT(pRenderer);
    ASSERT(pQueryPool);

    wgpuBufferDestroy(pQueryPool->mWgp.pResolveBuffer);
    wgpuBufferDestroy(pQueryPool->mWgp.pReadbackBuffer);
    wgpuQuerySetRelease(pQueryPool->mWgp.pQuerySet);

    SAFE_FREE(pQueryPool);
}

void wgpu_cmdBeginQuery(Cmd* pCmd, QueryPool* pQueryPool, QueryDesc* pQuery)
{
    ASSERT(pCmd);
    ASSERT(pQueryPool);
    ASSERT(pQuery);

    if (WGPUQueryType_Timestamp == pQueryPool->mWgp.mType)
    {
        if (pCmd->mWgp.mInsideComputePass || pCmd->mWgp.mInsideRenderPass)
        {
            ASSERTFAIL("Timestamp queries are not allowed inside render/compute passes");
            return;
        }

        const uint32_t index = pQuery->mIndex * 2;
        wgpuCommandEncoderWriteTimestamp(pCmd->mWgp.pEncoder, pQueryPool->mWgp.pQuerySet, index);
    }
#if defined(WEBGPU_NATIVE)
    else if (WGPUNativeQueryType_PipelineStatistics == pQueryPool->mWgp.mType)
    {
        if (pCmd->mWgp.mInsideComputePass)
        {
            wgpuComputePassEncoderBeginPipelineStatisticsQuery(pCmd->mWgp.pComputeEncoder, pQueryPool->mWgp.pQuerySet, pQuery->mIndex);
        }
        else if (pCmd->mWgp.mInsideRenderPass)
        {
            wgpuRenderPassEncoderBeginPipelineStatisticsQuery(pCmd->mWgp.pRenderEncoder, pQueryPool->mWgp.pQuerySet, pQuery->mIndex);
        }
    }
#endif
}

void wgpu_cmdEndQuery(Cmd* pCmd, QueryPool* pQueryPool, QueryDesc* pQuery)
{
    ASSERT(pCmd);
    ASSERT(pQueryPool);
    ASSERT(pQuery);

    if (WGPUQueryType_Timestamp == pQueryPool->mWgp.mType)
    {
        if (pCmd->mWgp.mInsideComputePass || pCmd->mWgp.mInsideRenderPass)
        {
            ASSERTFAIL("Timestamp queries are not allowed inside render/compute passes");
            return;
        }

        const uint32_t index = pQuery->mIndex * 2 + 1;
        wgpuCommandEncoderWriteTimestamp(pCmd->mWgp.pEncoder, pQueryPool->mWgp.pQuerySet, index);
    }
#if defined(WEBGPU_NATIVE)
    else if (WGPUNativeQueryType_PipelineStatistics == pQueryPool->mWgp.mType)
    {
        if (pCmd->mWgp.mInsideComputePass)
        {
            wgpuComputePassEncoderEndPipelineStatisticsQuery(pCmd->mWgp.pComputeEncoder);
        }
        else if (pCmd->mWgp.mInsideRenderPass)
        {
            wgpuRenderPassEncoderEndPipelineStatisticsQuery(pCmd->mWgp.pRenderEncoder);
        }
    }
#endif
}

void wgpu_cmdResolveQuery(Cmd* pCmd, QueryPool* pQueryPool, uint32_t startQuery, uint32_t queryCount)
{
    ASSERT(pCmd);
    ASSERT(!pCmd->mWgp.mInsideComputePass);
    ASSERT(!pCmd->mWgp.mInsideRenderPass);
    ASSERT(pQueryPool);

    const uint32_t internalQueryCount = (WGPUQueryType_Timestamp == pQueryPool->mWgp.mType ? 2 : 1);
    startQuery = startQuery * internalQueryCount;
    queryCount = queryCount * internalQueryCount;

    wgpuCommandEncoderResolveQuerySet(pCmd->mWgp.pEncoder, pQueryPool->mWgp.pQuerySet, startQuery, queryCount,
                                      pQueryPool->mWgp.pResolveBuffer, (uint64_t)startQuery * pQueryPool->mStride);

    wgpuCommandEncoderCopyBufferToBuffer(pCmd->mWgp.pEncoder, pQueryPool->mWgp.pResolveBuffer, startQuery * pQueryPool->mStride,
                                         pQueryPool->mWgp.pReadbackBuffer, startQuery * pQueryPool->mStride,
                                         queryCount * pQueryPool->mStride);
}

void wgpu_cmdResetQuery(Cmd*, QueryPool*, uint32_t, uint32_t) {}

void wgpu_getQueryData(Renderer* pRenderer, QueryPool* pQueryPool, uint32_t queryIndex, QueryData* pOutData)
{
    ASSERT(pRenderer);
    ASSERT(pQueryPool);
    ASSERT(pOutData);

    const WGPUQueryType type = pQueryPool->mWgp.mType;
    *pOutData = {};
    pOutData->mValid = true;

    const uint32_t queryCount = (WGPUQueryType_Timestamp == type ? 2 : 1);
    ReadRange      range = {};
    range.mOffset = queryIndex * queryCount * pQueryPool->mStride;
    range.mSize = queryCount * pQueryPool->mStride;
    Buffer buffer = {};
    buffer.mWgp.pBuffer = pQueryPool->mWgp.pReadbackBuffer;
    buffer.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_TO_CPU;
    mapBuffer(pRenderer, &buffer, &range);
    uint64_t* queries = (uint64_t*)((uint8_t*)buffer.pCpuMappedAddress);

    switch (type)
    {
    case WGPUQueryType_Timestamp:
    {
        pOutData->mBeginTimestamp = queries[0];
        pOutData->mEndTimestamp = queries[1];
        break;
    }
    case WGPUQueryType_Occlusion:
    {
        ASSERTFAIL("Not supported");
        pOutData->mOcclusionCounts = queries[0];
        break;
    }
#if defined(WEBGPU_NATIVE)
    case WGPUNativeQueryType_PipelineStatistics:
    {
        PipelineStatisticsQueryData* stats = &pOutData->mPipelineStats;
        stats->mCInvocations = queries[WGPUPipelineStatisticName_ClipperInvocations];
        stats->mCPrimitives = queries[WGPUPipelineStatisticName_ClipperPrimitivesOut];
        stats->mCSInvocations = queries[WGPUPipelineStatisticName_ComputeShaderInvocations];
        stats->mPSInvocations = queries[WGPUPipelineStatisticName_FragmentShaderInvocations];
        stats->mVSInvocations = queries[WGPUPipelineStatisticName_VertexShaderInvocations];
        break;
    }
#endif
    default:
        ASSERTFAIL("Not implemented");
        break;
    }

    unmapBuffer(pRenderer, &buffer);
}
/************************************************************************/
// Memory Stats Implementation
/************************************************************************/
void wgpu_calculateMemoryStats(Renderer*, char**) {}

void wgpu_freeMemoryStats(Renderer*, char*) {}

void wgpu_calculateMemoryUse(Renderer*, uint64_t*, uint64_t*) {}
/************************************************************************/
// Debug Marker Implementation
/************************************************************************/
void wgpu_cmdBeginDebugMarker(Cmd* pCmd, float, float, float, const char* pName)
{
    if (pCmd->mWgp.mInsideRenderPass)
    {
        wgpuRenderPassEncoderPushDebugGroup(pCmd->mWgp.pRenderEncoder, pName);
    }
    else if (pCmd->mWgp.mInsideComputePass)
    {
        wgpuComputePassEncoderPushDebugGroup(pCmd->mWgp.pComputeEncoder, pName);
    }
    else
    {
        wgpuCommandEncoderPushDebugGroup(pCmd->mWgp.pEncoder, pName);
    }
}

void wgpu_cmdEndDebugMarker(Cmd* pCmd)
{
    if (pCmd->mWgp.mInsideRenderPass)
    {
        wgpuRenderPassEncoderPopDebugGroup(pCmd->mWgp.pRenderEncoder);
    }
    else if (pCmd->mWgp.mInsideComputePass)
    {
        wgpuComputePassEncoderPopDebugGroup(pCmd->mWgp.pComputeEncoder);
    }
    else
    {
        wgpuCommandEncoderPopDebugGroup(pCmd->mWgp.pEncoder);
    }
}

void wgpu_cmdAddDebugMarker(Cmd* pCmd, float, float, float, const char* pName)
{
    if (pCmd->mWgp.mInsideRenderPass)
    {
        wgpuRenderPassEncoderInsertDebugMarker(pCmd->mWgp.pRenderEncoder, pName);
    }
    else if (pCmd->mWgp.mInsideComputePass)
    {
        wgpuComputePassEncoderInsertDebugMarker(pCmd->mWgp.pComputeEncoder, pName);
    }
    else
    {
        wgpuCommandEncoderInsertDebugMarker(pCmd->mWgp.pEncoder, pName);
    }
}

void wgpu_cmdWriteMarker(Cmd* pCmd, const MarkerDesc* pDesc)
{
#if defined(WEBGPU_DAWN)
    ASSERT(pCmd);
    ASSERT(pDesc);

    const uint8_t* value = (const uint8_t*)&pDesc->mValue;
    wgpuCommandEncoderWriteBuffer(pCmd->mWgp.pEncoder, pDesc->pBuffer->mWgp.pBuffer, pDesc->mOffset, value, GPU_MARKER_SIZE);
#else
    ASSERTFAIL("cmdWriteMarker not supported");
#endif
}
/************************************************************************/
// Resource Debug Naming Interface
/************************************************************************/
void wgpu_setBufferName(Renderer*, Buffer*, const char*) {}

void wgpu_setTextureName(Renderer*, Texture*, const char*) {}

void wgpu_setRenderTargetName(Renderer*, RenderTarget*, const char*) {}

void wgpu_setPipelineName(Renderer*, Pipeline*, const char*) {}

void initWebGpuRenderer(const char* appName, const RendererDesc* pSettings, Renderer** ppRenderer)
{
    // API functions
    addFence = wgpu_addFence;
    removeFence = wgpu_removeFence;
    addSemaphore = wgpu_addSemaphore;
    removeSemaphore = wgpu_removeSemaphore;
    addQueue = wgpu_addQueue;
    removeQueue = wgpu_removeQueue;
    addSwapChain = wgpu_addSwapChain;
    removeSwapChain = wgpu_removeSwapChain;

    // command pool functions
    addCmdPool = wgpu_addCmdPool;
    removeCmdPool = wgpu_removeCmdPool;
    addCmd = wgpu_addCmd;
    removeCmd = wgpu_removeCmd;
    addCmd_n = wgpu_addCmd_n;
    removeCmd_n = wgpu_removeCmd_n;

    addRenderTarget = wgpu_addRenderTarget;
    removeRenderTarget = wgpu_removeRenderTarget;
    addSampler = wgpu_addSampler;
    removeSampler = wgpu_removeSampler;

    // Resource Load functions
    addResourceHeap = wgpu_addResourceHeap;
    removeResourceHeap = wgpu_removeResourceHeap;
    getBufferSizeAlign = wgpu_getBufferSizeAlign;
    getTextureSizeAlign = wgpu_getTextureSizeAlign;
    addBuffer = wgpu_addBuffer;
    removeBuffer = wgpu_removeBuffer;
    mapBuffer = wgpu_mapBuffer;
    unmapBuffer = wgpu_unmapBuffer;
    cmdUpdateBuffer = wgpu_cmdUpdateBuffer;
    cmdUpdateSubresource = wgpu_cmdUpdateSubresource;
    cmdCopySubresource = wgpu_cmdCopySubresource;
    addTexture = wgpu_addTexture;
    removeTexture = wgpu_removeTexture;

    // shader functions
    addShaderBinary = wgpu_addShaderBinary;
    removeShader = wgpu_removeShader;

    addRootSignature = wgpu_addRootSignature;
    removeRootSignature = wgpu_removeRootSignature;
    getDescriptorIndexFromName = wgpu_getDescriptorIndexFromName;

    // pipeline functions
    addPipeline = wgpu_addPipeline;
    removePipeline = wgpu_removePipeline;
    addPipelineCache = wgpu_addPipelineCache;
    getPipelineCacheData = wgpu_getPipelineCacheData;
    removePipelineCache = wgpu_removePipelineCache;

    // Descriptor Set functions
    addDescriptorSet = wgpu_addDescriptorSet;
    removeDescriptorSet = wgpu_removeDescriptorSet;
    updateDescriptorSet = wgpu_updateDescriptorSet;

    // command buffer functions
    resetCmdPool = wgpu_resetCmdPool;
    beginCmd = wgpu_beginCmd;
    endCmd = wgpu_endCmd;
    cmdBindRenderTargets = wgpu_cmdBindRenderTargets;
    cmdSetSampleLocations = wgpu_cmdSetSampleLocations;
    cmdSetViewport = wgpu_cmdSetViewport;
    cmdSetScissor = wgpu_cmdSetScissor;
    cmdSetStencilReferenceValue = wgpu_cmdSetStencilReferenceValue;
    cmdBindPipeline = wgpu_cmdBindPipeline;
    cmdBindDescriptorSet = wgpu_cmdBindDescriptorSet;
    cmdBindPushConstants = wgpu_cmdBindPushConstants;
    cmdBindDescriptorSetWithRootCbvs = wgpu_cmdBindDescriptorSetWithRootCbvs;
    cmdBindIndexBuffer = wgpu_cmdBindIndexBuffer;
    cmdBindVertexBuffer = wgpu_cmdBindVertexBuffer;
    cmdDraw = wgpu_cmdDraw;
    cmdDrawInstanced = wgpu_cmdDrawInstanced;
    cmdDrawIndexed = wgpu_cmdDrawIndexed;
    cmdDrawIndexedInstanced = wgpu_cmdDrawIndexedInstanced;
    cmdDispatch = wgpu_cmdDispatch;

    // Transition Commands
    cmdResourceBarrier = wgpu_cmdResourceBarrier;

    // queue/fence/swapchain functions
    acquireNextImage = wgpu_acquireNextImage;
    queueSubmit = wgpu_queueSubmit;
    queuePresent = wgpu_queuePresent;
    waitQueueIdle = wgpu_waitQueueIdle;
    getFenceStatus = wgpu_getFenceStatus;
    waitForFences = wgpu_waitForFences;
    toggleVSync = wgpu_toggleVSync;

    getSupportedSwapchainFormat = wgpu_getSupportedSwapchainFormat;
    getRecommendedSwapchainImageCount = wgpu_getRecommendedSwapchainImageCount;

    // indirect Draw functions
    addIndirectCommandSignature = wgpu_addIndirectCommandSignature;
    removeIndirectCommandSignature = wgpu_removeIndirectCommandSignature;
    cmdExecuteIndirect = wgpu_cmdExecuteIndirect;

    /************************************************************************/
    // GPU Query Interface
    /************************************************************************/
    getTimestampFrequency = wgpu_getTimestampFrequency;
    addQueryPool = wgpu_addQueryPool;
    removeQueryPool = wgpu_removeQueryPool;
    cmdBeginQuery = wgpu_cmdBeginQuery;
    cmdEndQuery = wgpu_cmdEndQuery;
    cmdResolveQuery = wgpu_cmdResolveQuery;
    cmdResetQuery = wgpu_cmdResetQuery;
    getQueryData = wgpu_getQueryData;
    /************************************************************************/
    // Stats Info Interface
    /************************************************************************/
    calculateMemoryStats = wgpu_calculateMemoryStats;
    calculateMemoryUse = wgpu_calculateMemoryUse;
    freeMemoryStats = wgpu_freeMemoryStats;
    /************************************************************************/
    // Debug Marker Interface
    /************************************************************************/
    cmdBeginDebugMarker = wgpu_cmdBeginDebugMarker;
    cmdEndDebugMarker = wgpu_cmdEndDebugMarker;
    cmdAddDebugMarker = wgpu_cmdAddDebugMarker;
    cmdWriteMarker = wgpu_cmdWriteMarker;
    /************************************************************************/
    // Resource Debug Naming Interface
    /************************************************************************/
    setBufferName = wgpu_setBufferName;
    setTextureName = wgpu_setTextureName;
    setRenderTargetName = wgpu_setRenderTargetName;
    setPipelineName = wgpu_setPipelineName;

    wgpu_initRenderer(appName, pSettings, ppRenderer);
}

void exitWebGpuRenderer(Renderer* pRenderer)
{
    ASSERT(pRenderer);

    wgpu_exitRenderer(pRenderer);
}

void initWebGpuRendererContext(const char* appName, const RendererContextDesc* pSettings, RendererContext** ppContext)
{
    // No need to initialize API function pointers, initRenderer MUST be called before using anything else anyway.
    wgpu_initRendererContext(appName, pSettings, ppContext);
}

void exitWebGpuRendererContext(RendererContext* pContext)
{
    ASSERT(pContext);

    wgpu_exitRendererContext(pContext);
}

#endif
