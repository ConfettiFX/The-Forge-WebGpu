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

#pragma once

#include "../../../../OS/Interfaces/IOperatingSystem.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// spirv cross
// This is a C API DLL
// We do this to sperate the stl code out of our codebase
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct SPIRV_Type
{
   // Resources are identified with their SPIR-V ID.
   // This is the ID of the OpVariable.
   uint32_t id;

   // The type ID of the variable which includes arrays and all type modifications.
   // This type ID is not suitable for parsing OpMemberDecoration of a struct and other decorations in general
   // since these modifications typically happen on the base_type_id.
   uint32_t type_id;

   // The base type of the declared resource.
   // This type is the base type which ignores pointers and arrays of the type_id.
   // This is mostly useful to parse decorations of the underlying type.
   // base_type_id can also be obtained with get_type(get_type(type_id).self).
   uint32_t base_type_id;
};

enum SPIRV_Resource_Type
{
   SPIRV_TYPE_STAGE_INPUTS = 0,
   SPIRV_TYPE_STAGE_OUTPUTS,
   SPIRV_TYPE_UNIFORM_BUFFERS,
   SPIRV_TYPE_STORAGE_BUFFERS,
   SPIRV_TYPE_IMAGES,
   SPIRV_TYPE_STORAGE_IMAGES,
   SPIRV_TYPE_SAMPLERS,
   SPIRV_TYPE_PUSH_CONSTANT,
   SPIRV_TYPE_SUBPASS_INPUTS,
   SPIRV_TYPE_UNIFORM_TEXEL_BUFFERS,
   SPIRV_TYPE_STORAGE_TEXEL_BUFFERS,
   SPIRV_TYPE_ACCELERATION_STRUCTURES,
   SPIRV_TYPE_COMBINED_SAMPLERS,
   SPIRV_TYPE_COUNT
};

enum SPIRV_Resource_Dim
{
	SPIRV_DIM_UNDEFINED = 0,
	SPIRV_DIM_BUFFER = 1,
	SPIRV_DIM_TEXTURE1D = 2,
	SPIRV_DIM_TEXTURE1DARRAY = 3,
	SPIRV_DIM_TEXTURE2D = 4,
	SPIRV_DIM_TEXTURE2DARRAY = 5,
	SPIRV_DIM_TEXTURE2DMS = 6,
	SPIRV_DIM_TEXTURE2DMSARRAY = 7,
	SPIRV_DIM_TEXTURE3D = 8,
	SPIRV_DIM_TEXTURECUBE = 9,
	SPIRV_DIM_TEXTURECUBEARRAY = 10,
	SPIRV_DIM_COUNT = 11,
};

enum SPIRV_Resource_Access
{
	SPIRV_ACCESS_READONLY = 0,
	SPIRV_ACCESS_WRITEONLY = 1,
	SPIRV_ACCESS_READWRITE = 2,
};

enum SPIRV_Image_Format
{
    SPIRV_FORMAT_UNKNOWN = 0,
    SPIRV_FORMAT_RGBA32F = 1,
    SPIRV_FORMAT_RGBA16F = 2,
    SPIRV_FORMAT_R32F = 3,
    SPIRV_FORMAT_RGBA8 = 4,
    SPIRV_FORMAT_RGBA8SNORM = 5,
    SPIRV_FORMAT_RG32F = 6,
    SPIRV_FORMAT_RG16F = 7,
    SPIRV_FORMAT_R11FG11FB10F = 8,
    SPIRV_FORMAT_R16F = 9,
    SPIRV_FORMAT_RGBA16 = 10,
    SPIRV_FORMAT_RGB10A2 = 11,
    SPIRV_FORMAT_RG16 = 12,
    SPIRV_FORMAT_RG8 = 13,
    SPIRV_FORMAT_R16 = 14,
    SPIRV_FORMAT_R8 = 15,
    SPIRV_FORMAT_RGBA16SNORM = 16,
    SPIRV_FORMAT_RG16SNORM = 17,
    SPIRV_FORMAT_RG8SNORM = 18,
    SPIRV_FORMAT_R16SNORM = 19,
    SPIRV_FORMAT_R8SNORM = 20,
    SPIRV_FORMAT_RGBA32I = 21,
    SPIRV_FORMAT_RGBA16I = 22,
    SPIRV_FORMAT_RGBA8I = 23,
    SPIRV_FORMAT_R32I = 24,
    SPIRV_FORMAT_RG32I = 25,
    SPIRV_FORMAT_RG16I = 26,
    SPIRV_FORMAT_RG8I = 27,
    SPIRV_FORMAT_R16I = 28,
    SPIRV_FORMAT_R8I = 29,
    SPIRV_FORMAT_RGBA32UI = 30,
    SPIRV_FORMAT_RGBA16UI = 31,
    SPIRV_FORMAT_RGBA8UI = 32,
    SPIRV_FORMAT_R32UI = 33,
    SPIRV_FORMAT_RGB10A2UI = 34,
    SPIRV_FORMAT_RG32UI = 35,
    SPIRV_FORMAT_RG16UI = 36,
    SPIRV_FORMAT_RG8UI = 37,
    SPIRV_FORMAT_R16UI = 38,
    SPIRV_FORMAT_R8UI = 39,
    SPIRV_FORMAT_R64UI = 40,
    SPIRV_FORMAT_R64I = 41,
    SPIRV_FORMAT_COUNT
};

struct SPIRV_Resource
{
   // The declared name (OpName) of the resource.
   // For Buffer blocks, the name actually reflects the externally
   // visible Block name.
   const char* name;

   // Spirv data type
   SPIRV_Type SPIRV_code;

   // resource Type
   SPIRV_Resource_Type type;

   // Texture dimension. Undefined if not a texture.
   SPIRV_Resource_Dim dim;

   // Access.
   SPIRV_Resource_Access access;

   // Storage format
   SPIRV_Image_Format format;

   // The resouce set if it has one
   uint32_t set;

   // The resource binding location
   uint32_t binding;

   // The size of the resouce. This will be the descriptor array size for textures
   uint32_t size;

   // name size
   uint32_t name_size;

   // If the resouce was used in the shader
   bool is_used;
};

struct SPIRV_Variable
{
   // Spirv data type
   uint32_t SPIRV_type_id;

   // parents SPIRV code
   SPIRV_Type parent_SPIRV_code;

   // parents resource index
   uint32_t parent_index;

   // If the data was used
   bool is_used;

   // The offset of the Variable.
   uint32_t offset;

   // The size of the Variable.
   uint32_t size;

   // Variable name
   const char* name;

   // name size
   uint32_t name_size;
};

struct CrossCompiler
{
   // this points to the internal compiler class
   void* pCompiler;
   SPIRV_Resource* pShaderResouces;
   SPIRV_Variable* pUniformVariables;
   char* pEntryPoint;

   uint32_t ShaderResourceCount;
   uint32_t UniformVariablesCount;

   uint32_t EntryPointSize;
};

void CreateCrossCompiler(const uint32_t* SpirvBinary, uint32_t BinarySize, CrossCompiler* outCompiler);
void DestroyCrossCompiler(CrossCompiler* compiler);

void ReflectEntryPoint(CrossCompiler* compiler);

void ReflectShaderResources(CrossCompiler* compiler);
void ReflectShaderVariables(CrossCompiler* compiler);

void ReflectComputeShaderWorkGroupSize(CrossCompiler* compiler, uint32_t* pSizeX, uint32_t* pSizeY, uint32_t* pSizeZ);
void ReflectHullShaderControlPoint(CrossCompiler* pCompiler, uint32_t* pSizeX);
