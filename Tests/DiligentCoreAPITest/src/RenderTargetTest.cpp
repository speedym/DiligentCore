/*
 *  Copyright 2025 Diligent Graphics LLC
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

#include "GPUTestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"
#include "GraphicsTypesX.hpp"
#include "FastRand.hpp"

#include "gtest/gtest.h"

#include <array>

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

namespace HLSL
{

// clang-format off
const std::string VS{
R"(
struct VSInput
{
    float4 Color  : ATTRIB0;
    uint   VertId : SV_VertexID;
};

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float4 Color : COLOR;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    float4 Pos[6];
    Pos[0] = float4(-1.0, -0.5, 0.0, 1.0);
    Pos[1] = float4(-0.5, +0.5, 0.0, 1.0);
    Pos[2] = float4( 0.0, -0.5, 0.0, 1.0);

    Pos[3] = float4(+0.0, -0.5, 0.0, 1.0);
    Pos[4] = float4(+0.5, +0.5, 0.0, 1.0);
    Pos[5] = float4(+1.0, -0.5, 0.0, 1.0);

    PSIn.Pos   = Pos[VSIn.VertId];
    PSIn.Color = VSIn.Color;
}
)"
};

const std::string PS{
R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float4 Color : COLOR;
};

float4 main(in PSInput PSIn) : SV_Target
{
    return PSIn.Color;
}
)"
};

// clang-format on

} // namespace HLSL


constexpr std::array<float4, 6> RefColors = {
    float4{1, 0, 0, 0.0},
    float4{0, 1, 0, 0.5},
    float4{0, 0, 1, 1.0},

    float4{0, 1, 1, 1.0},
    float4{1, 0, 1, 0.5},
    float4{1, 1, 0, 0.0},
};

class RenderTargetTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        GPUTestingEnvironment* pEnv       = GPUTestingEnvironment::GetInstance();
        IRenderDevice*         pDevice    = pEnv->GetDevice();
        ISwapChain*            pSwapChain = pEnv->GetSwapChain();
        const SwapChainDesc&   SCDesc     = pSwapChain->GetDesc();

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

        {
            ShaderCI.Desc       = {"Render Target Test VS", SHADER_TYPE_VERTEX, true};
            ShaderCI.EntryPoint = "main";
            ShaderCI.Source     = HLSL::VS.c_str();
            pDevice->CreateShader(ShaderCI, &sm_Resources.pVS);
            ASSERT_NE(sm_Resources.pVS, nullptr);
        }

        {
            ShaderCI.Desc       = {"Render Target Test PS", SHADER_TYPE_PIXEL, true};
            ShaderCI.EntryPoint = "main";
            ShaderCI.Source     = HLSL::PS.c_str();
            pDevice->CreateShader(ShaderCI, &sm_Resources.pPS);
            ASSERT_NE(sm_Resources.pPS, nullptr);
        }

        GraphicsPipelineStateCreateInfoX PSOCreateInfo{"Render Target Test Reference"};

        InputLayoutDescX InputLayout{{0u, 0u, 4u, VT_FLOAT32}};

        PSOCreateInfo
            .AddRenderTarget(SCDesc.ColorBufferFormat)
            .SetPrimitiveTopology(PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .SetInputLayout(InputLayout)
            .AddShader(sm_Resources.pVS)
            .AddShader(sm_Resources.pPS);
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &sm_Resources.pPSO);
        ASSERT_NE(sm_Resources.pPSO, nullptr);

        sm_Resources.pPSO->CreateShaderResourceBinding(&sm_Resources.pSRB, true);
        ASSERT_NE(sm_Resources.pSRB, nullptr);

        sm_Resources.pColorsVB = pEnv->CreateBuffer({"Render Target Test - Ref Colors", sizeof(RefColors), BIND_VERTEX_BUFFER}, RefColors.data());
        ASSERT_NE(sm_Resources.pColorsVB, nullptr);

        sm_Resources.pRT = pEnv->CreateTexture("Render Target Test - RTV", SCDesc.ColorBufferFormat, BIND_RENDER_TARGET | BIND_SHADER_RESOURCE, SCDesc.Width, SCDesc.Height);
    }

    static void TearDownTestSuite()
    {
        sm_Resources = {};

        GPUTestingEnvironment* pEnv = GPUTestingEnvironment::GetInstance();
        pEnv->Reset();
    }

    void RenderReference(COLOR_MASK Mask, const float4& ClearColor)
    {
        GPUTestingEnvironment* pEnv       = GPUTestingEnvironment::GetInstance();
        IDeviceContext*        pContext   = pEnv->GetDeviceContext();
        ISwapChain*            pSwapChain = pEnv->GetSwapChain();

        RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain{pSwapChain, IID_TestingSwapChain};
        ASSERT_NE(pTestingSwapChain, nullptr);

        std::array<float4, 6> Colors = RefColors;
        for (float4& Color : Colors)
        {
            if ((Mask & COLOR_MASK_RED) == 0)
                Color.r = ClearColor.r;
            if ((Mask & COLOR_MASK_GREEN) == 0)
                Color.g = ClearColor.g;
            if ((Mask & COLOR_MASK_BLUE) == 0)
                Color.b = ClearColor.b;
            if ((Mask & COLOR_MASK_ALPHA) == 0)
                Color.a = ClearColor.a;
        }

        RefCntAutoPtr<IBuffer> pColorsVB = pEnv->CreateBuffer({"Render Target Test - Ref Colors", sizeof(Colors), BIND_VERTEX_BUFFER}, Colors.data());

        ITextureView* pRTVs[] = {sm_Resources.pRT->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET)};
        pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->ClearRenderTarget(pRTVs[0], ClearColor.Data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        IBuffer* pVBs[] = {pColorsVB};
        pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
        pContext->SetPipelineState(sm_Resources.pPSO);
        pContext->CommitShaderResources(sm_Resources.pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->Draw({6, DRAW_FLAG_VERIFY_ALL});

        StateTransitionDesc Barrier{sm_Resources.pRT, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_COPY_SOURCE, STATE_TRANSITION_FLAG_UPDATE_STATE};
        pContext->TransitionResourceStates(1, &Barrier);

        pContext->Flush();
        pContext->WaitForIdle();

        pTestingSwapChain->TakeSnapshot(sm_Resources.pRT);

        pContext->InvalidateState();
    }

    struct Resources
    {
        RefCntAutoPtr<IPipelineState>         pPSO;
        RefCntAutoPtr<IShader>                pVS;
        RefCntAutoPtr<IShader>                pPS;
        RefCntAutoPtr<IShaderResourceBinding> pSRB;
        RefCntAutoPtr<IBuffer>                pColorsVB;
        RefCntAutoPtr<ITexture>               pRT;
    };
    static Resources     sm_Resources;
    static FastRandFloat sm_Rnd;
};

RenderTargetTest::Resources RenderTargetTest::sm_Resources;
FastRandFloat               RenderTargetTest::sm_Rnd{31, 0.f, 1.f};

TEST_F(RenderTargetTest, RenderTargetWriteMask)
{
    GPUTestingEnvironment* pEnv       = GPUTestingEnvironment::GetInstance();
    IRenderDevice*         pDevice    = pEnv->GetDevice();
    IDeviceContext*        pContext   = pEnv->GetDeviceContext();
    ISwapChain*            pSwapChain = pEnv->GetSwapChain();
    const SwapChainDesc&   SCDesc     = pSwapChain->GetDesc();

    for (COLOR_MASK Mask : {COLOR_MASK_RED, COLOR_MASK_GREEN, COLOR_MASK_BLUE, COLOR_MASK_ALPHA, COLOR_MASK_ALL})
    {
        float4 ClearColor{sm_Rnd(), sm_Rnd(), sm_Rnd(), sm_Rnd()};

        RenderReference(Mask, ClearColor);

        ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
        pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->ClearRenderTarget(pRTVs[0], ClearColor.Data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        RefCntAutoPtr<IPipelineState> pPSO;
        {
            GraphicsPipelineStateCreateInfoX PSOCreateInfo{"RenderTargetTest.RenderTargetWriteMask"};

            InputLayoutDescX InputLayout{{0u, 0u, 4u, VT_FLOAT32}};

            PSOCreateInfo
                .AddRenderTarget(SCDesc.ColorBufferFormat)
                .SetPrimitiveTopology(PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .SetInputLayout(InputLayout)
                .AddShader(sm_Resources.pVS)
                .AddShader(sm_Resources.pPS);
            PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode                          = CULL_MODE_NONE;
            PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable                     = False;
            PSOCreateInfo.GraphicsPipeline.BlendDesc.RenderTargets[0].RenderTargetWriteMask = Mask;

            pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
            ASSERT_NE(pPSO, nullptr);
        }

        IBuffer* pVBs[] = {sm_Resources.pColorsVB};
        pContext->SetVertexBuffers(0, _countof(pVBs), pVBs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(sm_Resources.pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->Draw({6, DRAW_FLAG_VERIFY_ALL});

        pSwapChain->Present();
    }
}

} // namespace
