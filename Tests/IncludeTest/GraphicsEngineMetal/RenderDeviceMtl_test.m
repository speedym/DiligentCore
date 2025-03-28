/*
 *  Copyright 2019-2025 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include <Metal/Metal.h>

#define Rect DiligentRect
#include "DiligentCore/Graphics/GraphicsEngineMetal/interface/RenderDeviceMtl.h"

void TestRenderDeviceMtl_CInterface(IRenderDeviceMtl* pDevice)
{
    id<MTLDevice> mtlDevice = IRenderDeviceMtl_GetMtlDevice(pDevice);
    (void) mtlDevice;

    IRenderDeviceMtl_CreateTextureFromMtlResource(pDevice, (id<MTLTexture>)NULL, RESOURCE_STATE_SHADER_RESOURCE, (ITexture**)NULL);
    IRenderDeviceMtl_CreateBufferFromMtlResource(pDevice, (id<MTLBuffer>)NULL, (BufferDesc*)NULL, RESOURCE_STATE_CONSTANT_BUFFER, (IBuffer**)NULL);

    if (@available(macos 11.0, ios 14.0, *))
    {
        IRenderDeviceMtl_CreateBLASFromMtlResource(pDevice, (id<MTLAccelerationStructure>)nil, (const BottomLevelASDesc*)NULL, RESOURCE_STATE_RAY_TRACING, (IBottomLevelAS**)NULL);
        IRenderDeviceMtl_CreateTLASFromMtlResource(pDevice, (id<MTLAccelerationStructure>)nil, (const TopLevelASDesc*)NULL, RESOURCE_STATE_RAY_TRACING, (ITopLevelAS**)NULL);
    }
    
    if (@available(macos 10.15.4, ios 13.0, *))
    {
        IRenderDeviceMtl_CreateRasterizationRateMap(pDevice, (const RasterizationRateMapCreateInfo*)nil, (IRasterizationRateMapMtl**)NULL);
        IRenderDeviceMtl_CreateRasterizationRateMapFromMtlResource(pDevice, (id<MTLRasterizationRateMap>)nil, (IRasterizationRateMapMtl**)NULL);
    }
}
