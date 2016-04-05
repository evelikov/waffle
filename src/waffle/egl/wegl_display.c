// Copyright 2012 Intel Corporation
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <assert.h>
#include <string.h>

#include "wcore_error.h"
#include "wcore_platform.h"

#include "wegl_display.h"
#include "wegl_imports.h"
#include "wegl_util.h"
#include "wegl_platform.h"

static bool
parse_version_extensions(struct wegl_display *dpy, EGLint major, EGLint minor)
{
    struct wegl_platform *plat = wegl_platform(dpy->wcore.platform);
    const char *apis = plat->eglQueryString(dpy->egl, EGL_CLIENT_APIS);
    const char *extensions;

    // Our minimum requirement - EGL 1.2 ...
    if (major != 1 || minor < 2) {
        wcore_errorf(WAFFLE_ERROR_UNSUPPORTED_ON_PLATFORM,
                     "EGL 1.2 or later is required");
        return false;
    }

    // ... plus working eglQueryString(EGL_CLIENT_APIS) and "OpenGL_ES" in the
    // APIs string.
    if (!apis || !strstr(apis, "OpenGL_ES")) {
        wegl_emit_error(plat, "eglQueryString(EGL_CLIENT_APIS)");
        return false;
    }

    // Optional bits, if we're running EGL 1.4 ...
    if (major == 1 && minor >= 4) {
        extensions = plat->eglQueryString(dpy->egl, EGL_EXTENSIONS);

        // Should never fail.
        if (!extensions) {
            wegl_emit_error(plat, "eglQueryString(EGL_EXTENSIONS)");
            return false;
        }

        // waffle_is_extension_in_string() resets the error state. That's ok,
        // however, because if we've reached this point then no error should be
        // pending emission.
        assert(wcore_error_get_code() == 0);

        dpy->KHR_create_context = waffle_is_extension_in_string(extensions, "EGL_KHR_create_context");

        // ... and OpenGL is in the APIs string, then we should be fine.
        dpy->supports_opengl = waffle_is_extension_in_string(apis, "OpenGL");
    }

    return true;
}

/// On Linux, according to eglplatform.h, EGLNativeDisplayType and intptr_t
/// have the same size regardless of platform.
bool
wegl_display_init(struct wegl_display *dpy,
                  struct wcore_platform *wc_plat,
                  intptr_t native_display)
{
    struct wegl_platform *plat = wegl_platform(wc_plat);
    bool ok;
    EGLint major, minor;

    ok = wcore_display_init(&dpy->wcore, wc_plat);
    if (!ok)
        goto fail;

    dpy->egl = plat->eglGetDisplay((EGLNativeDisplayType) native_display);
    if (!dpy->egl) {
        wegl_emit_error(plat, "eglGetDisplay");
        goto fail;
    }

    ok = plat->eglInitialize(dpy->egl, &major, &minor);
    if (!ok) {
        wegl_emit_error(plat, "eglInitialize");
        goto fail;
    }

    ok = parse_version_extensions(dpy, major, minor);
    if (!ok)
        goto fail;

    return true;

fail:
    wegl_display_teardown(dpy);
    return false;
}

bool
wegl_display_teardown(struct wegl_display *dpy)
{
    struct wegl_platform *plat = wegl_platform(dpy->wcore.platform);
    bool ok = true;

    if (dpy->egl) {
        ok = plat->eglTerminate(dpy->egl);
        if (!ok)
            wegl_emit_error(plat, "eglTerminate");
    }

    return ok;
}

bool
wegl_display_supports_context_api(struct wcore_display *wc_dpy,
                                  int32_t waffle_context_api)
{
    struct wegl_display *dpy = wegl_display(wc_dpy);

    switch (waffle_context_api) {
        case WAFFLE_CONTEXT_OPENGL:
            return dpy->supports_opengl;
        case WAFFLE_CONTEXT_OPENGL_ES1:
        case WAFFLE_CONTEXT_OPENGL_ES2:
            return true;
        case WAFFLE_CONTEXT_OPENGL_ES3:
            return dpy->KHR_create_context;
        default:
            assert(false);
            return false;
    }
}
