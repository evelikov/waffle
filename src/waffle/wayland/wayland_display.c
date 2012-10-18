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

#define WL_EGL_PLATFORM 1

#include <stdlib.h>
#include <string.h>

#include <wayland-client.h>
#undef container_of

#include "waffle/core/wcore_error.h"
#include "waffle/core/wcore_display.h"

#include "wayland_display.h"
#include "wayland_platform.h"
#include "wayland_priv_egl.h"

static const struct wcore_display_vtbl wayland_display_wcore_vtbl;

static bool
wayland_display_destroy(struct wcore_display *wc_self)
{
    struct wayland_display *self = wayland_display(wc_self);
    bool ok = true;

    if (!self)
        return ok;

    if (self->egl)
        ok &= egl_terminate(self->egl);

    if (self->wl_display)
        wl_display_disconnect(self->wl_display);

    ok &= wcore_display_teardown(&self->wcore);
    free(self);
    return ok;
}

static void
wayland_display_listener(void *data,
                         struct wl_registry *registry,
                         uint32_t name,
                         const char *interface,
                         uint32_t version)
{
    struct wayland_display *self = data;

    if (!strncmp(interface, "wl_compositor", 14)) {
        self->wl_compositor = wl_registry_bind(self->wl_registry, name,
                                               &wl_compositor_interface, 1);
    }
    else if (!strncmp(interface, "wl_shell", 9)) {
        self->wl_shell = wl_registry_bind(self->wl_registry, name,
                                          &wl_shell_interface, 1);
    }
}

static const struct wl_registry_listener registry_listener = {
                 .global = wayland_display_listener,
                 .global_remove = NULL,
};

struct wcore_display*
wayland_display_connect(struct wcore_platform *wc_plat,
                        const char *name)
{
    struct wayland_display *self;
    bool ok = true;
    int error = 0;

    self = wcore_calloc(sizeof(*self));
    if (self == NULL)
        return NULL;

    ok = wcore_display_init(&self->wcore, wc_plat);
    if (!ok)
        goto error;

    self->wl_display = wl_display_connect(name);
    if (!self->wl_display) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN, "wl_display_connect failed");
        goto error;
    }

    self->wl_registry = wl_display_get_registry(self->wl_display);
    if (!self->wl_registry) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN, "wl_display_get_registry failed");
        goto error;
    }

    error = wl_registry_add_listener(self->wl_registry,
                                     &registry_listener,
                                     self);
    if (error < 0) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN, "wl_registry_add_listener failed");
        goto error;
    }

    error = wl_display_dispatch(self->wl_display);
    if (error < 0) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN, "wl_display_dispatch failed");
        goto error;
    }

    self->egl = wayland_egl_initialize(self->wl_display);
    if (!self->egl)
        goto error;

    self->wcore.vtbl = &wayland_display_wcore_vtbl;
    return &self->wcore;

error:
    wayland_display_destroy(&self->wcore);
    return NULL;
}


static bool
wayland_display_supports_context_api(struct wcore_display *wc_self,
                                     int32_t waffle_context_api)
{
    return egl_supports_context_api(wc_self->platform, waffle_context_api);
}

void
wayland_display_fill_native(struct wayland_display *self,
                            struct waffle_wayland_display *n_dpy)
{
    n_dpy->wl_display = self->wl_display;
    n_dpy->wl_compositor = self->wl_compositor;
    n_dpy->wl_shell = self->wl_shell;
    n_dpy->egl_display = self->egl;
}

static union waffle_native_display*
wayland_display_get_native(struct wcore_display *wc_self)
{
    struct wayland_display *self = wayland_display(wc_self);
    union waffle_native_display *n_dpy;

    WCORE_CREATE_NATIVE_UNION(n_dpy, wayland);
    if (!n_dpy)
        return NULL;

    wayland_display_fill_native(self, n_dpy->wayland);

    return n_dpy;
}

static const struct wcore_display_vtbl wayland_display_wcore_vtbl = {
    .destroy = wayland_display_destroy,
    .get_native = wayland_display_get_native,
    .supports_context_api = wayland_display_supports_context_api,
};
