/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.hh"

int main (int argc, char *argv[])
{

  g_thread_init(NULL);
  gdk_threads_init();
  
  gdk_threads_enter();
  gtk_set_locale ();
  gtk_init (&argc, &argv);

  // scroom = create_scroom ();
  // gtk_widget_show (scroom);

  on_scroom_bootstrap();
  create_scroom(NULL);
  
  gtk_main ();
  gdk_threads_leave();
  return 0;
}

