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

// Interfaces
#include "../../../../Common_3/Application/Interfaces/IApp.h"
#include "../../../../Common_3/Application/Interfaces/ICameraController.h"
#include "../../../../Common_3/Application/Interfaces/IFont.h"
#include "../../../../Common_3/Application/Interfaces/IInput.h"
#include "../../../../Common_3/Application/Interfaces/IProfiler.h"
#include "../../../../Common_3/Application/Interfaces/IScreenshot.h"
#include "../../../../Common_3/Application/Interfaces/IUI.h"
#include "../../../../Common_3/Game/Interfaces/IScripting.h"
#include "../../../../Common_3/Utilities/Interfaces/IFileSystem.h"
#include "../../../../Common_3/Utilities/Interfaces/ILog.h"
#include "../../../../Common_3/Utilities/Interfaces/ITime.h"

#include "../../../../Common_3/Utilities/RingBuffer.h"

// Renderer
#include "../../../../Common_3/Graphics/Interfaces/IGraphics.h"
#include "../../../../Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"

// Math
#include "../../../../Common_3/Resources/ResourceLoader/ThirdParty/OpenSource/tinyktx/tinyktx.h"

#include "../../../../Common_3/Resources/ResourceLoader/TextureContainers.h"
#include "../../../../Common_3/Utilities/Math/MathTypes.h"

#include "SunTempleGeometry.h"

#include "../../../../Common_3/Utilities/Interfaces/IMemory.h"

// Shadow defines
#include "Shaders/FSL/Culling/Light/light_cull_resources.h.fsl"
#include "Shaders/FSL/ShadowMapping/shadow_resources_defs.h.fsl"

typedef struct CullingViewPort
{
    float2 windowSize;
    uint   sampleCount;
    uint   _pad0;
} CullingViewPort;

typedef struct DrawArgsBound
{
    float3 Min;
    float3 Max;
} DrawArgsBound;

typedef struct DirectionalLightData
{
    float3 mColor;
    float  mIntensity;

    float3 mDirection;
    float  mPadding;
} DirectionalLightData;

typedef struct PointLightData
{
    float3 mColor;
    float  mIntensity;

    float3 mPosition;
    float  mRadius;
} PointLightData;

typedef struct UniformBlock
{
    mat4                 mProjectView;
    mat4                 mView;
    mat4                 mProjection;
    float4               mCamPos;
    DirectionalLightData mDirectionalLight = { float3(1.0f, 1.0f, 1.0f), 10.0f, float3(-0.856996f, 0.486757f, -0.169190f), 0.0f };

    // Point light information
    PointLightData mPointLights[MAX_POINT_LIGHTS] = {
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(-1.667957f, 1.999129f, 70.885094f), 1.0f },
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(1.464530f, 1.999129f, 70.885094f), 1.0f },
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(-0.030298f, 0.969758f, 58.014256f), 1.0f },
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(2.088037f, 1.998678f, 50.801563f), 1.0f },
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(-7.194956f, 1.993270f, 43.253101f), 1.0f },
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(-0.030202f, 1.222947f, 31.555765f), 1.0f },
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(7.150401f, 1.993270f, 43.253101f), 1.0f },
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(-2.413177f, 1.993272f, 28.526716f), 1.0f },
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(-7.464443f, 3.991693f, 18.076118f), 1.0f },
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(-2.072891f, 3.991697f, -7.759324f), 1.0f },
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(-7.890412f, 3.983514f, -2.141179f), 1.0f },
        { float3(1.0f, 0.392161f, 0.122f), 18.5f, float3(8.020940f, 3.983514f, -2.059732f), 1.0f }
    };
    float4 mPointLightCount = float4(12.0f, 0.0f, 0.0f, 0.0f); // x:Count of point light..

    CullingViewPort mCullingViewPort[NUM_CULLING_VIEWPORTS];
} UniformBlock;

struct UniformBlockSky
{
    mat4 mProjectView;
};

struct
{
    float3 mSunControl = { 33.333f, 18.974f, -41.667f };
    float  mSunSpeedY = 0.025f;
} gLightCpuSettings;

#define MAX_BLUR_KERNEL_SIZE 64
struct BlurWeights
{
    float mBlurWeights[MAX_BLUR_KERNEL_SIZE];
};

// But we only need Two sets of resources (one in flight and one being used on CPU)
const uint32_t gDataBufferCount = 2;

Renderer* pRenderer = NULL;

Queue*     pGraphicsQueue = NULL;
GpuCmdRing gGraphicsCmdRing = {};

SwapChain*    pSwapChain = NULL;
RenderTarget* pDepthBuffer = NULL;
Semaphore*    pImageAcquiredSemaphore = NULL;

/************************************************************************/
// Scene
/************************************************************************/
Scene*       pScene = NULL;
VertexLayout gSceneVertexLayout;
VertexLayout gSceneVertexLayoutPositionsOnly;
VertexLayout gSceneVertexLayoutPosAndTex;
uint32_t     gMeshCount;
uint32_t     gMaterialCount;

CommandSignature* pCmdSignatureScenePass = nullptr;
Buffer*           gInstanceDataBuffer = NULL;
Buffer*           gIndirectInstanceDataBuffer[gDataBufferCount] = { NULL }; // to be filled
/************************************************************************/
// Bindless texture array
/************************************************************************/
struct MaterialInfo
{
    Texture* pDiffuseMap;
    Texture* pNormalMap;
    Texture* pSpecularMap;
    Texture* pEmissiveMap;

    MeshSetting mSetting;
    uint32_t    mDrawArg; // the index into pDrawArgs
};
MaterialInfo* gMaterialsInfo;
Buffer*       gMaterialsBuffer;

Shader*   pForwardShaders = NULL;
Pipeline* pForwardPipeline = NULL;
Shader*   pTerrainShaders = NULL;
Pipeline* pTerrainPipeline = NULL;
Pipeline* pTransparentForwardPipeline = NULL;

DescriptorSet* pDescriptorSetMaterials = { NULL };
DescriptorSet* pDescriptorSetMaterialsTesting = { NULL };

uint32_t  gMeshTypesCount[MT_COUNT_MAX] = { 0 };
uint32_t* gSceneDrawArgsIndices[MT_COUNT_MAX];

int MaterialCompare(const void* a, const void*)
{
    MaterialInfo* mi = (MaterialInfo*)a;
    bool          bIsTransparent = (mi->mSetting.mFlags & MATERIAL_FLAG_ALPHA_TESTED) != 0;
    return bIsTransparent;
}

/************************************************************************/
// Skybox
/************************************************************************/
Shader*        pSkyBoxDrawShader = NULL;
Buffer*        pSkyBoxVertexBuffer = NULL;
Pipeline*      pSkyBoxDrawPipeline = NULL;
RootSignature* pRootSignatureScene = NULL;
RootSignature* pRootSignatureSkybox = NULL;

Sampler* pSamplerSkyBox = NULL;
Sampler* pSamplerSunTempleAlbedo = NULL;
Sampler* pSamplerSunTempleLightmap = NULL;
Sampler* pSamplerSunTempleTerrainNormal = NULL;
Sampler* pSamplerMiplessNearest = NULL;
Sampler* pSamplerBilinearClamp = NULL;
Sampler* pSamplerNearestClamp = NULL;

Texture*       pSkyBoxTexture = { NULL };
DescriptorSet* pDescriptorSetSkyboxTexture = { NULL };
DescriptorSet* pDescriptorSetUniformsScene = { NULL };
DescriptorSet* pDescriptorSetUniformsSceneAABB = { NULL };
DescriptorSet* pDescriptorSetUniformsSkybox = { NULL };

Buffer* pProjViewUniformBuffer[gDataBufferCount] = { NULL };
Buffer* pSkyboxUniformBuffer[gDataBufferCount] = { NULL };

Buffer* pBufferBlurWeights = NULL;

/************************************************************************/
// Shadow Mapping
/************************************************************************/
float gCascadeSplitLambda = 0.5f;

typedef struct ShadowMapping
{
    RootSignature* pRootSignature = NULL;
    RootSignature* pRootSignatureAlpha = NULL;

    Shader* pShaderDepth = NULL;
    Shader* pShaderDepthAlpha = NULL;

    Pipeline* pPipelineDepth = NULL;
    Pipeline* pPipelineDepthAlpha = NULL;

    DescriptorSet* pDescriptorSetUniformsCascades = { NULL };
    DescriptorSet* pDescriptorSetAlphaTextures = { NULL };
    DescriptorSet* pDescriptorSetAlphaUniforms = { NULL };

    Buffer* pBufferUniform[gDataBufferCount] = { NULL };

    // Render Targets
    Texture*      pShadowMapTextures[kShadowMapCascadeCount] = { NULL };
    RenderTarget* pShadowMaps[kShadowMapCascadeCount] = { NULL };
    const char*   pCascadeTextureNames[kMaxShadowMapCascadeCount] = { "ShadowTextureCascade0", "ShadowTextureCascade1",
                                                                    "ShadowTextureCascade2", "ShadowTextureCascade3",
                                                                    "ShadowTextureCascade4" };
} ShadowMapping;

uint32_t kShadowMapResWidth = 2048u;
uint32_t kShadowMapResHeight = 2048u;

ShadowMapping gShadowMapping;
RenderTarget* pRenderTargetShadowMap = { NULL };
RenderTarget* pRenderTargetShaderMapBlur = NULL;

bool gUseRealTimeShadows = false;
bool gRealTimeShadowsEnabled = false;

struct ShadowCascade
{
    mat4   mViewProjMatrix[kShadowMapCascadeCount];
    float4 mSplitDepth[3];
    float4 mSettings;
};
ShadowCascade gShadowCascades;
Buffer*       gBufferShadowCascades[gDataBufferCount];

/************************************************************************/
// Baked lighting
/************************************************************************/
Texture* pBakedLightMap = NULL;

/************************************************************************/
// IBL
/************************************************************************/
Texture* pIrradianceTexture = { NULL };
Texture* pPrefilteredEnvTexture = { NULL };
Texture* pBrdfTexture = NULL;

/************************************************************************/
// Clear light clusters pipeline
/************************************************************************/
Shader*        pShaderClearLightClusters = NULL;
Pipeline*      pPipelineClearLightClusters = NULL;
RootSignature* pRootSignatureLightClusters = NULL;
DescriptorSet* pDescriptorSetLightClusters = NULL;
/************************************************************************/
// Compute light clusters pipeline
/************************************************************************/
Shader*        pShaderClusterLights = NULL;
Pipeline*      pPipelineClusterLights = NULL;

Buffer* pLightClustersCount = { NULL };
Buffer* pLightClusters = { NULL };

/************************************************************************/
// Gaussian Blur pipelines
/************************************************************************/
//#define BLUR_PIPELINE
#define PASS_TYPE_HORIZONTAL 0
#define PASS_TYPE_VERTICAL   1
#ifdef BLUR_PIPELINE
Shader*        pShaderBlurComp[2] = { NULL };
Pipeline*      pPipelineBlur[2] = { NULL };
RootSignature* pRootSignatureBlurCompute[2] = { NULL };
DescriptorSet* pDescriptorSetBlurCompute[2] = { NULL };
#endif

uint32_t     gFrameIndex = 0;
uint64_t     gFrameCount = 0;
ProfileToken gGraphicsProfileToken = PROFILE_INVALID_TOKEN;

UniformBlock    gUniformData;
UniformBlockSky gUniformDataSky;
UniformBlockSky gUniformDataDebug;

#ifdef BLUR_PIPELINE
BlurWeights gBlurWeightsUniform;
float       gGaussianBlurSigma[2] = { 1.0f, 1.0f };
#endif

ICameraController* pCameraController = NULL;

UIComponent* pGuiWindow = NULL;
UIComponent* pDebugTexturesWindow = NULL;

uint32_t gFontID = 0;

QueryPool* pPipelineStatsQueryPool[gDataBufferCount] = {};

const char* pSkyBoxImageFileName = "suntemple_cube.tex";

FontDrawDesc gFrameTimeDraw;

const uint32_t gMaxRenderTargetFormats = 3;
uint32_t       gRenderTargetFormatWidgetData = 0;
char*          gRenderTargetFormatNames[gMaxRenderTargetFormats] = { NULL };
uint32_t       gNumRenderTargetFormats = 0;

// Intermediate render target to align non-srgb swapchain images..
RenderTarget* pIntermediateRenderTarget = NULL;

/************************************************************************/
// Gamma Correction
/************************************************************************/
typedef struct GammaCorrectionData
{
    typedef struct GammaCorrectionUniformData
    {
        float4 mGammaCorrectionData = float4(1.5f, 2.2f, 0.0f, 0.0f); // xy: [Gamma, Exposure], zw: Padding/Extra
    } GammaCorrectionUniformData;
    GammaCorrectionUniformData mGammaCorrectionUniformData;

    Shader*        pShader;
    Pipeline*      pPipeline;
    RootSignature* pRootSignature;

    Buffer*        pGammaCorrectionBuffer[gDataBufferCount] = { NULL };
    DescriptorSet* pSetTexture;
    DescriptorSet* pSetUniform;
} GammaCorrectionData;
GammaCorrectionData gGammaCorrectionData;

/************************************************************************/
// Camera Walk through data
/************************************************************************/
typedef struct CameraWalkData
{
    uint32_t mNumTimes;
    uint32_t mNumPositions;
    uint32_t mNumRotations;

    float*  mTimes;
    float3* mPositions;
    float4* mRotations;

    bool     mIsWalking = false;
    float    mWalkSpeed = 1.0f;
    uint32_t mCurrentFrame = 0;
    float    mWalkingTime = 0.0f;
} CameraWalkData;
CameraWalkData gCameraWalkData;

/************************************************************************/
// Frustum Culling Data
/************************************************************************/
struct CameraFrustumPlane
{
    CameraFrustumPlane()
    {
        mNormal = vec3(0.0f);
        mDistance = 0.0f;
    }

    CameraFrustumPlane(const Vector3& argsNormal, float argsDistance)
    {
        mNormal = normalize(argsNormal);
        mDistance = argsDistance;
    }

    vec3 AbsNormal() { return vec3(abs(mNormal[0]), abs(mNormal[1]), abs(mNormal[2])); }

    Vector3 mNormal;
    float   mDistance;
};

typedef struct CameraFrustumSettings
{
    float mAspectRatio;
    float mWidthMultiplier;
    float mNearPlaneDistance;
    float mFarPlaneDistance;
} CameraFrustumSettings;

typedef struct CameraFrustum
{
    // Frustum planes..
    CameraFrustumPlane mNearPlane, mFarPlane, mTopPlane, mBottomPlane, mLeftPlane, mRightPlane;

    // Debug vertices...
    vec3 mFarTopLeftVert, mFarTopRightVert, mFarBottomLeftVert, mFarBottomRightVert;
    vec3 mNearTopLeftVert, mNearTopRightVert, mNearBottomLeftVert, mNearBottomRightVert;

    float mFarPlaneHeight, mFarPlaneWidth, mNearPlaneHeight, mNearPlaneWidth;

    CameraFrustumSettings mSettings;
} CameraFrustum;
CameraFrustum         gCameraFrustum;
CameraFrustumSettings gCFSettings;

/************************************************************************/
// Occlusion Culling
/************************************************************************/
typedef enum CullShaderType
{
    CST_FRUSTUM_CULL_PASS = 0,
    CST_COUNT_MAX = 1
} OcclusionShaderType;
typedef struct CullUniformBlock
{
    mat4  mProject;
    mat4  mProjectView;
    vec4  mCameraFrustumPlanes[6];
    uint4 mNumMeshes = uint4(0);
} CullUniformBlock;
typedef struct CullRenderData
{
    Shader*        pShaders[CST_COUNT_MAX] = { NULL };
    Pipeline*      pPipelines[CST_COUNT_MAX] = { NULL };
    RootSignature* pRootSignatures[CST_COUNT_MAX] = { NULL };

    Buffer* pBufferUniformCull[gDataBufferCount] = { NULL };
    Buffer* pBoundsBuffer = { NULL };

    DescriptorSet* pSetUpdatePerFrame[CST_COUNT_MAX] = { NULL };
    DescriptorSet* pSetUpdateNone[CST_COUNT_MAX] = { NULL };
} CullRenderData;
bool gUseFrustumCulling = true;

CullRenderData   gCullData;
CullUniformBlock gCullUniformBlock;

bool gLightCullingEnabled = true;
bool gUseLightCulling = true;

constexpr uint32_t kNumViewPositions = 4;
const char*        gViewPositionNames[kNumViewPositions] = { "None", "Perf. 1", "Perf. 2", "Perf. 3" };
float3             gViewPositions[kNumViewPositions - 1] = { float3(5.0f, 12.5f, 7.5f), float3(-6.667f, 0.833f, 36.667f),
                                                 float3(-0.833f, 0.833f, 70.833f) };
float3             gViewLookAtPositions[kNumViewPositions - 1] = { float3(0.0f, 7.5f, 0.0f), float3(22.5f, 13.333f, 0.0f),
                                                       float3(20.0f, 30.833f, 0.0f) };
uint32_t           gViewPoistionsWidgetData = 0;

/************************************************************************/
// CPU Stress Testing
/************************************************************************/
#define CPU_STRESS_TESTING_ENABLED
#if defined(CPU_STRESS_TESTING_ENABLED)
typedef enum CPUStressTestType
{
    CSTT_NONE = -1,
    CSTT_COMMAND_ENCODING = 0,   // Time it takes to encode command buffers...
    CSTT_COMMAND_SUBMISSION = 1, // Time it takes to submit command buffers...
    CSTT_BIND_GROUP_UPDATES = 2,
    CSTT_BIND_GROUP_BINDINGS = 3,
    CSTT_ALL = 4
} CPUStressTestType;

const uint32_t kNumCpuStressTests = 8u;
const uint32_t kNumCpuStressTestSamples = 12u;
const uint32_t kNumCpuStressTestDrawCallIncrements = 1024u;

typedef struct CPUStressTestSample
{
    uint32_t mCount; // DrawCalls, Updates... etc
    float    mTime;
} CPUStressTestSample;

typedef struct CPUStressTest
{
    CPUStressTestSample mSamples[2][kNumCpuStressTests][kNumCpuStressTestSamples];
    ProfileToken        mToken;
    uint64_t            mTotalSamplesTaken[2];

    void (*Run)(GpuCmdRingElement* pElem, RenderTarget* pRenderTargetSwapChain);
} CPUStressTest;

typedef struct GraphLineData2D
{
    float2 start;
    float2 end;
} GraphLineData2D;
typedef struct CPUStressTestData
{
#define CPU_VULKAN_IDX 0
#define CPU_WEBGPU_IDX 1
    const char* kTypeStrings[CSTT_ALL + 1] = { "CommandsEncoding", "CommandsSubmission", "BindGroupUpdates", "BindGroupBinding", "All" };

    uint32_t mTypeWidgetData = 0;

    uint32_t mCurrentSample = 0u;
    uint32_t mCurrentTest = 0u;

    uint32_t mCurrentTestType = CSTT_COMMAND_ENCODING;
    uint32_t mNewTestType = CSTT_COMMAND_ENCODING;

    GraphLineData2D* mPlotData = NULL;
    uint32_t         mNumPlotData = 0;

    Shader*        pShader;
    RootSignature* pRootSignature;
    Pipeline*      pPipeline;
    DescriptorSet* pSet;

    RenderTarget* pRenderTarget;

    Buffer* pUniformBuffer = NULL;
    Buffer* pVertexBuffer = NULL;

    HiresTimer mTimer;

    bool bShouldStartTest = false;
    bool bIsTestRunning = false;
    bool bWasTestRunning = false;

    bool bAlreadyReloaded = false;

    bool bShouldTakeScreenshot = false;
    char screenShotName[512];

    Fence* pSubmissionFence = NULL;
} CPUStressTestData;
typedef struct GridInfo
{
    float mWidth;
    float mHeight;

    float2 mCenter;
    float2 mGridOrigin;

    float2 mRectSize;
    float2 mTickSize;
} GridInfo;

CPUStressTestData gCpuStressTestData;
CPUStressTest     gCpuStressTests[CSTT_ALL];
UIComponent*      pCpuStressTestWindow = NULL;

// For showing stress test completion data live..
static char gCpuStressTestStr[4][64] = {};
#endif

/************************************************************************/
// CPU Profiling
/************************************************************************/
ProfileToken gCpuFrameTimeToken;
ProfileToken gCpuUpdateToken;
ProfileToken gCpuDrawToken;
ProfileToken gCpuDrawPresentationToken;
ProfileToken gCpuDrawSceneForwardToken;
ProfileToken gCpuDrawSceneForwardSubmissionToken;

//#define BAKE_SHADOW_MAPS
#if defined(BAKE_SHADOW_MAPS)
bool gShadowMapsReadyForBake = false;
bool gShadowMapsBaked = false;
#endif

void reloadRequest(void*)
{
    ReloadDesc reload{ RELOAD_TYPE_SHADER };
    requestReload(&reload);
}

void saveRenderTargetToPng(RenderTarget* pRenderTarget, char* ssName)
{
    // Allocate temp space
    const uint32_t rowAlignment = max(1u, pRenderer->pGpu->mSettings.mUploadBufferTextureRowAlignment);

    const uint32_t width = pRenderTarget->pTexture->mWidth;
    const uint32_t height = pRenderTarget->pTexture->mHeight;
    const uint8_t  channelCount = (uint8_t)TinyImageFormat_ChannelCount(pRenderTarget->mFormat);

    uint32_t bpp = TinyImageFormat_BitSizeOfBlock(pRenderTarget->mFormat);
    uint16_t byteSize = (uint16_t)TinyImageFormat_BitSizeOfBlock(pRenderTarget->mFormat) / 8;

    uint32_t blockWidth = TinyImageFormat_WidthOfBlock(pRenderTarget->mFormat);
    uint32_t blockHeight = TinyImageFormat_HeightOfBlock(pRenderTarget->mFormat);
    uint32_t numBlocksWide = 0;
    uint32_t numBlocksHigh = 0;
    if (width > 0)
    {
        numBlocksWide = max(1U, (width + (blockWidth - 1)) / blockWidth);
    }
    if (height > 0)
    {
        numBlocksHigh = max(1u, (height + (blockHeight - 1)) / blockHeight);
    }

    uint32_t rowBytes = round_up(numBlocksWide * (bpp >> 3), rowAlignment);
    uint32_t rowBytesNoAlign = numBlocksWide * (bpp >> 3);
    // uint32_t numRows = round_up(numBlocksHigh, rowAlignment);
    uint32_t numBytes = rowBytes * numBlocksHigh;

    uint8_t* alloc = (uint8_t*)tf_malloc(numBytes);

    // Generate image data buffer.
    //
    // Add a staging buffer.
    SyncToken      stPlotVB = {};
    Buffer*        buffer = 0;
    BufferLoadDesc bufferDesc = {};
    bufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_RW_BUFFER;
    bufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_TO_CPU;
    bufferDesc.mDesc.mSize = numBytes;
    bufferDesc.mDesc.mStartState = RESOURCE_STATE_COPY_DEST;
    bufferDesc.pData = NULL;
    bufferDesc.ppBuffer = &buffer;
    addResource(&bufferDesc, &stPlotVB);
    waitForToken(&stPlotVB);

    SyncToken       stTextureCopy = 0;
    TextureCopyDesc copyDesc = {};
    copyDesc.pTexture = pRenderTarget->pTexture;
    copyDesc.pBuffer = buffer;
    copyDesc.pWaitSemaphore = NULL;
    copyDesc.mTextureState = RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    // Barrier - Info to copy engine that the resource will use transfer queue to copy the texture...
    copyDesc.mQueueType = QUEUE_TYPE_TRANSFER;
    copyResource(&copyDesc, &stTextureCopy);
    waitForToken(&stTextureCopy);

    // Copy to CPU memory.
    BufferUpdateDesc sbDesc = { buffer, 0, byteSize };
    beginUpdateResource(&sbDesc);
    memcpy(alloc, sbDesc.pMappedData, numBytes);
    endUpdateResource(&sbDesc);

    // We have to realign the rows on webgpu
    if (pRenderer->mRendererApi == RENDERER_API_WEBGPU)
    {
        uint32_t size = pRenderTarget->mWidth * pRenderTarget->mHeight * max((uint16_t)4U, byteSize);
        uint8_t* nalloc = (uint8_t*)tf_malloc(size);
        for (uint32_t h = 0; h < pRenderTarget->mHeight; ++h)
        {
            uint32_t pixelIndex = h * rowBytes;
            uint32_t nPixelIndex = h * rowBytesNoAlign;
            memcpy((void*)&nalloc[nPixelIndex], (void*)&alloc[pixelIndex], rowBytesNoAlign);
        }
        saveRenderTargetDataToPng(pSwapChain, pRenderTarget, nalloc, byteSize, channelCount, ssName, false, false);
        tf_free(nalloc);
    }
    else
    {
        saveRenderTargetDataToPng(pSwapChain, pRenderTarget, alloc, byteSize, channelCount, ssName, false, false);
    }

    removeResource(buffer);
    tf_free(alloc);
}

#if defined(CPU_STRESS_TESTING_ENABLED)
uint32_t GetCpuApiDataIndex() { return pRenderer->mRendererApi == RENDERER_API_WEBGPU ? CPU_WEBGPU_IDX : CPU_VULKAN_IDX; }

bool cpuIsValidTest(uint32_t test) { return gCpuStressTestData.mCurrentTestType == test && gCpuStressTestData.bIsTestRunning; }
bool cpuIsTestRunning() { return gCpuStressTestData.bIsTestRunning; }
void cpuToggleStressTest(void* pUserData)
{
    gCpuStressTestData.bIsTestRunning = !gCpuStressTestData.bIsTestRunning;
    if (gCpuStressTestData.bIsTestRunning)
    {
        gCpuStressTestData.bShouldStartTest = true;
    }
}

void cpuStressTestUpdate(float deltaTime);

void cpuStressTestRun(GpuCmdRingElement* pElem, RenderTarget* pRenderTargetSwapchain)
{
    if (gCpuStressTestData.mCurrentTestType != CSTT_ALL)
        gCpuStressTests[gCpuStressTestData.mCurrentTestType].Run(pElem, pRenderTargetSwapchain);
}
void cpuStressTestCommandsEncoding(GpuCmdRingElement* pElem, RenderTarget* pRenderTargetSwapchain);
void cpuStressTestCommandsSubmission(GpuCmdRingElement* pElem, RenderTarget* pRenderTargetSwapchain);
void cpuStressTestBindGroupUpdates(GpuCmdRingElement* pElem, RenderTarget* pRenderTargetSwapchain);
void cpuStressTestBindGroupBindings(GpuCmdRingElement* pElem, RenderTarget* pRenderTargetSwapchain);

void cpuUpdateGraphDataForTest(uint32_t testTypeIdx, const GridInfo& gridInfo, uint32_t& numGridPoints, uint32_t& graphedLineStartIdx,
                               float& xIntervals, float& yIntervals);
void cpuDrawStressTestDataToRenderTarget(Cmd* pCmd, RenderTarget* pRenderTarget, const GridInfo& gInfo, uint32_t testTypeIdx,
                                         uint32_t numGridPoints, uint32_t graphedLineStartIdx, float xIntervals, float yIntervals);
void cpuGraphStressTestData(GpuCmdRingElement* pGraphicsElem, RenderTarget* pRenderTarget);
void cpuSaveGraphStressTestData(void* pUserData)
{
    if (!gCpuStressTestData.bShouldTakeScreenshot && gFrameCount > kNumCpuStressTests)
    {
        if (pRenderer->mRendererApi != RENDERER_API_WEBGPU && pRenderer->mRendererApi != RENDERER_API_VULKAN)
            return;

        gCpuStressTestData.bShouldTakeScreenshot = true;
    }
}
#else
bool cpuIsTestRunning() { return false; }
void cpuStressTestUpdate(float) {}
void cpuStressTestRun(GpuCmdRingElement*, RenderTarget*) {}
#endif

extern PlatformParameters gPlatformParameters;
extern uint32_t           gSelectedApiIndex;
class SunTemple: public IApp
{
public:
    static float gaussian(float x, float m, float sigma)
    {
        x = abs(x - m) / sigma;
        x *= x;
        return exp(-0.5f * x);
    }

    static void initCameraFrustum(CameraFrustum& frustum, CameraFrustumSettings& settings)
    {
        frustum.mSettings = settings;

        frustum.mNearPlaneHeight = settings.mNearPlaneDistance / settings.mWidthMultiplier;
        frustum.mNearPlaneWidth = frustum.mNearPlaneHeight * settings.mAspectRatio;

        frustum.mFarPlaneHeight = settings.mFarPlaneDistance / settings.mWidthMultiplier;
        frustum.mFarPlaneWidth = frustum.mFarPlaneHeight * settings.mAspectRatio;
    }

    static void createCameraFrustumPlane(const vec3& a, const vec3& b, const vec3& c, CameraFrustumPlane& outPlane)
    {
        vec3 edgeA = b - a;
        vec3 edgeB = c - b;
        vec3 normal = cross(edgeA, edgeB);
        normal = normalize(normal);

        outPlane = CameraFrustumPlane(normal, dot(normal, a));
    }

    static void createCameraFrustum(CameraFrustum& frustum, const mat4& cameraModel, const vec3& cameraPosition)
    {
        vec3 CameraRight = cameraModel.getCol0().getXYZ() * 0.5f;
        vec3 CameraUp = cameraModel.getCol1().getXYZ() * 0.5f;
        vec3 CameraForward = cameraModel.getCol2().getXYZ();
        // vec3 CameraPosition = cameraModel.getCol3().getXYZ();

        vec3 farPlaneCenter = cameraPosition + CameraForward * frustum.mSettings.mFarPlaneDistance;
        vec3 nearPlaneCenter = cameraPosition + CameraForward * frustum.mSettings.mNearPlaneDistance;

        vec3 cameraUpFPH = CameraUp * frustum.mFarPlaneHeight;
        vec3 cameraUpNPH = CameraUp * frustum.mNearPlaneHeight;

        vec3 cameraRightFPW = CameraRight * frustum.mFarPlaneWidth;
        vec3 cameraRightNPW = CameraRight * frustum.mNearPlaneWidth;

        frustum.mFarTopLeftVert = farPlaneCenter + cameraUpFPH - cameraRightFPW;
        frustum.mFarTopRightVert = farPlaneCenter + cameraUpFPH + cameraRightFPW;
        frustum.mFarBottomLeftVert = farPlaneCenter - cameraUpFPH - cameraRightFPW;
        frustum.mFarBottomRightVert = farPlaneCenter - cameraUpFPH + cameraRightFPW;

        frustum.mNearTopLeftVert = nearPlaneCenter + cameraUpNPH - cameraRightNPW;
        frustum.mNearTopRightVert = nearPlaneCenter + cameraUpNPH + cameraRightNPW;
        frustum.mNearBottomLeftVert = nearPlaneCenter - cameraUpNPH - cameraRightNPW;
        frustum.mNearBottomRightVert = nearPlaneCenter - cameraUpNPH + cameraRightNPW;

        createCameraFrustumPlane(frustum.mFarBottomLeftVert, frustum.mFarTopLeftVert, frustum.mFarTopRightVert, frustum.mFarPlane);
        createCameraFrustumPlane(frustum.mNearTopRightVert, frustum.mNearTopLeftVert, frustum.mNearBottomLeftVert, frustum.mNearPlane);
        createCameraFrustumPlane(frustum.mFarTopRightVert, frustum.mFarTopLeftVert, frustum.mNearTopLeftVert, frustum.mTopPlane);
        createCameraFrustumPlane(frustum.mNearBottomLeftVert, frustum.mFarBottomLeftVert, frustum.mFarBottomRightVert,
                                 frustum.mBottomPlane);
        createCameraFrustumPlane(frustum.mNearBottomLeftVert, frustum.mFarTopLeftVert, frustum.mFarBottomLeftVert, frustum.mLeftPlane);
        createCameraFrustumPlane(frustum.mFarBottomRightVert, frustum.mFarTopRightVert, frustum.mNearTopRightVert, frustum.mRightPlane);
    }

    static bool isSphereInsideFrustum(const vec3& center, const float radius, CameraFrustum& frustum)
    {
        CameraFrustumPlane frus_planes[6] = { frustum.mBottomPlane, frustum.mTopPlane,  frustum.mLeftPlane,
                                              frustum.mRightPlane,  frustum.mNearPlane, frustum.mFarPlane };

        for (uint i = 0; i < 6; ++i)
        {
            float sphereCenterOffset = dot(frus_planes[i].mNormal, center);
            float sphereSignedDistance = sphereCenterOffset - frus_planes[i].mDistance;

            // behind the plane...
            if (sphereSignedDistance < -radius)
                return true;
        }

        return false;
    }

    static bool isAABBInsideFrustum(const DrawArgsBound& aabb, CameraFrustum& frustum)
    {
        CameraFrustumPlane frus_planes[6] = { frustum.mBottomPlane, frustum.mTopPlane,  frustum.mLeftPlane,
                                              frustum.mRightPlane,  frustum.mNearPlane, frustum.mFarPlane };

        // project aabb onto a sphere
        vec3 center = (f3Tov3(aabb.Min) + f3Tov3(aabb.Max)) * 0.5f;
        vec3 extents = f3Tov3(aabb.Max) - center;
        for (uint i = 0; i < 6; ++i)
        {
            float sphereProjectedRadius = dot(frus_planes[i].AbsNormal(), extents);

            // sphere to plane
            float sphereCenterOffset = dot(frus_planes[i].mNormal, center);
            float sphereSignedDistance = sphereCenterOffset - frus_planes[i].mDistance;

            // behind the plane...
            if (sphereSignedDistance < -sphereProjectedRadius)
                return true;
        }
        return false;
    }

    SunTemple()
    {
        mSettings.mDragToResize = false;
        mSettings.mVSyncEnabled = false;
        /*mSettings.mHeight = 720;
        mSettings.mWidth = 1200;*/
    }

    bool Init()
    {
        // FILE PATHS
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "CompiledShaders");
        fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_PIPELINE_CACHE, "PipelineCaches");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_GPU_CONFIG, "GPUCfg");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_TEXTURES, "Textures");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_FONTS, "Fonts");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_MESHES, "Meshes");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SCRIPTS, "Scripts");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_OTHER_FILES, "");
        fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_SCREENSHOTS, "Screenshots");
        fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_DEBUG, "Debug");

        // Camera Walking
        FileStream fh = {};
        if (fsOpenStreamFromPath(RD_OTHER_FILES, "cameraPath.bin", FM_READ, &fh))
        {
            const char CAMERA_PATH_FILE_MAGIC_STR[] = { 'C', 'A', 'M', 'P', 'A', 'T', 'H', 'T', 'F' };
            char       magic[TF_ARRAY_COUNT(CAMERA_PATH_FILE_MAGIC_STR)] = { 0 };
            COMPILE_ASSERT(sizeof(magic) == sizeof(CAMERA_PATH_FILE_MAGIC_STR));
            fsReadFromStream(&fh, magic, sizeof(magic));

            if (strncmp(magic, CAMERA_PATH_FILE_MAGIC_STR, TF_ARRAY_COUNT(magic)) != 0)
            {
                fsCloseStream(&fh);
            }
            else
            {
                fsReadFromStream(&fh, &gCameraWalkData.mNumTimes, sizeof(uint32_t));
                fsReadFromStream(&fh, &gCameraWalkData.mNumPositions, sizeof(uint32_t));
                fsReadFromStream(&fh, &gCameraWalkData.mNumRotations, sizeof(uint32_t));

                gCameraWalkData.mTimes = (float*)tf_calloc(gCameraWalkData.mNumTimes, sizeof(float));
                fsReadFromStream(&fh, gCameraWalkData.mTimes, sizeof(float) * gCameraWalkData.mNumTimes);

                gCameraWalkData.mPositions = (float3*)tf_calloc(gCameraWalkData.mNumPositions, sizeof(float3));
                fsReadFromStream(&fh, gCameraWalkData.mPositions, sizeof(float3) * gCameraWalkData.mNumPositions);

                gCameraWalkData.mRotations = (float4*)tf_calloc(gCameraWalkData.mNumRotations, sizeof(float4));
                fsReadFromStream(&fh, gCameraWalkData.mRotations, sizeof(float4) * gCameraWalkData.mNumRotations);

                fsCloseStream(&fh);
            }
        }

        static bool doOnce = true;
        if (doOnce)
        {
            doOnce = false;
            gPlatformParameters.mSelectedRendererApi = RENDERER_API_WEBGPU;
            ASSERT(gPlatformParameters.mSelectedRendererApi != RENDERER_API_VULKAN);
        }

        gGammaCorrectionData.mGammaCorrectionUniformData.mGammaCorrectionData = float4(2.2f, 2.0f, 0.0f, 0.0f);

#if defined(ANDROID)
        // To get performance numbers
        extern bool gSwappyEnabled;
        gSwappyEnabled = false;
#endif

        RendererDesc settings = {};
        settings.mEnableGpuBasedValidation = false;
        // settings.mD3D11Supported = true;
        // settings.mGLESSupported = true;
        initRenderer(GetName(), &settings, &pRenderer);
        // check for init success
        if (!pRenderer)
        {
            return false;
        }

        // #TODO: Remove - Make customizable through gpucfg
        // For performance comparison
        //((GpuInfo*)pRenderer->pGpu)->mSettings.mTimestampQueries = false;

        QueueDesc queueDesc = {};
        queueDesc.mType = QUEUE_TYPE_GRAPHICS;
        queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
        addQueue(pRenderer, &queueDesc, &pGraphicsQueue);

        GpuCmdRingDesc cmdRingDesc = {};
        cmdRingDesc.pQueue = pGraphicsQueue;
        cmdRingDesc.mPoolCount = gDataBufferCount;
        cmdRingDesc.mCmdPerPoolCount = 1;
#if defined(CPU_STRESS_TESTING_ENABLED)
        // One for Test
        // One for Submission
        cmdRingDesc.mCmdPerPoolCount = 2;
#endif
        cmdRingDesc.mAddSyncPrimitives = true;
        addGpuCmdRing(pRenderer, &cmdRingDesc, &gGraphicsCmdRing);

        addSemaphore(pRenderer, &pImageAcquiredSemaphore);

        initResourceLoaderInterface(pRenderer);

        waitForAllResourceLoads();

        SamplerDesc repeatSamplerDesc = {};
        repeatSamplerDesc.mAddressU = ADDRESS_MODE_REPEAT;
        repeatSamplerDesc.mAddressV = ADDRESS_MODE_REPEAT;
        repeatSamplerDesc.mAddressW = ADDRESS_MODE_REPEAT;

        repeatSamplerDesc.mMinLod = 0;
        repeatSamplerDesc.mMaxLod = 7;
        repeatSamplerDesc.mSetLodRange = true;

        repeatSamplerDesc.mMinFilter = FILTER_LINEAR;
        repeatSamplerDesc.mMagFilter = FILTER_LINEAR;
        repeatSamplerDesc.mMipMapMode = MIPMAP_MODE_LINEAR;
        addSampler(pRenderer, &repeatSamplerDesc, &pSamplerSkyBox);

        SamplerDesc sunTempleTexSamplerDesc = {};
        sunTempleTexSamplerDesc.mAddressU = ADDRESS_MODE_REPEAT;
        sunTempleTexSamplerDesc.mAddressV = ADDRESS_MODE_REPEAT;
        sunTempleTexSamplerDesc.mAddressW = ADDRESS_MODE_REPEAT;

        sunTempleTexSamplerDesc.mMinFilter = FILTER_LINEAR;
        sunTempleTexSamplerDesc.mMagFilter = FILTER_LINEAR;
        sunTempleTexSamplerDesc.mMipMapMode = MIPMAP_MODE_LINEAR;

        sunTempleTexSamplerDesc.mMinLod = 0;
        sunTempleTexSamplerDesc.mMaxLod = 6;
        sunTempleTexSamplerDesc.mSetLodRange = true;

        addSampler(pRenderer, &sunTempleTexSamplerDesc, &pSamplerSunTempleAlbedo);
        sunTempleTexSamplerDesc.mMaxLod = 7;
        addSampler(pRenderer, &sunTempleTexSamplerDesc, &pSamplerSunTempleTerrainNormal);

        sunTempleTexSamplerDesc.mAddressU = ADDRESS_MODE_CLAMP_TO_EDGE;
        sunTempleTexSamplerDesc.mAddressV = ADDRESS_MODE_CLAMP_TO_EDGE;
        sunTempleTexSamplerDesc.mAddressW = ADDRESS_MODE_CLAMP_TO_EDGE;

        sunTempleTexSamplerDesc.mMinFilter = FILTER_NEAREST;
        sunTempleTexSamplerDesc.mMagFilter = FILTER_NEAREST;
        sunTempleTexSamplerDesc.mMipMapMode = MIPMAP_MODE_NEAREST;
        sunTempleTexSamplerDesc.mSetLodRange = false;
        addSampler(pRenderer, &sunTempleTexSamplerDesc, &pSamplerSunTempleLightmap);

        SamplerDesc miplessLinearSamplerDesc = {};
        miplessLinearSamplerDesc.mMinFilter = FILTER_NEAREST;
        miplessLinearSamplerDesc.mMagFilter = FILTER_NEAREST;
        miplessLinearSamplerDesc.mMipMapMode = MIPMAP_MODE_NEAREST;
        miplessLinearSamplerDesc.mAddressU = ADDRESS_MODE_CLAMP_TO_EDGE;
        miplessLinearSamplerDesc.mAddressV = ADDRESS_MODE_CLAMP_TO_EDGE;
        miplessLinearSamplerDesc.mAddressW = ADDRESS_MODE_CLAMP_TO_EDGE;
        miplessLinearSamplerDesc.mMipLodBias = 0.f;
        miplessLinearSamplerDesc.mMaxAnisotropy = 0.f;
        addSampler(pRenderer, &miplessLinearSamplerDesc, &pSamplerMiplessNearest);

        SamplerDesc bilinearClampDesc = { FILTER_LINEAR,
                                          FILTER_LINEAR,
                                          MIPMAP_MODE_LINEAR,
                                          ADDRESS_MODE_CLAMP_TO_EDGE,
                                          ADDRESS_MODE_CLAMP_TO_EDGE,
                                          ADDRESS_MODE_CLAMP_TO_EDGE };

        bilinearClampDesc.mMaxLod = 6;
        bilinearClampDesc.mSetLodRange = true;
        addSampler(pRenderer, &bilinearClampDesc, &pSamplerBilinearClamp);

        SamplerDesc nearestClampDesc = { FILTER_NEAREST,
                                         FILTER_NEAREST,
                                         MIPMAP_MODE_NEAREST,
                                         ADDRESS_MODE_CLAMP_TO_EDGE,
                                         ADDRESS_MODE_CLAMP_TO_EDGE,
                                         ADDRESS_MODE_CLAMP_TO_EDGE };

        nearestClampDesc.mMaxLod = 6;
        nearestClampDesc.mSetLodRange = true;
        nearestClampDesc.mCompareFunc = CMP_NEVER;
        addSampler(pRenderer, &nearestClampDesc, &pSamplerNearestClamp);

        // Generate skybox vertex buffer
        const float gSkyBoxPoints[] = {
            0.5f,  -0.5f, -0.5f, 1.0f, // -z
            -0.5f, -0.5f, -0.5f, 1.0f,  -0.5f, 0.5f,  -0.5f, 1.0f,  -0.5f, 0.5f,
            -0.5f, 1.0f,  0.5f,  0.5f,  -0.5f, 1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,

            -0.5f, -0.5f, 0.5f,  1.0f, //-x
            -0.5f, -0.5f, -0.5f, 1.0f,  -0.5f, 0.5f,  -0.5f, 1.0f,  -0.5f, 0.5f,
            -0.5f, 1.0f,  -0.5f, 0.5f,  0.5f,  1.0f,  -0.5f, -0.5f, 0.5f,  1.0f,

            0.5f,  -0.5f, -0.5f, 1.0f, //+x
            0.5f,  -0.5f, 0.5f,  1.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.5f,  0.5f,
            0.5f,  1.0f,  0.5f,  0.5f,  -0.5f, 1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,

            -0.5f, -0.5f, 0.5f,  1.0f, // +z
            -0.5f, 0.5f,  0.5f,  1.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.5f,  0.5f,
            0.5f,  1.0f,  0.5f,  -0.5f, 0.5f,  1.0f,  -0.5f, -0.5f, 0.5f,  1.0f,

            -0.5f, 0.5f,  -0.5f, 1.0f, //+y
            0.5f,  0.5f,  -0.5f, 1.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.5f,  0.5f,
            0.5f,  1.0f,  -0.5f, 0.5f,  0.5f,  1.0f,  -0.5f, 0.5f,  -0.5f, 1.0f,

            0.5f,  -0.5f, 0.5f,  1.0f, //-y
            0.5f,  -0.5f, -0.5f, 1.0f,  -0.5f, -0.5f, -0.5f, 1.0f,  -0.5f, -0.5f,
            -0.5f, 1.0f,  -0.5f, -0.5f, 0.5f,  1.0f,  0.5f,  -0.5f, 0.5f,  1.0f,
        };

        uint64_t       skyBoxDataSize = 4 * 6 * 6 * sizeof(float);
        BufferLoadDesc skyboxVbDesc = {};
        skyboxVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        skyboxVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        skyboxVbDesc.mDesc.mSize = skyBoxDataSize;
        skyboxVbDesc.mDesc.mStartState = RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        skyboxVbDesc.pData = gSkyBoxPoints;
        skyboxVbDesc.ppBuffer = &pSkyBoxVertexBuffer;
        addResource(&skyboxVbDesc, NULL);

        BufferLoadDesc shadowCascadesUniformDesc = {};
        shadowCascadesUniformDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        shadowCascadesUniformDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        shadowCascadesUniformDesc.mDesc.mSize = round_up(sizeof(ShadowCascade), 16u);
        shadowCascadesUniformDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
        shadowCascadesUniformDesc.pData = NULL;

        BufferLoadDesc ubDesc = {};
        ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
        ubDesc.pData = NULL;
        for (uint32_t frameIdx = 0; frameIdx < gDataBufferCount; ++frameIdx)
        {
            ubDesc.mDesc.pName = "ProjViewUniformBuffer";
            ubDesc.mDesc.mSize = round_up(sizeof(UniformBlock), 16u);
            ubDesc.ppBuffer = &pProjViewUniformBuffer[frameIdx];
            addResource(&ubDesc, NULL);

            ubDesc.mDesc.pName = "SkyboxUniformBuffer";
            ubDesc.mDesc.mSize = round_up(sizeof(UniformBlockSky), 16u);
            ubDesc.ppBuffer = &pSkyboxUniformBuffer[frameIdx];
            addResource(&ubDesc, NULL);

            ubDesc.mDesc.pName = "Occlusion Cull Uniform Buffer";
            ubDesc.mDesc.mSize = round_up(sizeof(CullUniformBlock), 16u);
            ubDesc.ppBuffer = &gCullData.pBufferUniformCull[frameIdx];
            addResource(&ubDesc, NULL);

            ubDesc.mDesc.pName = "Gamma Correction Uniform Buffer";
            ubDesc.mDesc.mSize = round_up(sizeof(GammaCorrectionData::GammaCorrectionUniformData), 16u);
            ubDesc.ppBuffer = &gGammaCorrectionData.pGammaCorrectionBuffer[frameIdx];
            addResource(&ubDesc, NULL);

            shadowCascadesUniformDesc.ppBuffer = &gBufferShadowCascades[frameIdx];
            addResource(&shadowCascadesUniformDesc, NULL);
        }

        // Setup lights cluster data
        uint32_t       lightClustersInitData[LIGHT_CLUSTER_WIDTH * LIGHT_CLUSTER_HEIGHT] = {};
        BufferLoadDesc lightClustersCountBufferDesc = {};
        lightClustersCountBufferDesc.mDesc.mSize = LIGHT_CLUSTER_WIDTH * LIGHT_CLUSTER_HEIGHT * sizeof(uint32_t);
        lightClustersCountBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER;
        lightClustersCountBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        lightClustersCountBufferDesc.mDesc.mFirstElement = 0;
        lightClustersCountBufferDesc.mDesc.mElementCount = LIGHT_CLUSTER_WIDTH * LIGHT_CLUSTER_HEIGHT;
        lightClustersCountBufferDesc.mDesc.mStructStride = sizeof(uint32_t);
        lightClustersCountBufferDesc.mDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
        lightClustersCountBufferDesc.pData = lightClustersInitData;
        lightClustersCountBufferDesc.mDesc.pName = "Light Cluster Count Buffer Desc";
        lightClustersCountBufferDesc.ppBuffer = &pLightClustersCount;
        addResource(&lightClustersCountBufferDesc, NULL);

        BufferLoadDesc lightClustersDataBufferDesc = {};
        lightClustersDataBufferDesc.mDesc.mSize = MAX_POINT_LIGHTS * LIGHT_CLUSTER_WIDTH * LIGHT_CLUSTER_HEIGHT * sizeof(uint32_t);
        lightClustersDataBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER;
        lightClustersDataBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        lightClustersDataBufferDesc.mDesc.mFirstElement = 0;
        lightClustersDataBufferDesc.mDesc.mElementCount = MAX_POINT_LIGHTS * LIGHT_CLUSTER_WIDTH * LIGHT_CLUSTER_HEIGHT;
        lightClustersDataBufferDesc.mDesc.mStructStride = sizeof(uint32_t);
        lightClustersDataBufferDesc.mDesc.mStartState = RESOURCE_STATE_UNORDERED_ACCESS;
        lightClustersDataBufferDesc.pData = NULL;
        lightClustersDataBufferDesc.mDesc.pName = "Light Cluster Data Buffer Desc";
        lightClustersDataBufferDesc.ppBuffer = &pLightClusters;
        addResource(&lightClustersDataBufferDesc, NULL);

#ifdef BLUR_PIPELINE
        for (int i = 0; i < MAX_BLUR_KERNEL_SIZE; i++)
        {
            gBlurWeightsUniform.mBlurWeights[i] = gaussian((float)i, 0.0f, gGaussianBlurSigma[0]);
        }

        BufferLoadDesc blurWeightsUBDesc = {};
        blurWeightsUBDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        blurWeightsUBDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        blurWeightsUBDesc.mDesc.mSize = round_up(sizeof(BlurWeights), 16u);
        blurWeightsUBDesc.ppBuffer = &pBufferBlurWeights;
        blurWeightsUBDesc.pData = NULL;
        blurWeightsUBDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
        addResource(&blurWeightsUBDesc, NULL);

        BufferUpdateDesc blurWeightsUpdate = { pBufferBlurWeights, 0, blurWeightsUBDesc.mDesc.mSize };
        beginUpdateResource(&blurWeightsUpdate);
        memcpy(blurWeightsUpdate.pMappedData, &gBlurWeightsUniform, blurWeightsUBDesc.mDesc.mSize);
        endUpdateResource(&blurWeightsUpdate);
#endif

        // Load fonts
        FontDesc font = {};
        font.pFontPath = "TitilliumText/TitilliumText-Bold.otf";
        fntDefineFonts(&font, 1, &gFontID);

        FontSystemDesc fontRenderDesc = {};
        fontRenderDesc.pRenderer = pRenderer;
        if (!initFontSystem(&fontRenderDesc))
            return false; // report?

        // Initialize Forge User Interface Rendering
        UserInterfaceDesc uiRenderDesc = {};
        uiRenderDesc.pRenderer = pRenderer;
        initUserInterface(&uiRenderDesc);

        // Initialize micro profiler and its UI.
        ProfilerDesc profiler = {};
        profiler.pRenderer = pRenderer;
        profiler.mWidthUI = mSettings.mWidth;
        profiler.mHeightUI = mSettings.mHeight;
        initProfiler(&profiler);

        // Gpu profiler can only be added after initProfile.
        gGraphicsProfileToken = addGpuProfiler(pRenderer, pGraphicsQueue, "Graphics");

        /************************************************************************/
        // GUI
        /************************************************************************/
        UIComponentDesc guiDesc = {};
        guiDesc.mStartPosition = vec2(mSettings.mWidth * 0.01f, mSettings.mHeight * 0.2f);
        uiCreateComponent(GetName(), &guiDesc, &pGuiWindow);
#if defined(CPU_STRESS_TESTING_ENABLED)
        uiCreateComponent("CPU Stress Test", &guiDesc, &pCpuStressTestWindow);
#endif
        CameraMotionParameters cmp{ 80.0f, 300.0f, 150.0f };
        vec3                   camPos{ f3Tov3(gViewPositions[0]) };
        vec3                   lookAt{ f3Tov3(gViewLookAtPositions[0]) };

        pCameraController = initFpsCameraController(camPos, lookAt);

        pCameraController->setMotionParameters(cmp);

        InputSystemDesc inputDesc = {};
        inputDesc.pRenderer = pRenderer;
        inputDesc.pWindow = pWindow;
        inputDesc.pJoystickTexture = "circlepad.tex";
        if (!initInputSystem(&inputDesc))
            return false;

        /************************************************************************/
        // Load the scene using the SceneLoader class
        /************************************************************************/
        HiresTimer sceneLoadTimer;
        initHiresTimer(&sceneLoadTimer);

        gSceneVertexLayout.mAttribCount = 4;
        gSceneVertexLayout.mBindingCount = 4;
        gSceneVertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        gSceneVertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        gSceneVertexLayout.mAttribs[0].mBinding = 0;
        gSceneVertexLayout.mAttribs[0].mLocation = 0;

        gSceneVertexLayout.mAttribs[1].mSemantic = SEMANTIC_TEXCOORD0;
        gSceneVertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32_UINT;
        gSceneVertexLayout.mAttribs[1].mBinding = 1;
        gSceneVertexLayout.mAttribs[1].mLocation = 1;

        gSceneVertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD1;
        gSceneVertexLayout.mAttribs[2].mFormat = TinyImageFormat_R32_UINT;
        gSceneVertexLayout.mAttribs[2].mBinding = 2;
        gSceneVertexLayout.mAttribs[2].mLocation = 2;

        gSceneVertexLayout.mAttribs[3].mSemantic = SEMANTIC_NORMAL;
        gSceneVertexLayout.mAttribs[3].mFormat = TinyImageFormat_R32_UINT;
        gSceneVertexLayout.mAttribs[3].mBinding = 3;
        gSceneVertexLayout.mAttribs[3].mLocation = 3;

        gSceneVertexLayoutPositionsOnly = gSceneVertexLayout;
        gSceneVertexLayoutPositionsOnly.mAttribCount = 1;
        gSceneVertexLayoutPositionsOnly.mBindingCount = 1;

        gSceneVertexLayoutPosAndTex = gSceneVertexLayout;
        gSceneVertexLayoutPosAndTex.mAttribCount = 2;
        gSceneVertexLayoutPosAndTex.mBindingCount = 2;

        GeometryLoadDesc sceneLoadDesc = {};
        sceneLoadDesc.pVertexLayout = &gSceneVertexLayout;
        sceneLoadDesc.mFlags = GEOMETRY_LOAD_FLAG_SHADOWED;
        SyncToken token = {};
        pScene = loadSunTemple(&sceneLoadDesc, token, false);
        waitForToken(&token);

        if (!pScene)
            return false;
        LOGF(LogLevel::eINFO, "Load scene : %f ms", getHiresTimerUSec(&sceneLoadTimer, true) / 1000.0f);

        gMeshCount = pScene->pGeom->mDrawArgCount;
        gMaterialCount = gMeshCount;

        for (uint32_t i = 0; i < MT_COUNT_MAX; ++i)
            gMeshTypesCount[i] = 0;

        gMaterialsInfo = (MaterialInfo*)tf_malloc(gMaterialCount * sizeof(MaterialInfo));
        // Load all materials
        for (uint32_t i = 0; i < gMaterialCount; ++i)
        {
            gMaterialsInfo[i].mSetting = pScene->pMeshSettings[i];
            gMaterialsInfo[i].mDrawArg = i;

            gMeshTypesCount[pScene->pMeshSettings[i].mType]++;
        }

        // Sort based in alpha testing..
        qsort(gMaterialsInfo, gMaterialCount, sizeof(MaterialInfo), MaterialCompare);

        IndirectDrawIndexArguments* pNewDrawArgs = (IndirectDrawIndexArguments*)tf_calloc(gMeshCount, sizeof(IndirectDrawIndexArguments));
        DrawArgsBound*              pNewBounds = (DrawArgsBound*)tf_calloc(gMeshCount, sizeof(DrawArgsBound));
        DrawArgsBound*              pUserDataBounds = (DrawArgsBound*)pScene->pGeomData->pUserData;

        // Swap draw args and bounds around so they are in same order as material info..
        for (uint32_t si = 0; si < gMeshCount; ++si)
        {
            MaterialInfo& material = gMaterialsInfo[si];
            // swap bounds
            pNewBounds[si] = pUserDataBounds[material.mDrawArg];
            // swap draw args
            pNewDrawArgs[si] = pScene->pGeom->pDrawArgs[material.mDrawArg];
            pNewDrawArgs[si].mStartInstance = material.mDrawArg;
        }

        memcpy(pScene->pGeom->pDrawArgs, pNewDrawArgs, sizeof(IndirectDrawIndexArguments) * gMeshCount);
        memcpy(pUserDataBounds, pNewBounds, sizeof(DrawArgsBound) * gMeshCount);

        tf_free(pNewBounds);
        tf_free(pNewDrawArgs);

        // Break draw args into mesh types
        uint32_t meshTypesIndices[MT_COUNT_MAX] = { 0 };
        for (uint32_t i = 0; i < MT_COUNT_MAX; ++i)
        {
            gSceneDrawArgsIndices[i] = (uint32_t*)tf_calloc(gMeshTypesCount[i], sizeof(uint32_t*));
            meshTypesIndices[i] = 0;
        }

        for (uint32_t si = 0; si < gMeshCount; ++si)
        {
            MaterialInfo& material = gMaterialsInfo[si];
            gSceneDrawArgsIndices[material.mSetting.mType][meshTypesIndices[material.mSetting.mType]++] = si;
        }

        // Create indirect drawing buffers
        SyncToken      stInstancaDataBuffer = 0;
        BufferLoadDesc instanceDataBufferDesc = {};
        instanceDataBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER;
        instanceDataBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        instanceDataBufferDesc.mDesc.mElementCount = gMeshCount;
        instanceDataBufferDesc.mDesc.mStructStride = sizeof(IndirectDrawIndexArguments);
        instanceDataBufferDesc.mDesc.mSize = (uint64_t)round_up(sizeof(IndirectDrawIndexArguments) * gMeshCount, 16u);
        instanceDataBufferDesc.mDesc.mStartState = RESOURCE_STATE_COMMON;
        instanceDataBufferDesc.ppBuffer = &gInstanceDataBuffer;
        instanceDataBufferDesc.pData = NULL;
        instanceDataBufferDesc.mDesc.pName = "Instance Data Buffer";
        addResource(&instanceDataBufferDesc, &stInstancaDataBuffer);
        waitForToken(&stInstancaDataBuffer);

        BufferUpdateDesc instanceDataBufferUpdate = { gInstanceDataBuffer, 0, sizeof(IndirectDrawIndexArguments) * gMeshCount };
        beginUpdateResource(&instanceDataBufferUpdate);
        memcpy(instanceDataBufferUpdate.pMappedData, pScene->pGeom->pDrawArgs, sizeof(IndirectDrawIndexArguments) * gMeshCount);
        endUpdateResource(&instanceDataBufferUpdate);

        BufferLoadDesc indirectInstanceDataBufferDesc = {};
        indirectInstanceDataBufferDesc.mDesc.mDescriptors =
            DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER | DESCRIPTOR_TYPE_INDIRECT_BUFFER;
        indirectInstanceDataBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        indirectInstanceDataBufferDesc.mDesc.mElementCount = gMeshCount;
        indirectInstanceDataBufferDesc.mDesc.mStructStride = sizeof(IndirectDrawIndexArguments);
        indirectInstanceDataBufferDesc.mDesc.mSize = (uint64_t)round_up(sizeof(IndirectDrawIndexArguments) * gMeshCount, 16u);
        indirectInstanceDataBufferDesc.mDesc.mStartState = RESOURCE_STATE_COMMON;
        indirectInstanceDataBufferDesc.pData = NULL;
        indirectInstanceDataBufferDesc.ppBuffer = &gIndirectInstanceDataBuffer[0];
        indirectInstanceDataBufferDesc.mDesc.pName = "Indirect Instance Data Buffer 0";
        addResource(&indirectInstanceDataBufferDesc, NULL);
        indirectInstanceDataBufferDesc.ppBuffer = &gIndirectInstanceDataBuffer[1];
        indirectInstanceDataBufferDesc.mDesc.pName = "Indirect Instance Data Buffer 1";
        addResource(&indirectInstanceDataBufferDesc, NULL);

        // Load all material textures
        for (uint32_t i = 0; i < gMaterialCount; ++i)
        {
            MaterialInfo& pMaterial = gMaterialsInfo[i];
            uint32_t      texIdx = pMaterial.mDrawArg;

            TextureLoadDesc desc = {};
            desc.pFileName = pScene->ppNormalMaps[texIdx];
            desc.ppTexture = &pMaterial.pNormalMap;
            addResource(&desc, NULL);
            desc = {};

            desc.pFileName = pScene->ppDiffuseMaps[texIdx];
            desc.ppTexture = &pMaterial.pDiffuseMap;
            desc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
            addResource(&desc, NULL);
            desc = {};

            desc.pFileName = pScene->ppSpecularMaps[texIdx];
            desc.ppTexture = &pMaterial.pSpecularMap;
            addResource(&desc, NULL);
            desc = {};

            desc.pFileName = pScene->ppEmissiveMaps[texIdx];
            desc.ppTexture = &pMaterial.pEmissiveMap;
            addResource(&desc, NULL);
            desc = {};

            pMaterial.mDrawArg = i;
        }

        // Load IBL Cube maps..
        TextureLoadDesc iblTexDesc = {};
        iblTexDesc.pFileName = "brdf.tex";
        iblTexDesc.ppTexture = &pBrdfTexture;
        // Textures representing color should be stored in SRGB or HDR format
        addResource(&iblTexDesc, NULL);

        iblTexDesc = {};
        iblTexDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
        iblTexDesc.pFileName = "suntemple_cube_env.tex";
        iblTexDesc.ppTexture = &pPrefilteredEnvTexture;
        addResource(&iblTexDesc, NULL);
        iblTexDesc.pFileName = "suntemple_cube_irradiance.tex";
        iblTexDesc.ppTexture = &pIrradianceTexture;
        addResource(&iblTexDesc, NULL);

        TextureLoadDesc shadowTexturesDesc = {};
        shadowTexturesDesc.pFileName = "SuntempleShadowMap0.tex";
        shadowTexturesDesc.ppTexture = &gShadowMapping.pShadowMapTextures[0];
        addResource(&shadowTexturesDesc, NULL);

        TextureLoadDesc bakedLightMapTextureDesc = {};
        bakedLightMapTextureDesc.pFileName = "SuntempleLightMap.tex";
        bakedLightMapTextureDesc.ppTexture = &pBakedLightMap;
        addResource(&bakedLightMapTextureDesc, NULL);

        // Create all of the buffers
        SyncToken      stMaterialBuffer = 0;
        BufferLoadDesc mbDesc = {};
        mbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER;
        mbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        mbDesc.mDesc.mElementCount = gMaterialCount;
        mbDesc.mDesc.mStructStride = round_up(sizeof(Material), 16u);
        mbDesc.mDesc.mSize = (uint64_t)gMaterialCount * round_up(sizeof(Material), 16u);
        mbDesc.mDesc.mStartState = RESOURCE_STATE_COMMON;
        mbDesc.ppBuffer = &gMaterialsBuffer;
        mbDesc.pData = NULL;
        mbDesc.mDesc.pName = "Materials Buffer";
        addResource(&mbDesc, &stMaterialBuffer);
        waitForToken(&stMaterialBuffer);

        BufferUpdateDesc materialBufferUpdate = { gMaterialsBuffer, 0, mbDesc.mDesc.mSize };
        beginUpdateResource(&materialBufferUpdate);
        memcpy(materialBufferUpdate.pMappedData, pScene->pMaterials, mbDesc.mDesc.mSize);
        endUpdateResource(&materialBufferUpdate);

        // AABBS
        for (uint32_t i = 0; i < pScene->pGeom->mDrawArgCount; ++i)
        {
            DrawArgsBound& bound = pUserDataBounds[i];
            bound.Min.x *= -1.0f;
            bound.Max.x *= -1.0f;

            float3 nMin = float3(min(bound.Min.x, bound.Max.x), min(bound.Min.y, bound.Max.y), min(bound.Min.z, bound.Max.z));
            float3 nMax = float3(max(bound.Min.x, bound.Max.x), max(bound.Min.y, bound.Max.y), max(bound.Min.z, bound.Max.z));
            bound.Min = nMin;
            bound.Max = nMax;
        }

        // Add bounds buffer after updating bound information..
        BufferLoadDesc boundsBufferDesc = {};
        boundsBufferDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_RW_BUFFER;
        boundsBufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        boundsBufferDesc.mDesc.mFirstElement = 0;
        boundsBufferDesc.mDesc.mElementCount = gMeshCount * 6;
        boundsBufferDesc.mDesc.mStructStride = sizeof(float);
        boundsBufferDesc.mDesc.mSize = (uint64_t)round_up(gMeshCount * sizeof(float3) * 2, 16u);
        boundsBufferDesc.mDesc.mStartState = RESOURCE_STATE_COMMON;
        boundsBufferDesc.ppBuffer = &gCullData.pBoundsBuffer;
        boundsBufferDesc.pData = pUserDataBounds;
        boundsBufferDesc.mDesc.pName = "Bounds Buffer";
        addResource(&boundsBufferDesc, NULL);

        // App Actions
        InputActionDesc actionDesc = { DefaultInputActions::DUMP_PROFILE_DATA,
                                       [](InputActionContext* ctx)
                                       {
                                           dumpProfileData(((Renderer*)ctx->pUserData)->pName);
                                           return true;
                                       },
                                       pRenderer };
        addInputAction(&actionDesc);
        actionDesc = { DefaultInputActions::EXIT, [](InputActionContext*)
                       {
                           requestShutdown();
                           return true;
                       } };
        addInputAction(&actionDesc);
        InputActionCallback onAnyInput = [](InputActionContext* ctx)
        {
            if (ctx->mActionId > UISystemInputActions::UI_ACTION_START_ID_)
            {
                uiOnInput(ctx->mActionId, ctx->mBool, ctx->pPosition, &ctx->mFloat2);
            }

            return true;
        };

        typedef bool (*CameraInputHandler)(InputActionContext * ctx, DefaultInputActions::DefaultInputAction action);
        static CameraInputHandler onCameraInput = [](InputActionContext* ctx, DefaultInputActions::DefaultInputAction action)
        {
            if (*(ctx->pCaptured))
            {
                float2 delta = uiIsFocused() ? float2(0.f, 0.f) : ctx->mFloat2;
                switch (action)
                {
                case DefaultInputActions::ROTATE_CAMERA:
                    pCameraController->onRotate(delta);
                    break;
                case DefaultInputActions::TRANSLATE_CAMERA:
                    pCameraController->onMove(delta);
                    break;
                case DefaultInputActions::TRANSLATE_CAMERA_VERTICAL:
                    pCameraController->onMoveY(delta[0]);
                    break;
                default:
                    break;
                }
            }
            return true;
        };
        actionDesc = { DefaultInputActions::CAPTURE_INPUT,
                       [](InputActionContext* ctx)
                       {
                           setEnableCaptureInput(!uiIsFocused() && INPUT_ACTION_PHASE_CANCELED != ctx->mPhase);
                           return true;
                       },
                       NULL };
        addInputAction(&actionDesc);
        actionDesc = { DefaultInputActions::ROTATE_CAMERA,
                       [](InputActionContext* ctx) { return onCameraInput(ctx, DefaultInputActions::ROTATE_CAMERA); }, NULL };
        addInputAction(&actionDesc);
        actionDesc = { DefaultInputActions::TRANSLATE_CAMERA,
                       [](InputActionContext* ctx) { return onCameraInput(ctx, DefaultInputActions::TRANSLATE_CAMERA); }, NULL };
        addInputAction(&actionDesc);
        actionDesc = { DefaultInputActions::TRANSLATE_CAMERA_VERTICAL,
                       [](InputActionContext* ctx) { return onCameraInput(ctx, DefaultInputActions::TRANSLATE_CAMERA_VERTICAL); }, NULL };
        addInputAction(&actionDesc);
        actionDesc = { DefaultInputActions::RESET_CAMERA, [](InputActionContext*)
                       {
                           if (!uiWantTextInput())
                               pCameraController->resetView();
                           return true;
                       } };
        addInputAction(&actionDesc);
        GlobalInputActionDesc globalInputActionDesc = { GlobalInputActionDesc::ANY_BUTTON_ACTION, onAnyInput, this };
        setGlobalInputAction(&globalInputActionDesc);

        gFrameIndex = 0;

        mSettings.mShowPlatformUI = false;

        gCameraFrustum = {};
        gCFSettings.mAspectRatio = (float)mSettings.mWidth / mSettings.mHeight;
        gCFSettings.mWidthMultiplier = 0.883f;
        gCFSettings.mFarPlaneDistance = 300.0f;
        gCFSettings.mNearPlaneDistance = 0.1f;

        gShadowCascades.mSettings.x = float(kShadowMapResWidth);
        gShadowCascades.mSettings.y = float(kShadowMapResHeight);
        gShadowCascades.mSettings.z = 0.9f;
        gShadowCascades.mSettings.w = 1.0f;

#if defined(CPU_STRESS_TESTING_ENABLED)
        initHiresTimer(&gCpuStressTestData.mTimer);

        gCpuStressTestData.mNumPlotData = 0;
        gCpuStressTestData.mNumPlotData += 4;                        // Rect Border
        gCpuStressTestData.mNumPlotData += (kNumCpuStressTests * 2); // X/Y axis ticks
        gCpuStressTestData.mNumPlotData += (kNumCpuStressTests * 2); // Lines to graph (Webgpu/Vulkan)
        gCpuStressTestData.mPlotData = (GraphLineData2D*)tf_calloc(gCpuStressTestData.mNumPlotData, sizeof(GraphLineData2D));

        SyncToken      stPlotVB = {};
        BufferLoadDesc plotVbDesc = {};
        plotVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        plotVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        plotVbDesc.mDesc.mSize = gCpuStressTestData.mNumPlotData * sizeof(GraphLineData2D);
        plotVbDesc.mDesc.mStartState = RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        plotVbDesc.pData = gCpuStressTestData.mPlotData;
        plotVbDesc.ppBuffer = &gCpuStressTestData.pVertexBuffer;
        addResource(&plotVbDesc, &stPlotVB);
        waitForToken(&stPlotVB);

        stPlotVB = 0;
        BufferLoadDesc lineUbDesc = {};
        lineUbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lineUbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        lineUbDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
        lineUbDesc.pData = NULL;
        lineUbDesc.mDesc.pName = "Line Uniform Buffer";
        lineUbDesc.mDesc.mSize = sizeof(mat4);
        lineUbDesc.ppBuffer = &gCpuStressTestData.pUniformBuffer;
        addResource(&lineUbDesc, &stPlotVB);
        waitForToken(&stPlotVB);

        // Initialize profile tokens
        uint32_t cptIdx = GetCpuApiDataIndex();
        for (uint32_t j = 0; j < CSTT_ALL; ++j)
            gCpuStressTests[j].mToken =
                getCpuProfileToken(cptIdx == CPU_WEBGPU_IDX ? "CPU Stress Test (WebGpu)" : "CPU Stress Test (Vulkan)",
                                   gCpuStressTestData.kTypeStrings[j], cptIdx == CPU_WEBGPU_IDX ? 0xff00ff00 : 0xff0000ff);

        gCpuStressTests[CSTT_COMMAND_ENCODING].Run = cpuStressTestCommandsEncoding;
        gCpuStressTests[CSTT_BIND_GROUP_UPDATES].Run = cpuStressTestBindGroupUpdates;
        gCpuStressTests[CSTT_BIND_GROUP_BINDINGS].Run = cpuStressTestBindGroupBindings;
        gCpuStressTests[CSTT_COMMAND_SUBMISSION].Run = cpuStressTestCommandsSubmission;

        if (gCpuStressTestData.mCurrentTestType == CSTT_ALL && gCpuStressTestData.bWasTestRunning)
        {
            gCpuStressTestData.bWasTestRunning = false;

            gCpuStressTestData.mCurrentTestType = 0u;
            gCpuStressTestData.mCurrentSample = 0u;
            gCpuStressTestData.mCurrentTest = 0u;

            // Run all test for new API as well...
            cpuToggleStressTest(NULL);
        }

        addFence(pRenderer, &gCpuStressTestData.pSubmissionFence);
#endif

        gCpuFrameTimeToken = getCpuProfileToken("Total Frame Time", "Total", 0xff00ffff);
        gCpuUpdateToken = getCpuProfileToken("Total Frame Time", "Update", 0xffffff00);
        gCpuDrawToken = getCpuProfileToken("Total Frame Time", "Draw", 0xffffff00);
        gCpuDrawPresentationToken = getCpuProfileToken("Total Frame Time", "Queue Presentation", 0xffffff00);
        gCpuDrawSceneForwardToken = getCpuProfileToken("Total Frame Time", "Draw Scene Forward", 0xffffff00);
        gCpuDrawSceneForwardSubmissionToken = getCpuProfileToken("Total Frame Time", "Draw Submission", 0xffffff00);

        waitForAllResourceLoads();
        return true;
    }

    void Exit()
    {
        exitInputSystem();

        exitCameraController(pCameraController);

        exitUserInterface();

        exitFontSystem();

        // Exit profile
        exitProfiler();

        // Destroy scene buffers
        removeResource(pScene->pGeomData);
        unloadSunTemple(pScene);
        removeResource(pScene->pGeom);
        tf_free(pScene);

        for (uint32_t mti = 0; mti < MT_COUNT_MAX; ++mti)
            tf_free(gSceneDrawArgsIndices[mti]);

        for (uint32_t frameIdx = 0; frameIdx < gDataBufferCount; ++frameIdx)
        {
            removeResource(pProjViewUniformBuffer[frameIdx]);
            removeResource(gGammaCorrectionData.pGammaCorrectionBuffer[frameIdx]);
            removeResource(pSkyboxUniformBuffer[frameIdx]);
            removeResource(gBufferShadowCascades[frameIdx]);
            removeResource(gCullData.pBufferUniformCull[frameIdx]);
            removeResource(gIndirectInstanceDataBuffer[frameIdx]);
        }

        removeResource(gCullData.pBoundsBuffer);

        removeResource(pLightClustersCount);
        removeResource(pLightClusters);

        removeResource(gInstanceDataBuffer);

        for (uint32_t i = 0; i < kShadowMapCascadeCount; ++i)
        {
            if (gShadowMapping.pShadowMapTextures[i] != NULL)
            {
                removeResource(gShadowMapping.pShadowMapTextures[i]);
            }
        }

        uiDestroyComponent(pGuiWindow);
#ifdef CPU_STRESS_TESTING_ENABLED
        tf_free(gCpuStressTestData.mPlotData);
        removeResource(gCpuStressTestData.pVertexBuffer);
        removeResource(gCpuStressTestData.pUniformBuffer);
        uiDestroyComponent(pCpuStressTestWindow);

        removeFence(pRenderer, gCpuStressTestData.pSubmissionFence);
#endif
#ifdef BLUR_PIPELINE
        removeResource(pBufferBlurWeights);
#endif

        // Remove loaded scene
        /************************************************************************/
        // Remove Textures
        removeResource(pBakedLightMap);
        removeResource(pIrradianceTexture);
        removeResource(pPrefilteredEnvTexture);
        removeResource(pBrdfTexture);
        for (uint32_t i = 0; i < gMaterialCount; ++i)
        {
            removeResource(gMaterialsInfo[i].pDiffuseMap);
            removeResource(gMaterialsInfo[i].pNormalMap);
            removeResource(gMaterialsInfo[i].pSpecularMap);
            removeResource(gMaterialsInfo[i].pEmissiveMap);
        }
        removeResource(gMaterialsBuffer);
        tf_free(gMaterialsInfo);

        // Free Camera Path data
        tf_free(gCameraWalkData.mTimes);
        tf_free(gCameraWalkData.mPositions);
        tf_free(gCameraWalkData.mRotations);

        removeResource(pSkyBoxVertexBuffer);

        removeSampler(pRenderer, pSamplerSkyBox);
        removeSampler(pRenderer, pSamplerSunTempleAlbedo);
        removeSampler(pRenderer, pSamplerSunTempleLightmap);
        removeSampler(pRenderer, pSamplerSunTempleTerrainNormal);
        removeSampler(pRenderer, pSamplerMiplessNearest);
        removeSampler(pRenderer, pSamplerBilinearClamp);
        removeSampler(pRenderer, pSamplerNearestClamp);

        removeGpuCmdRing(pRenderer, &gGraphicsCmdRing);
        removeSemaphore(pRenderer, pImageAcquiredSemaphore);

        exitResourceLoaderInterface(pRenderer);

        removeQueue(pRenderer, pGraphicsQueue);

        exitRenderer(pRenderer);
        pRenderer = NULL;
    }

    bool Load(ReloadDesc* pReloadDesc)
    {
        // Set if light culling is enabled on load or not
        gUseLightCulling = gLightCullingEnabled;
        gUseRealTimeShadows = gRealTimeShadowsEnabled;

        if (!gUseRealTimeShadows)
            gLightCpuSettings.mSunControl = { 33.333f, 18.974f, -41.667f };

        if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
        {
            addShaders();
            addRootSignatures();
            addDescriptorSets();
        }

        if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
        {
            if (!addSwapChain())
                return false;

            // find all supported render target formats
            TinyImageFormat rtFmt[gMaxRenderTargetFormats] = { TinyImageFormat_B10G11R11_UFLOAT, pSwapChain->ppRenderTargets[0]->mFormat,
                                                               TinyImageFormat_R16G16B16A16_UNORM };
            for (uint32_t i = 0; i < gMaxRenderTargetFormats; ++i)
            {
                FormatCapability fmtCap = pRenderer->pGpu->mCapBits.mFormatCaps[rtFmt[i]];
                bool             canUseFormat = (fmtCap & FORMAT_CAP_LINEAR_FILTER) > 0 && (fmtCap & FORMAT_CAP_RENDER_TARGET) > 0;
                if (canUseFormat || pRenderer->mRendererApi == RENDERER_API_WEBGPU)
                {
                    const char* pFrom = TinyImageFormat_Name(rtFmt[i]);
                    char*       pTo = (char*)tf_calloc(strlen(pFrom) + 1, sizeof(char));
                    snprintf(pTo, strlen(pFrom) + 1, "%s", pFrom);
                    gRenderTargetFormatNames[gNumRenderTargetFormats++] = pTo;
                }
            }

            if (!addRenderTargets())
                return false;
        }

        TextureLoadDesc skyboxDesc = {};
        // Textures representing color should be stored in SRGB or HDR format
        skyboxDesc.mCreationFlag = TinyImageFormat_IsSRGB(pSwapChain->mFormat) ? TEXTURE_CREATION_FLAG_SRGB : TEXTURE_CREATION_FLAG_NONE;
        skyboxDesc.pFileName = pSkyBoxImageFileName;
        skyboxDesc.ppTexture = &pSkyBoxTexture;
        addResource(&skyboxDesc, NULL);

        // For all texture/buffers..
        waitForAllResourceLoads();

        if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
        {
            addPipelines();
        }

        prepareDescriptorSets();

        UserInterfaceLoadDesc uiLoad = {};
        uiLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
        uiLoad.mHeight = mSettings.mHeight;
        uiLoad.mWidth = mSettings.mWidth;
        uiLoad.mLoadType = pReloadDesc->mType;
        loadUserInterface(&uiLoad);

        FontSystemLoadDesc fontLoad = {};
        fontLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
        fontLoad.mHeight = mSettings.mHeight;
        fontLoad.mWidth = mSettings.mWidth;
        fontLoad.mLoadType = pReloadDesc->mType;
        loadFontSystem(&fontLoad);

        initScreenshotInterface(pRenderer, pGraphicsQueue);

        // generateBrdfLut(true);
        // generateIBLCubeMaps(true);

        addGui();

        return true;
    }

    void Unload(ReloadDesc* pReloadDesc)
    {
        waitQueueIdle(pGraphicsQueue);

        unloadFontSystem(pReloadDesc->mType);
        unloadUserInterface(pReloadDesc->mType);

        removeResource(pSkyBoxTexture);

        if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
        {
            removePipelines();
        }

        if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
        {
            removeSwapChain(pRenderer, pSwapChain);

            for (uint32_t i = 0; i < gNumRenderTargetFormats; ++i)
                tf_free(gRenderTargetFormatNames[i]);
            gNumRenderTargetFormats = 0;

            removeRenderTargets();
        }

        if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
        {
            removeDescriptorSets();
            removeRootSignatures();
            removeShaders();
        }

        uiDestroyAllComponentWidgets(pGuiWindow);
#if defined(CPU_STRESS_TESTING_ENABLED)
        uiDestroyAllComponentWidgets(pCpuStressTestWindow);
#endif

        exitScreenshotInterface();
    }

    mat4 UpdateCameraWalk(float deltaTime)
    {
        // Update
        gCameraWalkData.mWalkingTime += deltaTime * gCameraWalkData.mWalkSpeed;

        // Map time to frame
        float cTime = gCameraWalkData.mTimes[gCameraWalkData.mCurrentFrame];
        while (gCameraWalkData.mWalkingTime > cTime)
        {
            cTime = gCameraWalkData.mTimes[++gCameraWalkData.mCurrentFrame];

            // Reset
            if (gCameraWalkData.mCurrentFrame == gCameraWalkData.mNumTimes)
            {
                gCameraWalkData.mCurrentFrame = 0;
                gCameraWalkData.mWalkingTime = 0.0f;
                break;
            }
        }
        int32_t pFrame = gCameraWalkData.mCurrentFrame - 1;

        // Translate / Rotate based on current frame
        if (gCameraWalkData.mCurrentFrame > 0)
        {
            // Translate camera
            float pTime = gCameraWalkData.mTimes[pFrame];

            vec3 pPos = f3Tov3(gCameraWalkData.mPositions[pFrame]);
            vec3 cPos = f3Tov3(gCameraWalkData.mPositions[gCameraWalkData.mCurrentFrame]);

            float value = (gCameraWalkData.mWalkingTime - pTime) / (cTime - pTime);

            mat4 translate = mat4::translation(lerp(pPos, cPos, value));

            // Rotate Camera
            float4 pRotf = gCameraWalkData.mRotations[pFrame];
            Quat   pRot = Quat(pRotf.x, pRotf.y, pRotf.z, pRotf.w);

            float4 cRotf = gCameraWalkData.mRotations[gCameraWalkData.mCurrentFrame];
            Quat   cRot = Quat(cRotf.x, cRotf.y, cRotf.z, cRotf.w);

            Quat newRot = lerp(value, pRot, cRot);
            return translate * mat4(newRot, vec3(0.0f));
        }
        else
        {
            float4 cRotf = gCameraWalkData.mRotations[0];
            Quat   cRot = Quat(cRotf.x, cRotf.y, cRotf.z, cRotf.w);
            return mat4::translation(f3Tov3(gCameraWalkData.mPositions[0])) * mat4(cRot, vec3(0.0f));
        }
    }

    void CalculateShadowCascades(mat4& projView, float nearClip, float farClip)
    {
        Point3 lightSourcePos(gLightCpuSettings.mSunControl.x, gLightCpuSettings.mSunControl.y, gLightCpuSettings.mSunControl.z);
        gUniformData.mDirectionalLight.mDirection = gLightCpuSettings.mSunControl;
        gUniformData.mDirectionalLight.mDirection.x *= -1;

        if (kShadowMapCascadeCount == 1)
        {
            mat4 lightProjMat = mat4::orthographicLH(-140, 140, -210, 90, -100, 200);
            mat4 srclightView = mat4::lookAtLH(lightSourcePos, Point3(0, 0, 0), vec3(0, 1, 0));
            gShadowCascades.mViewProjMatrix[0] = lightProjMat * srclightView;
        }
        else
        {
            /************************************************************************/
            // Update Cascade Info..
            // Cascade Impl. By Sascha Wiliams...

            /************************************************************************/
            // reverse z
            /*float tFarClip = farClip;
            farClip = nearClip;
            nearClip = tFarClip;*/

            float cascadeSplits[kShadowMapCascadeCount];
            float clipRange = farClip - nearClip;

            float minZ = nearClip;
            float maxZ = nearClip + clipRange;

            float range = maxZ - minZ;
            float ratio = maxZ / minZ;

            // Calculate split depths based on view camera frustum
            // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
            for (uint32_t i = 0; i < kShadowMapCascadeCount; i++)
            {
                float p = (i + 1) / static_cast<float>(kShadowMapCascadeCount);
                float log = minZ * pow(ratio, p);
                float uniform = minZ + range * p;
                float d = gCascadeSplitLambda * (log - uniform) + uniform;
                cascadeSplits[i] = (d - nearClip) / clipRange;
            }

            // Calculate orthographic projection matrix for each cascade
            float lastSplitDist = 0.0;
            mat4  invCam = inverse(projView);
            for (uint32_t i = 0; i < kShadowMapCascadeCount; i++)
            {
                float splitDist = cascadeSplits[i];

                vec3 frustumCorners[8] = {
                    vec3(-1.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(1.0f, -1.0f, 0.0f), vec3(-1.0f, -1.0f, 0.0f),
                    vec3(-1.0f, 1.0f, 1.0f), vec3(1.0f, 1.0f, 1.0f), vec3(1.0f, -1.0f, 1.0f), vec3(-1.0f, -1.0f, 1.0f),
                };

                // Project frustum corners into world space
                for (uint32_t j = 0; j < 8; j++)
                {
                    vec4 invCorner = invCam * vec4(frustumCorners[j], 1.0f);
                    frustumCorners[j] = invCorner.getXYZ() / invCorner.getW();
                }

                for (uint32_t j = 0; j < 4; j++)
                {
                    vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
                    frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                    frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
                }

                // Get frustum center
                vec3 frustumCenter = vec3(0.0f);
                for (uint32_t j = 0; j < 8; j++)
                {
                    frustumCenter += frustumCorners[j];
                }
                frustumCenter /= 8.0f;

                float radius = 0.0f;
                for (uint32_t j = 0; j < 8; j++)
                {
                    float distance = length(frustumCorners[j] - frustumCenter);
                    radius = max(radius, distance);
                }
                radius = ceil(radius * 16.0f) / 16.0f;

                vec3 maxExtents = vec3(radius);
                vec3 minExtents = -maxExtents;

                /************************************************************************/
                // Light Matrix Update
                /************************************************************************/
                vec3 nLightDir = normalize(-vec3(lightSourcePos)); // align
                mat4 lightView =
                    mat4::lookAtLH(Point3(frustumCenter - nLightDir * -minExtents.getZ()), Point3(frustumCenter), vec3(0, 1, 0));
                mat4 lightOrthoMat = mat4::orthographicLH(minExtents.getX(), maxExtents.getX(), minExtents.getY(), maxExtents.getY(), 0.0f,
                                                          maxExtents.getZ() - minExtents.getZ());

                // Store split distance and matrix in cascade
                ((float*)&gShadowCascades.mSplitDepth[0].x)[i] = (nearClip + splitDist * clipRange) - 1.0f;
                gShadowCascades.mViewProjMatrix[i] = lightOrthoMat * lightView;

                lastSplitDist = cascadeSplits[i];
            }
        }
    }

    void Update(float deltaTime)
    {
        cpuProfileEnter(gCpuFrameTimeToken);
        cpuProfileEnter(gCpuUpdateToken);
        updateInputSystem(deltaTime, mSettings.mWidth, mSettings.mHeight);

        pCameraController->update(deltaTime);

        mat4 wCameraViewMatrix = mat4::identity();

        if (gViewPoistionsWidgetData)
        {
            pCameraController->moveTo(f3Tov3(gViewPositions[gViewPoistionsWidgetData - 1]));
            pCameraController->lookAt(f3Tov3(gViewLookAtPositions[gViewPoistionsWidgetData - 1]));
        }

        // Camera walk -- Overwrite controller data
        if (gCameraWalkData.mIsWalking)
        {
            wCameraViewMatrix = UpdateCameraWalk(deltaTime);
        }

        float nearClip = 0.1f;
        float farClip = 300.0f;
        /************************************************************************/
        // Scene Update
        /************************************************************************/
        vec4  cameraPosition = gCameraWalkData.mIsWalking ? wCameraViewMatrix.getCol3() : vec4(pCameraController->getViewPosition(), 1.0f);
        mat4  viewMat = gCameraWalkData.mIsWalking ? inverse(wCameraViewMatrix) : pCameraController->getViewMatrix();

        const float aspectInverse = (float)mSettings.mHeight / (float)mSettings.mWidth;
        const float horizontal_fov = PI / 2.0f;
        mat4        projMat = mat4::perspectiveLH_ReverseZ(horizontal_fov, aspectInverse, nearClip, farClip);
        gUniformData.mProjection = projMat;
        gUniformData.mProjectView = projMat * viewMat;
        gUniformData.mView = viewMat;
        gUniformData.mCamPos = v4ToF4(cameraPosition);

        // Skybox transforms last
        mat4 skyboxViewMat = viewMat;
        skyboxViewMat.setTranslation(vec3(0));
        gUniformDataSky = {};
        gUniformDataSky.mProjectView = projMat * skyboxViewMat;

        vec3 cfEyePos = vec3(150.0f, 150.0f, 0.0f);
        mat4 cfViewMat = mat4::lookAtLH(Point3(cfEyePos), Point3(-5, 5, 0), vec3(0, 1, 0));
        gUniformDataDebug.mProjectView = projMat * cfViewMat;

        if (gUseFrustumCulling)
        {
            initCameraFrustum(gCameraFrustum, gCFSettings);
            mat4 wViewMat = gCameraWalkData.mIsWalking ? wCameraViewMatrix : inverse(pCameraController->getViewMatrix());
            createCameraFrustum(gCameraFrustum, wViewMat, f3Tov3(gUniformData.mCamPos.getXYZ()));

            gCullUniformBlock.mCameraFrustumPlanes[0] = vec4(gCameraFrustum.mBottomPlane.mNormal, gCameraFrustum.mBottomPlane.mDistance);
            gCullUniformBlock.mCameraFrustumPlanes[1] = vec4(gCameraFrustum.mTopPlane.mNormal, gCameraFrustum.mTopPlane.mDistance);
            gCullUniformBlock.mCameraFrustumPlanes[2] = vec4(gCameraFrustum.mLeftPlane.mNormal, gCameraFrustum.mLeftPlane.mDistance);
            gCullUniformBlock.mCameraFrustumPlanes[3] = vec4(gCameraFrustum.mRightPlane.mNormal, gCameraFrustum.mRightPlane.mDistance);
            gCullUniformBlock.mCameraFrustumPlanes[4] = vec4(gCameraFrustum.mNearPlane.mNormal, gCameraFrustum.mNearPlane.mDistance);
            gCullUniformBlock.mCameraFrustumPlanes[5] = vec4(gCameraFrustum.mFarPlane.mNormal, gCameraFrustum.mFarPlane.mDistance);
        }

        /************************************************************************/
        // Culling data
        /************************************************************************/
        gCullUniformBlock.mProject = mat4::perspectiveLH(horizontal_fov, aspectInverse, nearClip, farClip);
        gCullUniformBlock.mProjectView = gCullUniformBlock.mProject * viewMat;
        gCullUniformBlock.mNumMeshes.x = gMeshCount;
        gCullUniformBlock.mNumMeshes.y = gUseFrustumCulling;
        gCullUniformBlock.mNumMeshes.z = 0;

        gUniformData.mCullingViewPort[0].sampleCount = 0;
        gUniformData.mCullingViewPort[0].windowSize = { (float)mSettings.mWidth, (float)mSettings.mHeight };

        CalculateShadowCascades(gUniformData.mProjectView, nearClip, farClip);

        cpuStressTestUpdate(deltaTime);

        cpuProfileLeave(gCpuUpdateToken, gFrameCount);
    }

    void drawShadowMap(Cmd* cmd)
    {
#ifdef SHADOWS_ENABLED
        // Shadow..
        RenderTargetBarrier barriers[kShadowMapCascadeCount + 1] = {};
        uint32_t            barrierCount = 0;
        for (uint32_t sci = 0; sci < kShadowMapCascadeCount; ++sci)
        {
            barriers[barrierCount++] = { gShadowMapping.pShadowMaps[sci], RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                         RESOURCE_STATE_RENDER_TARGET };
        }
        barriers[barrierCount++] = { pRenderTargetShadowMap, RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_DEPTH_WRITE };
        // Barrier to allow writing depth..
        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, barrierCount, barriers);
        barrierCount = 0;

        cmdBeginGpuTimestampQuery(cmd, gGraphicsProfileToken, "Draw Shadow Map");

        char tokenBuffer[15];
        for (uint32_t sci = 0; sci < kShadowMapCascadeCount; ++sci)
        {
            snprintf(tokenBuffer, 15, "Cascade %u", sci);
            cmdBeginGpuTimestampQuery(cmd, gGraphicsProfileToken, tokenBuffer);

            // Start render pass and apply load actions
            BindRenderTargetsDesc bindRenderTargets = {};
            bindRenderTargets.mRenderTargetCount = 1;
            bindRenderTargets.mRenderTargets[0] = { gShadowMapping.pShadowMaps[sci], LOAD_ACTION_CLEAR };
            bindRenderTargets.mDepthStencil = { pRenderTargetShadowMap, LOAD_ACTION_CLEAR };
            cmdBindRenderTargets(cmd, &bindRenderTargets);
            cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTargetShadowMap->mWidth, (float)pRenderTargetShadowMap->mHeight, 0.0f, 1.0f);
            cmdSetScissor(cmd, 0, 0, pRenderTargetShadowMap->mWidth, pRenderTargetShadowMap->mHeight);

            uint32_t opaqueMeshCount = gMeshCount - gMeshTypesCount[MT_ALPHA_TESTED];
            {
                cmdBindPipeline(cmd, gShadowMapping.pPipelineDepth);
                cmdBindDescriptorSet(cmd, gFrameIndex, gShadowMapping.pDescriptorSetUniformsCascades);

                for (uint32_t i = 0; i < opaqueMeshCount; ++i)
                {
                    const IndirectDrawIndexArguments& args = pScene->pGeom->pDrawArgs[i];

                    cmdBindIndexBuffer(cmd, pScene->pGeom->pIndexBuffer, pScene->pGeom->mIndexType, 0);
                    cmdBindVertexBuffer(cmd, 1, pScene->pGeom->pVertexBuffers, pScene->pGeom->mVertexStrides, NULL);
                    cmdDrawIndexedInstanced(cmd, args.mIndexCount, args.mStartIndex, 1, args.mVertexOffset, sci);
                }
            }

            {
                cmdBindPipeline(cmd, gShadowMapping.pPipelineDepthAlpha);
                cmdBindDescriptorSet(cmd, gFrameIndex, gShadowMapping.pDescriptorSetAlphaUniforms);

                for (uint32_t i = opaqueMeshCount; i < opaqueMeshCount + gMeshTypesCount[MT_ALPHA_TESTED]; ++i)
                {
                    const IndirectDrawIndexArguments& args = pScene->pGeom->pDrawArgs[i];

                    cmdBindIndexBuffer(cmd, pScene->pGeom->pIndexBuffer, pScene->pGeom->mIndexType, 0);
                    cmdBindVertexBuffer(cmd, 2, pScene->pGeom->pVertexBuffers, pScene->pGeom->mVertexStrides, NULL);
                    cmdBindDescriptorSet(cmd, (int32_t)i - (int32_t)opaqueMeshCount, gShadowMapping.pDescriptorSetAlphaTextures);
                    cmdDrawIndexedInstanced(cmd, args.mIndexCount, args.mStartIndex, 1, args.mVertexOffset, sci);
                }
            }

            cmdBindRenderTargets(cmd, NULL);

            cmdEndGpuTimestampQuery(cmd, gGraphicsProfileToken);
        }

        cmdEndGpuTimestampQuery(cmd, gGraphicsProfileToken);

        // Barriers to allow reading depth...
        for (uint32_t sci = 0; sci < kShadowMapCascadeCount; ++sci)
        {
            barriers[barrierCount++] = { gShadowMapping.pShadowMaps[sci], RESOURCE_STATE_RENDER_TARGET,
                                         RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
        }
        // Barriers to allow reading depth...
        barriers[barrierCount++] = { pRenderTargetShadowMap, RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, barrierCount, barriers);
#endif
    }

    void blurShadowMap(Cmd* cmd)
    {
        UNREF_PARAM(cmd);
#ifdef BLUR_PIPELINE
        cmdBeginGpuTimestampQuery(cmd, gGraphicsProfileToken, "Shadow Map Blur");

        BufferUpdateDesc bufferUpdate = { pBufferBlurWeights };
        beginUpdateResource(&bufferUpdate);
        memcpy(bufferUpdate.pMappedData, &gBlurWeightsUniform, sizeof(gBlurWeightsUniform));
        endUpdateResource(&bufferUpdate);

        for (uint32_t sci = 0; sci < kShadowMapCascadeCount; ++sci)
        {
            RenderTarget* pRenderTargets[2] = { gShadowMapping.pShadowMaps[sci], pRenderTargetShaderMapBlur };

            RenderTargetBarrier rt[2];
            rt[0] = { pRenderTargets[0], RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS };
            rt[1] = { pRenderTargets[1], RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_UNORDERED_ACCESS };
            cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, rt);

            // Horizontal Pass
            cmdBindPipeline(cmd, pPipelineBlur[PASS_TYPE_HORIZONTAL]);
            cmdBindDescriptorSet(cmd, sci, pDescriptorSetBlurCompute[PASS_TYPE_HORIZONTAL]);

            const uint32_t threadGroupSizeX = uint32_t(kShadowMapResWidth / 16 + 1);
            const uint32_t threadGroupSizeY = uint32_t(kShadowMapResHeight / 16 + 1);

            cmdDispatch(cmd, threadGroupSizeX, threadGroupSizeY, 1);

            // Barrier
            rt[0] = { pRenderTargets[0], RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_UNORDERED_ACCESS };
            rt[1] = { pRenderTargets[1], RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_UNORDERED_ACCESS };
            cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, rt);

            // Vertical Pass
            cmdBindPipeline(cmd, pPipelineBlur[PASS_TYPE_VERTICAL]);
            cmdBindDescriptorSet(cmd, sci, pDescriptorSetBlurCompute[PASS_TYPE_VERTICAL]);

            cmdDispatch(cmd, threadGroupSizeX, threadGroupSizeY, 1);

            rt[0] = { pRenderTargets[0], RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
            rt[1] = { pRenderTargets[1], RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
            cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, rt);
        }

        cmdEndGpuTimestampQuery(cmd, gGraphicsProfileToken);
#endif
    }

    // Executes a compute shader to clear (reset) the the light clusters on the GPU
    void clearLightClusters(Cmd* cmd, uint32_t frameIdx)
    {
        cmdBindPipeline(cmd, pPipelineClearLightClusters);
        cmdBindDescriptorSet(cmd, frameIdx, pDescriptorSetLightClusters);
        cmdDispatch(cmd, 1, 1, 1);
    }

    // Executes a compute shader that computes the light clusters on the GPU
    void computeLightClusters(Cmd* cmd, uint32_t frameIdx)
    {
        cmdBindPipeline(cmd, pPipelineClusterLights);
        cmdBindDescriptorSet(cmd, frameIdx, pDescriptorSetLightClusters);
        cmdDispatch(cmd, (uint32_t)gUniformData.mPointLightCount[0], 1, 1);
    }

    void doLightCulling(Cmd* cmd, uint32_t frameIndex)
    {
        if (gUseLightCulling)
        {
            cmdBeginGpuTimestampQuery(cmd, gGraphicsProfileToken, "Compute Light Clusters");

            clearLightClusters(cmd, frameIndex);

            BufferBarrier barriers[] = { { pLightClustersCount, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_UNORDERED_ACCESS } };
            cmdResourceBarrier(cmd, 1, barriers, 0, NULL, 0, NULL);

            // Update Light clusters on the GPU
            computeLightClusters(cmd, frameIndex);

            barriers[0] = { pLightClusters, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
            cmdResourceBarrier(cmd, 1, barriers, 0, NULL, 0, NULL);

            // #TODO: Find a better way. WebGpu complains if we issue timestamp inside compute pass. Currently no API in IGraphics.h
            // that can let us end a compute pass
            if (gPlatformParameters.mSelectedRendererApi == RENDERER_API_WEBGPU)
            {
                extern void EndComputeEncoder(Cmd * cmd);
                EndComputeEncoder(cmd);
            }
            cmdEndGpuTimestampQuery(cmd, gGraphicsProfileToken); // Compute Light Clusters
        }
    }

    void doFrustumCulling(Cmd* cmd)
    {
        cmdBeginGpuTimestampQuery(cmd, gGraphicsProfileToken, "Culling");

        /************************************************************************/
        // Frustum cull
        /************************************************************************/
        BufferBarrier bufferBarriers[1] = {};
        bufferBarriers[0] = { gIndirectInstanceDataBuffer[gFrameIndex], RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_UNORDERED_ACCESS };
        cmdResourceBarrier(cmd, 1, bufferBarriers, 0, NULL, 0, NULL);

        cmdBindPipeline(cmd, gCullData.pPipelines[CST_FRUSTUM_CULL_PASS]);

        cmdBindDescriptorSet(cmd, 0, gCullData.pSetUpdateNone[CST_FRUSTUM_CULL_PASS]);
        cmdBindDescriptorSet(cmd, gFrameIndex, gCullData.pSetUpdatePerFrame[CST_FRUSTUM_CULL_PASS]);

        cmdDispatch(cmd, round_up(gMeshCount, 64u) / 64, 1, 1);

        cmdResourceBarrier(cmd, 1, bufferBarriers, 0, NULL, 0, NULL);

        // #TODO: Find a better way. WebGpu complains if we issue timestamp inside compute pass. Currently no API in IGraphics.h that
        // can let us end a compute pass
        if (gPlatformParameters.mSelectedRendererApi == RENDERER_API_WEBGPU)
        {
            extern void EndComputeEncoder(Cmd * cmd);
            EndComputeEncoder(cmd);
        }
        cmdEndGpuTimestampQuery(cmd, gGraphicsProfileToken); // Occlusion Culling
    }

    void drawGammaCorrection(Cmd* cmd, RenderTarget* pRenderTarget, RenderTarget* pRenderTargetSwapchain)
    {
        cmdBindRenderTargets(cmd, NULL);

        cmdBeginGpuTimestampQuery(cmd, gGraphicsProfileToken, "Gamma Correction");

        RenderTargetBarrier rtBarriers[2] = { { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
                                              { pRenderTargetSwapchain, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET } };
        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, rtBarriers);

        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount = 1;
        bindRenderTargets.mRenderTargets[0] = { pRenderTargetSwapchain, LOAD_ACTION_CLEAR };
        bindRenderTargets.mDepthStencil = { NULL, LOAD_ACTION_DONTCARE };
        cmdBindRenderTargets(cmd, &bindRenderTargets);

        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

        cmdBindPipeline(cmd, gGammaCorrectionData.pPipeline);
        cmdBindDescriptorSet(cmd, 0, gGammaCorrectionData.pSetTexture);           // Texture
        cmdBindDescriptorSet(cmd, gFrameIndex, gGammaCorrectionData.pSetUniform); // Uniform Buffer
        cmdDraw(cmd, 3, 0);                                                       // Full Screen Vert

        cmdBindRenderTargets(cmd, NULL);
        rtBarriers[0] = { pRenderTargetSwapchain, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, rtBarriers);

        cmdEndGpuTimestampQuery(cmd, gGraphicsProfileToken); // Gamma Correction
    }

    void drawSceneForward(Cmd* graphicsCmd, RenderTarget* pRenderTarget)
    {
        cpuProfileEnter(gCpuDrawSceneForwardToken);

        //// draw skybox
        {
            cmdSetViewport(graphicsCmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 1.0f, 1.0f);
            const uint32_t skyboxVbStride = sizeof(float) * 4;
            cmdBindPipeline(graphicsCmd, pSkyBoxDrawPipeline);
            cmdBindDescriptorSet(graphicsCmd, 0, pDescriptorSetSkyboxTexture);
            cmdBindDescriptorSet(graphicsCmd, gFrameIndex, pDescriptorSetUniformsSkybox);
            cmdBindVertexBuffer(graphicsCmd, 1, &pSkyBoxVertexBuffer, &skyboxVbStride, NULL);
            cmdDraw(graphicsCmd, 36, 0);
        }

        cmdSetViewport(graphicsCmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);

        {
            // Opaque Pass
            cmdBindPipeline(graphicsCmd, pForwardPipeline);
            cmdBindDescriptorSet(graphicsCmd, gFrameIndex, pDescriptorSetUniformsScene);
            cmdBindIndexBuffer(graphicsCmd, pScene->pGeom->pIndexBuffer, pScene->pGeom->mIndexType, 0);
            cmdBindVertexBuffer(graphicsCmd, 4, pScene->pGeom->pVertexBuffers, pScene->pGeom->mVertexStrides, NULL);

            // Draw non-terrain meshes
            for (uint32_t i = 0; i < gMeshTypesCount[MT_OPAQUE]; ++i)
            {
                uint32_t dci = gSceneDrawArgsIndices[MT_OPAQUE][i];
                /**
                 * Binding descriptor set for every draw opaque draw call is expensive
                 * This takes around 2.5+ms in WEBGPU and 1.5ms+ on VULKAN cpu frame time...
                 */
                cmdBindDescriptorSet(graphicsCmd, dci, pDescriptorSetMaterials);
                cmdExecuteIndirect(graphicsCmd, pCmdSignatureScenePass, 1, gIndirectInstanceDataBuffer[gFrameIndex],
                                   dci * sizeof(IndirectDrawIndexArguments), NULL, 0);
            }
        }

        {
            cmdBindPipeline(graphicsCmd, pTerrainPipeline);
            cmdBindDescriptorSet(graphicsCmd, gFrameIndex, pDescriptorSetUniformsScene);
            cmdBindIndexBuffer(graphicsCmd, pScene->pGeom->pIndexBuffer, pScene->pGeom->mIndexType, 0);
            cmdBindVertexBuffer(graphicsCmd, 4, pScene->pGeom->pVertexBuffers, pScene->pGeom->mVertexStrides, NULL);

            // Only need to bind once
            // Terrain material is same..
            cmdBindDescriptorSet(graphicsCmd, gSceneDrawArgsIndices[MT_TERRAIN][0], pDescriptorSetMaterials);

            // Draw terrain meshes
            for (uint32_t i = 0; i < gMeshTypesCount[MT_TERRAIN]; ++i)
            {
                uint32_t dci = gSceneDrawArgsIndices[MT_TERRAIN][i];
                cmdExecuteIndirect(graphicsCmd, pCmdSignatureScenePass, 1, gIndirectInstanceDataBuffer[gFrameIndex],
                                   dci * sizeof(IndirectDrawIndexArguments), NULL, 0);
            }
        }

        // Transparency Pass
        {
            cmdBindPipeline(graphicsCmd, pTransparentForwardPipeline);
            cmdBindDescriptorSet(graphicsCmd, gFrameIndex, pDescriptorSetUniformsScene);
            cmdBindIndexBuffer(graphicsCmd, pScene->pGeom->pIndexBuffer, pScene->pGeom->mIndexType, 0);
            cmdBindVertexBuffer(graphicsCmd, 4, pScene->pGeom->pVertexBuffers, pScene->pGeom->mVertexStrides, NULL);

            for (uint32_t i = 0; i < gMeshTypesCount[MT_ALPHA_TESTED]; ++i)
            {
                uint32_t dci = gSceneDrawArgsIndices[MT_ALPHA_TESTED][i];
                cmdBindDescriptorSet(graphicsCmd, dci, pDescriptorSetMaterials);
                cmdExecuteIndirect(graphicsCmd, pCmdSignatureScenePass, 1, gIndirectInstanceDataBuffer[gFrameIndex],
                                   dci * sizeof(IndirectDrawIndexArguments), NULL, 0);
            }
        }

        cpuProfileLeave(gCpuDrawSceneForwardToken, gFrameCount);
    }

    void Draw()
    {
        cpuProfileEnter(gCpuDrawToken);

        if (pSwapChain->mEnableVsync != (uint32_t)mSettings.mVSyncEnabled)
        {
            waitQueueIdle(pGraphicsQueue);
            ::toggleVSync(pRenderer, &pSwapChain);
        }

#if defined(CPU_STRESS_TESTING_ENABLED)
        // Two command buffers
        // One for Submission, Another for binding test...
        GpuCmdRingElement graphicsElem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 2);
#else
        GpuCmdRingElement graphicsElem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);
#endif

        /************************************************************************/
        // Run Graphics Pipeline
        /************************************************************************/
        uint32_t swapchainImageIndex;
        acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL, &swapchainImageIndex);
        RenderTarget* pRenderTargetSwapchain = pSwapChain->ppRenderTargets[swapchainImageIndex];

        // Stall if CPU is running "gDataBufferCount" frames ahead of GPU
        FenceStatus fenceStatus;
        getFenceStatus(pRenderer, graphicsElem.pFence, &fenceStatus);
        if (fenceStatus == FENCE_STATUS_INCOMPLETE)
            waitForFences(pRenderer, 1, &graphicsElem.pFence);

        // Update uniform buffers
        BufferUpdateDesc viewProjCbv = { pProjViewUniformBuffer[gFrameIndex], 0, round_up(sizeof(UniformBlock), 16u) };
        beginUpdateResource(&viewProjCbv);
        memcpy(viewProjCbv.pMappedData, &gUniformData, round_up(sizeof(UniformBlock), 16u));
        endUpdateResource(&viewProjCbv);

        BufferUpdateDesc occlusionCullBuffer = { gCullData.pBufferUniformCull[gFrameIndex], 0, round_up(sizeof(CullUniformBlock), 16u) };
        beginUpdateResource(&occlusionCullBuffer);
        memcpy(occlusionCullBuffer.pMappedData, &gCullUniformBlock, round_up(sizeof(CullUniformBlock), 16u));
        endUpdateResource(&occlusionCullBuffer);

        BufferUpdateDesc skyboxViewProjCbv = { pSkyboxUniformBuffer[gFrameIndex], 0, round_up(sizeof(UniformBlockSky), 16u) };
        beginUpdateResource(&skyboxViewProjCbv);
        memcpy(skyboxViewProjCbv.pMappedData, &gUniformDataSky, round_up(sizeof(UniformBlockSky), 16u));
        endUpdateResource(&skyboxViewProjCbv);

        if (kShadowMapCascadeCount > 0)
        {
            BufferUpdateDesc cascadeBufferCbv = { gBufferShadowCascades[gFrameIndex], 0, round_up(sizeof(ShadowCascade), 16u) };
            beginUpdateResource(&cascadeBufferCbv);
            memcpy(cascadeBufferCbv.pMappedData, &gShadowCascades, round_up(sizeof(ShadowCascade), 16u));
            endUpdateResource(&cascadeBufferCbv);
        }

        BufferUpdateDesc gammaCorrrectionBufferCbv = { gGammaCorrectionData.pGammaCorrectionBuffer[gFrameIndex], 0,
                                                       round_up(sizeof(GammaCorrectionData::GammaCorrectionUniformData), 16u) };
        beginUpdateResource(&gammaCorrrectionBufferCbv);
        memcpy(gammaCorrrectionBufferCbv.pMappedData, &gGammaCorrectionData.mGammaCorrectionUniformData,
               round_up(sizeof(GammaCorrectionData::GammaCorrectionUniformData), 16u));
        endUpdateResource(&gammaCorrrectionBufferCbv);

        // Reset cmd pool for this frame
        resetCmdPool(pRenderer, graphicsElem.pCmdPool);

        if (cpuIsTestRunning())
        {
            cpuStressTestRun(&graphicsElem, pRenderTargetSwapchain);
        }
        else
        {
            Cmd* graphicsCmd = graphicsElem.pCmds[0];
            beginCmd(graphicsCmd);

            bakeShadowMap(graphicsCmd);

            cmdBeginGpuFrameProfile(graphicsCmd, gGraphicsProfileToken);

            /************************************************************************/
            // Run Compute Pipeline
            /************************************************************************/
            doLightCulling(graphicsCmd, gFrameIndex);

            /************************************************************************/
            /************************************************************************/
            if (gUseRealTimeShadows)
            {
                drawShadowMap(graphicsCmd);
                blurShadowMap(graphicsCmd);
            }

            doFrustumCulling(graphicsCmd);

            RenderTarget*       pRenderTarget = pIntermediateRenderTarget;
            RenderTargetBarrier barriers[] = {
                { pRenderTarget, RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET },
            };
            cmdResourceBarrier(graphicsCmd, 0, NULL, 0, NULL, 1, barriers);

            cmdBeginGpuTimestampQuery(graphicsCmd, gGraphicsProfileToken, "Draw Scene");

            // simply record the screen cleaning command
            BindRenderTargetsDesc bindRenderTargets = {};
            bindRenderTargets.mRenderTargetCount = 1;
            bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
            bindRenderTargets.mDepthStencil = { pDepthBuffer, LOAD_ACTION_CLEAR };
            cmdBindRenderTargets(graphicsCmd, &bindRenderTargets);

            cmdSetViewport(graphicsCmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
            cmdSetScissor(graphicsCmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

            drawSceneForward(graphicsCmd, pRenderTarget);

            cmdBindRenderTargets(graphicsCmd, NULL);
            cmdEndGpuTimestampQuery(graphicsCmd, gGraphicsProfileToken); // Draw Scene

            drawGammaCorrection(graphicsCmd, pRenderTarget, pRenderTargetSwapchain);

            RenderTargetBarrier rtSwapchainBarrier = { pRenderTargetSwapchain, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET };
            cmdResourceBarrier(graphicsCmd, 0, NULL, 0, NULL, 1, &rtSwapchainBarrier);

            cmdBeginGpuTimestampQuery(graphicsCmd, gGraphicsProfileToken, "Draw UI");

            bindRenderTargets = {};
            bindRenderTargets.mRenderTargetCount = 1;
            bindRenderTargets.mRenderTargets[0] = { pRenderTargetSwapchain, LOAD_ACTION_LOAD };
            bindRenderTargets.mDepthStencil = { NULL, LOAD_ACTION_DONTCARE };
            cmdBindRenderTargets(graphicsCmd, &bindRenderTargets);

            gFrameTimeDraw.mFontColor = 0xff00ffff;
            gFrameTimeDraw.mFontSize = 12.5f;
            gFrameTimeDraw.mFontID = gFontID;
            float2 txtSizePx = cmdDrawCpuProfile(graphicsCmd, float2(8.f, 15.f), &gFrameTimeDraw);
            cmdDrawGpuProfile(graphicsCmd, float2(8.f, txtSizePx.y + 75.0f), gGraphicsProfileToken, &gFrameTimeDraw);

            cmdDrawUserInterface(graphicsCmd);
            cmdBindRenderTargets(graphicsCmd, NULL);

            rtSwapchainBarrier = { pRenderTargetSwapchain, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
            cmdResourceBarrier(graphicsCmd, 0, NULL, 0, NULL, 1, &rtSwapchainBarrier);

            cmdEndGpuTimestampQuery(graphicsCmd, gGraphicsProfileToken); // Draw UI

#if defined(BAKE_SHADOW_MAPS)
            // End of draw we can transfer the shadow maps to transfer queue..
            // Next frame we will copy...
            // Allow only native apis to bake.
            if (pRenderer->mRendererApi != RENDERER_API_WEBGPU && !gShadowMapsBaked)
            {
                // Get shadow textures ready for copying..
                TextureBarrier texBarriers[kShadowMapCascadeCount];
                for (uint32_t i = 0; i < kShadowMapCascadeCount; ++i)
                {
                    texBarriers[i] = { gShadowMapping.pShadowMaps[i]->pTexture, RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                       RESOURCE_STATE_COPY_SOURCE };
                    texBarriers[i].mRelease = true;
                    // Release the texture from graphics queue..
                    texBarriers[i].mQueueType = QUEUE_TYPE_GRAPHICS;
                }
                cmdResourceBarrier(graphicsCmd, 0, NULL, kShadowMapCascadeCount, texBarriers, 0, NULL);

                // Signal..
                gShadowMapsReadyForBake = true;
            }
#endif

            cmdEndGpuFrameProfile(graphicsCmd, gGraphicsProfileToken);
            endCmd(graphicsCmd);

            FlushResourceUpdateDesc flushUpdateDesc = {};
            flushUpdateDesc.mNodeIndex = 0;
            flushResourceUpdates(&flushUpdateDesc);
            Semaphore* waitSemaphores[] = { flushUpdateDesc.pOutSubmittedSemaphore, pImageAcquiredSemaphore };

            QueueSubmitDesc submitDesc = {};
            submitDesc.mCmdCount = 1;
            submitDesc.mSignalSemaphoreCount = 1;
            submitDesc.mWaitSemaphoreCount = TF_ARRAY_COUNT(waitSemaphores);
            submitDesc.ppCmds = &graphicsCmd;
            submitDesc.ppSignalSemaphores = &graphicsElem.pSemaphore;
            submitDesc.ppWaitSemaphores = waitSemaphores;
            submitDesc.pSignalFence = graphicsElem.pFence;

            cpuProfileEnter(gCpuDrawSceneForwardSubmissionToken);
            queueSubmit(pGraphicsQueue, &submitDesc);
            cpuProfileLeave(gCpuDrawSceneForwardSubmissionToken, gFrameCount);
        }

        QueuePresentDesc presentDesc = {};
        presentDesc.mIndex = (uint8_t)swapchainImageIndex;
        presentDesc.mWaitSemaphoreCount = 1;
        presentDesc.pSwapChain = pSwapChain;
        presentDesc.ppWaitSemaphores = &graphicsElem.pSemaphore;
        presentDesc.mSubmitDone = true;
        cpuProfileEnter(gCpuDrawPresentationToken);
        queuePresent(pGraphicsQueue, &presentDesc);
        cpuProfileLeave(gCpuDrawPresentationToken, gFrameCount);

#if defined(CPU_STRESS_TESTING_ENABLED)
        if (gCpuStressTestData.bShouldTakeScreenshot)
        {
            waitQueueIdle(pGraphicsQueue);
            graphicsElem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);
            cpuGraphStressTestData(&graphicsElem, gCpuStressTestData.pRenderTarget);
        }

        // Update Sample
        if (gCpuStressTestData.bIsTestRunning)
            gCpuStressTestData.mCurrentSample++;
#endif

        cpuProfileLeave(gCpuDrawToken, gFrameCount);
        cpuProfileLeave(gCpuFrameTimeToken, gFrameCount);

        flipProfiler();

        ++gFrameCount;
        gFrameIndex = (gFrameIndex + 1) % gDataBufferCount;
    }

    const char* GetName() { return "SunTemple"; }

    bool addSwapChain()
    {
        SwapChainDesc swapChainDesc = {};
        swapChainDesc.mColorClearValue.r = 0.1f;
        swapChainDesc.mWindowHandle = pWindow->handle;
        swapChainDesc.mPresentQueueCount = 1;
        swapChainDesc.ppPresentQueues = &pGraphicsQueue;
        swapChainDesc.mWidth = mSettings.mWidth;
        swapChainDesc.mHeight = mSettings.mHeight;
        swapChainDesc.mImageCount = getRecommendedSwapchainImageCount(pRenderer, &pWindow->handle);
        swapChainDesc.mColorFormat = getSupportedSwapchainFormat(pRenderer, &swapChainDesc, COLOR_SPACE_SDR_LINEAR);
        swapChainDesc.mColorSpace = COLOR_SPACE_SDR_LINEAR;
        swapChainDesc.mEnableVsync = mSettings.mVSyncEnabled;
        swapChainDesc.mFlags = SWAP_CHAIN_CREATION_FLAG_ENABLE_FOVEATED_RENDERING_VR;
        ::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

        return pSwapChain != NULL;
    }

    bool addRenderTargets()
    {
        const ClearValue greaterEqualDepthStencilClear = { { 0.f, 0 } };
        // const ClearValue lessEqualDepthStencilClear = { { 1.f, 0 } };

        // Add depth buffer
        RenderTargetDesc depthRT = {};
        depthRT.mArraySize = 1;
        depthRT.mClearValue = greaterEqualDepthStencilClear;
        depthRT.mDepth = 1;
        depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
        depthRT.mStartState = RESOURCE_STATE_DEPTH_WRITE;
        depthRT.mHeight = mSettings.mHeight;
        depthRT.mSampleCount = SAMPLE_COUNT_1;
        depthRT.mSampleQuality = 0;
        depthRT.mWidth = mSettings.mWidth;
        depthRT.mFlags = TEXTURE_CREATION_FLAG_VR_MULTIVIEW;
        addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);

        /************************************************************************/
        // Intermediate render target
        /************************************************************************/
        RenderTargetDesc intermediateRTDesc = {};
        intermediateRTDesc.mArraySize = 1;
        intermediateRTDesc.mClearValue = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        intermediateRTDesc.mDepth = 1;
        intermediateRTDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
        intermediateRTDesc.mStartState = RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        intermediateRTDesc.mHeight = mSettings.mHeight;
        intermediateRTDesc.mWidth = mSettings.mWidth;
        intermediateRTDesc.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
        intermediateRTDesc.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
        intermediateRTDesc.mFlags = TEXTURE_CREATION_FLAG_ESRAM;
        intermediateRTDesc.pName = "pIntermediateRenderTarget";
        intermediateRTDesc.mFormat = TinyImageFormat_FromName(gRenderTargetFormatNames[gRenderTargetFormatWidgetData]);
        addRenderTarget(pRenderer, &intermediateRTDesc, &pIntermediateRenderTarget);
#if defined(CPU_STRESS_TESTING_ENABLED)
        intermediateRTDesc.mFormat = pSwapChain->ppRenderTargets[0]->mFormat;
        addRenderTarget(pRenderer, &intermediateRTDesc, &gCpuStressTestData.pRenderTarget);
#endif

        if (kShadowMapCascadeCount > 0)
        {
            addShadowRenderTargets();
        }

        return pDepthBuffer != NULL;
    }

    void addShadowRenderTargets()
    {
        const ClearValue lessEqualDepthStencilClear = { { 1.f, 0 } };

        RenderTargetDesc defShadowRTDesc = {};
        defShadowRTDesc.mArraySize = 1;
        defShadowRTDesc.mClearValue.depth = lessEqualDepthStencilClear.depth;
        defShadowRTDesc.mDepth = 1;
        defShadowRTDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
        defShadowRTDesc.mFormat = TinyImageFormat_D32_SFLOAT;
        defShadowRTDesc.mStartState = RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        defShadowRTDesc.mWidth = kShadowMapResWidth;
        defShadowRTDesc.mHeight = kShadowMapResHeight;
        defShadowRTDesc.mSampleCount = (SampleCount)1;
        defShadowRTDesc.mSampleQuality = 0;
        defShadowRTDesc.pName = "Default Shadow Map RT";
        addRenderTarget(pRenderer, &defShadowRTDesc, &pRenderTargetShadowMap);

        /************************************************************************/
        // Shadow Map Render Target
        /************************************************************************/
        RenderTargetDesc shadowRTDesc = {};
        shadowRTDesc.mArraySize = 1;
        shadowRTDesc.mClearValue.depth = lessEqualDepthStencilClear.depth;
        shadowRTDesc.mDepth = 1;
        shadowRTDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
#if defined(BLUR_PIPELINE)
        shadowRTDesc.mDescriptors |= DESCRIPTOR_TYPE_RW_TEXTURE;
#endif
        shadowRTDesc.mFormat = TinyImageFormat_R16G16_UNORM;
        shadowRTDesc.mStartState = RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        shadowRTDesc.mWidth = kShadowMapResWidth;
        shadowRTDesc.mHeight = kShadowMapResHeight;
        shadowRTDesc.mSampleCount = (SampleCount)1;
        shadowRTDesc.mSampleQuality = 0;
        for (uint32_t i = 0; i < kShadowMapCascadeCount; ++i)
        {
            shadowRTDesc.pName = "Shadow Map RT";
            addRenderTarget(pRenderer, &shadowRTDesc, &gShadowMapping.pShadowMaps[i]);
        }

        // We only want the texture
        RenderTargetDesc shadowBlurTexDesc = {};
        shadowBlurTexDesc.mArraySize = 1;
        shadowBlurTexDesc.mClearValue.depth = lessEqualDepthStencilClear.depth;
        shadowBlurTexDesc.mDepth = 1;
        shadowBlurTexDesc.mDescriptors = shadowRTDesc.mDescriptors;
        shadowBlurTexDesc.mFormat = shadowRTDesc.mFormat;
        shadowBlurTexDesc.mStartState = RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        shadowBlurTexDesc.mWidth = kShadowMapResWidth;
        shadowBlurTexDesc.mHeight = kShadowMapResHeight;
        shadowBlurTexDesc.mSampleCount = (SampleCount)1;
        shadowBlurTexDesc.mSampleQuality = 0;
        addRenderTarget(pRenderer, &shadowBlurTexDesc, &pRenderTargetShaderMapBlur);
    }

    void removeShadowRenderTargets()
    {
        removeRenderTarget(pRenderer, pRenderTargetShadowMap);
        removeRenderTarget(pRenderer, pRenderTargetShaderMapBlur);

        for (uint32_t i = 0; i < kShadowMapCascadeCount; ++i)
        {
            removeRenderTarget(pRenderer, gShadowMapping.pShadowMaps[i]);
        }
    }

    void removeRenderTargets()
    {
        removeRenderTarget(pRenderer, pDepthBuffer);
        removeRenderTarget(pRenderer, pIntermediateRenderTarget);

#if defined(CPU_STRESS_TESTING_ENABLED)
        removeRenderTarget(pRenderer, gCpuStressTestData.pRenderTarget);
#endif

        if (kShadowMapCascadeCount > 0)
        {
            removeShadowRenderTargets();
        }
    }

    void addDescriptorSets()
    {
        DescriptorSetDesc desc = { pRootSignatureSkybox, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
        addDescriptorSet(pRenderer, &desc, &pDescriptorSetSkyboxTexture);

        desc = { pRootSignatureSkybox, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount };
        addDescriptorSet(pRenderer, &desc, &pDescriptorSetUniformsSkybox);

        desc = { pRootSignatureScene, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount };
        addDescriptorSet(pRenderer, &desc, &pDescriptorSetUniformsScene);
        desc = { pRootSignatureScene, DESCRIPTOR_UPDATE_FREQ_NONE, gMaterialCount };
        addDescriptorSet(pRenderer, &desc, &pDescriptorSetMaterials);
        addDescriptorSet(pRenderer, &desc, &pDescriptorSetMaterialsTesting);

        desc = { gGammaCorrectionData.pRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
        addDescriptorSet(pRenderer, &desc, &gGammaCorrectionData.pSetTexture);
        desc = { gGammaCorrectionData.pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount };
        addDescriptorSet(pRenderer, &desc, &gGammaCorrectionData.pSetUniform);

        // Light Clusters
        if (gLightCullingEnabled)
        {
            desc = { pRootSignatureLightClusters, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount };
            addDescriptorSet(pRenderer, &desc, &pDescriptorSetLightClusters);
        }

        // Frustum
        desc = { gCullData.pRootSignatures[CST_FRUSTUM_CULL_PASS], DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
        addDescriptorSet(pRenderer, &desc, &gCullData.pSetUpdateNone[CST_FRUSTUM_CULL_PASS]);

        desc = { gCullData.pRootSignatures[CST_FRUSTUM_CULL_PASS], DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount };
        addDescriptorSet(pRenderer, &desc, &gCullData.pSetUpdatePerFrame[CST_FRUSTUM_CULL_PASS]);

        if (kShadowMapCascadeCount > 0)
        {
            addShadowDescriptorSets();
        }

#ifdef BLUR_PIPELINE
        // Gaussian blur
        desc = { pRootSignatureBlurCompute[PASS_TYPE_HORIZONTAL], DESCRIPTOR_UPDATE_FREQ_NONE, kShadowMapCascadeCount };
        addDescriptorSet(pRenderer, &desc, &pDescriptorSetBlurCompute[PASS_TYPE_HORIZONTAL]);

        desc = { pRootSignatureBlurCompute[PASS_TYPE_VERTICAL], DESCRIPTOR_UPDATE_FREQ_NONE, kShadowMapCascadeCount };
        addDescriptorSet(pRenderer, &desc, &pDescriptorSetBlurCompute[PASS_TYPE_VERTICAL]);
#endif
#if defined(CPU_STRESS_TESTING_ENABLED)
        desc = { gCpuStressTestData.pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, 1 };
        addDescriptorSet(pRenderer, &desc, &gCpuStressTestData.pSet);
#endif
    }

    void addShadowDescriptorSets()
    {
        DescriptorSetDesc desc = { gShadowMapping.pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount };
        addDescriptorSet(pRenderer, &desc, &gShadowMapping.pDescriptorSetUniformsCascades);

        desc = { gShadowMapping.pRootSignatureAlpha, DESCRIPTOR_UPDATE_FREQ_NONE, gMeshTypesCount[MT_ALPHA_TESTED] };
        addDescriptorSet(pRenderer, &desc, &gShadowMapping.pDescriptorSetAlphaTextures);

        desc = { gShadowMapping.pRootSignatureAlpha, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gDataBufferCount };
        addDescriptorSet(pRenderer, &desc, &gShadowMapping.pDescriptorSetAlphaUniforms);
    }

    void removeShadowDescriptorSets()
    {
        removeDescriptorSet(pRenderer, gShadowMapping.pDescriptorSetUniformsCascades);
        removeDescriptorSet(pRenderer, gShadowMapping.pDescriptorSetAlphaTextures);
        removeDescriptorSet(pRenderer, gShadowMapping.pDescriptorSetAlphaUniforms);
    }

    void removeDescriptorSets()
    {
        removeDescriptorSet(pRenderer, pDescriptorSetSkyboxTexture);
        removeDescriptorSet(pRenderer, pDescriptorSetUniformsScene);
        removeDescriptorSet(pRenderer, pDescriptorSetUniformsSkybox);
        removeDescriptorSet(pRenderer, pDescriptorSetMaterials);
        removeDescriptorSet(pRenderer, pDescriptorSetMaterialsTesting);

        removeDescriptorSet(pRenderer, gGammaCorrectionData.pSetTexture);
        removeDescriptorSet(pRenderer, gGammaCorrectionData.pSetUniform);

        if (gUseLightCulling)
        {
            removeDescriptorSet(pRenderer, pDescriptorSetLightClusters);
        }

        for (uint32_t i = 0; i < CST_COUNT_MAX; ++i)
        {
            if (gCullData.pSetUpdateNone[i] != NULL)
                removeDescriptorSet(pRenderer, gCullData.pSetUpdateNone[i]);

            if (gCullData.pSetUpdatePerFrame[i] != NULL)
                removeDescriptorSet(pRenderer, gCullData.pSetUpdatePerFrame[i]);
        }

        if (kShadowMapCascadeCount > 0)
        {
            removeShadowDescriptorSets();
        }

#ifdef BLUR_PIPELINE
        removeDescriptorSet(pRenderer, pDescriptorSetBlurCompute[PASS_TYPE_VERTICAL]);
        removeDescriptorSet(pRenderer, pDescriptorSetBlurCompute[PASS_TYPE_HORIZONTAL]);
#endif
#if defined(CPU_STRESS_TESTING_ENABLED)
        removeDescriptorSet(pRenderer, gCpuStressTestData.pSet);
#endif
    }

    void addRootSignatures()
    {
        const char* pSampler0Name[1] = { "uSampler0" };

        const uint32_t numStaticSamplers = 5;
        const char*    pSceneStaticSamplerNames[numStaticSamplers] = { "uSamplerSunTempleAlbedo", "uSamplerSunTempleTerrainNormal",
                                                                    "clampMiplessLinearSampler", "brdfIntegrationSampler",
                                                                    "uSamplerSunTempleLightmap" };
        Sampler*       pSceneStaticSamplers[numStaticSamplers] = { pSamplerSunTempleAlbedo, pSamplerSunTempleTerrainNormal,
                                                             pSamplerMiplessNearest, pSamplerBilinearClamp, pSamplerSunTempleLightmap };

        Shader*           ppSceneShaders[2] = { pForwardShaders, pTerrainShaders };
        RootSignatureDesc rootDesc = {};
        rootDesc.mStaticSamplerCount = numStaticSamplers;
        rootDesc.ppStaticSamplerNames = pSceneStaticSamplerNames;
        rootDesc.ppStaticSamplers = pSceneStaticSamplers;
        rootDesc.mShaderCount = 2;
        rootDesc.ppShaders = ppSceneShaders;
        addRootSignature(pRenderer, &rootDesc, &pRootSignatureScene);

        rootDesc.mStaticSamplerCount = 1;
        rootDesc.ppStaticSamplers = &pSamplerSkyBox;
        rootDesc.ppStaticSamplerNames = pSampler0Name;
        rootDesc.mShaderCount = 1;
        rootDesc.ppShaders = &pSkyBoxDrawShader;
        addRootSignature(pRenderer, &rootDesc, &pRootSignatureSkybox);

        rootDesc.mStaticSamplerCount = 1;
        rootDesc.ppStaticSamplers = &pSamplerBilinearClamp;
        rootDesc.ppStaticSamplerNames = pSampler0Name;
        rootDesc.mShaderCount = 1;
        rootDesc.ppShaders = &gGammaCorrectionData.pShader;
        addRootSignature(pRenderer, &rootDesc, &gGammaCorrectionData.pRootSignature);

        if (gLightCullingEnabled)
        {
            Shader*           pClusterShaders[] = { pShaderClearLightClusters, pShaderClusterLights };
            RootSignatureDesc clearLightRootDesc = { pClusterShaders, 2 };
            addRootSignature(pRenderer, &clearLightRootDesc, &pRootSignatureLightClusters);
        }

        rootDesc.mStaticSamplerCount = 0;
        rootDesc.mShaderCount = 1;
        rootDesc.ppShaders = &gCullData.pShaders[CST_FRUSTUM_CULL_PASS];
        addRootSignature(pRenderer, &rootDesc, &gCullData.pRootSignatures[CST_FRUSTUM_CULL_PASS]);

        if (kShadowMapCascadeCount > 0)
        {
            addShadowRootSignatures();
        }

#ifdef BLUR_PIPELINE
        Shader*           blurShaders[1] = { pShaderBlurComp[PASS_TYPE_HORIZONTAL] };
        RootSignatureDesc BlurRootDesc = { blurShaders, 1 };
        addRootSignature(pRenderer, &BlurRootDesc, &pRootSignatureBlurCompute[PASS_TYPE_HORIZONTAL]);

        blurShaders[0] = pShaderBlurComp[PASS_TYPE_VERTICAL];
        addRootSignature(pRenderer, &BlurRootDesc, &pRootSignatureBlurCompute[PASS_TYPE_VERTICAL]);
#endif
#if defined(CPU_STRESS_TESTING_ENABLED)
        rootDesc.mStaticSamplerCount = 0;
        rootDesc.mShaderCount = 1;
        rootDesc.ppShaders = &gCpuStressTestData.pShader;
        addRootSignature(pRenderer, &rootDesc, &gCpuStressTestData.pRootSignature);
#endif

        /************************************************************************/
        // Setup indirect command signatures
        /************************************************************************/
        IndirectArgumentDescriptor indirectArgs = {};
        indirectArgs.mType = INDIRECT_DRAW_INDEX;
        indirectArgs.mIndex = getDescriptorIndexFromName(pRootSignatureScene, "indirectRootConstant");
        indirectArgs.mByteSize = sizeof(uint32_t);

        CommandSignatureDesc scenePassDesc = { pRootSignatureScene, &indirectArgs, 1 };
        scenePassDesc.mPacked = true;
        addIndirectCommandSignature(pRenderer, &scenePassDesc, &pCmdSignatureScenePass);
    }

    void addShadowRootSignatures()
    {
        RootSignatureDesc smDEsc = {};
        smDEsc.mShaderCount = 1;
        smDEsc.ppShaders = &gShadowMapping.pShaderDepth;
        smDEsc.mStaticSamplerCount = 0;
        smDEsc.ppStaticSamplers = NULL;
        smDEsc.ppStaticSamplerNames = NULL;
        addRootSignature(pRenderer, &smDEsc, &gShadowMapping.pRootSignature);

        smDEsc.mShaderCount = 1;
        smDEsc.ppShaders = &gShadowMapping.pShaderDepthAlpha;
        smDEsc.mStaticSamplerCount = 1;
        smDEsc.ppStaticSamplers = &pSamplerSunTempleAlbedo;
        const char* pSamplerNames[1] = { "uSamplerSunTempleAlbedo" };
        smDEsc.ppStaticSamplerNames = pSamplerNames;
        addRootSignature(pRenderer, &smDEsc, &gShadowMapping.pRootSignatureAlpha);
    }

    void removeShadowRootSignatures()
    {
        removeRootSignature(pRenderer, gShadowMapping.pRootSignature);
        removeRootSignature(pRenderer, gShadowMapping.pRootSignatureAlpha);
    }

    void removeRootSignatures()
    {
        removeRootSignature(pRenderer, pRootSignatureScene);
        removeRootSignature(pRenderer, pRootSignatureSkybox);
        removeRootSignature(pRenderer, gGammaCorrectionData.pRootSignature);

        if (gUseLightCulling)
        {
            removeRootSignature(pRenderer, pRootSignatureLightClusters);
        }

        for (uint32_t i = 0; i < CST_COUNT_MAX; ++i)
            removeRootSignature(pRenderer, gCullData.pRootSignatures[i]);

        if (kShadowMapCascadeCount > 0)
        {
            removeShadowRootSignatures();
        }

#ifdef BLUR_PIPELINE
        removeRootSignature(pRenderer, pRootSignatureBlurCompute[PASS_TYPE_HORIZONTAL]);
        removeRootSignature(pRenderer, pRootSignatureBlurCompute[PASS_TYPE_VERTICAL]);
#endif
#if defined(CPU_STRESS_TESTING_ENABLED)
        removeRootSignature(pRenderer, gCpuStressTestData.pRootSignature);
#endif

        // Remove indirect command signatures
        removeIndirectCommandSignature(pRenderer, pCmdSignatureScenePass);
    }

    void addShaders()
    {
        char           pShaderVariantFrag[128];
        ShaderLoadDesc skyShader = {};
        skyShader.mStages[0].pFileName = "skybox.vert";
        skyShader.mStages[1].pFileName = "skybox.frag";
        addShader(pRenderer, &skyShader, &pSkyBoxDrawShader);

        ShaderLoadDesc basicShader = {};
        basicShader.mStages[0].pFileName = "pbr.vert";
        snprintf(pShaderVariantFrag, 128, "pbr%s.frag", gLightCullingEnabled ? "_light_cull" : "_no_cull");
        basicShader.mStages[1].pFileName = pShaderVariantFrag;
        addShader(pRenderer, &basicShader, &pForwardShaders);

        ShaderLoadDesc terrainShader = {};
        terrainShader.mStages[0].pFileName = "terrain.vert";
        terrainShader.mStages[1].pFileName = "terrain.frag";
        addShader(pRenderer, &terrainShader, &pTerrainShaders);

        ShaderLoadDesc gammaCorrectionShader = {};
        gammaCorrectionShader.mStages[0].pFileName = "fullscreen.vert";
        gammaCorrectionShader.mStages[1].pFileName = "gammaCorrection.frag";
        addShader(pRenderer, &gammaCorrectionShader, &gGammaCorrectionData.pShader);

        if (gLightCullingEnabled)
        {
            ShaderLoadDesc clearLights = {};
            ShaderLoadDesc clusterLights = {};
            // Clear light clusters compute shader
            clearLights.mStages[0].pFileName = "clear_light_clusters.comp";
            // Cluster lights compute shader
            clusterLights.mStages[0].pFileName = "cluster_lights.comp";
            addShader(pRenderer, &clearLights, &pShaderClearLightClusters);
            addShader(pRenderer, &clusterLights, &pShaderClusterLights);
        }

        ShaderLoadDesc occlusionShader = {};
        occlusionShader.mStages[0].pFileName = "frustum_cull_pass.comp";
        addShader(pRenderer, &occlusionShader, &gCullData.pShaders[CST_FRUSTUM_CULL_PASS]);

        if (kShadowMapCascadeCount > 0)
        {
            addShadowShaders();
        }

#ifdef BLUR_PIPELINE
        ShaderLoadDesc BlurCompShaderDesc = {};
        BlurCompShaderDesc.mStages[0].pFileName = "gaussianBlur_Horizontal.comp";
        addShader(pRenderer, &BlurCompShaderDesc, &pShaderBlurComp[PASS_TYPE_HORIZONTAL]);
        BlurCompShaderDesc.mStages[0].pFileName = "gaussianBlur_Vertical.comp";
        addShader(pRenderer, &BlurCompShaderDesc, &pShaderBlurComp[PASS_TYPE_VERTICAL]);
#endif
#if defined(CPU_STRESS_TESTING_ENABLED)
        ShaderLoadDesc lineShader = {};
        lineShader.mStages[0].pFileName = "line.vert";
        lineShader.mStages[1].pFileName = "line.frag";
        addShader(pRenderer, &lineShader, &gCpuStressTestData.pShader);
#endif
    }

    void addShadowShaders()
    {
        ShaderLoadDesc smDepthPassShaderDesc = {};
        smDepthPassShaderDesc.mStages[0].pFileName = "meshDepthPass.vert";
        smDepthPassShaderDesc.mStages[1].pFileName = "meshDepthPass.frag";
        addShader(pRenderer, &smDepthPassShaderDesc, &gShadowMapping.pShaderDepth);

        smDepthPassShaderDesc.mStages[0].pFileName = "meshDepthPassAlpha.vert";
        smDepthPassShaderDesc.mStages[1].pFileName = "meshDepthPassAlpha.frag";
        addShader(pRenderer, &smDepthPassShaderDesc, &gShadowMapping.pShaderDepthAlpha);
    }

    void removeShadowShaders()
    {
        removeShader(pRenderer, gShadowMapping.pShaderDepth);
        removeShader(pRenderer, gShadowMapping.pShaderDepthAlpha);
    }

    void removeShaders()
    {
        removeShader(pRenderer, pSkyBoxDrawShader);
        removeShader(pRenderer, pForwardShaders);
        removeShader(pRenderer, pTerrainShaders);
        removeShader(pRenderer, gGammaCorrectionData.pShader);

        if (gUseLightCulling)
        {
            removeShader(pRenderer, pShaderClearLightClusters);
            removeShader(pRenderer, pShaderClusterLights);
        }

        for (uint32_t i = 0; i < CST_COUNT_MAX; ++i)
            removeShader(pRenderer, gCullData.pShaders[i]);

        if (kShadowMapCascadeCount > 0)
        {
            removeShadowShaders();
        }

#ifdef BLUR_PIPELINE
        removeShader(pRenderer, pShaderBlurComp[PASS_TYPE_HORIZONTAL]);
        removeShader(pRenderer, pShaderBlurComp[PASS_TYPE_VERTICAL]);
#endif
#if defined(CPU_STRESS_TESTING_ENABLED)
        removeShader(pRenderer, gCpuStressTestData.pShader);
#endif
    }

    void addPipelines()
    {
        /************************************************************************/
        // Graphics Pipelines
        /************************************************************************/
        RasterizerStateDesc rasterStateCullNoneDesc = { CULL_MODE_NONE };
        RasterizerStateDesc basicRasterizerStateDesc = { CULL_MODE_NONE };

        DepthStateDesc depthStateReversedEnabledDesc = {};
        depthStateReversedEnabledDesc.mDepthFunc = CMP_GEQUAL;
        depthStateReversedEnabledDesc.mDepthWrite = true;
        depthStateReversedEnabledDesc.mDepthTest = true;

        DepthStateDesc depthStateEnabledDesc = {};
        depthStateEnabledDesc.mDepthFunc = CMP_LEQUAL;
        depthStateEnabledDesc.mDepthWrite = true;
        depthStateEnabledDesc.mDepthTest = true;

        BlendStateDesc blendStateAlphaDesc = {};
        blendStateAlphaDesc.mSrcFactors[0] = BC_SRC_ALPHA;
        blendStateAlphaDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;
        blendStateAlphaDesc.mBlendModes[0] = BM_ADD;
        blendStateAlphaDesc.mSrcAlphaFactors[0] = BC_ONE;
        blendStateAlphaDesc.mDstAlphaFactors[0] = BC_ZERO;
        blendStateAlphaDesc.mBlendAlphaModes[0] = BM_ADD;
        blendStateAlphaDesc.mColorWriteMasks[0] = COLOR_MASK_ALL;
        blendStateAlphaDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
        blendStateAlphaDesc.mIndependentBlend = false;

        PipelineDesc desc = {};
        desc.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pColorFormats = &pIntermediateRenderTarget->mFormat;
        pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
        pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
        pipelineSettings.mDepthStencilFormat = pDepthBuffer->mFormat;
        pipelineSettings.pRootSignature = pRootSignatureScene;
        pipelineSettings.pVertexLayout = &gSceneVertexLayout;
        pipelineSettings.pShaderProgram = pForwardShaders; //-V519
        pipelineSettings.pDepthState = &depthStateReversedEnabledDesc;
        pipelineSettings.pRasterizerState = &basicRasterizerStateDesc;
        addPipeline(pRenderer, &desc, &pForwardPipeline);

        pipelineSettings.pShaderProgram = pTerrainShaders; //-V519
        addPipeline(pRenderer, &desc, &pTerrainPipeline);

        // Transparent forward shading pipeline
        desc.mGraphicsDesc = {};
        GraphicsPipelineDesc& transparentForwardPipelineDesc = desc.mGraphicsDesc;
        transparentForwardPipelineDesc.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        transparentForwardPipelineDesc.pShaderProgram = pForwardShaders;
        transparentForwardPipelineDesc.pRootSignature = pRootSignatureScene;
        transparentForwardPipelineDesc.mRenderTargetCount = 1;
        transparentForwardPipelineDesc.pColorFormats = &pIntermediateRenderTarget->mFormat;
        transparentForwardPipelineDesc.mSampleCount = SAMPLE_COUNT_1;
        transparentForwardPipelineDesc.mSampleQuality = 0;
        transparentForwardPipelineDesc.mDepthStencilFormat = pDepthBuffer->mFormat;
        transparentForwardPipelineDesc.pVertexLayout = &gSceneVertexLayout;
        transparentForwardPipelineDesc.pRasterizerState = &basicRasterizerStateDesc;
        transparentForwardPipelineDesc.pDepthState = &depthStateReversedEnabledDesc;
        transparentForwardPipelineDesc.pBlendState = &blendStateAlphaDesc;
        addPipeline(pRenderer, &desc, &pTransparentForwardPipeline);

        // layout and pipeline for skybox draw
        VertexLayout skyboxVertexLayout = {};
        skyboxVertexLayout.mBindingCount = 1;
        skyboxVertexLayout.mAttribCount = 1;
        skyboxVertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        skyboxVertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
        skyboxVertexLayout.mAttribs[0].mBinding = 0;
        skyboxVertexLayout.mAttribs[0].mLocation = 0;
        skyboxVertexLayout.mAttribs[0].mOffset = 0;
        pipelineSettings.pVertexLayout = &skyboxVertexLayout;

        pipelineSettings.pDepthState = NULL;
        pipelineSettings.pRasterizerState = &rasterStateCullNoneDesc;
        pipelineSettings.pShaderProgram = pSkyBoxDrawShader; //-V519
        pipelineSettings.pRootSignature = pRootSignatureSkybox;
        addPipeline(pRenderer, &desc, &pSkyBoxDrawPipeline);

        /************************************************************************/
        // Setup Gamma Correction pipeline
        /************************************************************************/
        GraphicsPipelineDesc& pipelineSettingsGammaCorrection = desc.mGraphicsDesc;
        pipelineSettingsGammaCorrection = { 0 };
        pipelineSettingsGammaCorrection.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettingsGammaCorrection.pRasterizerState = &rasterStateCullNoneDesc;
        pipelineSettingsGammaCorrection.mRenderTargetCount = 1;
        pipelineSettingsGammaCorrection.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
        pipelineSettingsGammaCorrection.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
        pipelineSettingsGammaCorrection.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
        pipelineSettingsGammaCorrection.pRootSignature = gGammaCorrectionData.pRootSignature;
        pipelineSettingsGammaCorrection.pShaderProgram = gGammaCorrectionData.pShader;
        desc.pName = "Gamma Correction";
        addPipeline(pRenderer, &desc, &gGammaCorrectionData.pPipeline);

#if defined(CPU_STRESS_TESTING_ENABLED)
        VertexLayout lineVertexLayout = {};
        lineVertexLayout.mBindingCount = 1;
        lineVertexLayout.mAttribCount = 1;
        lineVertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        lineVertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32_SFLOAT;
        lineVertexLayout.mAttribs[0].mBinding = 0;
        lineVertexLayout.mAttribs[0].mLocation = 0;
        lineVertexLayout.mAttribs[0].mOffset = 0;

        pipelineSettings.pColorFormats = &gCpuStressTestData.pRenderTarget->mFormat;
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_LINE_LIST;
        pipelineSettings.pRootSignature = gCpuStressTestData.pRootSignature;
        pipelineSettings.pVertexLayout = &lineVertexLayout;
        pipelineSettings.pShaderProgram = gCpuStressTestData.pShader; //-V519
        addPipeline(pRenderer, &desc, &gCpuStressTestData.pPipeline);
#endif

        /************************************************************************/
        // Compute Pipelines
        /************************************************************************/
        desc.mType = PIPELINE_TYPE_COMPUTE;
        desc.mComputeDesc = {};
        if (gLightCullingEnabled)
        {
            ComputePipelineDesc& lightClusterPipelineSettings = desc.mComputeDesc;
            // Setup the clearing light clusters pipeline
            lightClusterPipelineSettings.pShaderProgram = pShaderClearLightClusters;
            lightClusterPipelineSettings.pRootSignature = pRootSignatureLightClusters;
            desc.pName = "lightClusterClearPipeline";
            addPipeline(pRenderer, &desc, &pPipelineClearLightClusters);

            // Setup the compute the light clusters pipeline
            lightClusterPipelineSettings.pShaderProgram = pShaderClusterLights;
            lightClusterPipelineSettings.pRootSignature = pRootSignatureLightClusters;
            desc.pName = "lightClusterPipeline";
            addPipeline(pRenderer, &desc, &pPipelineClusterLights);
        }

        ComputePipelineDesc& frustumPipelineSettings = desc.mComputeDesc;
        frustumPipelineSettings = {};
        frustumPipelineSettings.pShaderProgram = gCullData.pShaders[CST_FRUSTUM_CULL_PASS];
        frustumPipelineSettings.pRootSignature = gCullData.pRootSignatures[CST_FRUSTUM_CULL_PASS];
        desc.pName = "Frsutum Cull Pipeline";
        addPipeline(pRenderer, &desc, &gCullData.pPipelines[CST_FRUSTUM_CULL_PASS]);

        if (kShadowMapCascadeCount > 0)
        {
            addShadowPipelines();
        }

#ifdef BLUR_PIPELINE
        desc.mComputeDesc = {};
        ComputePipelineDesc& BlurCompPipelineSettings = desc.mComputeDesc;
        BlurCompPipelineSettings.pRootSignature = pRootSignatureBlurCompute[PASS_TYPE_HORIZONTAL];
        BlurCompPipelineSettings.pShaderProgram = pShaderBlurComp[PASS_TYPE_HORIZONTAL];
        addPipeline(pRenderer, &desc, &pPipelineBlur[PASS_TYPE_HORIZONTAL]);

        BlurCompPipelineSettings.pRootSignature = pRootSignatureBlurCompute[PASS_TYPE_VERTICAL];
        BlurCompPipelineSettings.pShaderProgram = pShaderBlurComp[PASS_TYPE_VERTICAL];
        addPipeline(pRenderer, &desc, &pPipelineBlur[PASS_TYPE_VERTICAL]);
#endif
    }

    void addShadowPipelines()
    {
        RasterizerStateDesc rasterStateCullNoneDesc = { CULL_MODE_NONE };

        DepthStateDesc depthStateLEQUALEnabledDesc = {};
        depthStateLEQUALEnabledDesc.mDepthFunc = CMP_LEQUAL;
        depthStateLEQUALEnabledDesc.mDepthWrite = true;
        depthStateLEQUALEnabledDesc.mDepthTest = true;

        PipelineDesc desc = {};
        desc.mType = PIPELINE_TYPE_GRAPHICS;
        desc.mGraphicsDesc = {};
        GraphicsPipelineDesc& smDepthPassPipelineDesc = desc.mGraphicsDesc;
        smDepthPassPipelineDesc.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        smDepthPassPipelineDesc.mRenderTargetCount = 1;
        smDepthPassPipelineDesc.pDepthState = &depthStateLEQUALEnabledDesc;
        smDepthPassPipelineDesc.mDepthStencilFormat = pRenderTargetShadowMap->mFormat;
        smDepthPassPipelineDesc.pColorFormats = &gShadowMapping.pShadowMaps[0]->mFormat;
        smDepthPassPipelineDesc.mSampleCount = gShadowMapping.pShadowMaps[0]->mSampleCount;
        smDepthPassPipelineDesc.mSampleQuality = gShadowMapping.pShadowMaps[0]->mSampleQuality;
        smDepthPassPipelineDesc.pRootSignature = gShadowMapping.pRootSignature;
        smDepthPassPipelineDesc.pRasterizerState = &rasterStateCullNoneDesc;
        smDepthPassPipelineDesc.pVertexLayout = &gSceneVertexLayoutPositionsOnly;
        smDepthPassPipelineDesc.pShaderProgram = gShadowMapping.pShaderDepth;
        addPipeline(pRenderer, &desc, &gShadowMapping.pPipelineDepth);

        smDepthPassPipelineDesc.pRootSignature = gShadowMapping.pRootSignatureAlpha;
        smDepthPassPipelineDesc.pVertexLayout = &gSceneVertexLayoutPosAndTex;
        smDepthPassPipelineDesc.pShaderProgram = gShadowMapping.pShaderDepthAlpha;
        addPipeline(pRenderer, &desc, &gShadowMapping.pPipelineDepthAlpha);
    }

    void removeShadowPipelines()
    {
        removePipeline(pRenderer, gShadowMapping.pPipelineDepth);
        removePipeline(pRenderer, gShadowMapping.pPipelineDepthAlpha);
    }

    void removePipelines()
    {
        removePipeline(pRenderer, pSkyBoxDrawPipeline);
        removePipeline(pRenderer, pForwardPipeline);
        removePipeline(pRenderer, pTerrainPipeline);
        removePipeline(pRenderer, pTransparentForwardPipeline);
        removePipeline(pRenderer, gGammaCorrectionData.pPipeline);

        if (gUseLightCulling)
        {
            removePipeline(pRenderer, pPipelineClearLightClusters);
            removePipeline(pRenderer, pPipelineClusterLights);
        }

        removePipeline(pRenderer, gCullData.pPipelines[CST_FRUSTUM_CULL_PASS]);

        if (kShadowMapCascadeCount > 0)
        {
            removeShadowPipelines();
        }

#ifdef BLUR_PIPELINE
        removePipeline(pRenderer, pPipelineBlur[PASS_TYPE_HORIZONTAL]);
        removePipeline(pRenderer, pPipelineBlur[PASS_TYPE_VERTICAL]);
#endif
#if defined(CPU_STRESS_TESTING_ENABLED)
        removePipeline(pRenderer, gCpuStressTestData.pPipeline);
#endif
    }

    void prepareDescriptorSets()
    {
        uint32_t numDescriptors = 0;
#define RESET_NUM_DESCRIPTORS numDescriptors = 0

        // Prepare descriptor sets
        DescriptorData threeParams[3] = {};
        threeParams[numDescriptors].pName = "skyboxTex";
        threeParams[numDescriptors++].ppTextures = &pSkyBoxTexture;
        updateDescriptorSet(pRenderer, 0, pDescriptorSetSkyboxTexture, numDescriptors, threeParams);

        RESET_NUM_DESCRIPTORS;
        threeParams[numDescriptors].pName = "uTex0";
        threeParams[numDescriptors++].ppTextures = &pIntermediateRenderTarget->pTexture;
        updateDescriptorSet(pRenderer, 0, gGammaCorrectionData.pSetTexture, numDescriptors, threeParams);

        RESET_NUM_DESCRIPTORS;
        threeParams[numDescriptors] = {};
        threeParams[numDescriptors].pName = "bounds";
        threeParams[numDescriptors++].ppBuffers = &gCullData.pBoundsBuffer;
        threeParams[numDescriptors].pName = "instanceBuffer";
        threeParams[numDescriptors++].ppBuffers = &gInstanceDataBuffer;
        updateDescriptorSet(pRenderer, 0, gCullData.pSetUpdateNone[CST_FRUSTUM_CULL_PASS], numDescriptors, threeParams);

#if defined(CPU_STRESS_TESTING_ENABLED)
        RESET_NUM_DESCRIPTORS;
        threeParams[numDescriptors] = {};
        threeParams[numDescriptors].pName = "uniformBlock";
        threeParams[numDescriptors++].ppBuffers = &gCpuStressTestData.pUniformBuffer;
        updateDescriptorSet(pRenderer, 0, gCpuStressTestData.pSet, numDescriptors, threeParams);
#endif

        for (uint32_t i = 0; i < gDataBufferCount; ++i)
        {
            //  Gamma Correction
            RESET_NUM_DESCRIPTORS;
            threeParams[numDescriptors] = {};
            threeParams[numDescriptors].pName = "uniformBlock";
            threeParams[numDescriptors++].ppBuffers = &gGammaCorrectionData.pGammaCorrectionBuffer[i];
            updateDescriptorSet(pRenderer, i, gGammaCorrectionData.pSetUniform, numDescriptors, threeParams);

            DescriptorData fourParams[4] = {};
            // Skybox
            RESET_NUM_DESCRIPTORS;
            fourParams[numDescriptors] = {};
            fourParams[numDescriptors].pName = "uniformBlock";
            fourParams[numDescriptors++].ppBuffers = &pSkyboxUniformBuffer[i];
            updateDescriptorSet(pRenderer, i, pDescriptorSetUniformsSkybox, numDescriptors, fourParams);

            // Scene
            RESET_NUM_DESCRIPTORS;
            fourParams[numDescriptors] = {};
            fourParams[numDescriptors].pName = "uniformBlock";
            fourParams[numDescriptors++].ppBuffers = &pProjViewUniformBuffer[i];

            fourParams[numDescriptors] = {};
            fourParams[numDescriptors].pName = "shadowCascadeBlock";
            fourParams[numDescriptors++].ppBuffers = &gBufferShadowCascades[i];

            if (gLightCullingEnabled)
            {
                fourParams[numDescriptors] = {};
                fourParams[numDescriptors].pName = "lightClustersCount";
                fourParams[numDescriptors++].ppBuffers = &pLightClustersCount;

                fourParams[numDescriptors] = {};
                fourParams[numDescriptors].pName = "lightClusters";
                fourParams[numDescriptors++].ppBuffers = &pLightClusters;
            }
            updateDescriptorSet(pRenderer, i, pDescriptorSetUniformsScene, numDescriptors, fourParams);

            {
                DescriptorData nineParams[10] = {};
                RESET_NUM_DESCRIPTORS;
                nineParams[numDescriptors].pName = "uniformBlock";
                nineParams[numDescriptors++].ppBuffers = &gCullData.pBufferUniformCull[i];
                nineParams[numDescriptors].pName = "indirectInstanceBuffer";
                nineParams[numDescriptors++].ppBuffers = &gIndirectInstanceDataBuffer[i];
                updateDescriptorSet(pRenderer, i, gCullData.pSetUpdatePerFrame[CST_FRUSTUM_CULL_PASS], numDescriptors, nineParams);
            }

            // Shadows
            RESET_NUM_DESCRIPTORS;
            fourParams[numDescriptors] = {};
            fourParams[numDescriptors].pName = "shadowCascadeBlock";
            fourParams[numDescriptors++].ppBuffers = &gBufferShadowCascades[i];
            if (gShadowMapping.pDescriptorSetUniformsCascades != NULL)
                updateDescriptorSet(pRenderer, i, gShadowMapping.pDescriptorSetUniformsCascades, numDescriptors, fourParams);
            if (gShadowMapping.pDescriptorSetAlphaUniforms != NULL)
                updateDescriptorSet(pRenderer, i, gShadowMapping.pDescriptorSetAlphaUniforms, numDescriptors, fourParams);

            // Light Cluster
            if (gLightCullingEnabled)
            {
                RESET_NUM_DESCRIPTORS;
                fourParams[numDescriptors] = {};
                fourParams[numDescriptors].pName = "uniformBlock";
                fourParams[numDescriptors++].ppBuffers = &pProjViewUniformBuffer[i];

                fourParams[numDescriptors] = {};
                fourParams[numDescriptors].pName = "lightClustersCount";
                fourParams[numDescriptors++].ppBuffers = &pLightClustersCount;

                fourParams[numDescriptors] = {};
                fourParams[numDescriptors].pName = "lightClusters";
                fourParams[numDescriptors++].ppBuffers = &pLightClusters;
                updateDescriptorSet(pRenderer, i, pDescriptorSetLightClusters, numDescriptors, fourParams);
            }
        }
        int32_t numOpaqueMeshes = gMeshTypesCount[MT_OPAQUE] + gMeshTypesCount[MT_TERRAIN];
        for (int32_t matIdx = 0; matIdx < (int32_t)gMaterialCount; ++matIdx)
        {
            DescriptorData matParams[20 + kShadowMapCascadeCount] = {};

            RESET_NUM_DESCRIPTORS;
            if (gMaterialsInfo[matIdx].mSetting.mType != MT_TERRAIN)
            {
                matParams[numDescriptors].pName = "diffuseMap";
                matParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pDiffuseMap;
                matParams[numDescriptors].pName = "normalMap";
                matParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pNormalMap;
                matParams[numDescriptors].pName = "specularMap";
                matParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pSpecularMap;
                matParams[numDescriptors].pName = "emissiveMap";
                matParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pEmissiveMap;

                matParams[numDescriptors].pName = "brdfLut";
                matParams[numDescriptors++].ppTextures = &pBrdfTexture;
            }
            else
            {
                matParams[numDescriptors].pName = "rocksTexture";
                matParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pDiffuseMap;
                matParams[numDescriptors].pName = "grassTexture";
                matParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pNormalMap;
                matParams[numDescriptors].pName = "rocksNormalTexture";
                matParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pSpecularMap;
                matParams[numDescriptors].pName = "grassNormalTexture";
                matParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pEmissiveMap;
            }

            matParams[numDescriptors].pName = "pbrMaterial";
            matParams[numDescriptors++].ppBuffers = &gMaterialsBuffer;

            matParams[numDescriptors].pName = "lightMap";
            matParams[numDescriptors++].ppTextures = &pBakedLightMap;

            matParams[numDescriptors].pName = "environmentMap";
            matParams[numDescriptors++].ppTextures = &pPrefilteredEnvTexture;

            matParams[numDescriptors].pName = "irradianceMap";
            matParams[numDescriptors++].ppTextures = &pIrradianceTexture;

            for (int32_t sci = 0; sci < kShadowMapCascadeCount; ++sci)
            {
                matParams[numDescriptors].pName = gShadowMapping.pCascadeTextureNames[sci];
                if (gUseRealTimeShadows)
                    matParams[numDescriptors++].ppTextures = &gShadowMapping.pShadowMaps[sci]->pTexture;
                else
                    matParams[numDescriptors++].ppTextures = &gShadowMapping.pShadowMapTextures[sci];
            }

            updateDescriptorSet(pRenderer, matIdx, pDescriptorSetMaterials, numDescriptors, matParams);
            updateDescriptorSet(pRenderer, matIdx, pDescriptorSetMaterialsTesting, numDescriptors, matParams);

            if (matIdx >= numOpaqueMeshes && gShadowMapping.pDescriptorSetAlphaTextures != NULL)
                updateDescriptorSet(pRenderer, matIdx - numOpaqueMeshes, gShadowMapping.pDescriptorSetAlphaTextures, 1, matParams);
        }
#undef RESET_NUM_DESCRIPTORS

        // Gaussian Blur
#ifdef BLUR_PIPELINE
        for (uint32_t sci = 0; sci < kShadowMapCascadeCount; ++sci)
        {
            Texture* texs[2] = { gShadowMapping.pShadowMaps[sci]->pTexture, pRenderTargetShaderMapBlur->pTexture };

            // Horizontal pass
            DescriptorData blurDescParams[3] = {};
            blurDescParams[0].pName = "srcTexture";
            blurDescParams[0].ppTextures = &texs[0];
            blurDescParams[1].pName = "dstTexture";
            blurDescParams[1].ppTextures = &texs[1];
            blurDescParams[2].pName = "BlurWeights";
            blurDescParams[2].ppBuffers = &pBufferBlurWeights;
            updateDescriptorSet(pRenderer, sci, pDescriptorSetBlurCompute[PASS_TYPE_HORIZONTAL], 3, blurDescParams);

            // Swap Textures for vertical pass
            blurDescParams[0].ppTextures = &texs[1];
            blurDescParams[1].ppTextures = &texs[0];
            updateDescriptorSet(pRenderer, sci, pDescriptorSetBlurCompute[PASS_TYPE_VERTICAL], 3, blurDescParams);
        }

#endif
    }

    // IBL
    void generateBrdfLut()
    {
        // This is fine as these are generated offline..
        // You can even implement this in external tool..
        if (pRenderer->mRendererApi == RENDERER_API_WEBGPU)
            return;

        Shader*        pShader = NULL;
        RootSignature* pRootSignature = NULL;
        RenderTarget*  pRenderTarget = NULL;
        Pipeline*      pPipeline = NULL;

        uint32_t width = 512, height = 512;

        ShaderLoadDesc brdfShader = {};
        brdfShader.mStages[0].pFileName = "fullscreen.vert";
        brdfShader.mStages[1].pFileName = "brdf.frag";
        addShader(pRenderer, &brdfShader, &pShader);

        RootSignatureDesc brdfRootSigDesc = {};
        brdfRootSigDesc.mStaticSamplerCount = 0;
        brdfRootSigDesc.ppStaticSamplerNames = NULL;
        brdfRootSigDesc.ppStaticSamplers = NULL;
        brdfRootSigDesc.mShaderCount = 1;
        brdfRootSigDesc.ppShaders = &pShader;
        addRootSignature(pRenderer, &brdfRootSigDesc, &pRootSignature);

        RenderTargetDesc brdfRTDesc = {};
        brdfRTDesc.mArraySize = 1;
        brdfRTDesc.mClearValue = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        brdfRTDesc.mDepth = 1;
        brdfRTDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
        brdfRTDesc.mFormat = TinyImageFormat_R16G16_UNORM;
        brdfRTDesc.mStartState = RESOURCE_STATE_RENDER_TARGET;
        brdfRTDesc.mHeight = height;
        brdfRTDesc.mWidth = width;
        brdfRTDesc.mSampleCount = SAMPLE_COUNT_1;
        brdfRTDesc.mSampleQuality = 0;
        brdfRTDesc.pName = "BRDF Render Target";
        addRenderTarget(pRenderer, &brdfRTDesc, &pRenderTarget);

        RasterizerStateDesc rasterStateCullNoneDesc = { CULL_MODE_NONE };

        PipelineDesc desc = {};
        desc.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettingsBrdf = desc.mGraphicsDesc;
        pipelineSettingsBrdf = { 0 };
        pipelineSettingsBrdf.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettingsBrdf.pRasterizerState = &rasterStateCullNoneDesc;
        pipelineSettingsBrdf.mRenderTargetCount = 1;
        pipelineSettingsBrdf.pColorFormats = &pRenderTarget->mFormat;
        pipelineSettingsBrdf.mSampleCount = pRenderTarget->mSampleCount;
        pipelineSettingsBrdf.mSampleQuality = pRenderTarget->mSampleQuality;
        pipelineSettingsBrdf.pRootSignature = pRootSignature;
        pipelineSettingsBrdf.pShaderProgram = pShader;
        desc.pName = "BRDF";
        addPipeline(pRenderer, &desc, &pPipeline);

        waitForAllResourceLoads();

        GpuCmdRingElement elem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);
        // Reset cmd pool for this frame
        resetCmdPool(pRenderer, elem.pCmdPool);
        //
        Cmd* cmd = elem.pCmds[0];
        beginCmd(cmd);

        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

        // simply record the screen cleaning command
        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount = 1;
        bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
        bindRenderTargets.mDepthStencil = { NULL, LOAD_ACTION_DONTCARE };
        cmdBindRenderTargets(cmd, &bindRenderTargets);

        cmdBindPipeline(cmd, pPipeline);
        cmdDraw(cmd, 3, 0);

        cmdBindRenderTargets(cmd, NULL);

        // Release the texture from graphics queue..
        TextureBarrier barrier = { pRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_RENDER_TARGET };
        barrier.mRelease = true;
        barrier.mQueueType = QUEUE_TYPE_GRAPHICS;
        cmdResourceBarrier(cmd, 0, NULL, 1, &barrier, 0, NULL);

        endCmd(cmd);

        FlushResourceUpdateDesc flushUpdateDesc = {};
        flushUpdateDesc.mNodeIndex = 0;
        flushResourceUpdates(&flushUpdateDesc);
        Semaphore* waitSemaphores[2] = { flushUpdateDesc.pOutSubmittedSemaphore, pImageAcquiredSemaphore };

        QueueSubmitDesc submitDesc = {};
        submitDesc.mCmdCount = 1;
        submitDesc.mSignalSemaphoreCount = 1;
        submitDesc.mWaitSemaphoreCount = TF_ARRAY_COUNT(waitSemaphores);
        submitDesc.ppCmds = &cmd;
        submitDesc.ppSignalSemaphores = &elem.pSemaphore;
        submitDesc.ppWaitSemaphores = waitSemaphores;
        submitDesc.pSignalFence = elem.pFence;
        queueSubmit(pGraphicsQueue, &submitDesc);

        // Stall CPU
        waitQueueIdle(pGraphicsQueue);

        const char pFileName[] = "brdf.tex";
        bool       bFileError = false;
        // File to write
        FileStream outFile = {};
        if (!fsOpenStreamFromPath(RD_TEXTURES, pFileName, FM_WRITE, &outFile))
        {
            LOGF(eERROR, "Could not open file '%s' for write.", pFileName);
            bFileError = true;
        }

        if (!bFileError)
        {
            Buffer*        pTextureBuffer = NULL;
            SyncToken      stTextureBuffer = 0;
            BufferLoadDesc bufferLoadDesc = {};
            bufferLoadDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER;
            bufferLoadDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_TO_CPU;
            bufferLoadDesc.mDesc.mSize = width * height * sizeof(float);
            bufferLoadDesc.mDesc.mStartState = RESOURCE_STATE_COPY_DEST;
            bufferLoadDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
            bufferLoadDesc.mDesc.mQueueType = QUEUE_TYPE_TRANSFER;
            bufferLoadDesc.ppBuffer = &pTextureBuffer;
            addResource(&bufferLoadDesc, &stTextureBuffer);
            waitForToken(&stTextureBuffer);

            SyncToken       stTextureCopy = 0;
            TextureCopyDesc copyDesc = {};
            copyDesc.pTexture = pRenderTarget->pTexture;
            copyDesc.pBuffer = pTextureBuffer;
            copyDesc.pWaitSemaphore = elem.pSemaphore;
            copyDesc.mTextureState = RESOURCE_STATE_RENDER_TARGET;
            // Barrier - Info to copy engine that the resource will use transfer queue to copy the texture...
            copyDesc.mQueueType = QUEUE_TYPE_TRANSFER;
            copyResource(&copyDesc, &stTextureCopy);
            waitForToken(&stTextureCopy);

            uint8_t* pTextureData = (uint8_t*)tf_malloc(bufferLoadDesc.mDesc.mSize);
            memcpy(pTextureData, pTextureBuffer->pCpuMappedAddress, bufferLoadDesc.mDesc.mSize);

            TinyKtx_WriteCallbacks ktxWriteCallbacks{ [](void*, char const* msg) { LOGF(eERROR, "%s", msg); },
                                                      [](void*, size_t size) { return tf_malloc(size); },
                                                      [](void*, void* memory) { tf_free(memory); },
                                                      [](void* user, const void* buffer, size_t byteCount)
                                                      { fsWriteToStream((FileStream*)user, buffer, (ssize_t)byteCount); } };

            uint32_t compressedDataSize[1] = { width };
            if (!TinyKtx_WriteImage(&ktxWriteCallbacks, &outFile, width, height, 1, 0, 1,
                                    TinyImageFormat_ToTinyKtxFormat(TinyImageFormat_R16G16_UNORM), false, compressedDataSize,
                                    (const void**)&pTextureData))
            {
                LOGF(eERROR, "Couldn't create ktx file '%s' with format '%s'", pFileName,
                     TinyImageFormat_Name(TinyImageFormat_R16G16_UNORM));
            }

            // Close out file stream
            fsCloseStream(&outFile);

            removeResource(pTextureBuffer);
            tf_free(pTextureData);
        }

        // Remove all resources
        removeShader(pRenderer, pShader);
        removeRootSignature(pRenderer, pRootSignature);
        removeRenderTarget(pRenderer, pRenderTarget);
        removePipeline(pRenderer, pPipeline);
    }

    void generateIBLCubeMaps(bool isIrradiance)
    {
        // Uses push constants, so will not be compatible with webgpu.
        // This is fine as these are generated offline..
        // You can even implement this in external tool..
        if (pRenderer->mRendererApi == RENDERER_API_WEBGPU)
            return;

        const int32_t  numFaces = 6; // max:6
        uint32_t       width = 512, height = 512;
        const uint32_t maxMipLevels = 10;
        uint32_t       numMipLevels = min(static_cast<uint32_t>(log2(width > height ? width : height)) - 2, maxMipLevels);

        struct UniformBlockIBL
        {
            mat4   mMvp;
            float2 mSettings; // x: Roughness, ywz: Padding..
        };

        mat4 cubeMatrices[] = { // +X
                                mat4::rotation(Quat::rotation(degToRad(180.0f), vec3(0.0f, 0.0f, 1.0f)) *
                                               Quat::rotation(degToRad(-90.0f), vec3(0.0f, 1.0f, 0.0f)) *
                                               Quat::rotation(degToRad(180.0f), vec3(1.0f, 0.0f, 0.0f))),
                                // -X
                                mat4::rotation(Quat::rotation(degToRad(180.0f), vec3(0.0f, 0.0f, 1.0f)) *
                                               Quat::rotation(degToRad(90.0f), vec3(0.0f, 1.0f, 0.0f)) *
                                               Quat::rotation(degToRad(180.0f), vec3(1.0f, 0.0f, 0.0f))),
                                // +Y
                                mat4::rotation(Quat::rotation(degToRad(90.0f), vec3(1.0f, 0.0f, 0.0f))),
                                // -Y
                                mat4::rotation(Quat::rotation(degToRad(-90.0f), vec3(1.0f, 0.0f, 0.0f))),
                                // +Z
                                mat4::rotation(Quat::rotation(degToRad(180.0f), vec3(0.0f, 0.0f, 1.0f)) *
                                               Quat::rotation(degToRad(180.0f), vec3(0.0f, 0.0f, 1.0f))),
                                // -Z
                                mat4::rotation(Quat::rotation(degToRad(180.0f), vec3(0.0f, 0.0f, 1.0f)) *
                                               Quat::rotation(degToRad(180.0f), vec3(1.0f, 0.0f, 0.0f)))
        };

        Shader*        pShader = NULL;
        RootSignature* pRootSignature = NULL;
        RenderTarget*  pRenderTargets[maxMipLevels][numFaces] = {}; // One for each cube map face..
        Pipeline*      pPipeline = NULL;
        DescriptorSet* pSet = NULL;

        UniformBlockIBL uniformData = {};

        ShaderLoadDesc cubeGenShader = {};
        cubeGenShader.mStages[0].pFileName = "iblCube.vert";
        if (isIrradiance)
        {
            cubeGenShader.mStages[1].pFileName = "irradiance.frag";
        }
        else
        {
            cubeGenShader.mStages[1].pFileName = "prefilteredEnv.frag";
        }
        addShader(pRenderer, &cubeGenShader, &pShader);

        RootSignatureDesc cubeGenRootSigDesc = {};
        const char*       pSampleSkyboxName[] = { "uSampler0" };
        cubeGenRootSigDesc.mStaticSamplerCount = 1;
        cubeGenRootSigDesc.ppStaticSamplerNames = pSampleSkyboxName;
        cubeGenRootSigDesc.ppStaticSamplers = &pSamplerSkyBox;
        cubeGenRootSigDesc.mShaderCount = 1;
        cubeGenRootSigDesc.ppShaders = &pShader;
        addRootSignature(pRenderer, &cubeGenRootSigDesc, &pRootSignature);

        uint32_t rootConstantIndex = getDescriptorIndexFromName(pRootSignature, "uRootConstants");

        TinyImageFormat renderTargetsFormat = TinyImageFormat_R16G16B16A16_SFLOAT;
        if (isIrradiance)
        {
            renderTargetsFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
        }
        RenderTargetDesc cubeGenRTDesc = {};
        cubeGenRTDesc.mArraySize = 1;
        cubeGenRTDesc.mClearValue = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        cubeGenRTDesc.mDepth = 1;
        cubeGenRTDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
        cubeGenRTDesc.mFormat = renderTargetsFormat;
        cubeGenRTDesc.mStartState = RESOURCE_STATE_RENDER_TARGET;
        cubeGenRTDesc.mHeight = height;
        cubeGenRTDesc.mWidth = width;
        cubeGenRTDesc.mSampleCount = SAMPLE_COUNT_1;
        cubeGenRTDesc.mSampleQuality = 0;
        cubeGenRTDesc.mMipLevels = 1;
        cubeGenRTDesc.pName = "Cube Gen Render Target";

        for (uint32_t m = 0; m < numMipLevels; ++m)
        {
            for (uint32_t i = 0; i < numFaces; ++i)
            {
                cubeGenRTDesc.mWidth = width >> m;
                cubeGenRTDesc.mHeight = height >> m;
                addRenderTarget(pRenderer, &cubeGenRTDesc, &pRenderTargets[m][i]);
            }
        }

        RasterizerStateDesc rasterStateCullNoneDesc = { CULL_MODE_NONE };

        PipelineDesc desc = {};
        desc.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettingsCubeGen = desc.mGraphicsDesc;
        pipelineSettingsCubeGen = { 0 };

        // layout and pipeline for skybox draw
        VertexLayout skyboxVertexLayout = {};
        skyboxVertexLayout.mBindingCount = 1;
        skyboxVertexLayout.mAttribCount = 1;
        skyboxVertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        skyboxVertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
        skyboxVertexLayout.mAttribs[0].mBinding = 0;
        skyboxVertexLayout.mAttribs[0].mLocation = 0;
        skyboxVertexLayout.mAttribs[0].mOffset = 0;
        pipelineSettingsCubeGen.pVertexLayout = &skyboxVertexLayout;

        pipelineSettingsCubeGen.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettingsCubeGen.pRasterizerState = &rasterStateCullNoneDesc;
        pipelineSettingsCubeGen.mRenderTargetCount = 1;
        pipelineSettingsCubeGen.pColorFormats = &pRenderTargets[0][0]->mFormat;
        pipelineSettingsCubeGen.mSampleCount = pRenderTargets[0][0]->mSampleCount;
        pipelineSettingsCubeGen.mSampleQuality = pRenderTargets[0][0]->mSampleQuality;
        pipelineSettingsCubeGen.pRootSignature = pRootSignature;
        pipelineSettingsCubeGen.pShaderProgram = pShader;
        desc.pName = "Cube Gen";
        addPipeline(pRenderer, &desc, &pPipeline);

        DescriptorSetDesc descriptorSetDesc = { pRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1 };
        addDescriptorSet(pRenderer, &descriptorSetDesc, &pSet);

        // Prepare descriptor sets
        DescriptorData params[1] = {};
        params[0].pName = "skyboxTex";
        params[0].ppTextures = &pSkyBoxTexture;
        updateDescriptorSet(pRenderer, 0, pSet, 1, params);

        waitForAllResourceLoads();

        if (isIrradiance)
        {
            uniformData.mSettings.x = (2.0f * float(PI)) / 180.0f;
            uniformData.mSettings.y = (0.5f * float(PI)) / 64.0f;
        }

        GpuCmdRingElement elem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);
        // Reset cmd pool for this frame
        resetCmdPool(pRenderer, elem.pCmdPool);
        //
        Cmd* cmd = elem.pCmds[0];
        beginCmd(cmd);

        const uint32_t skyboxVbStride = sizeof(float) * 4;
        for (uint32_t m = 0; m < numMipLevels; m++)
        {
            for (uint32_t i = 0; i < numFaces; ++i)
            {
                RenderTarget* pRenderTarget = pRenderTargets[m][i];

                cmdSetViewport(cmd, 0.0f, 0.0f, (float)(pRenderTarget->mWidth), (float)(pRenderTarget->mHeight), 0.0f, 1.0f);
                cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

                // simply record the screen cleaning command
                BindRenderTargetsDesc bindRenderTargets = {};
                bindRenderTargets.mRenderTargetCount = 1;
                bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
                bindRenderTargets.mDepthStencil = { NULL, LOAD_ACTION_DONTCARE };
                cmdBindRenderTargets(cmd, &bindRenderTargets);

                cmdBindPipeline(cmd, pPipeline);

                // update uniform data
                uniformData.mMvp = mat4::perspectiveLH(PI / 2.0f, 1.0f, 0.1f, 512.0f) * cubeMatrices[i];
                if (!isIrradiance)
                    uniformData.mSettings.x = (float)m / (float)(max(numMipLevels - 1, 1u));

                cmdBindPushConstants(cmd, pRootSignature, rootConstantIndex, &uniformData);

                cmdBindDescriptorSet(cmd, 0, pSet);
                cmdBindVertexBuffer(cmd, 1, &pSkyBoxVertexBuffer, &skyboxVbStride, NULL);
                cmdDrawInstanced(cmd, 36, 0, 1, i);

                cmdBindRenderTargets(cmd, NULL);

                // Release the texture from graphics queue..
                TextureBarrier barrier = { pRenderTarget->pTexture, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_RENDER_TARGET };
                barrier.mRelease = true;
                barrier.mQueueType = QUEUE_TYPE_GRAPHICS;
                cmdResourceBarrier(cmd, 0, NULL, 1, &barrier, 0, NULL);
            }
        }

        endCmd(cmd);

        FlushResourceUpdateDesc flushUpdateDesc = {};
        flushUpdateDesc.mNodeIndex = 0;
        flushResourceUpdates(&flushUpdateDesc);
        Semaphore* waitSemaphores[2] = { flushUpdateDesc.pOutSubmittedSemaphore, pImageAcquiredSemaphore };

        QueueSubmitDesc submitDesc = {};
        submitDesc.mCmdCount = 1;
        submitDesc.mSignalSemaphoreCount = 1;
        submitDesc.mWaitSemaphoreCount = TF_ARRAY_COUNT(waitSemaphores);
        submitDesc.ppCmds = &cmd;
        submitDesc.ppSignalSemaphores = &elem.pSemaphore;
        submitDesc.ppWaitSemaphores = waitSemaphores;
        submitDesc.pSignalFence = elem.pFence;
        queueSubmit(pGraphicsQueue, &submitDesc);

        // Max is 6
        const char* pFileNames[6] = { "xpos.ktx", "xneg.ktx", "ypos.ktx", "yneg.ktx", "zpos.ktx", "zneg.ktx" };

        uint64_t formatSize = (TinyImageFormat_BitSizeOfBlock(renderTargetsFormat) / 32) * sizeof(float);
        uint32_t dataBlockSize[maxMipLevels] = { 0 };
        uint8_t* pTextureData[maxMipLevels] = { NULL };

        Buffer*        pTextureBuffer = { NULL };
        SyncToken      stTextureBuffer = 0;
        BufferLoadDesc bufferLoadDesc = {};
        bufferLoadDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER;
        bufferLoadDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_TO_CPU;
        bufferLoadDesc.mDesc.mStartState = RESOURCE_STATE_COPY_DEST;
        bufferLoadDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
        bufferLoadDesc.mDesc.mQueueType = QUEUE_TYPE_TRANSFER;
        bufferLoadDesc.mDesc.mSize = formatSize * width * height;
        bufferLoadDesc.ppBuffer = &pTextureBuffer;
        addResource(&bufferLoadDesc, &stTextureBuffer);
        waitForToken(&stTextureBuffer);

        for (uint32_t m = 0; m < numMipLevels; ++m)
        {
            dataBlockSize[m] = (uint32_t)formatSize * (width >> m) * (height >> m);
            pTextureData[m] = (uint8_t*)tf_malloc(dataBlockSize[m]);
        }

        // Stall CPU
        waitQueueIdle(pGraphicsQueue);

        for (uint32_t i = 0; i < numFaces; ++i)
        {
            bool       bFileError = false;
            // File to write
            FileStream outFile = {};
            if (!fsOpenStreamFromPath(RD_TEXTURES, pFileNames[i], FM_WRITE, &outFile))
            {
                LOGF(eERROR, "Could not open file '%s' for write.", pFileNames[i]);
                bFileError = true;
            }

            if (!bFileError)
            {
                SyncToken       stTextureCopy = 0;
                TextureCopyDesc copyDesc = {};
                copyDesc.pWaitSemaphore = NULL;
                copyDesc.mTextureState = RESOURCE_STATE_RENDER_TARGET;
                // Barrier - Info to copy engine that the resource will use transfer queue to copy the texture...
                copyDesc.mQueueType = QUEUE_TYPE_TRANSFER;
                copyDesc.pBuffer = pTextureBuffer;

                for (uint32_t m = 0; m < numMipLevels; ++m)
                {
                    copyDesc.pTexture = pRenderTargets[m][i]->pTexture;
                    copyResource(&copyDesc, &stTextureCopy);
                    waitForToken(&stTextureCopy);

                    memset(pTextureData[m], 0, dataBlockSize[m]);
                    memcpy(pTextureData[m], pTextureBuffer->pCpuMappedAddress, dataBlockSize[m]);
                }

                TinyKtx_WriteCallbacks ktxWriteCallbacks{ [](void*, char const* msg) { LOGF(eERROR, "%s", msg); },
                                                          [](void*, size_t size) { return tf_malloc(size); },
                                                          [](void*, void* memory) { tf_free(memory); },
                                                          [](void* user, const void* buffer, size_t byteCount)
                                                          { fsWriteToStream((FileStream*)user, buffer, (ssize_t)byteCount); } };

                if (!TinyKtx_WriteImage(&ktxWriteCallbacks, &outFile, width, height, 1, 0, numMipLevels,
                                        TinyImageFormat_ToTinyKtxFormat(renderTargetsFormat), false, dataBlockSize,
                                        (const void**)&pTextureData[0]))
                {
                    LOGF(eERROR, "Couldn't create ktx file '%s' with format '%s'", pFileNames[i],
                         TinyImageFormat_Name(renderTargetsFormat));
                }

                // Close out file stream
                fsCloseStream(&outFile);
            }
        }

        removeResource(pTextureBuffer);
        for (uint32_t m = 0; m < numMipLevels; ++m)
        {
            tf_free(pTextureData[m]);
        }

        // Remove all resources
        removeDescriptorSet(pRenderer, pSet);
        removeShader(pRenderer, pShader);
        removeRootSignature(pRenderer, pRootSignature);
        for (uint32_t m = 0; m < numMipLevels; ++m)
        {
            for (uint32_t i = 0; i < numFaces; ++i)
            {
                removeRenderTarget(pRenderer, pRenderTargets[m][i]);
            }
        }
        removePipeline(pRenderer, pPipeline);
    }

    void bakeShadowMap(Cmd* pCmd)
    {
        UNREF_PARAM(pCmd);
#if defined(BAKE_SHADOW_MAPS)
        if (gShadowMapsReadyForBake && !gShadowMapsBaked)
        {
            gShadowMapsBaked = true;

            // Transfer buffer
            Buffer*        pTextureBuffer = { NULL };
            SyncToken      stTextureBuffer = 0;
            BufferLoadDesc bufferLoadDesc = {};
            uint32_t       formatSize =
                (uint32_t)(sizeof(float) * (float(TinyImageFormat_BitSizeOfBlock(gShadowMapping.pShadowMaps[0]->mFormat) / 8) / 4.0f));
            bufferLoadDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_BUFFER;
            bufferLoadDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_TO_CPU;
            bufferLoadDesc.mDesc.mStartState = RESOURCE_STATE_COPY_DEST;
            bufferLoadDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
            bufferLoadDesc.mDesc.mQueueType = QUEUE_TYPE_TRANSFER;
            bufferLoadDesc.mDesc.mSize = uint64_t(formatSize * kShadowMapResWidth * kShadowMapResHeight);
            bufferLoadDesc.ppBuffer = &pTextureBuffer;
            addResource(&bufferLoadDesc, &stTextureBuffer);
            waitForToken(&stTextureBuffer);

            // Copy textures from gpu to cpu and write to file..
            for (uint32_t i = 0; i < kShadowMapCascadeCount; ++i)
            {
                SyncToken       stTexCopyToken = 0;
                TextureCopyDesc desc = {};
                desc.mBufferOffset = 0;
                desc.mQueueType = QUEUE_TYPE_TRANSFER;
                desc.mTextureState = RESOURCE_STATE_COPY_SOURCE;
                desc.mTextureMipLevel = 0;
                desc.pBuffer = pTextureBuffer;
                desc.pTexture = gShadowMapping.pShadowMaps[i]->pTexture;
                copyResource(&desc, &stTexCopyToken);
                waitForToken(&stTexCopyToken);

                char pFileName[25] = {};
                snprintf(pFileName, 25, "SuntempleShadowMap%u", i);

                bool       bFileError = false;
                // File to write
                FileStream outFile = {};
                if (!fsOpenStreamFromPath(RD_TEXTURES, pFileName, FM_WRITE, &outFile))
                {
                    LOGF(eERROR, "Could not open file '%s' for write.", pFileName);
                    bFileError = true;
                }

                if (bFileError)
                {
                    continue;
                }

                TinyKtx_WriteCallbacks ktxWriteCallbacks{ [](void* user, char const* msg) { LOGF(eERROR, "%s", msg); },
                                                          [](void* user, size_t size) { return tf_malloc(size); },
                                                          [](void* user, void* memory) { tf_free(memory); },
                                                          [](void* user, const void* buffer, size_t byteCount)
                                                          { fsWriteToStream((FileStream*)user, buffer, (ssize_t)byteCount); } };

                uint32_t compressedDataSize[1] = { (uint32_t)bufferLoadDesc.mDesc.mSize };
                if (!TinyKtx_WriteImage(&ktxWriteCallbacks, &outFile, kShadowMapResWidth, kShadowMapResHeight, 1, 0, 1,
                                        TinyImageFormat_ToTinyKtxFormat(gShadowMapping.pShadowMaps[0]->mFormat), false, compressedDataSize,
                                        (const void**)&pTextureBuffer->pCpuMappedAddress))
                {
                    LOGF(eERROR, "Couldn't create ktx file '%s' with format '%s'", pFileName,
                         TinyImageFormat_Name(gShadowMapping.pShadowMaps[0]->mFormat));
                }

                // Close out file stream
                fsCloseStream(&outFile);
            }

            TextureBarrier texBarriers[kShadowMapCascadeCount];
            for (uint32_t i = 0; i < kShadowMapCascadeCount; ++i)
            {
                texBarriers[i] = { gShadowMapping.pShadowMaps[i]->pTexture, RESOURCE_STATE_COPY_SOURCE,
                                   RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
                texBarriers[i].mAcquire = true;
                texBarriers[i].mQueueType = QUEUE_TYPE_GRAPHICS;
            }
            cmdResourceBarrier(pCmd, 0, NULL, kShadowMapCascadeCount, texBarriers, 0, NULL);

            removeResource(pTextureBuffer);
        }
#endif
    }

    void addGui()
    {
        if (pRenderer->mRendererApi != RENDERER_API_WEBGPU)
        {
            DropdownWidget rtFormatWidgetDesc;
            rtFormatWidgetDesc.mCount = gNumRenderTargetFormats;
            rtFormatWidgetDesc.pNames = gRenderTargetFormatNames;
            rtFormatWidgetDesc.pData = &gRenderTargetFormatWidgetData;
            UIWidget* pRenderTargetFormatSelectionWidget =
                uiCreateComponentWidget(pGuiWindow, "Render Target Format", &rtFormatWidgetDesc, WIDGET_TYPE_DROPDOWN);
            pRenderTargetFormatSelectionWidget->pOnEdited = [](void*)
            {
                ReloadDesc reloadDesc{ RELOAD_TYPE_RENDERTARGET };
                requestReload(&reloadDesc);
            };
        }

        DropdownWidget viewPositionsWidget;
        viewPositionsWidget.mCount = kNumViewPositions;
        viewPositionsWidget.pNames = gViewPositionNames;
        viewPositionsWidget.pData = &gViewPoistionsWidgetData;
        luaRegisterWidget(uiCreateComponentWidget(pGuiWindow, "Select View Position", &viewPositionsWidget, WIDGET_TYPE_DROPDOWN));

        SliderFloatWidget exposureWidget;
        exposureWidget.pData = &gGammaCorrectionData.mGammaCorrectionUniformData.mGammaCorrectionData.y;
        exposureWidget.mMin = 0.0f;
        exposureWidget.mMax = 5.0f;
        exposureWidget.mStep = 0.00001f;
        luaRegisterWidget(uiCreateComponentWidget(pGuiWindow, "Exposure", &exposureWidget, WIDGET_TYPE_SLIDER_FLOAT));

        CheckboxWidget checkbox;
        checkbox.pData = &gCameraWalkData.mIsWalking;
        luaRegisterWidget(uiCreateComponentWidget(pGuiWindow, "Cinematic Camera Walking", &checkbox, WIDGET_TYPE_CHECKBOX));

        SliderFloatWidget cameraSpeedProp;
        cameraSpeedProp.pData = &gCameraWalkData.mWalkSpeed;
        cameraSpeedProp.mMin = 0.0f;
        cameraSpeedProp.mMax = 3.0f;
        luaRegisterWidget(uiCreateComponentWidget(pGuiWindow, "Cinematic Camera Speed", &cameraSpeedProp, WIDGET_TYPE_SLIDER_FLOAT));

        CheckboxWidget frustumCullCheckbox;
        frustumCullCheckbox.pData = &gUseFrustumCulling;
        UIWidget* pFrustumCullWidget = uiCreateComponentWidget(pGuiWindow, "Frustum Culling", &frustumCullCheckbox, WIDGET_TYPE_CHECKBOX);
        pFrustumCullWidget->pOnEdited = [](void*)
        {
            if (cpuIsTestRunning())
            {
                gUseFrustumCulling = !gUseFrustumCulling;
            }
        };

        CheckboxWidget lightCullCheckbox;
        lightCullCheckbox.pData = &gLightCullingEnabled;
        UIWidget* pLightCullWidget = NULL;
        luaRegisterWidget(pLightCullWidget =
                              uiCreateComponentWidget(pGuiWindow, "Light Culling", &lightCullCheckbox, WIDGET_TYPE_CHECKBOX));
        pLightCullWidget->pOnEdited = [](void*)
        {
            ReloadDesc reloadDesc{ RELOAD_TYPE_SHADER };
            requestReload(&reloadDesc);
        };

        CheckboxWidget realtimeShadowsCheckbox;
        realtimeShadowsCheckbox.pData = &gRealTimeShadowsEnabled;
        UIWidget* pRealTimeShadowsCheckboxWidget = NULL;
        luaRegisterWidget(pRealTimeShadowsCheckboxWidget =
                              uiCreateComponentWidget(pGuiWindow, "Real Time Shadows", &realtimeShadowsCheckbox, WIDGET_TYPE_CHECKBOX));
        pRealTimeShadowsCheckboxWidget->pOnEdited = [](void*)
        {
            ReloadDesc reloadDesc{ RELOAD_TYPE_SHADER };
            requestReload(&reloadDesc);
        };
        if (gUseRealTimeShadows)
        {
            SliderFloat3Widget sunX;
            sunX.pData = &gLightCpuSettings.mSunControl;
            sunX.mMin = float3(-1000);
            sunX.mMax = float3(1000);
            sunX.mStep = float3(0.00001f);
            uiCreateComponentWidget(pGuiWindow, "Sun Control", &sunX, WIDGET_TYPE_SLIDER_FLOAT3);
        }

#if defined(CPU_STRESS_TESTING_ENABLED)
        DropdownWidget cpuStressTestTypesWidget;
        cpuStressTestTypesWidget.mCount = CSTT_ALL + 1;
        cpuStressTestTypesWidget.pNames = gCpuStressTestData.kTypeStrings;
        cpuStressTestTypesWidget.pData = &gCpuStressTestData.mTypeWidgetData;
        UIWidget* pCpuStressTestTypesWidget = NULL;
        pCpuStressTestTypesWidget =
            uiCreateComponentWidget(pCpuStressTestWindow, "CPU Stress Test Type", &cpuStressTestTypesWidget, WIDGET_TYPE_DROPDOWN);
        pCpuStressTestTypesWidget->pOnEdited = [](void* pUserData)
        {
            if (cpuIsTestRunning() && gCpuStressTestData.mTypeWidgetData == CSTT_ALL)
            {
                gCpuStressTestData.mTypeWidgetData = gCpuStressTestData.mNewTestType;
            }
            else
            {
                gCpuStressTestData.mNewTestType = gCpuStressTestData.mTypeWidgetData;
            }
        };

        ButtonWidget cpuStartStressTestButton;
        UIWidget*    pCPUStartStressTestButton =
            uiCreateComponentWidget(pCpuStressTestWindow, "Toggle CPU Test", &cpuStartStressTestButton, WIDGET_TYPE_BUTTON);
        pCPUStartStressTestButton->pOnEdited = cpuToggleStressTest;

        ButtonWidget cpuStressTestDataToFileButton;
        UIWidget*    pCPUStressTestDataToFileButton =
            uiCreateComponentWidget(pCpuStressTestWindow, "Save Graph", &cpuStressTestDataToFileButton, WIDGET_TYPE_BUTTON);
        pCPUStressTestDataToFileButton->pOnEdited = cpuSaveGraphStressTestData;
#endif
    }
};

#if defined(CPU_STRESS_TESTING_ENABLED)

float                GetCpuSampleTime(const CPUStressTestSample& sample, uint32_t testTypeIdx) { return sample.mTime; }
uint32_t             GetCpuSampleIdx() { return gCpuStressTestData.mCurrentSample; }
CPUStressTestSample& GetCpuSampleAt(uint32_t testTypeIdx)
{
    return gCpuStressTests[testTypeIdx].mSamples[GetCpuApiDataIndex()][gCpuStressTestData.mCurrentTest][GetCpuSampleIdx()];
}
CPUStressTestSample& GetCpuSampleAt(uint32_t apiIdx, uint32_t testTypeIdx, uint32_t testIdx, uint32_t sampleIdx)
{
    return gCpuStressTests[testTypeIdx].mSamples[apiIdx][testIdx][sampleIdx];
}
ProfileToken& GetProfileTokenAt(uint32_t testTypeIdx) { return gCpuStressTests[testTypeIdx].mToken; }

void cpuStressTestUpdate(float deltaTime)
{
    // Update test if samples for current have been met..
    if (gCpuStressTestData.bIsTestRunning && gCpuStressTestData.mCurrentSample == kNumCpuStressTestSamples)
    {
        gCpuStressTestData.mCurrentSample = 0u; // Reset Sample
        gCpuStressTestData.mCurrentTest++;      // Go to next test

        if (gCpuStressTestData.mCurrentTest == kNumCpuStressTests)
        {
            if (gCpuStressTestData.mTypeWidgetData == CSTT_ALL)
            {
                gCpuStressTestData.mCurrentTestType++;
            }

            gCpuStressTestData.mCurrentTest = 0; // Reset Test
        }

        if (gCpuStressTestData.mCurrentTestType == CSTT_ALL)
        {
            // We need to reload..
            if (!gCpuStressTestData.bAlreadyReloaded)
            {
                // Swap APIs
                RendererApi newAPI = pRenderer->mRendererApi == RENDERER_API_WEBGPU ? RENDERER_API_VULKAN : RENDERER_API_WEBGPU;

#if defined(ANDROID)
                extern RendererApi gRendererApis[RENDERER_API_COUNT];
                for (uint32_t i = 0; i < RENDERER_API_COUNT; ++i)
                    if (newAPI == gRendererApis[i])
                        gSelectedApiIndex = i;
#elif defined(_WINDOWS)
                RendererApi pAPIs[4];
                uint32_t    numApis = 0;
#if defined(DIRECT3D12)
                pAPIs[numApis++] = RENDERER_API_D3D12;
#endif
#if defined(VULKAN)
                pAPIs[numApis++] = RENDERER_API_VULKAN;
#endif
#if defined(WEBGPU)
                pAPIs[numApis++] = RENDERER_API_WEBGPU;
#endif
#if defined(DIRECT3D11)
                pAPIs[numApis++] = RENDERER_API_D3D11;
#endif

                for (uint32_t i = 0; i < numApis; ++i)
                    if (newAPI == pAPIs[i])
                        gSelectedApiIndex = i;
#endif

                // Queue reset
                ResetDesc resetDesc{ RESET_TYPE_API_SWITCH };
                requestReset(&resetDesc);

                gCpuStressTestData.bAlreadyReloaded = true; // Do not want to loop resets...
                cpuToggleStressTest(NULL);
                gCpuStressTestData.bWasTestRunning = true;
            }
            else
            {
                cpuSaveGraphStressTestData(NULL); // Both APIs should have finished their tests...

                gCpuStressTestData.bAlreadyReloaded = false;
                if (cpuIsTestRunning())
                    cpuToggleStressTest(NULL);

                uiSetComponentActive(pCpuStressTestWindow, true);
            }
        }
    }

    if (gCpuStressTestData.bShouldStartTest)
    {
        gCpuStressTestData.bIsTestRunning = true;
        gCpuStressTestData.bShouldStartTest = false;

        gUseFrustumCulling = false;
        gUseLightCulling = false;
        gUseRealTimeShadows = false;

        if (gCpuStressTestData.mTypeWidgetData == CSTT_ALL)
        {
            gCpuStressTestData.mCurrentTestType = 0u;
            gCpuStressTestData.mCurrentSample = 0u;
            gCpuStressTestData.mCurrentTest = 0u;

            uiSetComponentActive(pCpuStressTestWindow, false);
        }
    }

    if (gCpuStressTestData.bIsTestRunning)
    {
        if (gCpuStressTestData.mTypeWidgetData != CSTT_ALL)
            gCpuStressTestData.mCurrentTestType = gCpuStressTestData.mNewTestType;
    }

    if (gCpuStressTestData.bIsTestRunning)
    {
        float progress = float((gCpuStressTestData.mCurrentTest * kNumCpuStressTestSamples) + gCpuStressTestData.mCurrentSample) /
                         float(kNumCpuStressTests * kNumCpuStressTestSamples);
        snprintf(gCpuStressTestStr[0], 64, "%s Test:", gCpuStressTestData.kTypeStrings[gCpuStressTestData.mCurrentTestType]);
        snprintf(gCpuStressTestStr[1], 64, "     Test   #: %u", gCpuStressTestData.mCurrentTest);
        snprintf(gCpuStressTestStr[2], 64, "     Sample #: %u", gCpuStressTestData.mCurrentSample);
        snprintf(gCpuStressTestStr[3], 64, "     Progress: %f", progress);
    }
}

void cpuStressTestSubmit(GpuCmdRingElement* pElem)
{
    FlushResourceUpdateDesc flushUpdateDesc = {};
    flushUpdateDesc.mNodeIndex = 0;
    flushResourceUpdates(&flushUpdateDesc);
    Semaphore* waitSemaphores[] = { flushUpdateDesc.pOutSubmittedSemaphore, pImageAcquiredSemaphore };

    QueueSubmitDesc submitDesc = {};
    submitDesc.mCmdCount = 1;
    submitDesc.mSignalSemaphoreCount = 1;
    submitDesc.mWaitSemaphoreCount = TF_ARRAY_COUNT(waitSemaphores);
    submitDesc.ppCmds = &pElem->pCmds[0];
    submitDesc.ppSignalSemaphores = &pElem->pSemaphore;
    submitDesc.ppWaitSemaphores = waitSemaphores;
    submitDesc.pSignalFence = pElem->pFence;

    queueSubmit(pGraphicsQueue, &submitDesc);
}
void cpuStressTestDrawUI(Cmd* pSubmissionCmd, RenderTarget* pRenderTargetSwapchain)
{
    cmdBindRenderTargets(pSubmissionCmd, NULL);
    RenderTargetBarrier rtSwapchainBarrier = { pRenderTargetSwapchain, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET };
    cmdResourceBarrier(pSubmissionCmd, 0, NULL, 0, NULL, 1, &rtSwapchainBarrier);

    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = { pRenderTargetSwapchain, LOAD_ACTION_CLEAR };
    bindRenderTargets.mDepthStencil = { NULL, LOAD_ACTION_DONTCARE };
    cmdBindRenderTargets(pSubmissionCmd, &bindRenderTargets);

    cmdSetViewport(pSubmissionCmd, 0.0f, 0.0f, (float)pRenderTargetSwapchain->mWidth, (float)pRenderTargetSwapchain->mHeight, 0.0f, 1.0f);
    cmdSetScissor(pSubmissionCmd, 0, 0, pRenderTargetSwapchain->mWidth, pRenderTargetSwapchain->mHeight);

    // Draw UI, Allows for us to see realtime benchmarks...
    gFrameTimeDraw.mFontColor = 0xff00ffff;
    gFrameTimeDraw.mFontSize = 12.5f;
    gFrameTimeDraw.mFontID = gFontID;
    float2 txtSizePx = cmdDrawCpuProfile(pSubmissionCmd, float2(8.f, 15.f), &gFrameTimeDraw);
    cmdDrawGpuProfile(pSubmissionCmd, float2(8.f, txtSizePx.y + 75.0f), gGraphicsProfileToken, &gFrameTimeDraw);

    for (uint32_t i = 0; i < 4; ++i)
    {
        gFrameTimeDraw.pText = (const char*)gCpuStressTestStr[i];
        cmdDrawTextWithFont(pSubmissionCmd, float2(8.f, txtSizePx.y + 275.0f + 30 * (i + 1)), &gFrameTimeDraw);
    }

    cmdDrawUserInterface(pSubmissionCmd);
    cmdBindRenderTargets(pSubmissionCmd, NULL);

    rtSwapchainBarrier = { pRenderTargetSwapchain, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
    cmdResourceBarrier(pSubmissionCmd, 0, NULL, 0, NULL, 1, &rtSwapchainBarrier);
}
void cpuStressTestDrawAndSubmitDefaultSwapchainRT(GpuCmdRingElement* pElem, RenderTarget* pRenderTargetSwapchain)
{
    // Draw UI to swapchain image..
    Cmd* submissionCmd = pElem->pCmds[0];
    beginCmd(submissionCmd);
    cpuStressTestDrawUI(submissionCmd, pRenderTargetSwapchain);
    endCmd(submissionCmd);
    cpuStressTestSubmit(pElem);
}
void cpuStressTestCommandsEncoding(GpuCmdRingElement* pElem, RenderTarget* pRenderTargetSwapchain)
{
    if (!cpuIsValidTest(CSTT_COMMAND_ENCODING))
        return;

    ProfileToken& cToken = GetProfileTokenAt(CSTT_COMMAND_ENCODING); // 0: Commands Encoding..
    cpuProfileEnter(cToken);

    resetHiresTimer(&gCpuStressTestData.mTimer);
    Cmd* testOnlyCmd = pElem->pCmds[1];
    beginCmd(testOnlyCmd);

    RenderTarget*       pRenderTarget = pIntermediateRenderTarget;
    RenderTargetBarrier barrier = { pRenderTarget, RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET };
    cmdResourceBarrier(testOnlyCmd, 0, NULL, 0, NULL, 1, &barrier);

    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = { pIntermediateRenderTarget, LOAD_ACTION_CLEAR };
    bindRenderTargets.mDepthStencil = { pDepthBuffer, LOAD_ACTION_CLEAR };
    cmdBindRenderTargets(testOnlyCmd, &bindRenderTargets);
    cmdSetViewport(testOnlyCmd, 0.0f, 0.0f, (float)pIntermediateRenderTarget->mWidth, (float)pIntermediateRenderTarget->mHeight, 0.0f,
                   1.0f);
    cmdSetScissor(testOnlyCmd, 0, 0, pIntermediateRenderTarget->mWidth, pIntermediateRenderTarget->mHeight);

    // Only Opaque Pass test as all we want is the time it is taking for command to encoded..
    cmdBindPipeline(testOnlyCmd, pForwardPipeline);
    cmdBindDescriptorSet(testOnlyCmd, gFrameIndex, pDescriptorSetUniformsScene);
    cmdBindIndexBuffer(testOnlyCmd, pScene->pGeom->pIndexBuffer, pScene->pGeom->mIndexType, 0);
    cmdBindVertexBuffer(testOnlyCmd, 4, pScene->pGeom->pVertexBuffers, pScene->pGeom->mVertexStrides, NULL);

    uint32_t totalDrawCalls = kNumCpuStressTestDrawCallIncrements * (gCpuStressTestData.mCurrentTest + 1);
    for (uint32_t j = 0; j < totalDrawCalls; ++j)
    {
        uint32_t                          dci = j % gMeshTypesCount[MT_OPAQUE];
        const IndirectDrawIndexArguments& args = pScene->pGeom->pDrawArgs[dci];
        cmdBindDescriptorSet(testOnlyCmd, dci, pDescriptorSetMaterials);
        cmdDrawIndexed(testOnlyCmd, args.mIndexCount, 0, 0);
    }
    endCmd(testOnlyCmd);

    cpuProfileLeave(cToken, gFrameCount);

    int64_t time = 0u;
    time += getHiresTimerUSec(&gCpuStressTestData.mTimer, true);

    CPUStressTestSample& cSample = GetCpuSampleAt(CSTT_COMMAND_ENCODING);
    cSample.mTime = float(time) / 1000.0f;
    cSample.mCount = totalDrawCalls + 8; // + 8 for pipeline binding..etc.. commands..
    gCpuStressTests[CSTT_COMMAND_ENCODING].mTotalSamplesTaken[GetCpuApiDataIndex()]++;

    cpuStressTestDrawAndSubmitDefaultSwapchainRT(pElem, pRenderTargetSwapchain);
}
void cpuStressTestCommandsSubmission(GpuCmdRingElement* pElem, RenderTarget* pRenderTargetSwapchain)
{
    if (!cpuIsValidTest(CSTT_COMMAND_SUBMISSION))
    {
        return;
    }

    Cmd* testOnlyCmd = pElem->pCmds[1]; // To be submitted...
    beginCmd(testOnlyCmd);

    RenderTarget*       pRenderTarget = pIntermediateRenderTarget;
    RenderTargetBarrier barrier = { pRenderTarget, RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET };
    cmdResourceBarrier(testOnlyCmd, 0, NULL, 0, NULL, 1, &barrier);

    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = { pIntermediateRenderTarget, LOAD_ACTION_CLEAR };
    bindRenderTargets.mDepthStencil = { pDepthBuffer, LOAD_ACTION_CLEAR };
    cmdBindRenderTargets(testOnlyCmd, &bindRenderTargets);
    cmdSetViewport(testOnlyCmd, 0.0f, 0.0f, (float)pIntermediateRenderTarget->mWidth, (float)pIntermediateRenderTarget->mHeight, 0.0f,
                   1.0f);
    cmdSetScissor(testOnlyCmd, 0, 0, pIntermediateRenderTarget->mWidth, pIntermediateRenderTarget->mHeight);

    // Only Opaque Pass test as all we want is the time it is taking for command to encoded..
    cmdBindPipeline(testOnlyCmd, pForwardPipeline);
    cmdBindDescriptorSet(testOnlyCmd, gFrameIndex, pDescriptorSetUniformsScene);
    cmdBindDescriptorSet(testOnlyCmd, 0, pDescriptorSetMaterials); // Bind first material (Random)
    cmdBindIndexBuffer(testOnlyCmd, pScene->pGeom->pIndexBuffer, pScene->pGeom->mIndexType, 0);
    cmdBindVertexBuffer(testOnlyCmd, 4, pScene->pGeom->pVertexBuffers, pScene->pGeom->mVertexStrides, NULL);

    uint32_t totalDrawCalls = kNumCpuStressTestDrawCallIncrements * (gCpuStressTestData.mCurrentTest + 1);
    for (uint32_t j = 0; j < totalDrawCalls; ++j)
    {
        uint32_t                          dci = j % gMeshTypesCount[MT_OPAQUE];
        const IndirectDrawIndexArguments& args = pScene->pGeom->pDrawArgs[dci];
        cmdBindDescriptorSet(testOnlyCmd, dci, pDescriptorSetMaterials);
        cmdDrawIndexed(testOnlyCmd, args.mIndexCount, 0, 0);
    }

    cmdBindRenderTargets(testOnlyCmd, NULL);
    barrier = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
    cmdResourceBarrier(testOnlyCmd, 0, NULL, 0, NULL, 1, &barrier);
    endCmd(testOnlyCmd);

    {
        // Do not need to wait for fence to be signaled..
        // Just cannot use pElem->pFence...
        /*FenceStatus fenceStatus;
        getFenceStatus(pRenderer, gCpuStressTestData.pSubmissionFence, &fenceStatus);
        if (fenceStatus == FENCE_STATUS_INCOMPLETE)
            waitForFences(pRenderer, 1, &gCpuStressTestData.pSubmissionFence);*/

        Semaphore*      waitSemaphores[] = { pImageAcquiredSemaphore };
        QueueSubmitDesc submitDesc = {};
        submitDesc.mCmdCount = 1;
        submitDesc.mSignalSemaphoreCount = 0;
        submitDesc.mWaitSemaphoreCount = 1;
        submitDesc.ppCmds = &testOnlyCmd;
        submitDesc.ppWaitSemaphores = waitSemaphores;
        submitDesc.pSignalFence = gCpuStressTestData.pSubmissionFence;

        ProfileToken& cToken = GetProfileTokenAt(CSTT_COMMAND_SUBMISSION);
        cpuProfileEnter(cToken);
        resetHiresTimer(&gCpuStressTestData.mTimer);
        queueSubmit(pGraphicsQueue, &submitDesc);
        cpuProfileLeave(cToken, gFrameCount);
    }

    CPUStressTestSample& cSample = GetCpuSampleAt(CSTT_COMMAND_SUBMISSION);
    cSample.mTime = float(getHiresTimerUSec(&gCpuStressTestData.mTimer, true)) / 1000.0f;
    cSample.mCount = totalDrawCalls;
    gCpuStressTests[CSTT_COMMAND_SUBMISSION].mTotalSamplesTaken[GetCpuApiDataIndex()]++;

    cpuStressTestDrawAndSubmitDefaultSwapchainRT(pElem, pRenderTargetSwapchain);
}
void cpuStressTestBindGroupUpdates(GpuCmdRingElement* pElem, RenderTarget* pRenderTargetSwapchain)
{
    if (!cpuIsValidTest(CSTT_BIND_GROUP_UPDATES))
        return;

    int64_t  time = 0;
    uint32_t totalUpdates = kNumCpuStressTestDrawCallIncrements * (gCpuStressTestData.mCurrentTest + 1);
    waitQueueIdle(pGraphicsQueue);

    ProfileToken& cToken = GetProfileTokenAt(CSTT_BIND_GROUP_UPDATES); // 2: Bind goup updates..
    cpuProfileEnter(cToken);

    uint32_t numDescriptors = 0;
#define RESET_NUM_DESCRIPTORS numDescriptors = 0

    for (uint32_t i = 0; i < totalUpdates; ++i)
    {
        uint32_t matIdx = i % gMaterialCount;

        RESET_NUM_DESCRIPTORS;
        DescriptorData testParams[9 + kShadowMapCascadeCount] = {};
        testParams[numDescriptors].pName = "diffuseMap";
        testParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pDiffuseMap;
        testParams[numDescriptors].pName = "normalMap";
        testParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pNormalMap;
        testParams[numDescriptors].pName = "specularMap";
        testParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pSpecularMap;
        testParams[numDescriptors].pName = "emissiveMap";
        testParams[numDescriptors++].ppTextures = &gMaterialsInfo[matIdx].pEmissiveMap;
        testParams[numDescriptors].pName = "brdfLut";
        testParams[numDescriptors++].ppTextures = &pBrdfTexture;
        testParams[numDescriptors].pName = "pbrMaterial";
        testParams[numDescriptors++].ppBuffers = &gMaterialsBuffer;
        testParams[numDescriptors].pName = "lightMap";
        testParams[numDescriptors++].ppTextures = &pBakedLightMap;

        testParams[numDescriptors].pName = "environmentMap";
        testParams[numDescriptors++].ppTextures = &pPrefilteredEnvTexture;

        testParams[numDescriptors].pName = "irradianceMap";
        testParams[numDescriptors++].ppTextures = &pIrradianceTexture;

        for (int32_t sci = 0; sci < kShadowMapCascadeCount; ++sci)
        {
            testParams[numDescriptors].pName = gShadowMapping.pCascadeTextureNames[sci];
            if (gUseRealTimeShadows)
                testParams[numDescriptors++].ppTextures = &gShadowMapping.pShadowMaps[sci]->pTexture;
            else
                testParams[numDescriptors++].ppTextures = &gShadowMapping.pShadowMapTextures[sci];
        }

        resetHiresTimer(&gCpuStressTestData.mTimer);
        updateDescriptorSet(pRenderer, matIdx, pDescriptorSetMaterialsTesting, numDescriptors, testParams);
        time += getHiresTimerUSec(&gCpuStressTestData.mTimer, true);
    }

    cpuProfileLeave(cToken, gFrameCount);

    CPUStressTestSample& cSample = GetCpuSampleAt(CSTT_BIND_GROUP_UPDATES); // 2: Bind goup updates..
    cSample.mCount = totalUpdates;
    cSample.mTime = float(time) / 1000.0f;
    gCpuStressTests[CSTT_BIND_GROUP_UPDATES].mTotalSamplesTaken[GetCpuApiDataIndex()]++;

    cpuStressTestDrawAndSubmitDefaultSwapchainRT(pElem, pRenderTargetSwapchain);
}
void cpuStressTestBindGroupBindings(GpuCmdRingElement* pElem, RenderTarget* pRenderTargetSwapchain)
{
    if (!cpuIsValidTest(CSTT_BIND_GROUP_BINDINGS))
        return;

    int64_t  time = 0;
    uint32_t totalUpdates = kNumCpuStressTestDrawCallIncrements * (gCpuStressTestData.mCurrentTest + 1);

    Cmd* testOnlyCmd = pElem->pCmds[1];
    beginCmd(testOnlyCmd);

    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = { pIntermediateRenderTarget, LOAD_ACTION_CLEAR };
    bindRenderTargets.mDepthStencil = { pDepthBuffer, LOAD_ACTION_CLEAR };
    cmdBindRenderTargets(testOnlyCmd, &bindRenderTargets);
    cmdSetViewport(testOnlyCmd, 0.0f, 0.0f, (float)pIntermediateRenderTarget->mWidth, (float)pIntermediateRenderTarget->mHeight, 0.0f,
                   1.0f);
    cmdSetScissor(testOnlyCmd, 0, 0, pIntermediateRenderTarget->mWidth, pIntermediateRenderTarget->mHeight);
    // Bind pipeline..
    cmdBindPipeline(testOnlyCmd, pForwardPipeline);

    ProfileToken& cToken = GetProfileTokenAt(CSTT_BIND_GROUP_BINDINGS);
    cpuProfileEnter(cToken);

    for (uint32_t i = 0; i < totalUpdates; ++i)
    {
        uint32_t dci = i % gMeshTypesCount[MT_OPAQUE];
        resetHiresTimer(&gCpuStressTestData.mTimer);
        cmdBindDescriptorSet(testOnlyCmd, dci, pDescriptorSetMaterials);
        time += getHiresTimerUSec(&gCpuStressTestData.mTimer, true);
    }

    cpuProfileLeave(cToken, gFrameCount);
    endCmd(testOnlyCmd);

    CPUStressTestSample& cSample = GetCpuSampleAt(CSTT_BIND_GROUP_BINDINGS);
    cSample.mCount = totalUpdates;
    cSample.mTime = float(time) / 1000.0f;
    gCpuStressTests[CSTT_BIND_GROUP_BINDINGS].mTotalSamplesTaken[GetCpuApiDataIndex()]++;

    cpuStressTestDrawAndSubmitDefaultSwapchainRT(pElem, pRenderTargetSwapchain);
}

void cpuUpdateGraphDataForTest(uint32_t testTypeIdx, const GridInfo& gridInfo, uint32_t& numGridPoints, uint32_t& graphedLineStartIdx,
                               float& xIntervals, float& yIntervals)
{
#define ADD_LINE(idx, s, e) gCpuStressTestData.mPlotData[idx] = { s, e }

    uint32_t numGridTickPoints = 0;
    uint32_t numGraphedPoints = 0;
    // Create Rect Border
    // Left Vertical Line
    ADD_LINE(numGridPoints++, gridInfo.mGridOrigin, float2(gridInfo.mGridOrigin.x, gridInfo.mGridOrigin.y + gridInfo.mRectSize.y));
    // Right Vertical Line
    ADD_LINE(numGridPoints++, float2(gridInfo.mGridOrigin.x + gridInfo.mRectSize.x, gridInfo.mGridOrigin.y),
             gridInfo.mGridOrigin + gridInfo.mRectSize);
    // Bottom Horizontal Line
    ADD_LINE(numGridPoints++, gridInfo.mGridOrigin, float2(gridInfo.mGridOrigin.x + gridInfo.mRectSize.x, gridInfo.mGridOrigin.y));
    // Top Horizontal Line
    ADD_LINE(numGridPoints++, float2(gridInfo.mGridOrigin.x, gridInfo.mGridOrigin.y + gridInfo.mRectSize.y),
             gridInfo.mGridOrigin + gridInfo.mRectSize);
    // Ticks data
    float2 tickInterval = (gridInfo.mRectSize - gridInfo.mCenter * 0.05f) / float(kNumCpuStressTests);

    // Test Average Data
    float yMax = 0.0f;
    float xMax = 0.0f;
    float yAverages[2][kNumCpuStressTests];
    float xAverages[2][kNumCpuStressTests];
    for (uint32_t apiIdx = 0; apiIdx < 2; ++apiIdx)
    {
        // Calculate test averages
        for (uint32_t ti = 0; ti < kNumCpuStressTests; ++ti)
        {
            yAverages[apiIdx][ti] = 0.0f;
            xAverages[apiIdx][ti] = 0.0f;
            for (uint32_t si = 0; si < kNumCpuStressTestSamples; ++si)
            {
                CPUStressTestSample& data = GetCpuSampleAt(apiIdx, testTypeIdx, ti, si);
                yAverages[apiIdx][ti] += GetCpuSampleTime(data, testTypeIdx);
                xAverages[apiIdx][ti] += float(data.mCount);
            }

            yAverages[apiIdx][ti] /= float(kNumCpuStressTestSamples);
            xAverages[apiIdx][ti] /= float(kNumCpuStressTestSamples);

            yMax = max(yMax, yAverages[apiIdx][ti]);
            xMax = max(xMax, xAverages[apiIdx][ti]);
        }
    }

    yMax += (yMax * 0.1f) + 0.01f;
    xMax += (xMax * 0.1f) + 0.01f;

    yIntervals = yMax / float(kNumCpuStressTests);
    xIntervals = xMax / float(kNumCpuStressTests);

    // Add vertical tick
    for (uint32_t i = 0; i < kNumCpuStressTests; ++i)
    {
        uint32_t idx = numGridPoints + numGridTickPoints++;

        float2 cTickInterval = float(i + 1) * tickInterval;
        float2 s = gridInfo.mGridOrigin + float2(cTickInterval.x, 0.0f);
        float2 e = gridInfo.mGridOrigin + float2(cTickInterval.x, -gridInfo.mTickSize.y);

        ADD_LINE(idx, s, e);
    }
    for (uint32_t i = 0; i < kNumCpuStressTests; ++i)
    {
        uint32_t idx = numGridPoints + numGridTickPoints++;

        float2 cTickInterval = float(i + 1) * tickInterval;
        float2 s = gridInfo.mGridOrigin + float2(0.0f, cTickInterval.y);
        float2 e = gridInfo.mGridOrigin + float2(-gridInfo.mTickSize.x, cTickInterval.y);

        ADD_LINE(idx, s, e);
    }

    // add graphed line points
    graphedLineStartIdx = numGridPoints + numGridTickPoints;
    for (uint32_t apiIdx = 0; apiIdx < 2; ++apiIdx)
    {
        for (uint32_t i = 0; i < kNumCpuStressTests - 1; ++i)
        {
            uint32_t idx = graphedLineStartIdx + numGraphedPoints++;

            float x1 = xAverages[apiIdx][i];
            float x2 = xAverages[apiIdx][i + 1];

            float t1 = yAverages[apiIdx][i];
            float t2 = yAverages[apiIdx][i + 1];

            float  sx = x1 / xMax;
            float  sy = t1 / yMax;
            float2 s = gridInfo.mGridOrigin + float2(sx, sy) * gridInfo.mRectSize;

            float  ex = x2 / xMax;
            float  ey = t2 / yMax;
            float2 e = gridInfo.mGridOrigin + float2(ex, ey) * gridInfo.mRectSize;

            ADD_LINE(idx, s, e);
        }
    }

    // Update line vertex buffer
    uint32_t         lineDataSize = gCpuStressTestData.mNumPlotData * sizeof(GraphLineData2D);
    BufferUpdateDesc plotVbDesc = { gCpuStressTestData.pVertexBuffer, 0, lineDataSize };
    beginUpdateResource(&plotVbDesc);
    memcpy(plotVbDesc.pMappedData, gCpuStressTestData.mPlotData, lineDataSize);
    endUpdateResource(&plotVbDesc);

#undef ADD_LINE
}
void cpuDrawStressTestDataToRenderTarget(Cmd* pCmd, RenderTarget* pRenderTarget, const GridInfo& gInfo, uint32_t testTypeIdx,
                                         uint32_t numGridPoints, uint32_t graphedLineStartIdx, float xIntervals, float yIntervals)
{
    RenderTargetBarrier rtBarriers[1] = { { pRenderTarget, RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET } };
    cmdResourceBarrier(pCmd, 0, NULL, 0, NULL, 1, rtBarriers);

    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
    bindRenderTargets.mDepthStencil = { NULL, LOAD_ACTION_CLEAR };
    cmdBindRenderTargets(pCmd, &bindRenderTargets);

    cmdSetViewport(pCmd, 0.0f, 0.0f, gInfo.mWidth, gInfo.mHeight, 0.0f, 1.0f);
    cmdSetScissor(pCmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

    // Draw Grid Line
    cmdBindPipeline(pCmd, gCpuStressTestData.pPipeline);
    const uint32_t vStrides = { sizeof(float2) };
    cmdBindVertexBuffer(pCmd, 1, &gCpuStressTestData.pVertexBuffer, &vStrides, NULL);
    cmdBindDescriptorSet(pCmd, 0, gCpuStressTestData.pSet);

    // Draw grid
    cmdDrawInstanced(pCmd, graphedLineStartIdx * 2, 0, 1, 0);
    // Draw graphed lines
    // Vulkan Data
    cmdDrawInstanced(pCmd, (kNumCpuStressTests - 1) * 2, graphedLineStartIdx * 2, 1, 0x00ff0000);
    // WebGpu Data
    cmdDrawInstanced(pCmd, (kNumCpuStressTests - 1) * 2, graphedLineStartIdx * 2 + (kNumCpuStressTests - 1) * 2, 1, 0x0000ff00);

    // Draw text for x-axis
    float dpiScale = 1.0f;
#if defined(ANDROID)
    dpiScale = 2.625f;
#endif
    FontDrawDesc drawDesc = {};
    drawDesc.mFontColor = 0xff000000;
    drawDesc.mFontSize = 11.5f;
    drawDesc.mFontID = gFontID;
    float  pxTickWidth = 30.0f * dpiScale;
    float  pxTickHeight = 6.67f * dpiScale;
    float2 tickSize = gInfo.mTickSize * dpiScale;
    for (uint32_t i = 0; i < kNumCpuStressTests; ++i)
    {
        /*if ((i + 1) % 2 == 0)
            continue;*/

        char pxAxisStrings[10] = { 0 };
        char pyAxisStrings[10] = { 0 };

        snprintf(pxAxisStrings, 10, " %u ", uint32_t(xIntervals * (i + 1)));
        snprintf(pyAxisStrings, 10, "%.2fms", yIntervals * float(i + 1));

        uint32_t tickIdx = numGridPoints + i;
        float2   pos = gCpuStressTestData.mPlotData[tickIdx].end;

        drawDesc.pText = pxAxisStrings;
        cmdDrawTextWithFont(pCmd, float2(pos.x - pxTickWidth * 0.5f, gInfo.mHeight - pos.y + tickSize.y * 0.5f), &drawDesc);

        tickIdx = numGridPoints + i + kNumCpuStressTests;
        pos = gCpuStressTestData.mPlotData[tickIdx].end;

        drawDesc.pText = pyAxisStrings;
        cmdDrawTextWithFont(pCmd, float2(pos.x - pxTickWidth - tickSize.x * 0.5f, gInfo.mHeight - pos.y - pxTickHeight), &drawDesc);
    }

    const char* numDrawCallsStr = "# of Drawcalls";
    const char* numCommandsStr = "# of Commands";
    const char* numUpdatesStr = " # of Updates";
    const char* numBindingStr = " # of Binding";
    float       pxWidth = 73.0f * dpiScale;
    float       pxHeight = 8.41f * dpiScale;

    const char* pxAxisTitle = numDrawCallsStr;
    switch (testTypeIdx)
    {
    case CSTT_BIND_GROUP_UPDATES:
        pxAxisTitle = numUpdatesStr;
        break;
    case CSTT_BIND_GROUP_BINDINGS:
        pxAxisTitle = numBindingStr;
        break;
    case CSTT_COMMAND_ENCODING:
        pxAxisTitle = numCommandsStr;
        break;
    default:
        pxAxisTitle = numDrawCallsStr;
    };

    drawDesc.mFontSize = 14.5f;
    drawDesc.pText = pxAxisTitle;
    float2 tPos = float2(gInfo.mGridOrigin.x + gInfo.mRectSize.x * 0.5f, gInfo.mGridOrigin.y);
    tPos -= float2(pxWidth * 0.5f, pxHeight + pxTickHeight + tickSize.y * 2.0f);
    cmdDrawTextWithFont(pCmd, float2(tPos.x, gInfo.mHeight - tPos.y), &drawDesc);

    pxWidth = 52.0f * dpiScale;

    const char* pyAxisTitle = "Time (ms)";
    drawDesc.pText = pyAxisTitle;
    tPos = float2(gInfo.mGridOrigin.x, gInfo.mGridOrigin.y + gInfo.mRectSize.y * 0.5f);
    tPos -= float2(pxWidth + pxTickWidth + tickSize.x * 2.0f, -pxHeight);
    cmdDrawTextWithFont(pCmd, float2(tPos.x, gInfo.mHeight - tPos.y), &drawDesc);

    pxWidth = 30.0f * dpiScale;

    const char* pApiStringWebgpu = "Webgpu";
    drawDesc.pText = pApiStringWebgpu;
    drawDesc.mFontColor = 0xff00ff00;
    tPos = float2(gInfo.mGridOrigin.x + gInfo.mRectSize.x, gInfo.mGridOrigin.y + gInfo.mRectSize.y * 0.5f);
    tPos += float2(pxWidth + 0.5f, pxHeight);
    cmdDrawTextWithFont(pCmd, float2(tPos.x, gInfo.mHeight - tPos.y), &drawDesc);

    const char* pApiStringVulkan = "Vulkan";
    drawDesc.pText = pApiStringVulkan;
    drawDesc.mFontColor = 0xff0000ff;
    tPos = float2(gInfo.mGridOrigin.x + gInfo.mRectSize.x, gInfo.mGridOrigin.y + gInfo.mRectSize.y * 0.5f);
    tPos += float2(pxWidth + 0.5f, -pxHeight);
    cmdDrawTextWithFont(pCmd, float2(tPos.x, gInfo.mHeight - tPos.y), &drawDesc);

    cmdBindRenderTargets(pCmd, NULL);

    rtBarriers[0] = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
    cmdResourceBarrier(pCmd, 0, NULL, 0, NULL, 1, rtBarriers);
}
void cpuGraphStressTestData(GpuCmdRingElement* pGraphicsElem, RenderTarget* pRenderTarget)
{
    if (!gCpuStressTestData.bShouldTakeScreenshot)
        return;

    // Stall if CPU is running "gDataBufferCount" frames ahead of GPU
    FenceStatus fenceStatus;
    getFenceStatus(pRenderer, pGraphicsElem->pFence, &fenceStatus);
    if (fenceStatus == FENCE_STATUS_INCOMPLETE)
        waitForFences(pRenderer, 1, &pGraphicsElem->pFence);

    // Create line
    GridInfo gInfo = {};
    gInfo.mWidth = float(pRenderTarget->mWidth);
    gInfo.mHeight = float(pRenderTarget->mHeight);
    gInfo.mCenter = float2(gInfo.mWidth, gInfo.mHeight) * 0.5f;
    float2 centerHalf = gInfo.mCenter * 0.5f;
    gInfo.mGridOrigin = gInfo.mCenter - centerHalf;

    gInfo.mRectSize = gInfo.mCenter;
    gInfo.mTickSize = gInfo.mCenter * 0.05f;
    gInfo.mTickSize = float2(min(gInfo.mTickSize.x, gInfo.mTickSize.y));

    // Update uniform buffer
    BufferUpdateDesc ubUpdate = { gCpuStressTestData.pUniformBuffer, 0, sizeof(mat4) };
    beginUpdateResource(&ubUpdate);
    float        nearClip = 0.1f;
    float        farClip = 300.0f;
    CameraMatrix projMat = CameraMatrix::orthographic(0, gInfo.mWidth, 0.0f, gInfo.mHeight, farClip, nearClip);
    memcpy(ubUpdate.pMappedData, &projMat, sizeof(mat4));
    endUpdateResource(&ubUpdate);

    uint32_t numTestTypes = gCpuStressTestData.mTypeWidgetData == CSTT_ALL ? CSTT_ALL : 1;
    uint32_t cTestIdx = gCpuStressTestData.mTypeWidgetData == CSTT_ALL ? 0 : gCpuStressTestData.mTypeWidgetData;
    for (uint32_t i = 0; i < numTestTypes; ++i)
    {
        if (gCpuStressTests[cTestIdx].mTotalSamplesTaken[GetCpuApiDataIndex()] < kNumCpuStressTests * kNumCpuStressTestSamples)
            continue;

        const char* pString = "CPU_STRESS_TEST_DATA";
        snprintf(gCpuStressTestData.screenShotName, 512, "%s_%s", pString, gCpuStressTestData.kTypeStrings[cTestIdx]);

        uint32_t numGridPoints = 0;
        uint32_t graphedLineStartIdx = 0;
        float    xIntervals = 0.0f;
        float    yIntervals = 0.0f;
        cpuUpdateGraphDataForTest(cTestIdx, gInfo, numGridPoints, graphedLineStartIdx, xIntervals, yIntervals);

        resetCmdPool(pRenderer, pGraphicsElem->pCmdPool);
        Cmd* pCmd = pGraphicsElem->pCmds[0];

        beginCmd(pCmd);
        cpuDrawStressTestDataToRenderTarget(pCmd, pRenderTarget, gInfo, cTestIdx, numGridPoints, graphedLineStartIdx, xIntervals,
                                            yIntervals);
        endCmd(pCmd);

        FlushResourceUpdateDesc flushUpdateDesc = {};
        flushUpdateDesc.mNodeIndex = 0;
        flushResourceUpdates(&flushUpdateDesc);
        Semaphore* waitSemaphores[] = { flushUpdateDesc.pOutSubmittedSemaphore };

        QueueSubmitDesc submitDesc = {};
        submitDesc.mCmdCount = 1;
        submitDesc.mSignalSemaphoreCount = 1;
        submitDesc.mWaitSemaphoreCount = TF_ARRAY_COUNT(waitSemaphores);
        submitDesc.ppCmds = &pCmd;
        submitDesc.ppSignalSemaphores = &pGraphicsElem->pSemaphore;
        submitDesc.ppWaitSemaphores = waitSemaphores;
        submitDesc.pSignalFence = pGraphicsElem->pFence;
        queueSubmit(pGraphicsQueue, &submitDesc);

        waitQueueIdle(pGraphicsQueue);
        saveRenderTargetToPng(gCpuStressTestData.pRenderTarget, gCpuStressTestData.screenShotName);

        cTestIdx++;
    }

    gCpuStressTestData.bShouldTakeScreenshot = false;

    return;
}
#endif

DEFINE_APPLICATION_MAIN(SunTemple)
