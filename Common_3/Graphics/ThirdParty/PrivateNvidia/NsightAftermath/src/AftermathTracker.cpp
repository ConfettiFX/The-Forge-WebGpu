/*
 * Copyright (c) 2018-2022 The Forge Interactive Inc.
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

#include "../include/AftermathTracker.h"

#include "../../../../OS/Interfaces/ILog.h"
#include "../../../../OS/Interfaces/IThread.h"

#include "../../../..//ThirdParty/OpenSource/EASTL/unordered_map.h"

#include "../SDK/include/GFSDK_Aftermath.h"
#include "../SDK/include/GFSDK_Aftermath_GpuCrashDump.h"
#include "../SDK/include/GFSDK_Aftermath_GpuCrashDumpDecoding.h"

#define AFTERMATH_CHECK_ERROR(RESULT)			\
    if (!GFSDK_Aftermath_SUCCEED(RESULT))		\
    {											\
		ASSERT(true); /* error code here */		\
    }											\
/**/


class GpuCrashTracker
{
public:
	GpuCrashTracker(const char* appName);
	~GpuCrashTracker();

	//*********************************************************
	// Helper function for command lists
	//*********************************************************
#if defined(DIRECT3D12)
	GFSDK_Aftermath_ContextHandle FindContextHandle(ID3D12GraphicsCommandList* pCmd);
#elif defined(DIRECT3D11)
	GFSDK_Aftermath_ContextHandle FindContextHandle(ID3D11DeviceContext* pDeviceContext);
#endif

private:
	//*********************************************************
    // Callback handlers for GPU crash dumps and related data.
    //*********************************************************

    // Handler for GPU crash dump callbacks.
    void OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);

    // Handler for shader debug information callbacks.
    void OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize);

    // Handler for GPU crash dump description callbacks.
    void OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription);


    //*********************************************************
    // Helpers for writing a GPU crash dump and debug information data to files.
    //*********************************************************

    // Helper for writing a GPU crash dump to a file.
    void WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);

    // Helper for writing shader debug information to a file
    void WriteShaderDebugInformationToFile(GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier, const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize);


    //*********************************************************
    // Static callback wrappers.
    //*********************************************************

    // GPU crash dump callback.
    static void GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData);

    // Shader debug information callback.
    static void ShaderDebugInfoCallback(
        const void* pShaderDebugInfo,
        const uint32_t shaderDebugInfoSize,
        void* pUserData);

    // GPU crash dump description callback.
    static void CrashDumpDescriptionCallback(
        PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription,
        void* pUserData);

private:
	// States
	const char* mAppName;
	bool mInitialized;
	Mutex mMutex;

#if defined(DIRECT3D12)
	eastl::unordered_map<ID3D12GraphicsCommandList*, GFSDK_Aftermath_ContextHandle> mCmdToCtx;
#elif defined(DIRECT3D11)
	eastl::unordered_map<ID3D11DeviceContext*, GFSDK_Aftermath_ContextHandle> mDeviceCtxToHandle;
#endif
};


//*********************************************************
// GpuCrashTracker implementation
//*********************************************************
GpuCrashTracker::GpuCrashTracker(const char* appName)
	: mAppName(appName)
	, mInitialized(false)
	, mMutex()
#if defined(DIRECT3D12)
	, mCmdToCtx()
#elif defined(DIRECT3D11)
	,mDeviceCtxToHandle()
#endif
{
	// Enable GPU crash dumps and set up the callbacks for crash dump notifications,
	// shader debug information notifications, and providing additional crash
	// dump description data.Only the crash dump callback is mandatory. The other two
	// callbacks are optional and can be omitted, by passing nullptr, if the corresponding
	// functionality is not used.
	// The DeferDebugInfoCallbacks flag enables caching of shader debug information data
	// in memory. If the flag is set, ShaderDebugInfoCallback will be called only
	// in the event of a crash, right before GpuCrashDumpCallback. If the flag is not set,
	// ShaderDebugInfoCallback will be called for every shader that is compiled.
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_EnableGpuCrashDumps(
		GFSDK_Aftermath_Version_API,
		AFTERMATH_API,
		GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,	// Let the Nsight Aftermath library cache shader debug information.
		GpuCrashDumpCallback,												// Register callback for GPU crash dumps.
		ShaderDebugInfoCallback,											// Register callback for shader debug information.
		CrashDumpDescriptionCallback,										// Register callback for GPU crash dump description.
		this));																// Set the GpuCrashTracker object as user data for the above callbacks.

	mMutex.Init();
	mInitialized = true;
}

GpuCrashTracker::~GpuCrashTracker()
{
    // If initialized, disable GPU crash dumps
    if (mInitialized)
    {
		mMutex.Destroy();
		GFSDK_Aftermath_DisableGpuCrashDumps();
    }
}

void GpuCrashTracker::OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
{
    // Make sure only one thread at a time...
	MutexLock lock(mMutex);

    // Write to file for later in-depth analysis with Nsight Graphics.
	WriteGpuCrashDumpToFile(pGpuCrashDump, gpuCrashDumpSize);
}

void GpuCrashTracker::OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize)
{
    // Make sure only one thread at a time...
    MutexLock lock(mMutex);

    // Get shader debug information identifier
    GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
    AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(
        GFSDK_Aftermath_Version_API,
        pShaderDebugInfo,
        shaderDebugInfoSize,
        &identifier));

    // Write to file for later in-depth analysis of crash dumps with Nsight Graphics
    WriteShaderDebugInformationToFile(identifier, pShaderDebugInfo, shaderDebugInfoSize);
}

void GpuCrashTracker::OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription)
{
    // Add some basic description about the crash. This is called after the GPU crash happens, but before
    // the actual GPU crash dump callback. The provided data is included in the crash dump and can be
    // retrieved using GFSDK_Aftermath_GpuCrashDump_GetDescription().
    addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, mAppName);
    addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "v1.0");
}

//*********************************************************
// Helpers for writing a GPU crash dump and debug information data to files.
//*********************************************************
void GpuCrashTracker::WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
{
    // Create a GPU crash dump decoder object for the GPU crash dump.
    GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
    AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
        GFSDK_Aftermath_Version_API,
        pGpuCrashDump,
        gpuCrashDumpSize,
        &decoder));

    // Use the decoder object to read basic information, like application
    // name, PID, etc. from the GPU crash dump.
    GFSDK_Aftermath_GpuCrashDump_BaseInfo baseInfo = {};
    AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &baseInfo));

	// Create a unique file name for writing the crash dump data to a file.
	// Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
	// driver release) we may see redundant crash dumps. As a workaround,
	// attach a unique count to each generated file name.
	static uint32_t count = 0;

	// Write the the crash dump data to a file using the .nv-gpudmp extension
	const char* extension = "nv-gpudmp";
	char outputFileName[512] = {'\0'};
	sprintf(outputFileName, "%s-%u-%u.%s", mAppName, baseInfo.pid, ++count, extension);

	FileStream fh = {};
	bool success = fsOpenStreamFromPath(RD_LOG, outputFileName, FM_WRITE, &fh);
	if (success)
	{
		fsWriteToStream(&fh, pGpuCrashDump, gpuCrashDumpSize);
		fsFlushStream(&fh);
		fsCloseStream(&fh);
	}

    // Destroy the GPU crash dump decoder object.
    AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder));
}

void GpuCrashTracker::WriteShaderDebugInformationToFile(GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier, const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize)
{
	// Create a unique file name.
	char outputFileName[512] = {'\0'};
	sprintf(outputFileName, "shader-%llu%llu.nvdbg", identifier.id[0], identifier.id[1]);
	FileStream fh = {};
	bool success = fsOpenStreamFromPath(RD_LOG, outputFileName, FM_WRITE, &fh);
	if (success)
	{
		fsWriteToStream(&fh, pShaderDebugInfo, shaderDebugInfoSize);
		fsFlushStream(&fh);
		fsCloseStream(&fh);
	}
}

//*********************************************************
// Static callback wrappers.
//*********************************************************
void GpuCrashTracker::GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnCrashDump(pGpuCrashDump, gpuCrashDumpSize);
}

void GpuCrashTracker::ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnShaderDebugInfo(pShaderDebugInfo, shaderDebugInfoSize);
}

void GpuCrashTracker::CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnDescription(addDescription);
}

//*********************************************************
// Helper function for command lists
//*********************************************************
#if defined(DIRECT3D12)
GFSDK_Aftermath_ContextHandle GpuCrashTracker::FindContextHandle(ID3D12GraphicsCommandList* pCmd)
{
	// Check if the handle is created for this command list
	if (mCmdToCtx.end() == mCmdToCtx.find(pCmd))
	{
		// Create an Nsight Aftermath context handle for setting Aftermath event markers in this command list.
		mCmdToCtx[pCmd] = NULL;
		AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX12_CreateContextHandle(pCmd, &mCmdToCtx[pCmd]));
	}

	return mCmdToCtx[pCmd];
}
#elif defined(DIRECT3D11)
GFSDK_Aftermath_ContextHandle GpuCrashTracker::FindContextHandle(ID3D11DeviceContext* pDeviceContext)
{
	// Check if the handle is created for this command list
	if (mDeviceCtxToHandle.end() == mDeviceCtxToHandle.find(pDeviceContext))
	{
		// Create an Nsight Aftermath context handle for setting Aftermath event markers in this command list.
		mDeviceCtxToHandle[pDeviceContext] = NULL;
		AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX11_CreateContextHandle(pDeviceContext, &mDeviceCtxToHandle[pDeviceContext]));
	}

	return mDeviceCtxToHandle[pDeviceContext];
}
#endif

//*********************************************************
// Interface implementation
//*********************************************************
void CreateAftermathTracker(const char* appName, AftermathTracker* outTracker)
{
	if (NULL == outTracker) 
	{
		return; // error code here
	}

	// Create the actual tracker
	GpuCrashTracker* tracker = tf_new(GpuCrashTracker, appName);
	outTracker->pHandle = tracker;
}

void DestroyAftermathTracker(AftermathTracker* pTracker)
{
	if (NULL == pTracker) 
	{
		return; // error code here
	}
	if (NULL == pTracker->pHandle) 
	{
		return; // error code here
	}

	GpuCrashTracker* tracker = (GpuCrashTracker*)pTracker->pHandle;
	tf_delete(tracker); // If initialized, disable GPU crash dumps
}

void SetAftermathDevice(void* pDevice)
{
#if defined(DIRECT3D12) || defined(DIRECT3D11)	
	const uint32_t aftermathFlags =
	GFSDK_Aftermath_FeatureFlags_EnableMarkers |             // Enable event marker tracking.
	GFSDK_Aftermath_FeatureFlags_EnableResourceTracking |    // Enable tracking of resources.
	GFSDK_Aftermath_FeatureFlags_CallStackCapturing |        // Capture call stacks for all draw calls, compute dispatches, and resource copies.
	GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo;    // Generate debug information for shaders.
#endif

#if defined(DIRECT3D12)
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX12_Initialize(
		GFSDK_Aftermath_Version_API,
		aftermathFlags,
		(ID3D12Device*)pDevice));
#elif defined(DIRECT3D11)
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX11_Initialize(
		GFSDK_Aftermath_Version_API,
		aftermathFlags,
		(ID3D11Device*)pDevice));
#endif
}

void SetAftermathMarker(AftermathTracker* pTracker, const void* pNativeHandle, const char* name)
{
#if defined(DIRECT3D12)
	ID3D12GraphicsCommandList* pDxCmdList = (ID3D12GraphicsCommandList*)pNativeHandle;
	GpuCrashTracker* tracker = (GpuCrashTracker*)pTracker->pHandle;
	GFSDK_Aftermath_ContextHandle contextHandle = tracker->FindContextHandle(pDxCmdList);
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_SetEventMarker(contextHandle, name, uint32_t(strlen(name)) + 1));
#elif defined(DIRECT3D11)
	ID3D11DeviceContext* pDxDeviceContext = (ID3D11DeviceContext*)pNativeHandle;
	GpuCrashTracker* tracker = (GpuCrashTracker*)pTracker->pHandle;
	GFSDK_Aftermath_ContextHandle contextHandle = tracker->FindContextHandle(pDxDeviceContext);
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_SetEventMarker(contextHandle, name, uint32_t(strlen(name)) + 1));
#endif
}
