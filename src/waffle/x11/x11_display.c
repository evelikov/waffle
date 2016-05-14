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

#include "wcore_error.h"

#include "x11_display.h"
#include "x11_wrappers.h"

bool
x11_display_init(struct x11_display *self, const char *name)
{
    assert(self);

    self->xlib = wrapped_XOpenDisplay(name);
    if (!self->xlib) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN, "XOpenDisplay failed");
        return false;
    }

    self->xcb = wrapped_XGetXCBConnection(self->xlib);
    if (!self->xcb) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN, "XGetXCBConnection failed");
        wrapped_XCloseDisplay(self->xlib);
        return false;
    }

    self->screen = DefaultScreen(self->xlib);

    return true;
}

bool
x11_display_teardown(struct x11_display *self)
{
    int error = 0;

    if (!self->xlib)
       return !error;

    error = wrapped_XCloseDisplay(self->xlib);
    if (error)
        wcore_errorf(WAFFLE_ERROR_UNKNOWN, "XCloseDisplay failed");

    return !error;
}
