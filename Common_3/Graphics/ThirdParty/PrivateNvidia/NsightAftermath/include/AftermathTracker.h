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

#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Nsight Aftermath
// Implements GPU crash dump tracking using the Nsight Aftermath API.
// We do this to sperate the stl code out of our codebase
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define NSIGHT_VULKAN_VERSION(major, minor, patch) \
    (((major) << 22) | ((minor) << 12) | (patch))
/**/

#if defined(VULKAN)
#define AFTERMATH_API GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan
#if VK_HEADER_VERSION_COMPLETE < NSIGHT_VULKAN_VERSION(1, 2, 135) && defined(NSIGHT_AFTERMATH)
#error Minimum requirement for the Aftermath application integration is the Vulkan 1.2.135 SDK
#endif
#elif defined (DIRECT3D12)
#include <d3d12.h>
#define AFTERMATH_API GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX
#elif defined (DIRECT3D11)
#include <d3d11.h>
#define AFTERMATH_API GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX
#elif defined(NSIGHT_AFTERMATH)
#error Selected API is not supported by Nsight Aftermath
#endif

#define USE_NSIGHT_AFTERMATH 1
#undef NSIGHT_VULKAN_VERSION


struct AftermathTracker
{  
	// this points to the internal class
	void* pHandle;
};

void CreateAftermathTracker(const char* appName, AftermathTracker* outTracker);
void DestroyAftermathTracker(AftermathTracker* pTracker);
void SetAftermathDevice(void* pDevice);
void SetAftermathMarker(AftermathTracker* pTracker, const void* pNativeHandle, const char* name);