## How to build:
1. The project is located at `/Common_3/ThirdParty/PrivateNvidia/NsightAftermath/Win64`
2. The generated lib is located at `/Common_3/ThirdParty/PrivateNvidia/NsightAftermath/Win64/x64/$API`

## How to use:
1. define `NSIGHT_AFTERMATH`
2. link `NsightAftermath.lib` to the appropriate Renderer.


## Supported platforms:
- Windows 10 17763 (RS5)
- Windows 10 18362 (19H1)
- Windows 10 18363 (19H2)
- Ubuntu v16.04, v18.04

## Supported Hardware:
- NVIDIA Pascal architecture GPUs and newer

## Supported APIs:
- DX11
- DX12
- Vulkan 1.2.135 or later.

## Notes:
1. It partially works without meeting these prerequisites, but to use all functionalities, they are required.
2. Linux version needs extra environment setups:
	* Unpack the Nsight Aftermath SDK on your platform and make sure to set the
  `NSIGHT_AFTERMATH_SDK` environment variable to the directory containing the
  files.




## Additional notes by the vendor:
### Limitations and Known Issues
* Nsight Aftermath covers only GPU crashes. CPU crashes in the NVIDIA display
  driver, the D3D runtime, the Vulkan loader, or the application cannot be
  captured.
* Nsight Aftermath is only fully supported on Turing or later GPUs.

### D3D
* Nsight Aftermath is only fully supported for D3D12 devices. Only basic support
  with a reduced feature set (no resource tracking and no shader address
  mapping) is available for D3D11 devices.
* Nsight Aftermath is fully supported on Windows 10, with limited support on
  Windows 7.
* Nsight Aftermath event markers and resource tracking is incompatible with the
  D3D debug layer and tools using D3D API interception, such as Microsoft PIX
  or Nsight Graphics.
* Shader line mappings for active warps are only supported for DXIL shaders,
  i.e. Shader Model 6 or above.
* Due to a known driver bug, the captured GPU state may be incomplete for
  drivers < R440. This can affect the capturing of shader instruction addresses
  for active warps.

### Vulkan
* On Linux, due to a driver limitation, the device status reported by
  `GFSDK_Aftermath_GpuCrashDump_GetDeviceInfo` is always
  `GFSDK_Aftermath_Device_Status_Unknown`. This will be fixed in an upcoming
  Linux display driver release.
