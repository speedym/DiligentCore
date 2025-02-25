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

#pragma once

#include <string>

#ifndef GLEW_STATIC
#    define GLEW_STATIC // Must be defined to use static version of glew
#endif
#ifndef GLEW_NO_GLU
#    define GLEW_NO_GLU
#endif

#ifdef PLATFORM_WEB
#    include <GLES3/gl32.h>
#    include "../../Graphics/GraphicsEngineOpenGL/include/GLStubsEmscripten.h"
#else
#    include "GL/glew.h"
#endif

#include "GPUTestingEnvironment.hpp"

namespace Diligent
{

namespace Testing
{

class TestingEnvironmentGL final : public GPUTestingEnvironment
{
public:
    using CreateInfo = GPUTestingEnvironment::CreateInfo;
    TestingEnvironmentGL(const CreateInfo&    CI,
                         const SwapChainDesc& SCDesc);
    ~TestingEnvironmentGL();

    static TestingEnvironmentGL* GetInstance() { return ClassPtrCast<TestingEnvironmentGL>(GPUTestingEnvironment::GetInstance()); }

    GLuint CompileGLShader(const std::string& Source, GLenum ShaderType);
    GLuint LinkProgram(GLuint Shaders[], GLuint NumShaders);

    GLuint GetDummyVAO() { return m_DummyVAO; }

    virtual void Reset() override final;

private:
    GLuint m_DummyVAO = 0;
};

} // namespace Testing

} // namespace Diligent
