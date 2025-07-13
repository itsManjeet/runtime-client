#include "runtime-window.h"
#include "app-resources.h"
#include "app-uri-scheme.h"
#include "system-uri-scheme.h"
#include <unistd.h>
#include <webkit2/webkit2.h>

struct _RuntimeWindow
{
  GtkApplicationWindow parent_instance;
  WebKitWebView *webview;
};

G_DEFINE_TYPE (RuntimeWindow, runtime_window, GTK_TYPE_APPLICATION_WINDOW)

static void
on_title_changed (WebKitWebView *webview, GParamSpec *pspec,
                  gpointer user_data)
{
  RuntimeWindow *self = RUNTIME_WINDOW (user_data);
  const gchar *title = webkit_web_view_get_title (webview);
  gtk_window_set_title (GTK_WINDOW (self), title ? title : "");
}

static void
on_favicon_changed (WebKitWebView *webview, GParamSpec *pspec,
                    gpointer user_data)
{
  RuntimeWindow *self = RUNTIME_WINDOW (user_data);
  cairo_surface_t *icon = webkit_web_view_get_favicon (webview);
  if (icon)
    {
      int w = cairo_image_surface_get_width (icon);
      int h = cairo_image_surface_get_height (icon);
      GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface (icon, 0, 0, w, h);
      gtk_window_set_icon (GTK_WINDOW (self), pixbuf);
    }
}

static void
inject_global_css_js (RuntimeWindow *self)
{
  WebKitUserContentManager *user_content_manager
      = webkit_web_view_get_user_content_manager (self->webview);

  g_autoptr (GBytes) css_data
      = g_resource_lookup_data (app_get_resource (), "/app/style/global.css",
                                G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  if (css_data)
    {
      gsize len = 0;
      const gchar *css_str = g_bytes_get_data (css_data, &len);
      WebKitUserStyleSheet *style = webkit_user_style_sheet_new (
          css_str, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_USER, NULL, NULL);
      webkit_user_content_manager_add_style_sheet (user_content_manager,
                                                   style);
    }

  g_autoptr (GBytes) js_data
      = g_resource_lookup_data (app_get_resource (), "/app/js/global.js",
                                G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  if (js_data)
    {
      gsize len = 0;
      const gchar *js_str = g_bytes_get_data (js_data, &len);
      WebKitUserScript *script = webkit_user_script_new (
          js_str, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_USER, NULL, NULL);
      webkit_user_content_manager_add_script (user_content_manager, script);
    }
}

RuntimeWindow *
runtime_window_new (GtkApplication *app, const char *uri)
{
  RuntimeWindow *self
      = g_object_new (TYPE_RUNTIME_WINDOW, "application", app, NULL);

  gtk_window_set_default_size (GTK_WINDOW (self), 800, 600);

  GtkWidget *header = gtk_header_bar_new ();
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), TRUE);
  gtk_window_set_titlebar (GTK_WINDOW (self), header);

  self->webview = WEBKIT_WEB_VIEW (webkit_web_view_new ());
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->webview));

  WebKitWebContext *context = webkit_web_view_get_context (self->webview);
  app_uri_scheme_register (context);
  system_uri_scheme_register (context);

  inject_global_css_js (self);

  WebKitSettings *settings = webkit_web_view_get_settings (self->webview);
  webkit_settings_set_enable_site_specific_quirks (settings, TRUE);

  g_signal_connect (self->webview, "notify::title",
                    G_CALLBACK (on_title_changed), self);
  g_signal_connect (self->webview, "notify::favicon",
                    G_CALLBACK (on_favicon_changed), self);
  webkit_settings_set_enable_write_console_messages_to_stdout (settings, TRUE);
  webkit_web_view_load_uri (self->webview, uri);

  gtk_widget_show_all (GTK_WIDGET (self));
  return self;
}

static void
runtime_window_class_init (RuntimeWindowClass *klass)
{
}
static void
runtime_window_init (RuntimeWindow *self)
{
}
