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
  GtkWidget *address_bar;
};

G_DEFINE_TYPE (RuntimeWindow, runtime_window, GTK_TYPE_APPLICATION_WINDOW)

static void
on_uri_changed (WebKitWebView *webview, GParamSpec *pspec, gpointer user_data)
{
  RuntimeWindow *self = RUNTIME_WINDOW (user_data);
  const gchar *uri = webkit_web_view_get_uri (self->webview);
  gtk_entry_set_text (GTK_ENTRY (self->address_bar), uri ? uri : "");
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

static void
go_uri (GtkWidget *widget, gpointer user_data)
{
  RuntimeWindow *self = RUNTIME_WINDOW (user_data);
  const gchar *uri = gtk_entry_get_text (GTK_ENTRY (self->address_bar));
  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (self->webview), uri);
}

static void
go_back (GtkWidget *widget, gpointer user_data)
{
  RuntimeWindow *self = RUNTIME_WINDOW (user_data);
  webkit_web_view_go_back (self->webview);
}

static void
go_forward (GtkWidget *widget, gpointer user_data)
{
  RuntimeWindow *self = RUNTIME_WINDOW (user_data);
  webkit_web_view_go_forward (self->webview);
}

static void
go_refresh (GtkWidget *widget, gpointer user_data)
{
  RuntimeWindow *self = RUNTIME_WINDOW (user_data);
  webkit_web_view_reload (self->webview);
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

  GtkWidget *back_button
      = gtk_button_new_from_icon_name ("go-previous", GTK_ICON_SIZE_BUTTON);
  g_signal_connect (back_button, "clicked", G_CALLBACK (go_back), self);
  gtk_header_bar_pack_start (GTK_HEADER_BAR (header), back_button);

  GtkWidget *next_button
      = gtk_button_new_from_icon_name ("go-next", GTK_ICON_SIZE_BUTTON);
  g_signal_connect (next_button, "clicked", G_CALLBACK (go_forward), self);
  gtk_header_bar_pack_start (GTK_HEADER_BAR (header), next_button);

  GtkWidget *reload_button
      = gtk_button_new_from_icon_name ("view-refresh", GTK_ICON_SIZE_BUTTON);
  g_signal_connect (reload_button, "clicked", G_CALLBACK (go_refresh), self);
  gtk_header_bar_pack_start (GTK_HEADER_BAR (header), reload_button);

  self->address_bar = gtk_entry_new ();
  gtk_widget_set_hexpand (self->address_bar, TRUE);
  g_signal_connect (self->address_bar, "activate", G_CALLBACK (go_uri), self);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (self->address_bar),
                                     GTK_ICON_SIZE_BUTTON, "view-refresh");
  gtk_header_bar_set_custom_title (GTK_HEADER_BAR (header), self->address_bar);

  GtkWidget *pref
      = gtk_button_new_from_icon_name ("open-menu", GTK_ICON_SIZE_BUTTON);
  gtk_header_bar_pack_end (GTK_HEADER_BAR (header), pref);

  self->webview = WEBKIT_WEB_VIEW (webkit_web_view_new ());
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->webview));

  WebKitWebContext *context = webkit_web_view_get_context (self->webview);
  app_uri_scheme_register (context);
  system_uri_scheme_register (context);

  inject_global_css_js (self);

  WebKitSettings *settings = webkit_web_view_get_settings (self->webview);
  webkit_settings_set_enable_site_specific_quirks (settings, TRUE);

  g_signal_connect (self->webview, "notify::uri", G_CALLBACK (on_uri_changed),
                    self);
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
