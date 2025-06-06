/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include "GraphicsUtilities.h"

#include <Metal/Metal.h>

#include "RenderDeviceMtl.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

MTLPixelFormat TextureFormatToMTLPixelFormat(TEXTURE_FORMAT TexFormat);
TEXTURE_FORMAT MTLPixelFormatToTextureFormat(MTLPixelFormat mtlPixelFormat);

void CreateSparseTextureMtl(IRenderDevice*     pDevice,
                            const TextureDesc& TexDesc,
                            IDeviceMemory*     pMemory,
                            ITexture**         ppTexture)
{
    RefCntAutoPtr<IRenderDeviceMtl> pDeviceMtl{pDevice, IID_RenderDeviceMtl};
    if (!pDeviceMtl)
        return;
        
    if (pMemory == nullptr)
    {
        UNEXPECTED("Device memory must not be null");
        return;
    }
    
    DEV_CHECK_ERR(TexDesc.Usage == USAGE_SPARSE, "This function should be used to create sparse textures.");
    
    pDeviceMtl->CreateSparseTexture(TexDesc, pMemory, ppTexture);
}

int64_t GetNativeTextureFormatMtl(TEXTURE_FORMAT TexFormat)
{
    return static_cast<int64_t>(TextureFormatToMTLPixelFormat(TexFormat));
}

TEXTURE_FORMAT GetTextureFormatFromNativeMtl(int64_t NativeFormat)
{
    return MTLPixelFormatToTextureFormat(static_cast<MTLPixelFormat>(NativeFormat));
}

} // namespace Diligent
