#include "system-uri-scheme.h"

static void
system_scheme_request_cb (WebKitURISchemeRequest *request, gpointer user_data)
{
  const char *uri = NULL;
  uid_t uid = -1;
  g_autofree gchar *sock_path = NULL;
  g_autoptr (GUnixSocketAddress) address = NULL;
  g_autoptr (GSocketClient) client = NULL;
  g_autoptr (GSocketConnection) connection = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GOutputStream) output = NULL;
  g_autoptr (GInputStream) input = NULL;
  g_autoptr (GInputStream) mem_stream = NULL;

  uri = webkit_uri_scheme_request_get_uri (request);
  uid = getuid ();

  sock_path = g_strdup_printf ("/run/user/%d/runtime.sock", uid);
  address = G_UNIX_SOCKET_ADDRESS (g_unix_socket_address_new (sock_path));

  client = g_socket_client_new ();
  connection = g_socket_client_connect (client, G_SOCKET_CONNECTABLE (address),
                                        NULL, &error);

  if (!connection)
    {
      webkit_uri_scheme_request_finish_error (request, error);
      return;
    }

  output = g_io_stream_get_output_stream (G_IO_STREAM (connection));
  input = g_io_stream_get_input_stream (G_IO_STREAM (connection));

  g_output_stream_write (output, uri, strlen (uri), NULL, &error);
  g_output_stream_write (output, "\n", 1, NULL, &error);
  g_output_stream_flush (output, NULL, NULL);

  if (error)
    {
      webkit_uri_scheme_request_finish_error (request, error);
      return;
    }

  char buffer[65536];
  gssize len
      = g_input_stream_read (input, buffer, sizeof (buffer) - 1, NULL, &error);

  if (len <= 0 || error)
    {
      if (!error)
        {
          error
              = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, "Failed");
        }
      webkit_uri_scheme_request_finish_error (request, error);
      return;
    }

  buffer[len] = '\0';
  const char *mime = "text/plain";
  if (g_str_has_prefix (buffer, "<!DOCTYPE html")
      || g_str_has_prefix (buffer, "<html"))
    mime = "text/html";
  else if (g_str_has_prefix (buffer, "{") || g_str_has_prefix (buffer, "["))
    mime = "application/json";

  mem_stream
      = g_memory_input_stream_new_from_data (g_strdup (buffer), len, g_free);
  webkit_uri_scheme_request_finish (request, mem_stream, len, mime);
}

void
system_uri_scheme_register (WebKitWebContext *context)
{
  webkit_web_context_register_uri_scheme (
      context, "system", system_scheme_request_cb, NULL, NULL);
}