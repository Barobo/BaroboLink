#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#define PLAT_GTK 1
#define GTK
#include <Scintilla.h>
#include <SciLexer.h>
#include <ScintillaWidget.h>
#include "RoboMancer.h"
#include "RobotManager.h"
#ifdef __MACH__
#include <sys/types.h>
#include <unistd.h>
#include <gtk-mac-integration.h>
#endif
#include <sys/stat.h>

#ifndef _WIN32
#define MAX_PATH 1024
#endif

GtkBuilder *g_builder;
GtkWidget *g_window;
GtkWidget *g_scieditor;
ScintillaObject *g_sci;

CRobotManager *g_robotManager;

char *g_interfaceFiles[512] = {
  "interface/interface.glade",
  "interface.glade",
  "../share/RoboMancer/interface.glade",
  NULL,
  NULL
};

int main(int argc, char* argv[])
{
  GError *error = NULL;
 
  gtk_init(&argc, &argv);

  /* Create the GTK Builder */
  g_builder = gtk_builder_new();

#ifdef __MACH__
  char *datadir = getenv("XDG_DATA_DIRS");
  if(datadir != NULL) {
    g_interfaceFiles[3] = (char*)malloc(sizeof(char)*512);
    sprintf(g_interfaceFiles[3], "%s/RoboMancer/interface.glade", datadir);
  }
#endif

  /* Load the UI */
  /* Find ther interface file */
  struct stat s;
  int err;
  int i;
  for(i = 0; g_interfaceFiles[i] != NULL; i++) {
    err = stat(g_interfaceFiles[i], &s);
    if(err == 0) {
      if( ! gtk_builder_add_from_file(g_builder, g_interfaceFiles[i], &error) )
      {
        g_warning("%s", error->message);
        //g_free(error);
        return -1;
      } else {
        break;
      }
    }
  }

  if(g_interfaceFiles[i] == NULL) {
    /* Could not find the interface file */
    g_warning("Could not find interface file.");
    return -1;
  }
  /* Initialize the subsystem */
  initialize();

  /* Get the main window */
  g_window = GTK_WIDGET( gtk_builder_get_object(g_builder, "window1"));
  /* Connect signals */
  gtk_builder_connect_signals(g_builder, NULL);

#ifdef __MACH__
  //g_signal_connect(GtkOSXMacmenu, "NSApplicationBlockTermination",
      //G_CALLBACK(app_should_quit_cb), NULL);
  GtkWidget* quititem = GTK_WIDGET(gtk_builder_get_object(g_builder, "imagemenuitem5"));
  gtk_mac_menu_set_quit_menu_item(GTK_MENU_ITEM(quititem));
#endif

  /* Show the window */
  gtk_widget_show(g_window);
  gtk_main();
  return 0;
}

void initialize()
{
  g_robotManager = new CRobotManager();
  /* Read the configuration file */
  g_robotManager->read( Mobot_getConfigFilePath() );

  refreshConnectDialog();
  //g_timeout_add(1000, connectDialogPulse, NULL);

  g_notebookRoot = GTK_NOTEBOOK(gtk_builder_get_object(g_builder, "notebook_root"));
  g_reflashConnectSpinner = GTK_SPINNER(gtk_builder_get_object(g_builder, "spinner_reflashConnect"));
  initControlDialog();
  initProgramDialog();
  initializeComms();
}

int getIterModelFromTreeSelection(GtkTreeView *treeView, GtkTreeModel **model, GtkTreeIter *iter)
{
  GtkTreeSelection *treeSelection;

  treeSelection = gtk_tree_view_get_selection( treeView );
  gtk_tree_selection_set_mode(treeSelection, GTK_SELECTION_BROWSE);

  bool success =
    gtk_tree_selection_get_selected( treeSelection, model, iter );
  if(!success) {
    return -1;
  }
  return 0;
}

int getChHome(char *chhome)
{
  int retval = 0;
#if defined(_WIN32)
  /* the following code will work for 9x/NT */
  DWORD chHomeDirLen = MAX_PATH;
  HKEY hkResult, hStartKey = HKEY_LOCAL_MACHINE;
  LONG nResult;
  LONG NTversionLen = 128;
  unsigned char NTversion[128];
#endif

/* CHHOME set in registry Windows 95/98, during installation */
#if defined(_WIN32)
  if(!getenv("CHHOME")) 
  {
    /* the following code will work for 9x/NT */
#if defined(_WIN64)
      nResult = RegOpenKeyEx(hStartKey, "SOFTWARE\\SoftIntegration", 0L, KEY_READ | KEY_WOW64_32KEY, &hkResult);
#else
      nResult = RegOpenKeyEx(hStartKey, "SOFTWARE\\SoftIntegration", 0L, KEY_READ, &hkResult);
#endif
    if (ERROR_SUCCESS == nResult)
    {
       nResult = RegQueryValueEx(hkResult, "CHHOME", 0 ,0, 
                                 (unsigned char*)chhome, &chHomeDirLen);
       if (ERROR_SUCCESS != nResult) { /* should not reach here */
/*
         fprintf(stderr, "ERROR: register value CHHOME cannot be obtained from registry\n");
*/
         retval = -1;
       }
       RegCloseKey(hkResult);
    }
  }
  else
    strcpy(chhome, getenv("CHHOME"));
#else /* For Unix, set CHHOME */
  if(getenv("CHHOME")) {
    strcpy(chhome, getenv("CHHOME"));
  }
  else if(!access("/usr/ch",F_OK)) {
    if(!access("/usr/ch",F_OK)) {
      strcpy(chhome, "/usr/ch");
    }
    else {
      fprintf(stderr, "ERROR: /usr/ch is not readable\n");
      retval = -1;
    }
  }
  else if(!access("/usr/local/ch",F_OK)) {
    if(!access("/usr/local/ch",F_OK)) {
      strcpy(chhome, "/usr/local/ch");
    }
    else {
      fprintf(stderr, "ERROR: /usr/local/ch is not readable\n");
      retval = -1;
    }
  }
  else if(!access("/opt/ch",F_OK)) {
    if(!access("/opt/ch",F_OK)) {
      strcpy(chhome, "/opt/ch");
    }
    else {
      fprintf(stderr, "ERROR: /opt/ch is not readable\n");
      retval = -1;
    }
  }
  else {
    fprintf(stderr, "ERROR: ch has not been installed in /usr/ch, /usr/local/ch, or /opt/ch\n");
    fprintf(stderr, "       Setup environmental variable CHHOME to the directory where Ch is installed\n");
    fprintf(stderr, "       In C-shell, add the code below to ~/.cshrc\n"
                    "           setenv CHHOME /where/Ch/installed/dir\n");
    fprintf(stderr, "       In Bourne-shell, Korn shell, BASH add the code below to ~/.profile\n"
                    "            CHHOME=/where/Ch/installed/dir\n"
                    "            export CHHOME\n");

    fprintf(stderr, "       you may also set it using 'option.chhome' in Embedded Ch API Ch_Initialize2(&interp, option)\n");
    retval = -1;
  }
#endif
  return retval;
}

void on_menuitem_demoPrograms_activate(GtkWidget *w, gpointer data)
{
  /* Start ChIDE with Mobot demos */
  char *command;
  char *chHome;
  command = (char*)malloc(sizeof(char)*1024);
  chHome = (char*)malloc(sizeof(char)*1024);
  getChHome(chHome);
#ifdef _MSYS
  sprintf(command, "%s\\bin\\chide.exe -d %s\\package\\chmobot\\demos %s\\package\\chmobot\\demos\\start.ch",
      chHome, chHome, chHome);
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  CreateProcessA(
	  NULL,
	  command,
      NULL,
      NULL,
      FALSE,
      0,
      NULL,
      NULL,
      &si,
      &pi );
#else
  sprintf(command, "%s/bin/chide -d %s/package/chmobot/demos %s/package/chmobot/demos/start.ch",
      chHome, chHome, chHome);
  if(fork() == 0) {
    system(command);
    exit(0);
  }
#endif
}

void on_menuitem_help_activate(GtkWidget *w, gpointer data)
{
  char chHome[MAX_PATH];
  char command[MAX_PATH];
  getChHome(chHome);
#ifdef _MSYS
  sprintf(command, "%s\\package\\chmobot\\docs\\index.html", chHome);
  ShellExecuteA(
      NULL,
      "open",
      command,
      NULL,
      NULL,
      0);
#elif defined (__MACH__)
  sprintf(command, "open %s/package/chmobot/docs/index.html", chHome);
  system(command); 
#endif
}

void on_imagemenuitem_about_activate(GtkWidget *widget, gpointer data)
{
  /* Find the about dialog and show it */
  GtkWidget *w;
  w = GTK_WIDGET(gtk_builder_get_object(g_builder, "aboutdialog"));
  gtk_dialog_run(GTK_DIALOG(w));
}

void on_aboutdialog_activate_link(GtkAboutDialog *label, gchar* uri, gpointer data)
{
#ifdef _MSYS
  ShellExecuteA(
      NULL,
      "open",
      uri,
      NULL,
      NULL,
      0);
#elif defined (__MACH__) 
  char command[MAX_PATH];
  sprintf(command, "open %s", uri);
  system(command); 
#endif
}

void on_aboutdialog_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
  gtk_widget_hide(GTK_WIDGET(dialog));
}

void on_aboutdialog_close(GtkDialog *dialog, gpointer user_data)
{
  gtk_widget_hide(GTK_WIDGET(dialog));
}

double normalizeAngleRad(double radians)
{
  while(radians > M_PI) {
    radians -= 2*M_PI;
  }
  while(radians < -M_PI) {
    radians += 2*M_PI;
  }
  return radians;
}
