#include "app-resources.h"
#include "runtime-window.h"

static void
activate (GtkApplication *app, gpointer user_data)
{
  runtime_window_new (app, "app://dev.rlxos.store");
}

static void
open (GtkApplication *app, GFile **files, int n_files, const char *hint)
{
  for (int i = 0; i < n_files; i++)
    {
      char *uri = g_file_get_uri (files[i]);
      runtime_window_new (app, uri);
      g_free (uri);
    }
}

int
main (int argc, char *argv[])
{
  g_autoptr (GtkApplication) app
      = gtk_application_new ("dev.rlxos.runtime", G_APPLICATION_HANDLES_OPEN);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  g_signal_connect (app, "open", G_CALLBACK (open), NULL);

  g_resources_register (app_get_resource ());

  return g_application_run (G_APPLICATION (app), argc, argv);
}
