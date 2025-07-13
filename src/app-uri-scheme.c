#include "app-uri-scheme.h"

static void
app_scheme_request_cb (WebKitURISchemeRequest *request, gpointer user_data)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GBytes) content = NULL;
  g_autofree char *app_path = NULL;
  g_autofree char *file_path = NULL;

  const char *uri = webkit_uri_scheme_request_get_uri (request);
  const char *path = uri + strlen ("app://");

  if (strstr (path, ".."))
    {
      error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                                   "Can't access backward path");
      webkit_uri_scheme_request_finish_error (request, error);
      return;
    }

  app_path = g_strdup_printf ("/apps/%s", path);

  if (g_file_test (app_path, G_FILE_TEST_IS_DIR)
      || path[strlen (path) - 1] == '/')
    {
      file_path = g_strconcat (app_path, "/index.html", NULL);
    }
  else
    {
      file_path = g_strdup (app_path);
    }

  if (g_file_test (file_path, G_FILE_TEST_IS_REGULAR))
    {
      gchar *data = NULL;
      gsize size = 0;
      if (g_file_get_contents (file_path, &data, &size, &error))
        {
          content = g_bytes_new_take (data, size);
        }
    }
  else
    {
      error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                                   "File not found");
    }

  const gchar *mime_type = g_content_type_guess (file_path, NULL, 0, NULL);
  gchar *real_mime = g_content_type_get_mime_type (mime_type);

  if (content)
    {
      webkit_uri_scheme_request_finish (
          request, g_memory_input_stream_new_from_bytes (content),
          g_bytes_get_size (content), real_mime);
    }
  else
    {
      webkit_uri_scheme_request_finish_error (request, error);
    }
}

void
app_uri_scheme_register (WebKitWebContext *context)
{
  webkit_web_context_register_uri_scheme (context, "app",
                                          app_scheme_request_cb, NULL, NULL);
}
