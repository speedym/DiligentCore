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

#include <string>

namespace
{

namespace HLSL
{

// clang-format off
const std::string FillTextureCS{
R"(

#ifndef VK_IMAGE_FORMAT
#   define VK_IMAGE_FORMAT(x)
#endif

VK_IMAGE_FORMAT("rgba8") RWTexture2D</*format=rgba8*/ float4> g_tex2DUAV : register(u0);

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 ui2Dim;
	g_tex2DUAV.GetDimensions(ui2Dim.x, ui2Dim.y);
	if (DTid.x >= ui2Dim.x || DTid.y >= ui2Dim.y)
        return;

	g_tex2DUAV[DTid.xy] = float4(float2(DTid.xy % 256u) / 256.0, 0.0, 1.0);
}
)"
};

const std::string FillTextureCS2{
R"(
Texture2D<float4> g_tex2DWhiteTexture;

VK_IMAGE_FORMAT("rgba8") RWTexture2D</*format=rgba8*/ float4> g_tex2DUAV : register(u0);

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 ui2Dim;
	g_tex2DUAV.GetDimensions(ui2Dim.x, ui2Dim.y);
	if (DTid.x >= ui2Dim.x || DTid.y >= ui2Dim.y)
        return;

	g_tex2DUAV[DTid.xy] = float4(float2(DTid.xy % 256u) / 256.0, 0.0, 1.0) * g_tex2DWhiteTexture.Load(int3(DTid.xy, 0));
}
)"
};


const std::string FillTextureVS{
R"(
struct VSOutput
{
    float4 Pos : SV_Position;
};

void main(in uint VertID : SV_VertexID,
          out VSOutput VSOut)
{
    float2 PosXY[3];
    PosXY[0] = float2(-1.0, -1.0);
    PosXY[1] = float2(-1.0, +3.0);
    PosXY[2] = float2(+3.0, -1.0);

    VSOut.Pos = float4(PosXY[VertID], 0.0, 1.0);
}
)"
};

const std::string FillTexturePS{
R"(
struct VSOutput
{
    float4 Pos : SV_Position;
};

VK_IMAGE_FORMAT("rgba8") RWTexture2D</*format=rgba8*/ float4> g_tex2DUAV : register(u0);

void main(in VSOutput VSOut)
{
	g_tex2DUAV[uint2(VSOut.Pos.xy)] = float4(float2(uint2(VSOut.Pos.xy) % 256u) / 256.0, 0.0, 1.0);
}
)"
};


const std::string FillTexturePS2{
R"(
struct VSOutput
{
    float4 Pos : SV_Position;
};

cbuffer Constants
{
	float4 g_Color;
};

VK_IMAGE_FORMAT("rgba8") RWTexture2D</*format=rgba8*/ float4> g_tex2DUAV : register(u0);

void main(in VSOutput VSOut)
{
	g_tex2DUAV[uint2(VSOut.Pos.xy)] = float4(float2(uint2(VSOut.Pos.xy) % 256u) / 256.0, 0.0, 1.0) * g_Color;
}
)"
};

// clang-format on

} // namespace HLSL

} // namespace
