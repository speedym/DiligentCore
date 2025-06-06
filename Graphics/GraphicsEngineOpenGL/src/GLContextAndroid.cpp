/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include "pch.h"
#include <utility>
#include <vector>
#include <cstring>

#include "GLContextAndroid.hpp"

#if DILIGENT_USE_OPENXR
#    include "OpenXR_GLHelpers.hpp"
#endif

#ifndef EGL_CONTEXT_MINOR_VERSION_KHR
#    define EGL_CONTEXT_MINOR_VERSION_KHR 0x30FB
#endif

#ifndef EGL_CONTEXT_MAJOR_VERSION_KHR
#    define EGL_CONTEXT_MAJOR_VERSION_KHR EGL_CONTEXT_CLIENT_VERSION
#endif

namespace Diligent
{

bool GLContext::InitEGLDisplay()
{
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY)
    {
        LOG_ERROR_AND_THROW("No EGL display found");
    }
    return true;
}

bool GLContext::InitEGLSurface()
{
    if (!window_)
    {
        LOG_ERROR_AND_THROW("Native window pointer must be passed to EngineCreateInfo for Diligent to make EGL Surface");
    }

    auto success = eglInitialize(display_, &egl_major_version_, &egl_minor_version_);
    if (!success)
    {
        LOG_ERROR_AND_THROW("Failed to initialise EGL");
    }
    LOG_INFO_MESSAGE("Initialized EGL ", egl_major_version_, '.', egl_minor_version_);

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    color_size_ = 8;
    depth_size_ = 24;
    const EGLint attribs[] =
        {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, //Request opengl ES2.0
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            //EGL_COLORSPACE, EGL_COLORSPACE_sRGB, // does not work
            EGL_BLUE_SIZE, color_size_,
            EGL_GREEN_SIZE, color_size_,
            EGL_RED_SIZE, color_size_,
            EGL_ALPHA_SIZE, color_size_,
            EGL_DEPTH_SIZE, depth_size_,
            //EGL_SAMPLE_BUFFERS  , 1,
            //EGL_SAMPLES         , 4,
            EGL_NONE //
        };

    // Get a list of EGL frame buffer configurations that match specified attributes
    EGLint num_configs;
    success = eglChooseConfig(display_, attribs, &config_, 1, &num_configs);
    if (!success)
    {
        LOG_ERROR_AND_THROW("Failed to choose config");
    }

    if (!num_configs)
    {
        //Fall back to 16bit depth buffer
        depth_size_ = 16;
        const EGLint attribs[] =
            {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, //Request opengl ES2.0
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_BLUE_SIZE, color_size_,
                EGL_GREEN_SIZE, color_size_,
                EGL_RED_SIZE, color_size_,
                EGL_ALPHA_SIZE, color_size_,
                EGL_DEPTH_SIZE, depth_size_,
                EGL_NONE //
            };
        success = eglChooseConfig(display_, attribs, &config_, 1, &num_configs);
        if (!success)
        {
            LOG_ERROR_AND_THROW("Failed to choose 16-bit depth config");
        }
    }

    if (!num_configs)
    {
        LOG_ERROR_AND_THROW("Unable to retrieve EGL config");
    }

    LOG_INFO_MESSAGE("Chosen EGL config: ", color_size_, " bit color, ", depth_size_, " bit depth");

    surface_ = eglCreateWindowSurface(display_, config_, window_, NULL);
    if (surface_ == EGL_NO_SURFACE)
    {
        LOG_ERROR_AND_THROW("Failed to create EGLSurface");
    }

    UpdateScreenSize();

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    EGLint format;
    eglGetConfigAttrib(display_, config_, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(window_, 0, 0, format);

    eglGetConfigAttrib(display_, config_, EGL_MIN_SWAP_INTERVAL, &min_swap_interval_);
    eglGetConfigAttrib(display_, config_, EGL_MAX_SWAP_INTERVAL, &max_swap_interval_);

    return true;
}

bool GLContext::InitEGLContext()
{
#if DILIGENT_USE_OPENXR
    Version OpenXRRequiredGLESVersion;
    if (openxr_attribs_)
    {
        OpenXRRequiredGLESVersion = GetOpenXRRequiredGLVersion(openxr_attribs_.get());
    }
#endif

    std::pair<int, int> es_versions[] = {{3, 2}, {3, 1}, {3, 0}};
    for (size_t i = 0; i < _countof(es_versions) && context_ == EGL_NO_CONTEXT; ++i)
    {
        const auto& version = es_versions[i];
        major_version_      = version.first;
        minor_version_      = version.second;

        // clang-format off
        std::vector<EGLint> context_attribs =
        {
            EGL_CONTEXT_MAJOR_VERSION, major_version_,
            EGL_CONTEXT_MINOR_VERSION, minor_version_
        };
        // clang-format on

#if 0
        // No matter what I do, eglCreateContext fails when EGL_CONTEXT_OPENGL_DEBUG attribute
        // is present, even when it is EGL 1.5.
        if (create_debug_context_)
        {
            // EGL_CONTEXT_OPENGL_DEBUG is only valid as of EGL 1.5.
            if (egl_major_version_ >= 2 || (egl_major_version_ == 1 && egl_minor_version_ >= 5))
            {
                context_attribs.push_back(EGL_CONTEXT_OPENGL_DEBUG);
                context_attribs.push_back(EGL_TRUE);
            }
            else
            {
                LOG_WARNING_MESSAGE("EGL_CONTEXT_OPENGL_DEBUG is only available in EGL 1.5+");
            }
        }
#endif
        context_attribs.push_back(EGL_NONE);

        context_ = eglCreateContext(display_, config_, NULL, context_attribs.data());
    }

    if (context_ == EGL_NO_CONTEXT)
    {
        LOG_ERROR_AND_THROW("Failed to create EGLContext");
    }

#if DILIGENT_USE_OPENXR
    if (openxr_attribs_ && OpenXRRequiredGLESVersion > Version{static_cast<Uint32>(major_version_), static_cast<Uint32>(minor_version_)})
    {
        LOG_ERROR("OpenGLES version ", major_version_, '.', minor_version_, " does not meet minimum required version for OpenXR: ",
                  OpenXRRequiredGLESVersion.Major, '.', OpenXRRequiredGLESVersion.Minor);
    }
#endif

    if (eglMakeCurrent(display_, surface_, surface_, context_) == EGL_FALSE)
    {
        LOG_ERROR_AND_THROW("Unable to eglMakeCurrent");
    }

    LOG_INFO_MESSAGE("Created OpenGLES Context ", major_version_, '.', minor_version_);
    return true;
}

void GLContext::AttachToCurrentEGLSurface()
{
    if (eglGetCurrentSurface(EGL_DRAW) == EGL_NO_SURFACE)
    {
        LOG_ERROR_AND_THROW("Failed to attach to EGLSurface: no active surface");
    }
    UpdateScreenSize();
}

void GLContext::AttachToCurrentEGLContext()
{
    if (eglGetCurrentContext() == EGL_NO_CONTEXT)
    {
        LOG_ERROR_AND_THROW("Failed to attach to EGLContext: no active context");
    }
    glGetIntegerv(GL_MAJOR_VERSION, &major_version_);
    glGetIntegerv(GL_MINOR_VERSION, &minor_version_);
}

GLContext::NativeGLContextType GLContext::GetCurrentNativeGLContext()
{
    return eglGetCurrentContext();
}

void GLContext::InitGLES()
{
    if (gles_initialized_)
        return;

    LoadGLFunctions();

    glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    if (glGetError() != GL_NO_ERROR)
        LOG_ERROR_MESSAGE("Failed to enable primitive restart fixed index");

    gles_initialized_ = true;
}

bool GLContext::Init(ANativeWindow* window)
{
    if (egl_context_initialized_)
        return true;

    //
    //Initialize EGL
    //
    window_ = window;
    InitEGLDisplay();

    if (eglGetCurrentSurface(EGL_DRAW) != EGL_NO_SURFACE)
    {
        AttachToCurrentEGLSurface();
    }
    else
    {
        InitEGLSurface();
    }

    if (eglGetCurrentContext() != EGL_NO_CONTEXT)
    {
        AttachToCurrentEGLContext();
    }
    else
    {
        InitEGLContext();
    }

    InitGLES();

    egl_context_initialized_ = true;

    return true;
}

GLContext::GLContext(const EngineGLCreateInfo& InitAttribs,
                     RENDER_DEVICE_TYPE&       DevType,
                     struct Version&           APIVersion,
                     const struct SwapChainDesc* /*pSCDesc*/) :
    display_(EGL_NO_DISPLAY),
    surface_(EGL_NO_SURFACE),
    context_(EGL_NO_CONTEXT),
    //create_debug_context_(InitAttribs.EnableValidation),
    major_version_(0),
    minor_version_(0),
    gles_initialized_(false),
    egl_context_initialized_(false)
{
#if DILIGENT_USE_OPENXR
    if (InitAttribs.pXRAttribs != nullptr && InitAttribs.pXRAttribs->Instance != 0)
    {
        openxr_attribs_ = std::make_unique<OpenXRAttribs>(*InitAttribs.pXRAttribs);
    }
#endif

    auto* NativeWindow = reinterpret_cast<ANativeWindow*>(InitAttribs.Window.pAWindow);
    Init(NativeWindow);

    DevType          = RENDER_DEVICE_TYPE_GLES;
    APIVersion.Major = static_cast<Uint32>(major_version_);
    APIVersion.Minor = static_cast<Uint32>(minor_version_);
}

GLContext::~GLContext()
{
    Terminate();
}

void GLContext::SwapBuffers(int SwapInterval)
{
    EGLSurface CurrSurface = (surface_ == EGL_NO_SURFACE) ? eglGetCurrentSurface(EGL_DRAW) : surface_;

    if (CurrSurface == EGL_NO_SURFACE)
    {
        LOG_WARNING_MESSAGE("No EGL surface when swapping buffers. This happens when SwapBuffers() is called after Suspend(). The operation will be ignored.");
        return;
    }

    SwapInterval = std::max(SwapInterval, min_swap_interval_);
    SwapInterval = std::min(SwapInterval, max_swap_interval_);
    eglSwapInterval(display_, SwapInterval);

    bool b = eglSwapBuffers(display_, CurrSurface);
    if (!b)
    {
        EGLint err = eglGetError();
        if (err == EGL_BAD_SURFACE)
        {
            LOG_INFO_MESSAGE("EGL surface has been lost. Attempting to recreate");
            try
            {
                if (surface_ != EGL_NO_SURFACE)
                {
                    InitEGLSurface();
                }
                else
                {
                    // Attaching to an external surface was already attempted at the beginning
                    LOG_ERROR_AND_THROW("Couldn't attach to an external surface");
                }
            }
            catch (std::runtime_error&)
            {
                LOG_ERROR_MESSAGE("Failed to recreate EGL surface");
            }
            //return EGL_SUCCESS; //Still consider glContext is valid
        }
        else if (err == EGL_CONTEXT_LOST || err == EGL_BAD_CONTEXT)
        {
            //Context has been lost!!
            Terminate();
            if (context_)
            {
                InitEGLContext();
            }
            else
            {
                AttachToCurrentEGLContext();
            }
        }
        //return err;
    }
    //return EGL_SUCCESS;
}

void GLContext::Terminate()
{
    if (context_ != EGL_NO_CONTEXT)
    {
        eglDestroyContext(display_, context_);
    }

    if (surface_ != EGL_NO_SURFACE)
    {
        eglDestroySurface(display_, surface_);
    }

    if (display_ != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(display_);
    }

    display_ = EGL_NO_DISPLAY;
    context_ = EGL_NO_CONTEXT;
    surface_ = EGL_NO_SURFACE;
}


void GLContext::UpdateScreenSize()
{
    EGLSurface CurrSurface = (surface_ == EGL_NO_SURFACE) ? eglGetCurrentSurface(EGL_DRAW) : surface_;

    if (CurrSurface == EGL_NO_SURFACE)
    {
        LOG_ERROR("Failed to attach to EGLSurface: no active surface");
        return;
    }

    int32_t new_screen_width  = 0;
    int32_t new_screen_height = 0;
    eglQuerySurface(display_, CurrSurface, EGL_WIDTH, &new_screen_width);
    eglQuerySurface(display_, CurrSurface, EGL_HEIGHT, &new_screen_height);

    if (new_screen_width != screen_width_ || new_screen_height != screen_height_)
    {
        screen_width_  = new_screen_width;
        screen_height_ = new_screen_height;
        //Screen resized
        LOG_INFO_MESSAGE("Window size changed to ", screen_width_, "x", screen_height_);
    }
}

EGLint GLContext::Resume(ANativeWindow* window)
{
    LOG_INFO_MESSAGE("Resuming gl context\n");

    if (egl_context_initialized_ == false)
    {
        Init(window);
        return EGL_SUCCESS;
    }

    //Create surface
    window_ = window;
    if (surface_ == EGL_NO_SURFACE && eglGetCurrentSurface(EGL_DRAW) != EGL_NO_SURFACE)
    {
        AttachToCurrentEGLSurface();
        return EGL_SUCCESS;
    }
    else
    {
        surface_ = eglCreateWindowSurface(display_, config_, window_, NULL);
    }
    UpdateScreenSize();

    if (eglMakeCurrent(display_, surface_, surface_, context_) == EGL_TRUE)
        return EGL_SUCCESS;

    EGLint err = eglGetError();
    LOG_WARNING_MESSAGE("Unable to eglMakeCurrent ", err, '\n');

    if (err == EGL_CONTEXT_LOST)
    {
        //Recreate context
        LOG_INFO_MESSAGE("Re-creating egl context\n");
        InitEGLContext();
    }
    else
    {
        //Recreate surface
        LOG_INFO_MESSAGE("Re-creating egl context and surface\n");
        Terminate();
        InitEGLSurface();
        InitEGLContext();
    }

    return err;
}

void GLContext::Suspend()
{
    LOG_INFO_MESSAGE("Suspending gl context\n");
    if (surface_ != EGL_NO_SURFACE)
    {
        LOG_INFO_MESSAGE("Destroying egl surface\n");
        eglDestroySurface(display_, surface_);
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_);
        surface_ = EGL_NO_SURFACE;
    }
}

bool GLContext::Invalidate()
{
    LOG_INFO_MESSAGE("Invalidating gl context\n");
    Terminate();

    egl_context_initialized_ = false;
    return true;
}

} // namespace Diligent
