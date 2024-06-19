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

#include "SunTempleGeometry.h"

#include "../../../../Common_3/Utilities/Interfaces/IFileSystem.h"
#include "../../../../Common_3/Utilities/Interfaces/ILog.h"

#include "../../../../Common_3/Utilities/Interfaces/IMemory.h"

#define DEFAULT_ALBEDO           "Default"
#define DEFAULT_NORMAL           "Default_NRM"
#define DEFAULT_SPEC             "Default_SPEC"
#define DEFAULT_EMS              "Default_EMS"
#define DEFAULT_SPEC_TRANSPARENT "Default_SPEC_TRANS"

#define SAFE_TEXTURE_FREE(idx, texName) \
    if (scene->texName[idx])            \
    {                                   \
        tf_free(scene->texName[idx]);   \
        scene->texName[idx] = NULL;     \
    }

static void setTextures(Scene* pScene, int index, const char* albedo, const char* specular, const char* normal, const char* emissive,
                        float3 emissiveFactor, float metallicFactor, float roughnessFactor, uint32_t matFlags, uint32_t lightMapDataIndex)
{
    pScene->pMeshSettings[index].mFlags = matFlags;

    pScene->pMaterials[index].mEmissiveFactor = emissiveFactor;
    pScene->pMaterials[index].mFlags = matFlags;

    pScene->pMaterials[index].mMetallicRoughnessFactors.x = metallicFactor;
    pScene->pMaterials[index].mMetallicRoughnessFactors.y = roughnessFactor;

    pScene->pMaterials[index].mLightUvScale = pScene->pLightMapDatas[lightMapDataIndex].mLightUvScale;
    pScene->pMaterials[index].mGiOffset = pScene->pLightMapDatas[lightMapDataIndex].mGiOffset;

    // default textures
    pScene->ppDiffuseMaps[index] = (char*)tf_calloc(strlen(albedo) + 5, sizeof(char));
    pScene->ppSpecularMaps[index] = (char*)tf_calloc(strlen(specular) + 5, sizeof(char));
    pScene->ppNormalMaps[index] = (char*)tf_calloc(strlen(normal) + 5, sizeof(char));
    pScene->ppEmissiveMaps[index] = (char*)tf_calloc(strlen(emissive) + 5, sizeof(char));

    MeshType mt = MT_OPAQUE;
    if (strstr(albedo, "Rock") != NULL)
    {
        mt = MT_TERRAIN;
    }
    if (matFlags & MATERIAL_FLAG_ALPHA_TESTED)
    {
        mt = MT_ALPHA_TESTED;
    }
    pScene->pMeshSettings[index].mType = mt;

    snprintf(pScene->ppDiffuseMaps[index], strlen(albedo) + 5, "%s.tex", albedo);
    snprintf(pScene->ppSpecularMaps[index], strlen(specular) + 5, "%s.tex", specular);
    snprintf(pScene->ppNormalMaps[index], strlen(normal) + 5, "%s.tex", normal);
    snprintf(pScene->ppEmissiveMaps[index], strlen(emissive) + 5, "%s.tex", emissive);
}

static void setLightmapData(Scene* pScene, uint32_t index, uint32_t giOffset, float4 uvScale)
{
    pScene->pLightMapDatas[index].mGiOffset = giOffset;
    pScene->pLightMapDatas[index].mLightUvScale = uvScale;
}

static void SetMaterialsAndLightmapData(Scene* pScene)
{
    int index = 0;

    {
        setLightmapData(pScene, 0, 0, float4(0.065430f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 1, 0, float4(0.709961f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 2, 0, float4(0.645508f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 3, 0, float4(0.065430f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 4, 0, float4(0.000977f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 5, 0, float4(0.581055f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 6, 0, float4(0.516602f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 7, 0, float4(0.452148f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 8, 0, float4(0.903320f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 9, 0, float4(0.516602f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 10, 0, float4(0.387695f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 11, 0, float4(0.000977f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 12, 0, float4(0.194336f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 13, 0, float4(0.774414f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 14, 0, float4(0.516602f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 15, 0, float4(0.709961f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 16, 0, float4(0.065430f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 17, 0, float4(0.000977f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 18, 0, float4(0.903320f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 19, 0, float4(0.838867f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 20, 0, float4(0.774414f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 21, 0, float4(0.581055f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 22, 1, float4(0.387695f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 23, 1, float4(0.903320f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 24, 1, float4(0.065430f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 25, 1, float4(0.000977f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 26, 0, float4(0.838867f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 27, 0, float4(0.774414f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 28, 0, float4(0.838867f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 29, 0, float4(0.774414f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 30, 0, float4(0.645508f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 31, 0, float4(0.194336f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 32, 0, float4(0.129883f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 33, 0, float4(0.323242f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 34, 0, float4(0.452148f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 35, 0, float4(0.387695f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 36, 0, float4(0.194336f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 37, 0, float4(0.065430f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 38, 0, float4(0.323242f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 39, 0, float4(0.258789f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 40, 0, float4(0.838867f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 41, 0, float4(0.903320f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 42, 0, float4(0.129883f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 43, 1, float4(0.258789f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 44, 0, float4(0.645508f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 45, 0, float4(0.258789f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 46, 0, float4(0.709961f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 47, 0, float4(0.452148f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 48, 0, float4(0.387695f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 49, 0, float4(0.323242f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 50, 0, float4(0.194336f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 51, 0, float4(0.323242f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 52, 0, float4(0.323242f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 53, 0, float4(0.709961f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 54, 0, float4(0.323242f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 55, 0, float4(0.581055f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 56, 0, float4(0.129883f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 57, 0, float4(0.452148f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 58, 0, float4(0.129883f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 59, 0, float4(0.065430f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 60, 1, float4(0.194336f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 61, 0, float4(0.000977f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 62, 0, float4(0.838867f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 63, 1, float4(0.065430f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 64, 0, float4(0.258789f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 65, 0, float4(0.000977f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 66, 1, float4(0.194336f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 67, 1, float4(0.129883f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 68, 1, float4(0.065430f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 69, 1, float4(0.903320f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 70, 1, float4(0.838867f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 71, 1, float4(0.709961f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 72, 1, float4(0.452148f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 73, 1, float4(0.387695f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 74, 1, float4(0.323242f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 75, 1, float4(0.258789f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 76, 0, float4(0.194336f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 77, 0, float4(0.129883f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 78, 0, float4(0.065430f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 79, 0, float4(0.000977f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 80, 1, float4(0.452148f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 81, 0, float4(0.000977f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 82, 0, float4(0.258789f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 83, 0, float4(0.903320f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 84, 0, float4(0.774414f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 85, 0, float4(0.709961f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 86, 0, float4(0.645508f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 87, 0, float4(0.581055f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 88, 0, float4(0.838867f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 89, 0, float4(0.258789f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 90, 1, float4(0.709961f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 91, 1, float4(0.645508f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 92, 1, float4(0.581055f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 93, 1, float4(0.516602f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 94, 1, float4(0.452148f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 95, 1, float4(0.387695f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 96, 1, float4(0.323242f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 97, 1, float4(0.258789f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 98, 1, float4(0.838867f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 99, 1, float4(0.194336f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 100, 1, float4(0.129883f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 101, 1, float4(0.774414f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 102, 1, float4(0.000977f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 103, 1, float4(0.774414f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 104, 1, float4(0.645508f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 105, 1, float4(0.581055f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 106, 1, float4(0.516602f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 107, 0, float4(0.581055f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 108, 0, float4(0.516602f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 109, 0, float4(0.903320f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 110, 0, float4(0.129883f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 111, 0, float4(0.645508f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 112, 0, float4(0.387695f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 113, 0, float4(0.516602f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 114, 0, float4(0.387695f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 115, 0, float4(0.903320f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 116, 0, float4(0.645508f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 117, 1, float4(0.323242f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 118, 0, float4(0.645508f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 119, 0, float4(0.452148f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 120, 0, float4(0.709961f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 121, 0, float4(0.774414f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 122, 0, float4(0.258789f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 123, 0, float4(0.903320f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 124, 0, float4(0.387695f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 125, 0, float4(0.323242f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 126, 1, float4(0.129883f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 127, 0, float4(0.258789f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 128, 0, float4(0.194336f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 129, 1, float4(0.000977f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 130, 0, float4(0.838867f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 131, 0, float4(0.774414f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 132, 0, float4(0.709961f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 133, 0, float4(0.581055f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 134, 0, float4(0.194336f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 135, 0, float4(0.452148f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 136, 0, float4(0.387695f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 137, 0, float4(0.516602f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 138, 0, float4(0.581055f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 139, 0, float4(0.065430f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 140, 0, float4(0.129883f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 141, 0, float4(0.516602f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 142, 0, float4(0.452148f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 143, 4, float4(0.838867f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 144, 4, float4(0.194336f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 145, 4, float4(0.129883f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 146, 4, float4(0.000977f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 147, 4, float4(0.065430f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 148, 4, float4(0.709961f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 149, 4, float4(0.645508f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 150, 4, float4(0.581055f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 151, 4, float4(0.516602f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 152, 4, float4(0.452148f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 153, 4, float4(0.903320f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 154, 4, float4(0.774414f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 155, 4, float4(0.258789f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 156, 4, float4(0.387695f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 157, 4, float4(0.323242f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 158, 4, float4(0.258789f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 159, 2, float4(0.838867f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 160, 3, float4(0.774414f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 161, 2, float4(0.516602f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 162, 2, float4(0.452148f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 163, 2, float4(0.129883f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 164, 2, float4(0.000977f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 165, 2, float4(0.903320f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 166, 2, float4(0.903320f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 167, 2, float4(0.645508f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 168, 2, float4(0.516602f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 169, 1, float4(0.000977f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 170, 2, float4(0.387695f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 171, 2, float4(0.323242f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 172, 2, float4(0.194336f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 173, 1, float4(0.065430f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 174, 1, float4(0.581055f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 175, 1, float4(0.645508f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 176, 1, float4(0.452148f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 177, 2, float4(0.258789f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 178, 2, float4(0.000977f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 179, 2, float4(0.709961f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 180, 1, float4(0.645508f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 181, 1, float4(0.516602f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 182, 2, float4(0.581055f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 183, 1, float4(0.065430f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 184, 2, float4(0.258789f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 185, 2, float4(0.065430f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 186, 2, float4(0.838867f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 187, 1, float4(0.387695f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 188, 2, float4(0.516602f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 189, 3, float4(0.387695f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 190, 3, float4(0.774414f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 191, 2, float4(0.452148f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 192, 2, float4(0.387695f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 193, 2, float4(0.323242f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 194, 2, float4(0.258789f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 195, 3, float4(0.323242f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 196, 2, float4(0.194336f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 197, 2, float4(0.129883f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 198, 2, float4(0.838867f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 199, 2, float4(0.581055f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 200, 3, float4(0.838867f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 201, 2, float4(0.581055f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 202, 3, float4(0.387695f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 203, 3, float4(0.258789f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 204, 1, float4(0.903320f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 205, 1, float4(0.903320f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 206, 1, float4(0.838867f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 207, 1, float4(0.774414f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 208, 1, float4(0.645508f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 209, 1, float4(0.387695f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 210, 1, float4(0.258789f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 211, 1, float4(0.516602f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 212, 1, float4(0.194336f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 213, 3, float4(0.000977f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 214, 2, float4(0.000977f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 215, 2, float4(0.903320f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 216, 3, float4(0.516602f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 217, 3, float4(0.452148f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 218, 3, float4(0.323242f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 219, 1, float4(0.581055f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 220, 1, float4(0.000977f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 221, 2, float4(0.065430f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 222, 2, float4(0.903320f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 223, 2, float4(0.774414f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 224, 2, float4(0.645508f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 225, 2, float4(0.387695f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 226, 1, float4(0.194336f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 227, 3, float4(0.065430f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 228, 3, float4(0.581055f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 229, 1, float4(0.709961f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 230, 1, float4(0.129883f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 231, 2, float4(0.452148f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 232, 3, float4(0.194336f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 233, 1, float4(0.000977f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 234, 2, float4(0.452148f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 235, 2, float4(0.258789f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 236, 3, float4(0.645508f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 237, 2, float4(0.581055f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 238, 2, float4(0.516602f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 239, 2, float4(0.452148f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 240, 1, float4(0.323242f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 241, 3, float4(0.258789f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 242, 2, float4(0.129883f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 243, 2, float4(0.065430f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 244, 2, float4(0.903320f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 245, 2, float4(0.838867f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 246, 2, float4(0.774414f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 247, 2, float4(0.709961f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 248, 2, float4(0.645508f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 249, 2, float4(0.387695f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 250, 2, float4(0.323242f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 251, 2, float4(0.258789f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 252, 2, float4(0.194336f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 253, 2, float4(0.129883f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 254, 2, float4(0.709961f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 255, 2, float4(0.000977f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 256, 1, float4(0.258789f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 257, 1, float4(0.194336f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 258, 3, float4(0.194336f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 259, 1, float4(0.129883f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 260, 1, float4(0.000977f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 261, 1, float4(0.258789f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 262, 2, float4(0.323242f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 263, 1, float4(0.709961f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 264, 2, float4(0.065430f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 265, 1, float4(0.645508f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 266, 1, float4(0.903320f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 267, 1, float4(0.838867f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 268, 1, float4(0.903320f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 269, 1, float4(0.774414f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 270, 1, float4(0.709961f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 271, 1, float4(0.194336f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 272, 1, float4(0.838867f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 273, 1, float4(0.581055f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 274, 1, float4(0.516602f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 275, 2, float4(0.000977f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 276, 1, float4(0.452148f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 277, 1, float4(0.387695f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 278, 2, float4(0.387695f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 279, 2, float4(0.258789f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 280, 2, float4(0.194336f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 281, 1, float4(0.065430f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 282, 1, float4(0.774414f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 283, 1, float4(0.323242f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 284, 1, float4(0.129883f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 285, 2, float4(0.452148f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 286, 2, float4(0.129883f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 287, 2, float4(0.065430f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 288, 2, float4(0.581055f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 289, 2, float4(0.516602f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 290, 3, float4(0.903320f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 291, 1, float4(0.516602f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 292, 1, float4(0.838867f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 293, 1, float4(0.774414f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 294, 1, float4(0.129883f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 295, 1, float4(0.323242f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 296, 1, float4(0.581055f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 297, 1, float4(0.516602f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 298, 2, float4(0.645508f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 299, 2, float4(0.709961f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 300, 1, float4(0.452148f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 301, 2, float4(0.452148f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 302, 2, float4(0.323242f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 303, 2, float4(0.194336f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 304, 2, float4(0.774414f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 305, 2, float4(0.065430f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 306, 2, float4(0.000977f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 307, 1, float4(0.258789f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 308, 2, float4(0.838867f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 309, 3, float4(0.709961f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 310, 3, float4(0.129883f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 311, 3, float4(0.581055f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 312, 2, float4(0.645508f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 313, 2, float4(0.581055f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 314, 3, float4(0.516602f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 315, 2, float4(0.516602f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 316, 3, float4(0.065430f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 317, 3, float4(0.709961f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 318, 2, float4(0.258789f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 319, 2, float4(0.194336f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 320, 3, float4(0.000977f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 321, 3, float4(0.129883f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 322, 2, float4(0.838867f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 323, 2, float4(0.774414f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 324, 1, float4(0.709961f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 325, 1, float4(0.645508f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 326, 1, float4(0.709961f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 327, 2, float4(0.323242f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 328, 2, float4(0.774414f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 329, 2, float4(0.709961f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 330, 2, float4(0.387695f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 331, 2, float4(0.323242f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 332, 2, float4(0.129883f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 333, 2, float4(0.000977f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 334, 2, float4(0.903320f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 335, 1, float4(0.774414f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 336, 2, float4(0.709961f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 337, 2, float4(0.645508f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 338, 1, float4(0.387695f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 339, 1, float4(0.323242f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 340, 1, float4(0.452148f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 341, 2, float4(0.516602f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 342, 1, float4(0.838867f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 343, 2, float4(0.774414f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 344, 1, float4(0.903320f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 345, 3, float4(0.452148f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 346, 3, float4(0.838867f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 347, 2, float4(0.387695f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 348, 2, float4(0.129883f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 349, 2, float4(0.774414f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 350, 2, float4(0.709961f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 351, 2, float4(0.645508f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 352, 2, float4(0.581055f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 353, 1, float4(0.581055f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 354, 2, float4(0.194336f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 355, 2, float4(0.065430f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 356, 2, float4(0.838867f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 357, 1, float4(0.065430f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 358, 2, float4(0.903320f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 359, 3, float4(0.645508f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 360, 3, float4(0.903320f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 361, 3, float4(0.194336f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 362, 3, float4(0.129883f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 363, 3, float4(0.065430f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 364, 3, float4(0.000977f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 365, 4, float4(0.774414f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 366, 4, float4(0.709961f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 367, 4, float4(0.000977f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 368, 4, float4(0.065430f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 369, 4, float4(0.645508f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 370, 4, float4(0.581055f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 371, 4, float4(0.903320f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 372, 4, float4(0.129883f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 373, 4, float4(0.838867f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 374, 4, float4(0.516602f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 375, 4, float4(0.452148f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 376, 4, float4(0.387695f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 377, 4, float4(0.323242f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 378, 3, float4(0.452148f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 379, 3, float4(0.258789f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 380, 4, float4(0.258789f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 381, 3, float4(0.516602f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 382, 3, float4(0.387695f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 383, 3, float4(0.838867f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 384, 3, float4(0.838867f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 385, 3, float4(0.000977f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 386, 3, float4(0.838867f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 387, 3, float4(0.709961f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 388, 3, float4(0.645508f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 389, 3, float4(0.645508f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 390, 3, float4(0.387695f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 391, 3, float4(0.516602f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 392, 3, float4(0.129883f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 393, 3, float4(0.452148f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 394, 3, float4(0.194336f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 395, 3, float4(0.258789f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 396, 3, float4(0.709961f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 397, 3, float4(0.258789f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 398, 3, float4(0.194336f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 399, 3, float4(0.065430f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 400, 3, float4(0.323242f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 401, 3, float4(0.065430f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 402, 3, float4(0.581055f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 403, 3, float4(0.387695f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 404, 3, float4(0.516602f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 405, 4, float4(0.645508f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 406, 4, float4(0.581055f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 407, 3, float4(0.323242f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 408, 3, float4(0.000977f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 409, 3, float4(0.903320f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 410, 4, float4(0.516602f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 411, 4, float4(0.387695f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 412, 3, float4(0.645508f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 413, 3, float4(0.581055f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 414, 3, float4(0.452148f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 415, 3, float4(0.387695f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 416, 3, float4(0.323242f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 417, 3, float4(0.258789f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 418, 3, float4(0.645508f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 419, 3, float4(0.194336f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 420, 3, float4(0.065430f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 421, 3, float4(0.709961f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 422, 3, float4(0.581055f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 423, 3, float4(0.774414f, 0.388672f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 424, 3, float4(0.581055f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 425, 3, float4(0.516602f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 426, 4, float4(0.000977f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 427, 3, float4(0.452148f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 428, 3, float4(0.903320f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 429, 3, float4(0.838867f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 430, 3, float4(0.452148f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 431, 3, float4(0.774414f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 432, 3, float4(0.709961f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 433, 3, float4(0.323242f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 434, 3, float4(0.516602f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 435, 3, float4(0.387695f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 436, 3, float4(0.323242f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 437, 3, float4(0.129883f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 438, 3, float4(0.000977f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 439, 3, float4(0.258789f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 440, 3, float4(0.129883f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 441, 4, float4(0.129883f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 442, 3, float4(0.000977f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 443, 4, float4(0.065430f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 444, 4, float4(0.000977f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 445, 4, float4(0.838867f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 446, 4, float4(0.774414f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 447, 4, float4(0.709961f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 448, 4, float4(0.323242f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 449, 4, float4(0.452148f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 450, 3, float4(0.903320f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 451, 3, float4(0.194336f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 452, 4, float4(0.194336f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 453, 4, float4(0.065430f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 454, 3, float4(0.774414f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 455, 3, float4(0.129883f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 456, 3, float4(0.065430f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 457, 3, float4(0.774414f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 458, 3, float4(0.903320f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 459, 3, float4(0.709961f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 460, 3, float4(0.903320f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 461, 3, float4(0.838867f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 462, 3, float4(0.774414f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 463, 3, float4(0.645508f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 464, 3, float4(0.581055f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 465, 4, float4(0.903320f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 466, 4, float4(0.129883f, 0.001953f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 467, 4, float4(0.645508f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 468, 4, float4(0.065430f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 469, 4, float4(0.000977f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 470, 4, float4(0.903320f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 471, 4, float4(0.516602f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 472, 4, float4(0.452148f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 473, 4, float4(0.838867f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 474, 4, float4(0.774414f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 475, 4, float4(0.581055f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 476, 4, float4(0.709961f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 477, 4, float4(0.387695f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 478, 4, float4(0.258789f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 479, 4, float4(0.194336f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 480, 4, float4(0.323242f, 0.517578f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 481, 4, float4(0.323242f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 482, 4, float4(0.194336f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 483, 4, float4(0.258789f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 484, 4, float4(0.129883f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 485, 4, float4(0.516602f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 486, 4, float4(0.452148f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 487, 4, float4(0.387695f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 488, 4, float4(0.323242f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 489, 4, float4(0.258789f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 490, 4, float4(0.903320f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 491, 4, float4(0.581055f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 492, 4, float4(0.194336f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 493, 4, float4(0.065430f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 494, 4, float4(0.129883f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 495, 4, float4(0.000977f, 0.259766f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 496, 4, float4(0.774414f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 497, 4, float4(0.709961f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 498, 4, float4(0.645508f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 499, 4, float4(0.194336f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 500, 4, float4(0.838867f, 0.130859f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 501, 4, float4(0.129883f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 502, 4, float4(0.065430f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 503, 4, float4(0.000977f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 504, 4, float4(0.452148f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 505, 4, float4(0.903320f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 506, 4, float4(0.838867f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 507, 4, float4(0.774414f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 508, 4, float4(0.709961f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 509, 4, float4(0.645508f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 510, 4, float4(0.581055f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 511, 4, float4(0.516602f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 512, 4, float4(0.387695f, 0.646484f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 513, 4, float4(0.387695f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 514, 4, float4(0.323242f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 515, 4, float4(0.258789f, 0.775391f, 0.062500f, 0.125000f));
        setLightmapData(pScene, 516, 4, float4(0.194336f, 0.775391f, 0.062500f, 0.125000f));
    }

    {
        setTextures(pScene, index++, "M_FloorTiles1_Inst_Inst2_0_BaseColor", "M_FloorTiles1_Inst_Inst2_0_Specular",
                    "M_FloorTiles1_Inst_Inst2_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    0);
        setTextures(pScene, index++, "M_FloorTiles2_Inst_0_BaseColor", "M_FloorTiles2_Inst_0_Specular", "M_FloorTiles2_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 0);
        setTextures(pScene, index++, "M_FloorTiles2_Inst_Inst_Inst_0_BaseColor", "M_FloorTiles2_Inst_Inst_Inst_0_Specular",
                    "M_FloorTiles2_Inst_Inst_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f,
                    1, 0);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 1);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 2);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 3);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 4);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 5);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 6);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 7);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 8);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 9);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 10);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 11);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 12);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 13);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 14);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 14);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 14);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 15);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 15);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 15);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 16);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 16);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 16);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 17);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 17);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 17);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 18);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 18);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 18);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 19);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 19);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 19);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 20);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 20);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 20);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 21);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 21);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 21);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 22);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 22);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 22);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 23);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 23);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 23);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 24);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 24);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 24);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 25);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 25);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 25);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 26);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 26);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 26);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 27);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 27);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 27);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 28);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 29);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 30);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 31);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 32);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 33);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 34);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 35);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 36);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 37);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 38);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 39);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 40);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 41);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 42);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 43);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 43);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 43);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 44);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 44);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 44);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 45);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 45);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 45);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 46);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 46);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 46);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 47);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 47);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 47);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 48);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 48);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 48);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 49);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 49);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 49);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 50);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 50);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 50);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 51);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 52);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 53);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 54);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 55);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 56);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 57);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 57);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 57);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 58);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 58);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 58);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 59);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 59);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 59);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 60);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 60);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 60);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 61);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 61);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 61);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 62);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 62);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 62);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 63);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 63);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 63);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 64);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 65);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 66);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 66);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 66);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 66);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 66);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 67);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 67);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 67);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 67);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 67);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 68);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 68);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 68);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 68);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 68);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 69);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 69);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 69);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 69);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 69);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 70);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 70);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 70);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 70);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 70);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 71);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 71);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 71);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 71);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 71);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 72);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 73);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 74);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 75);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 76);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 77);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 78);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 79);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 80);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 80);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 80);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 80);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 80);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 81);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 82);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 83);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 84);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 85);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 86);
        setTextures(pScene, index++, "M_Arch_Inst_Red_2_0_BaseColor", "M_Arch_Inst_Red_2_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 87);
        setTextures(pScene, index++, "M_Arch_Inst_Red_2_0_BaseColor", "M_Arch_Inst_Red_2_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 88);
        setTextures(pScene, index++, "M_Arch_Inst_Red_2_0_BaseColor", "M_Arch_Inst_Red_2_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 89);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 90);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 90);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 90);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 91);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 91);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 91);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 92);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 92);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 92);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 93);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 93);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 93);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 94);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 94);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 94);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 95);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 95);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 95);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 96);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 96);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 96);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 97);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 97);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 97);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 98);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 98);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 98);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 99);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 99);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 99);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 100);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 100);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 100);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 101);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 101);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 101);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 102);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 102);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 102);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 102);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 102);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 103);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 103);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 103);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 103);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 103);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 104);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 104);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 104);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 104);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 104);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 105);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 105);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 105);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 105);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 105);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 106);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 106);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 106);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 106);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 106);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 107);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 107);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 107);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 108);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 108);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 108);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 109);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 110);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 110);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 110);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 111);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 112);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 113);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 114);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 115);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 115);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 115);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 116);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 116);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 116);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 117);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 117);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 117);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 118);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 118);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 118);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 119);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 120);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 120);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 120);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 121);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 121);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 121);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 122);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 123);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 123);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 123);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 124);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 124);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 124);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 125);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 125);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 125);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 126);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 126);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 126);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 127);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 127);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 127);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 128);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 128);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 128);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 129);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 129);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 129);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 130);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 131);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 132);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 133);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 134);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 135);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 136);
        setTextures(pScene, index++, "M_Arch_Inst_Red_2_0_BaseColor", "M_Arch_Inst_Red_2_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 137);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 138);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 138);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 138);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 139);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 140);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 141);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 142);
        setTextures(pScene, index++, "M_FirePit_Inst_Glow_0_BaseColor", "M_FirePit_Inst_Glow_0_Specular", "M_FirePit_Inst_0_Normal",
                    "M_FirePit_Inst_Glow_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 1.000000f, 1, 143);
        setTextures(pScene, index++, "M_Shield_Inst_0_BaseColor", "M_Shield_Inst_0_Specular", "M_Shield_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 144);
        setTextures(pScene, index++, "M_Shield_Inst_0_BaseColor", "M_Shield_Inst_0_Specular", "M_Shield_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 145);
        setTextures(pScene, index++, "M_Shield_Inst_0_BaseColor", "M_Shield_Inst_0_Specular", "M_Shield_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 146);
        setTextures(pScene, index++, "M_Shield_Inst_0_BaseColor", "M_Shield_Inst_0_Specular", "M_Shield_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 147);
        setTextures(pScene, index++, "M_FirePit_Inst_nofire_0_BaseColor", "M_FirePit_Inst_nofire_0_Specular", "M_FirePit_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 148);
        setTextures(pScene, index++, "M_FirePit_Inst_Glow_0_BaseColor", "M_FirePit_Inst_Glow_0_Specular", "M_FirePit_Inst_0_Normal",
                    "M_FirePit_Inst_Glow_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 1.000000f, 1, 149);
        setTextures(pScene, index++, "M_FirePit_Inst_Glow_0_BaseColor", "M_FirePit_Inst_Glow_0_Specular", "M_FirePit_Inst_0_Normal",
                    "M_FirePit_Inst_Glow_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 1.000000f, 1, 150);
        setTextures(pScene, index++, "M_FirePit_Inst_Glow_0_BaseColor", "M_FirePit_Inst_Glow_0_Specular", "M_FirePit_Inst_0_Normal",
                    "M_FirePit_Inst_Glow_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 1.000000f, 1, 151);
        setTextures(pScene, index++, "M_FirePit_Inst_Glow_0_BaseColor", "M_FirePit_Inst_Glow_0_Specular", "M_FirePit_Inst_0_Normal",
                    "M_FirePit_Inst_Glow_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 1.000000f, 1, 152);
        setTextures(pScene, index++, "M_FirePit_Inst_Glow_0_BaseColor", "M_FirePit_Inst_Glow_0_Specular", "M_FirePit_Inst_0_Normal",
                    "M_FirePit_Inst_Glow_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 1.000000f, 1, 153);
        setTextures(pScene, index++, "M_FirePit_Inst_Glow_0_BaseColor", "M_FirePit_Inst_Glow_0_Specular", "M_FirePit_Inst_0_Normal",
                    "M_FirePit_Inst_Glow_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 1.000000f, 1, 154);
        setTextures(pScene, index++, "M_BottomTrim_Inst_Black_0_BaseColor", "M_BottomTrim_Inst_Black_0_Specular",
                    "M_BottomTrim_Inst_Black_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    155);
        setTextures(pScene, index++, "M_BottomTrim_Inst_Black_0_BaseColor", "M_BottomTrim_Inst_Black_0_Specular",
                    "M_BottomTrim_Inst_Black_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    156);
        setTextures(pScene, index++, "M_BottomTrim_Inst_Black_0_BaseColor", "M_BottomTrim_Inst_Black_0_Specular",
                    "M_BottomTrim_Inst_Black_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    157);
        setTextures(pScene, index++, "M_Statue_Inst_0_BaseColor", "M_Statue_Inst_0_Specular", "M_Statue_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 158);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 159);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 159);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 159);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 160);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 160);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 160);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 161);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 161);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 161);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 162);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 162);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 162);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 163);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 163);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 163);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 164);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 164);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 164);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 165);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 165);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 165);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 166);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 166);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 166);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 167);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 167);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 167);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 168);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 168);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 168);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 169);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 170);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 170);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 170);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 171);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 171);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 171);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 172);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 172);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 172);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 173);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 174);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 175);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 176);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 177);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 177);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 177);
        setTextures(pScene, index++, "M_Trim_Inst_Inst_0_BaseColor", "M_Trim_Inst_Inst_0_Specular", "M_Trim_Inst_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 178);
        setTextures(pScene, index++, "M_Trim_Inst_Inst_0_BaseColor", "M_Trim_Inst_Inst_0_Specular", "M_Trim_Inst_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 179);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 180);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 181);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 182);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 182);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 182);
        setTextures(pScene, index++, "M_StoneCeiling_Inst_0_BaseColor", "M_StoneCeiling_Inst_0_Specular", "M_StoneCeiling_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 183);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 184);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 184);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 184);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 185);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 185);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 185);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 186);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 186);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 186);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 187);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 188);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 188);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 188);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 189);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 189);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 189);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 189);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 189);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 190);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 191);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 191);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 191);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 192);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 192);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 192);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 193);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 193);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 193);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 194);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 194);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 194);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 195);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 195);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 195);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 195);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 195);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 196);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 196);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 196);
        setTextures(pScene, index++, "M_Trim_Inst_Inst_0_BaseColor", "M_Trim_Inst_Inst_0_Specular", "M_Trim_Inst_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 197);
        setTextures(pScene, index++, "M_Trim_Inst_Inst_0_BaseColor", "M_Trim_Inst_Inst_0_Specular", "M_Trim_Inst_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 198);
        setTextures(pScene, index++, "M_Trim_Inst_Inst_0_BaseColor", "M_Trim_Inst_Inst_0_Specular", "M_Trim_Inst_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 199);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 200);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 200);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 200);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 201);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 201);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 201);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 202);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 202);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 202);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 203);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 203);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 203);
        setTextures(pScene, index++, "M_Dome_2_0_BaseColor", "M_Dome_2_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 204);
        setTextures(pScene, index++, "M_Arch_Inst_Red_0_BaseColor", "M_Arch_Inst_Red_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 205);
        setTextures(pScene, index++, "M_Arch_Inst_Red_0_BaseColor", "M_Arch_Inst_Red_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 206);
        setTextures(pScene, index++, "M_Arch_Inst_Red_0_BaseColor", "M_Arch_Inst_Red_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 207);
        setTextures(pScene, index++, "M_Arch_Inst_Red_0_BaseColor", "M_Arch_Inst_Red_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 208);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 209);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 210);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 211);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 212);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 213);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 213);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 213);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 214);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 214);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 214);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 215);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 215);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 215);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 216);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 216);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 216);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 217);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 217);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 217);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 218);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 218);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 218);
        setTextures(pScene, index++, "M_Arch_Inst_Red_0_BaseColor", "M_Arch_Inst_Red_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 219);
        setTextures(pScene, index++, "M_Arch_Inst_Red_0_BaseColor", "M_Arch_Inst_Red_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 220);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 221);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 222);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 223);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 224);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 225);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 226);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 227);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 227);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 227);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 228);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 228);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 228);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 229);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 230);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 231);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 232);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 232);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 232);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 233);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 234);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 234);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 234);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 235);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 236);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 237);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 237);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 237);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 238);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 238);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 238);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 239);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 239);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 239);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 240);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 241);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 241);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 241);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 241);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 241);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 242);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 242);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 242);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 243);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 243);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 243);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 244);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 244);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 244);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 245);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 245);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 245);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 246);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 246);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 246);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 247);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 247);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 247);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 248);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 248);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 248);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 249);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 249);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 249);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 250);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 250);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 250);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 251);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 251);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 251);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 252);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 252);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 252);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 253);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 253);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 253);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 254);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 254);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 254);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 255);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 255);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 255);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 256);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 257);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 258);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 258);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 258);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 258);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 258);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 259);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 260);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 261);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 262);
        setTextures(pScene, index++, "M_BottomTrim_Inst_UTile4_0_BaseColor", "M_BottomTrim_Inst_UTile4_0_Specular",
                    "M_BottomTrim_Inst_UTile4_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    263);
        setTextures(pScene, index++, "M_Stairs_Inst_0_BaseColor", "M_Stairs_Inst_0_Specular", "M_Stairs_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 264);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 265);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 266);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 267);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 268);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 269);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 270);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 271);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 272);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 273);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 274);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 275);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 276);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 277);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 278);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 279);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 280);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 281);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 282);
        setTextures(pScene, index++, "M_Railing_Inst_0_BaseColor", "M_Railing_Inst_0_Specular", "M_Railing_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 283);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 284);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 285);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 285);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 285);
        setTextures(pScene, index++, "M_Stairs_Inst_0_BaseColor", "M_Stairs_Inst_0_Specular", "M_Stairs_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 286);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 287);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 287);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 287);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 288);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 288);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 288);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 289);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 289);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 289);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 290);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 290);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 290);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 291);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 292);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 293);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 294);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 295);
        setTextures(pScene, index++, "M_Dome_2_0_BaseColor", "M_Dome_2_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 296);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 296);
        setTextures(pScene, index++, "M_Dome_2_0_BaseColor", "M_Dome_2_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 297);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 297);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 298);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 298);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 298);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 299);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 299);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 299);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 300);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 301);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 301);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 301);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 302);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 302);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 302);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 303);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 303);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 303);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 304);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 304);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 304);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 305);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 305);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 305);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 306);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 306);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 306);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 307);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 308);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 308);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 308);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 309);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 309);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 309);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 310);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 310);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 310);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 310);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 310);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 311);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 312);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 312);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 312);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 313);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 313);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 313);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 314);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 314);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 314);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 314);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 314);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 315);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 315);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 315);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 316);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 316);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 316);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 316);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 316);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 317);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 318);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 318);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 318);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 319);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 319);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 319);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 320);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 320);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 320);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 320);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 320);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 321);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 321);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 321);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 322);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 322);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 322);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 323);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 323);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 323);
        setTextures(pScene, index++, "M_Dome_2_0_BaseColor", "M_Dome_2_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 324);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 324);
        setTextures(pScene, index++, "M_Dome_2_0_BaseColor", "M_Dome_2_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 325);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 325);
        setTextures(pScene, index++, "M_Pillar_Inst_Colored_0_BaseColor", "M_Pillar_Inst_Colored_0_Specular",
                    "M_Pillar_Inst_Colored_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 326);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 327);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 328);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 328);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 328);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 329);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 329);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 329);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 330);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 330);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 330);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 331);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 331);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 331);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 332);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 332);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 332);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 333);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 333);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 333);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 334);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 334);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 334);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 335);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 336);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 336);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 336);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 337);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 337);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 337);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 338);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 339);
        setTextures(pScene, index++, "M_BottomTrim_Inst_UTile4_0_BaseColor", "M_BottomTrim_Inst_UTile4_0_Specular",
                    "M_BottomTrim_Inst_UTile4_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    340);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 341);
        setTextures(pScene, index++, "M_StoneCeiling_Inst_0_BaseColor", "M_StoneCeiling_Inst_0_Specular", "M_StoneCeiling_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 342);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 343);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 343);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 343);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 344);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 345);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 345);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 345);
        setTextures(pScene, index++, "M_Pillar_Inst_0_BaseColor", "M_Pillar_Inst_0_Specular", "M_Pillar_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 345);
        setTextures(pScene, index++, "M_Dome_Inst_0_BaseColor", "M_Dome_Inst_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 345);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 346);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 347);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 347);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 347);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 348);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 348);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 348);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 349);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 349);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 349);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 350);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 350);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 350);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 351);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 351);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 351);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 352);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 352);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 352);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 353);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 354);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 355);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 355);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 355);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 356);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 356);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 356);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 357);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 358);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 358);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 358);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 359);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 359);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 359);
        setTextures(pScene, index++, "M_FloorTiles1_Inst_Blue_0_BaseColor", "M_FloorTiles1_Inst_Blue_0_Specular",
                    "M_FloorTiles1_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 360);
        setTextures(pScene, index++, "M_FloorTiles1_Inst_Inst2_0_BaseColor", "M_FloorTiles1_Inst_Inst2_0_Specular",
                    "M_FloorTiles1_Inst_Inst2_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    360);
        setTextures(pScene, index++, "M_FloorTiles1_Inst_0_BaseColor", "M_FloorTiles1_Inst_0_Specular", "M_FloorTiles1_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 361);
        setTextures(pScene, index++, "M_FloorTiles1_Inst_0_BaseColor", "M_FloorTiles1_Inst_0_Specular", "M_FloorTiles1_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 362);
        setTextures(pScene, index++, "M_FloorTiles1_Inst_Inst2_0_BaseColor", "M_FloorTiles1_Inst_Inst2_0_Specular",
                    "M_FloorTiles1_Inst_Inst2_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    362);
        setTextures(pScene, index++, "M_FloorTiles1_Inst_0_BaseColor", "M_FloorTiles1_Inst_0_Specular", "M_FloorTiles1_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 363);
        setTextures(pScene, index++, "M_FloorTiles1_Inst_Inst2_0_BaseColor", "M_FloorTiles1_Inst_Inst2_0_Specular",
                    "M_FloorTiles1_Inst_Inst2_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    363);
        setTextures(pScene, index++, "M_FloorTiles1_Inst_Blue_0_BaseColor", "M_FloorTiles1_Inst_Blue_0_Specular",
                    "M_FloorTiles1_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 364);
        setTextures(pScene, index++, "M_FloorTiles2_Inst_REd_0_BaseColor", "M_FloorTiles2_Inst_REd_0_Specular",
                    "M_FloorTiles2_Inst_REd_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    364);
        setTextures(pScene, index++, "M_FirePit_Inst_0_BaseColor", "Default_SPEC", "M_FirePit_Inst_0_Normal", "M_FirePit_Inst_0_Emissive",
                    float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 0.552786f, 1, 365);
        setTextures(pScene, index++, "M_FirePit_Inst_0_BaseColor", "Default_SPEC", "M_FirePit_Inst_0_Normal", "M_FirePit_Inst_0_Emissive",
                    float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 0.552786f, 1, 366);
        setTextures(pScene, index++, "M_Shield_Inst_0_BaseColor", "M_Shield_Inst_0_Specular", "M_Shield_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 367);
        setTextures(pScene, index++, "M_Shield_Inst_0_BaseColor", "M_Shield_Inst_0_Specular", "M_Shield_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 368);
        setTextures(pScene, index++, "M_FirePit_Inst_0_BaseColor", "Default_SPEC", "M_FirePit_Inst_0_Normal", "M_FirePit_Inst_0_Emissive",
                    float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 0.552786f, 1, 369);
        setTextures(pScene, index++, "M_FirePit_Inst_nofire_0_BaseColor", "M_FirePit_Inst_nofire_0_Specular", "M_FirePit_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 370);
        setTextures(pScene, index++, "M_FirePit_Inst_0_BaseColor", "Default_SPEC", "M_FirePit_Inst_0_Normal", "M_FirePit_Inst_0_Emissive",
                    float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 0.552786f, 1, 371);
        setTextures(pScene, index++, "M_Statue_Inst_0_BaseColor", "M_Statue_Inst_0_Specular", "M_Statue_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 372);
        setTextures(pScene, index++, "M_FirePit_Inst_Glow_0_BaseColor", "M_FirePit_Inst_Glow_0_Specular", "M_FirePit_Inst_0_Normal",
                    "M_FirePit_Inst_Glow_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 1.000000f, 1, 373);
        setTextures(pScene, index++, "M_BottomTrim_Inst_Black_0_BaseColor", "M_BottomTrim_Inst_Black_0_Specular",
                    "M_BottomTrim_Inst_Black_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    374);
        setTextures(pScene, index++, "M_BottomTrim_Inst_Black_0_BaseColor", "M_BottomTrim_Inst_Black_0_Specular",
                    "M_BottomTrim_Inst_Black_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    375);
        setTextures(pScene, index++, "M_BottomTrim_Inst_Black_0_BaseColor", "M_BottomTrim_Inst_Black_0_Specular",
                    "M_BottomTrim_Inst_Black_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    376);
        setTextures(pScene, index++, "M_BottomTrim_Inst_Black_0_BaseColor", "M_BottomTrim_Inst_Black_0_Specular",
                    "M_BottomTrim_Inst_Black_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    377);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 378);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 379);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 380);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 380);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 380);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 381);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 381);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 381);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 382);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 382);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 382);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 383);
        setTextures(pScene, index++, "M_Dome_2_0_BaseColor", "M_Dome_2_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 384);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 385);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 385);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 385);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 386);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 386);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 386);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 387);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 388);
        setTextures(pScene, index++, "M_Dome_2_0_BaseColor", "M_Dome_2_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 389);
        setTextures(pScene, index++, "M_Arch_Inst_2_0_BaseColor", "M_Arch_Inst_2_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 390);
        setTextures(pScene, index++, "M_Arch_Inst_2_0_BaseColor", "M_Arch_Inst_2_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 391);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 392);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 393);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 393);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 393);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 394);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 394);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 394);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 395);
        setTextures(pScene, index++, "M_Dome_2_0_BaseColor", "M_Dome_2_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 396);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 397);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 397);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 397);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 398);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 398);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 398);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 399);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 399);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 399);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 400);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 401);
        setTextures(pScene, index++, "M_Dome_2_0_BaseColor", "M_Dome_2_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 402);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 403);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 404);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 405);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 405);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 405);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 406);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 406);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 406);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 407);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 408);
        setTextures(pScene, index++, "M_Dome_2_0_BaseColor", "M_Dome_2_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 409);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 410);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 410);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 410);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 411);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 411);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 411);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 412);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 412);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 412);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 413);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 413);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 413);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 414);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 414);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 414);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 415);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 415);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 415);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 416);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 416);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 416);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 417);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 417);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 417);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 418);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 418);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 418);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 419);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 420);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 420);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 420);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 421);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 421);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 421);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 422);
        setTextures(pScene, index++, "M_Dome_2_0_BaseColor", "M_Dome_2_0_Specular", "M_Dome_2_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 423);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 424);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 424);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 424);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 425);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 425);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 425);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 426);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 426);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 426);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 427);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 428);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 428);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 428);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 429);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 429);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 429);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 430);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 430);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 430);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 431);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 431);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 431);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 432);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 432);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 432);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 433);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 433);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 433);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 434);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 434);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 434);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 435);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 435);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 435);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 436);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 436);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 436);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 437);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 437);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 437);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 438);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 438);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 438);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 439);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 439);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 439);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 440);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 440);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 440);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 441);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 441);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 441);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 442);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 442);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 442);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 443);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 443);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 443);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 444);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 444);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 444);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 445);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 445);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 445);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 446);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 446);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 446);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 447);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 447);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 447);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 448);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 448);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 448);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 449);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 449);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 449);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 450);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 451);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 451);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 451);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 452);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 452);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 452);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 453);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 453);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 453);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 454);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 454);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 454);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 455);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 455);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 455);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 456);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 456);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 456);
        setTextures(pScene, index++, "M_Arch_Inst_0_BaseColor", "M_Arch_Inst_0_Specular", "M_Arch_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 457);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 458);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 458);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 458);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 459);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 459);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 459);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 460);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 460);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 460);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 461);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 461);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 461);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 462);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 462);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 462);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 463);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 463);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 463);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 464);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 464);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 464);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 465);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 465);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 465);
        setTextures(pScene, index++, "M_BottomTrim_Inst_0_BaseColor", "M_BottomTrim_Inst_0_Specular", "M_BottomTrim_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 466);
        setTextures(pScene, index++, "M_StoneBrickWall_Inst_0_BaseColor", "M_StoneBrickWall_Inst_0_Specular",
                    "M_StoneBrickWall_Inst_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 466);
        setTextures(pScene, index++, "M_Trim_Inst_0_BaseColor", "M_Trim_Inst_0_Specular", "M_Trim_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 466);
        setTextures(pScene, index++, "M_FirePit_Inst_Glow_0_BaseColor", "M_FirePit_Inst_Glow_0_Specular", "M_FirePit_Inst_0_Normal",
                    "M_FirePit_Inst_Glow_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 1.000000f, 1, 467);
        setTextures(pScene, index++, "M_Shield_Inst_0_BaseColor", "M_Shield_Inst_0_Specular", "M_Shield_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 468);
        setTextures(pScene, index++, "M_Shield_Inst_0_BaseColor", "M_Shield_Inst_0_Specular", "M_Shield_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 469);
        setTextures(pScene, index++, "M_Shield_Inst_0_BaseColor", "M_Shield_Inst_0_Specular", "M_Shield_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 470);
        setTextures(pScene, index++, "M_FirePit_Inst_nofire_0_BaseColor", "M_FirePit_Inst_nofire_0_Specular", "M_FirePit_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 471);
        setTextures(pScene, index++, "M_FirePit_Inst_nofire_0_BaseColor", "M_FirePit_Inst_nofire_0_Specular", "M_FirePit_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 472);
        setTextures(pScene, index++, "M_FirePit_Inst_nofire_0_BaseColor", "M_FirePit_Inst_nofire_0_Specular", "M_FirePit_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 473);
        setTextures(pScene, index++, "M_FirePit_Inst_0_BaseColor", "Default_SPEC", "M_FirePit_Inst_0_Normal", "M_FirePit_Inst_0_Emissive",
                    float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 0.552786f, 1, 474);
        setTextures(pScene, index++, "M_FirePit_Inst_0_BaseColor", "Default_SPEC", "M_FirePit_Inst_0_Normal", "M_FirePit_Inst_0_Emissive",
                    float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 0.552786f, 1, 475);
        setTextures(pScene, index++, "M_FirePit_Inst_Glow_0_BaseColor", "M_FirePit_Inst_Glow_0_Specular", "M_FirePit_Inst_0_Normal",
                    "M_FirePit_Inst_Glow_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f, 1.000000f, 1, 476);
        setTextures(pScene, index++, "M_BottomTrim_Inst_Black_0_BaseColor", "M_BottomTrim_Inst_Black_0_Specular",
                    "M_BottomTrim_Inst_Black_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    477);
        setTextures(pScene, index++, "M_BottomTrim_Inst_Black_0_BaseColor", "M_BottomTrim_Inst_Black_0_Specular",
                    "M_BottomTrim_Inst_Black_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    478);
        setTextures(pScene, index++, "M_BottomTrim_Inst_Black_0_BaseColor", "M_BottomTrim_Inst_Black_0_Specular",
                    "M_BottomTrim_Inst_Black_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    479);
        setTextures(pScene, index++, "M_BottomTrim_Inst_Black_0_BaseColor", "M_BottomTrim_Inst_Black_0_Specular",
                    "M_BottomTrim_Inst_Black_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1,
                    480);
        setTextures(pScene, index++, "M_Statue_Inst_0_BaseColor", "M_Statue_Inst_0_Specular", "M_Statue_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 481);
        setTextures(pScene, index++, "M_Statue_Inst_0_BaseColor", "M_Statue_Inst_0_Specular", "M_Statue_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 482);
        setTextures(pScene, index++, "M_Statue_Inst_0_BaseColor", "M_Statue_Inst_0_Specular", "M_Statue_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 483);
        setTextures(pScene, index++, "M_Statue_Inst_0_BaseColor", "M_Statue_Inst_0_Specular", "M_Statue_Inst_0_Normal", "Default_EMS",
                    float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 484);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 485);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 486);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 487);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 488);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 489);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 490);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 491);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 1, 492);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 1, 493);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 1, 494);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 495);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 496);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 497);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 498);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 499);
        setTextures(pScene, index++, "T_Rocks_D", "T_Rocks_N", "T_Grass_D", "T_Grass_N", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f,
                    1.000000f, 17, 500);
        setTextures(pScene, index++, "Soul_Tree011M_Inst_0_BaseColor-Soul_Tree011M_Inst_0_BaseColor", "Soul_Tree011M_Inst_0_Specular",
                    "Soul_Tree011M_Inst_0_Normal", "Soul_Tree011M_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 501);
        setTextures(pScene, index++, "Soul_Tree011M_Inst_0_BaseColor-Soul_Tree011M_Inst_0_BaseColor", "Soul_Tree011M_Inst_0_Specular",
                    "Soul_Tree011M_Inst_0_Normal", "Soul_Tree011M_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 502);
        setTextures(pScene, index++, "Soul_Tree011M_Inst_0_BaseColor-Soul_Tree011M_Inst_0_BaseColor", "Soul_Tree011M_Inst_0_Specular",
                    "Soul_Tree011M_Inst_0_Normal", "Soul_Tree011M_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 503);
        setTextures(pScene, index++, "M_TreeTrunk01_Inst_0_BaseColor", "M_TreeTrunk01_Inst_0_Specular", "M_TreeTrunk01_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 504);
        setTextures(pScene, index++, "M_Tree_Branches_Inst_0_BaseColor-M_Tree_Branches_Inst_0_BaseColor", "M_Tree_Branches_Inst_0_Specular",
                    "M_Tree_Branches_Inst_0_Normal", "M_Tree_Branches_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 504);
        setTextures(pScene, index++, "Soul_Tree011M_Inst_0_BaseColor-Soul_Tree011M_Inst_0_BaseColor", "Soul_Tree011M_Inst_0_Specular",
                    "Soul_Tree011M_Inst_0_Normal", "Soul_Tree011M_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 505);
        setTextures(pScene, index++, "Soul_Tree011M_Inst_0_BaseColor-Soul_Tree011M_Inst_0_BaseColor", "Soul_Tree011M_Inst_0_Specular",
                    "Soul_Tree011M_Inst_0_Normal", "Soul_Tree011M_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 506);
        setTextures(pScene, index++, "Soul_Tree011M_Inst_0_BaseColor-Soul_Tree011M_Inst_0_BaseColor", "Soul_Tree011M_Inst_0_Specular",
                    "Soul_Tree011M_Inst_0_Normal", "Soul_Tree011M_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 507);
        setTextures(pScene, index++, "Soul_Tree011M_Inst_0_BaseColor-Soul_Tree011M_Inst_0_BaseColor", "Soul_Tree011M_Inst_0_Specular",
                    "Soul_Tree011M_Inst_0_Normal", "Soul_Tree011M_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 508);
        setTextures(pScene, index++, "Soul_Tree011M_Inst_0_BaseColor-Soul_Tree011M_Inst_0_BaseColor", "Soul_Tree011M_Inst_0_Specular",
                    "Soul_Tree011M_Inst_0_Normal", "Soul_Tree011M_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 509);
        setTextures(pScene, index++, "Soul_Tree011M_Inst_0_BaseColor-Soul_Tree011M_Inst_0_BaseColor", "Soul_Tree011M_Inst_0_Specular",
                    "Soul_Tree011M_Inst_0_Normal", "Soul_Tree011M_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 510);
        setTextures(pScene, index++, "M_TreeTrunk01_Inst_0_BaseColor", "M_TreeTrunk01_Inst_0_Specular", "M_TreeTrunk01_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 511);
        setTextures(pScene, index++, "M_Tree_Branches_Inst_0_BaseColor-M_Tree_Branches_Inst_0_BaseColor", "M_Tree_Branches_Inst_0_Specular",
                    "M_Tree_Branches_Inst_0_Normal", "M_Tree_Branches_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 511);
        setTextures(pScene, index++, "M_TreeTrunk01_Inst_0_BaseColor", "M_TreeTrunk01_Inst_0_Specular", "M_TreeTrunk01_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 512);
        setTextures(pScene, index++, "M_Tree_Branches_Inst_0_BaseColor-M_Tree_Branches_Inst_0_BaseColor", "M_Tree_Branches_Inst_0_Specular",
                    "M_Tree_Branches_Inst_0_Normal", "M_Tree_Branches_Inst_0_Emissive", float3(1.000000f, 1.000000f, 1.000000f), 1.000000f,
                    1.000000f, 3, 512);
        setTextures(pScene, index++, "M_TreeTrunk01_Inst_0_BaseColor", "M_TreeTrunk01_Inst_0_Specular", "M_TreeTrunk01_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 513);
        setTextures(pScene, index++, "M_Tree_Branches_0_BaseColor-M_Tree_Branches_0_BaseColor", "M_Tree_Branches_0_Specular",
                    "M_Tree_Branches_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 3, 513);
        setTextures(pScene, index++, "M_TreeTrunk01_Inst_0_BaseColor", "M_TreeTrunk01_Inst_0_Specular", "M_TreeTrunk01_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 514);
        setTextures(pScene, index++, "M_Tree_Branches_0_BaseColor-M_Tree_Branches_0_BaseColor", "M_Tree_Branches_0_Specular",
                    "M_Tree_Branches_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 3, 514);
        setTextures(pScene, index++, "M_TreeTrunk01_Inst_0_BaseColor", "M_TreeTrunk01_Inst_0_Specular", "M_TreeTrunk01_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 515);
        setTextures(pScene, index++, "M_Tree_Branches_0_BaseColor-M_Tree_Branches_0_BaseColor", "M_Tree_Branches_0_Specular",
                    "M_Tree_Branches_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 3, 515);
        setTextures(pScene, index++, "M_TreeTrunk01_Inst_0_BaseColor", "M_TreeTrunk01_Inst_0_Specular", "M_TreeTrunk01_Inst_0_Normal",
                    "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 1, 516);
        setTextures(pScene, index++, "M_Tree_Branches_0_BaseColor-M_Tree_Branches_0_BaseColor", "M_Tree_Branches_0_Specular",
                    "M_Tree_Branches_0_Normal", "Default_EMS", float3(0.000000f, 0.000000f, 0.000000f), 1.000000f, 1.000000f, 3, 516);
    }

    for (uint32_t i = index; i < pScene->pGeom->mDrawArgCount; i++)
    {
        pScene->pMeshSettings[i].mFlags = MATERIAL_FLAG_TWO_SIDED | MATERIAL_FLAG_ALPHA_TESTED;
        pScene->pMeshSettings[i].mType = MT_ALPHA_TESTED;

        /*Material& m = pScene->materials[i];
        m.twoSided = true;
        m.alphaTested = true;*/
        setTextures(pScene, i, DEFAULT_ALBEDO, DEFAULT_SPEC, DEFAULT_NORMAL, DEFAULT_ALBEDO, float3(0.0f), 0.0f, 0.0f, 0, 0);
    }
}

// Loads a scene and returns a Scene object with scene information
Scene* loadSunTemple(const GeometryLoadDesc* pTemplate, SyncToken& token, bool transparentFlags)
{
    UNREF_PARAM(transparentFlags);
    Scene* scene = (Scene*)tf_calloc(1, sizeof(Scene));

    GeometryLoadDesc loadDesc = *pTemplate;

    VertexLayout vertexLayout = {};
    if (!loadDesc.pVertexLayout)
    {
        vertexLayout.mAttribCount = 3;
        vertexLayout.mBindingCount = 3;
        vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        vertexLayout.mAttribs[0].mBinding = 0;
        vertexLayout.mAttribs[0].mLocation = 0;
        vertexLayout.mAttribs[1].mSemantic = SEMANTIC_TEXCOORD0;
        vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R16G16_SFLOAT;
        vertexLayout.mAttribs[1].mBinding = 1;
        vertexLayout.mAttribs[1].mLocation = 1;
        vertexLayout.mAttribs[2].mSemantic = SEMANTIC_NORMAL;
        vertexLayout.mAttribs[2].mFormat = TinyImageFormat_R16G16_UNORM;
        vertexLayout.mAttribs[2].mBinding = 2;
        vertexLayout.mAttribs[2].mLocation = 2;
        loadDesc.pVertexLayout = &vertexLayout;
    }

    loadDesc.pFileName = "suntemple.bin";
    loadDesc.ppGeometryData = &scene->pGeomData;
    loadDesc.ppGeometry = &scene->pGeom;

    addResource(&loadDesc, &token);

    waitForToken(&token);

    scene->materialCount = scene->pGeom->mDrawArgCount;
    scene->pMeshSettings = (MeshSetting*)tf_calloc(scene->pGeom->mDrawArgCount, sizeof(MeshSetting));

    scene->ppDiffuseMaps = (char**)tf_calloc(scene->pGeom->mDrawArgCount, sizeof(char*));
    scene->ppNormalMaps = (char**)tf_calloc(scene->pGeom->mDrawArgCount, sizeof(char*));
    scene->ppSpecularMaps = (char**)tf_calloc(scene->pGeom->mDrawArgCount, sizeof(char*));
    scene->ppEmissiveMaps = (char**)tf_calloc(scene->pGeom->mDrawArgCount, sizeof(char*));

    scene->pMaterials = (Material*)tf_calloc(scene->pGeom->mDrawArgCount, round_up(16, sizeof(Material)));

    scene->pLightMapDatas = (LightMapData*)tf_calloc(scene->pGeom->mDrawArgCount, sizeof(LightMapData));

    SetMaterialsAndLightmapData(scene);

    tf_free(scene->pLightMapDatas);

    return scene;
}

void unloadSunTemple(Scene* scene)
{
    for (uint32_t i = 0; i < scene->materialCount; ++i)
    {
        SAFE_TEXTURE_FREE(i, ppDiffuseMaps)
        SAFE_TEXTURE_FREE(i, ppNormalMaps)
        SAFE_TEXTURE_FREE(i, ppSpecularMaps)
        SAFE_TEXTURE_FREE(i, ppEmissiveMaps)
    }
    tf_free(scene->ppDiffuseMaps);
    tf_free(scene->ppNormalMaps);
    tf_free(scene->ppSpecularMaps);
    tf_free(scene->ppEmissiveMaps);

    tf_free(scene->pMaterials);
    tf_free(scene->pMeshSettings);
}

void createCubeBuffers(Renderer* pRenderer, Buffer** ppVertexBuffer, Buffer** ppIndexBuffer)
{
    UNREF_PARAM(pRenderer);
    // Create vertex buffer
    static float vertexData[] = {
        -1, -1, -1, 1, 1, -1, -1, 1, 1, 1, -1, 1, -1, 1, -1, 1, -1, -1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1, -1, 1, 1, 1,
    };

    BufferLoadDesc vbDesc = {};
    vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    vbDesc.mDesc.mSize = sizeof(vertexData);
    vbDesc.pData = vertexData;
    vbDesc.ppBuffer = ppVertexBuffer;
    vbDesc.mDesc.pName = "VB Desc";
    addResource(&vbDesc, NULL);

    // Create index buffer
    static constexpr uint16_t indices[6 * 6] = { 0, 1, 3, 3, 1, 2, 1, 5, 2, 2, 5, 6, 5, 4, 6, 6, 4, 7,
                                                 4, 0, 7, 7, 0, 3, 3, 2, 7, 7, 2, 6, 4, 5, 0, 0, 5, 1 };

    BufferLoadDesc ibDesc = {};
    ibDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
    ibDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    ibDesc.mDesc.mSize = sizeof(indices);
    ibDesc.pData = indices;
    ibDesc.ppBuffer = ppIndexBuffer;
    ibDesc.mDesc.pName = "IB Desc";
    addResource(&ibDesc, NULL);
}
