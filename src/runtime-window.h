#pragma once

#include <gtk/gtk.h>

#define TYPE_RUNTIME_WINDOW (runtime_window_get_type ())
G_DECLARE_FINAL_TYPE (RuntimeWindow, runtime_window, RUNTIME, WINDOW,
                      GtkApplicationWindow)

RuntimeWindow *runtime_window_new (GtkApplication *app, const char *uri);