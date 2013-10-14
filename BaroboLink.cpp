/*
   Copyright 2013 Barobo, Inc.

   This file is part of BaroboLink.

   BaroboLink is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   BaroboLink is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with BaroboLink.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef _MSYS
/* hlh: This is here to support USB hotplug detection. */
#define WINVER 0x0501   // Tell Windows headers we're targeting WinXP
#endif

#include "split.hpp"

#include <gtk/gtk.h>
#include <cstring>
#include <cassert>

#define PLAT_GTK 1
#define GTK
#include "BaroboLink.h"
#include "RobotManager.h"

#ifdef __MACH__
#include <sys/types.h>
#include <unistd.h>
#include <gtk-mac-integration.h>
#define MAX_PATH 4096
#endif

#include <sys/stat.h>
#include "thread_macros.h"

#ifdef _MSYS
#include "libbarobo/win32_error.h"
#include <windows.h>
#include <dbt.h>
#include <initguid.h>   // must be included before ddk/*, otherwise link error
                        // (because that makes complete sense)
#include <ddk/usbiodef.h>
#include <tchar.h>
#include <gdk/gdkwin32.h>
#endif

GtkBuilder *g_builder;
GtkWidget *g_window;
GtkWidget *g_scieditor;
GtkWidget *g_scieditor_ext;

CRobotManager *g_robotManager;

std::vector<std::string> g_interfaceFiles;

const char *g_interfaceFilesInit[] = {
  "interface/interface.glade",
  "interface.glade",
  "../share/BaroboLink/interface.glade"
};

#ifdef _WIN32
WNDPROC g_oldWindowProc;

static int processDeviceArrival (PDEV_BROADCAST_HDR devhdr) {
  if (DBT_DEVTYP_DEVICEINTERFACE != devhdr->dbch_devicetype) {
    return 0;
  }

  PDEV_BROADCAST_DEVICEINTERFACE device
    = (PDEV_BROADCAST_DEVICEINTERFACE)devhdr;

  _tprintf(_T("%s arrived\n"), device->dbcc_name);
  return 1;
}

static int processDeviceRemoveComplete (PDEV_BROADCAST_HDR devhdr) {
  if (DBT_DEVTYP_DEVICEINTERFACE != devhdr->dbch_devicetype) {
    return 0;
  }

  PDEV_BROADCAST_DEVICEINTERFACE device
    = (PDEV_BROADCAST_DEVICEINTERFACE)devhdr;

  _tprintf(_T("%s removed\n"), device->dbcc_name);
  return 1;
}

static LRESULT CALLBACK windowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  int processed = 0;

  if (WM_DEVICECHANGE == uMsg) {
    switch (wParam) {
      case DBT_CONFIGCHANGECANCELED:
        //printf("DBT_CONFIGCHANGECANCELED\n");
        break;
      case DBT_CONFIGCHANGED:
        //printf("DBT_CONFIGCHANGED\n");
        break;
      case DBT_DEVICEARRIVAL:
        //printf("DBT_DEVICEARRIVAL\n");
        processed = processDeviceArrival((PDEV_BROADCAST_HDR)lParam);
        break;
      case DBT_DEVICEQUERYREMOVE:
        //printf("DBT_DEVICEQUERYREMOVE\n");
        break;
      case DBT_DEVICEQUERYREMOVEFAILED:
        //printf("DBT_DEVICEQUERYREMOVEFAILED\n");
        break;
      case DBT_DEVICEREMOVECOMPLETE:
        //printf("DBT_DEVICEREMOVECOMPLETE\n");
        processed = processDeviceRemoveComplete((PDEV_BROADCAST_HDR)lParam);
        break;
      case DBT_DEVICEREMOVEPENDING:
        //printf("DBT_DEVICEREMOVEPENDING\n");
        break;
      case DBT_DEVICETYPESPECIFIC:
        //printf("DBT_DEVICETYPESPECIFIC\n");
        break;
      case DBT_DEVNODES_CHANGED:
        //printf("DBT_DEVNODES_CHANGED\n");
        break;
      case DBT_QUERYCHANGECONFIG:
        //printf("DBT_QUERYCHANGECONFIG\n");
        break;
      case DBT_USERDEFINED:
        //printf("DBT_USERDEFINED\n");
        break;
      default:
        //printf("(unknown WM_DEVICECHANGE event)\n");
        break;
    }
  }

  if (!processed) {
    /* We weren't interested in the message. Pass it along to whomever cares. */
    return CallWindowProc(g_oldWindowProc, hWnd, uMsg, wParam, lParam);
  }
  return TRUE;
}

static BOOL CALLBACK enumWindowProc (HWND hWnd, LPARAM lParam) {
  HINSTANCE hInst = (HINSTANCE)GetModuleHandle(NULL);

  if ((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE) == hInst
      && IsWindowVisible(hWnd)) {
    /* lParam is an output parameter window handle */
    *(HWND *)lParam = hWnd;
    SetLastError(ERROR_SUCCESS);
    return FALSE;
  }
  return TRUE;
}

static HDEVNOTIFY registerForDeviceChanges () {
  /* Getting the top-level window handle should be as simple as:
   * GdkWindow *gdkwin = gtk_widget_get_root_window(g_window);
   * HWND hWnd = (HWND)GDK_WINDOW_HWND(gdkwin);
   * But I couldn't get it working. Found this hacky method on stackoverflow. */
  HWND hWnd = NULL;
  if (!EnumWindows(enumWindowProc, (LPARAM)&hWnd)) {
    DWORD err = GetLastError();
    if (ERROR_SUCCESS != err) {
      win32_error(_T("EnumWindows"), err);
      exit(1);
    }
  }
  assert(hWnd);

  /* Now that we have our top-level window handle, we can replace its WindowProc
   * with our own to intercept WM_DEVICECHANGE messages. */
  g_oldWindowProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)windowProc);
  if (!g_oldWindowProc) {
    win32_error("SetWindowLongPtr", GetLastError());
    exit(1);
  }

  DEV_BROADCAST_DEVICEINTERFACE filter = { 0 };
  filter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
  filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
  //filter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;
  HDEVNOTIFY hNotify = RegisterDeviceNotification(hWnd, (LPVOID)&filter,
      DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
  if (!hNotify) {
    win32_error("RegisterDeviceNotification", GetLastError());
    exit(1);
  }

  return hNotify;
}

static void unregisterForDeviceChanges (HDEVNOTIFY hNotify) {
  if (!UnregisterDeviceNotification(hNotify)) {
    win32_error("UnregisterDeviceNotification", GetLastError());
    exit(1);
  }
}

#endif

int main(int argc, char* argv[])
{
  GError *error = NULL;

#ifdef _WIN32
  /* Make sure there isn't another instance of BaroboLink running by checking
   * for the existance of a named mutex. */
  HANDLE hMutex;
  hMutex = CreateMutex(NULL, TRUE, TEXT("Global\\RoboMancerMutex"));
  DWORD dwerror = GetLastError();
#endif
 
  gtk_init(&argc, &argv);

  /* Create the GTK Builder */
  g_builder = gtk_builder_new();

#ifdef _WIN32
  if(dwerror == ERROR_ALREADY_EXISTS) {
    GtkWidget* d = gtk_message_dialog_new(
        GTK_WINDOW(gtk_builder_get_object(g_builder, "window1")),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "Another instance of BaroboLink is already running. Please terminate the other process and try again.");
    int rc = gtk_dialog_run(GTK_DIALOG(d));
    exit(0);
  }
#endif

  /* hlh: This used to be ifdef __MACH__, but XDG is not a BSD-specific platform. */
#ifndef _WIN32
  for (int i = 0; i < sizeof(g_interfaceFilesInit) / sizeof(g_interfaceFilesInit[0]); ++i) {
    g_interfaceFiles.push_back(std::string(g_interfaceFilesInit[i]));
  }

  std::string datadir (getenv("XDG_DATA_DIRS"));

  if(!datadir.empty()) {
    std::vector<std::string> xdg_data_dirs = split_escaped(datadir, ':', '\\');
    for (std::vector<std::string>::iterator it = xdg_data_dirs.begin();
        xdg_data_dirs.end() != it; ++it) {
      g_interfaceFiles.push_back(*it + std::string("/BaroboLink/interface.glade"));
    }
  }
  else {
    g_interfaceFiles.push_back(std::string("/usr/share/BaroboLink/interface.glade"));
  }
#endif

  /* Load the UI */
  /* Find ther interface file */
  struct stat s;
  int err;
  bool iface_file_found = false;
  for (std::vector<std::string>::iterator it = g_interfaceFiles.begin();
      g_interfaceFiles.end() != it; ++it) {
    printf("checking %s\n", it->c_str());
    err = stat(it->c_str(), &s);
    if(err == 0) {
      if( ! gtk_builder_add_from_file(g_builder, it->c_str(), &error) )
      {
        g_warning("%s", error->message);
        //g_free(error);
        return -1;
      } else {
        iface_file_found = true;
        break;
      }
    }
  }

  if (!iface_file_found) {
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
  //gtk_mac_menu_set_quit_menu_item(GTK_MENU_ITEM(quititem));
#endif

  /* Hide the Program dialog */
  GtkWidget* w = GTK_WIDGET(gtk_builder_get_object(g_builder, "notebook1"));
  gtk_notebook_remove_page(GTK_NOTEBOOK(w), 3);

  /* Show the window */
  gtk_widget_show(g_window);

#ifdef _WIN32
  HDEVNOTIFY hNotify = registerForDeviceChanges();
#endif

  gtk_main();

#ifdef _WIN32
  unregisterForDeviceChanges(hNotify);
#endif

  return 0;
}

void initialize()
{
  g_mobotParent = NULL;
  g_robotManager = new CRobotManager();
  /* Read the configuration file */
  g_robotManager->read( Mobot_getConfigFilePath() );

  refreshConnectDialog();
  //g_timeout_add(1000, connectDialogPulse, NULL);

  g_notebookRoot = GTK_NOTEBOOK(gtk_builder_get_object(g_builder, "notebook_root"));
  g_reflashConnectSpinner = GTK_SPINNER(gtk_builder_get_object(g_builder, "spinner_reflashConnect"));
  initControlDialog();
  initScanMobotsDialog();
  initializeComms();

  /* Try to connect to a DOF dongle if possible */
  const char *dongle;
  int i, rc;
  GtkLabel* l = GTK_LABEL(gtk_builder_get_object(g_builder, "label_connectDongleCurrentPort"));
  for(
      i = 0, dongle = g_robotManager->getDongle(i); 
      dongle != NULL; 
      i++, dongle = g_robotManager->getDongle(i)
     ) 
  {
    g_mobotParent = (recordMobot_t*)malloc(sizeof(recordMobot_t));
    RecordMobot_init(g_mobotParent, "DONGLE");
    rc = Mobot_connectWithTTY((mobot_t*)g_mobotParent, dongle);
    if(rc == 0) {
      Mobot_setDongleMobot((mobot_t*)g_mobotParent);
      gtk_label_set_text(l, dongle);
      break;
    } 
  }
  if(rc) {
    gtk_label_set_text(l, "Not connected");
  }
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

void on_menuitem_help_activate(GtkWidget *widget, gpointer data)
{
#ifdef _MSYS
  /* Get the install path of BaroboLink from the registry */
  DWORD size;
  char path[1024];
  HKEY key;
  RegOpenKeyEx(
      HKEY_LOCAL_MACHINE,
      "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\BaroboLink.exe",
      0,
      KEY_QUERY_VALUE,
      &key);

  RegQueryValueEx(
      key,
      "PATH",
      NULL,
      NULL,
      (LPBYTE)path,
      &size);
  path[size] = '\0';

  strcat(path, "\\docs\\index.html");

  ShellExecute(
      NULL,
      "open",
      path,
      NULL,
      NULL,
      SW_SHOW);
#else
  
#endif
}

void on_menuitem_demos_activate(GtkWidget *widget, gpointer data)
{
#ifdef _MSYS
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof(STARTUPINFO));
  memset(&pi, 0, sizeof(PROCESS_INFORMATION));
  char cmdline[256];
  strncpy(cmdline, "-d C:\\ch\\package\\chmobot\\demos", sizeof(cmdline));
  CreateProcess(
	"C:\\ch\\bin\\chide.exe",
  cmdline,
	NULL,
	NULL,
	FALSE,
	NORMAL_PRIORITY_CLASS,
	NULL,
	NULL,
	&si,
	&pi);
	
/*
  ShellExecute(
      NULL,
      "open",
      "C:\\chide",
      "-d C:\\Ch\\package\\chmobot\\demos",
      NULL,
      0);
*/
#endif
}

#define Q(x) QUOTE(x)
#define QUOTE(x) #x

void on_imagemenuitem_about_activate(GtkWidget *widget, gpointer data)
{
  /* Find the about dialog and show it */
  GtkWidget *w;
  w = GTK_WIDGET(gtk_builder_get_object(g_builder, "aboutdialog"));
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(w), Q(BAROBOLINK_VERSION));
  gtk_dialog_run(GTK_DIALOG(w));
}

void on_menuitem_installLinkbotDriver_activate(GtkWidget *widget, gpointer data)
{
  /* Get the install path of BaroboLink from the registry */
#ifdef _MSYS
  DWORD size;
  char path[1024];
  HKEY key;
  RegOpenKeyEx(
      HKEY_LOCAL_MACHINE,
      "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\BaroboLink.exe",
      0,
      KEY_QUERY_VALUE,
      &key);

  RegQueryValueEx(
      key,
      "PATH",
      NULL,
      NULL,
      (LPBYTE)path,
      &size);
  path[size] = '\0';
  strcat(path, "\\Drivers");
  ShellExecute(NULL, "open", path, NULL, NULL, SW_SHOWDEFAULT);
#endif

#if 0
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof(STARTUPINFO));
  memset(&pi, 0, sizeof(PROCESS_INFORMATION));
  /*
  CreateProcess(
	path,
	"",
	NULL,
	NULL,
	FALSE,
	0x0200,
	NULL,
	NULL,
	&si,
	&pi);
  */
  system(path);
#endif
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

volatile gboolean g_disconnectThreadDone = 0;
void* disconnectThread(void* arg)
{
  g_robotManager->disconnectAll();
  g_disconnectThreadDone = TRUE;
}

gboolean exitTimeout(gpointer data)
{
  static int i = 0;
  if((i >= 6) || g_disconnectThreadDone) {
    gtk_main_quit();
    return FALSE;
  } else {
    i++;
    return TRUE;
  }
}

gboolean on_window1_delete_event(GtkWidget *w)
{
  /* First, disable the active robot, if there is one */
  MUTEX_LOCK(&g_activeMobotLock);
  g_activeMobot = NULL;
  MUTEX_UNLOCK(&g_activeMobotLock);
  /* Disconnect from all connected robots */
  THREAD_T thread;
  g_disconnectThreadDone = 0;
  THREAD_CREATE(&thread, disconnectThread, NULL);
  GtkWidget *d = gtk_message_dialog_new(
      GTK_WINDOW(gtk_builder_get_object(g_builder, "window1")),
      GTK_DIALOG_MODAL,
      GTK_MESSAGE_INFO,
      GTK_BUTTONS_NONE,
      "BaroboLink shutting down..."
      );
  gtk_window_set_decorated(
      GTK_WINDOW(d),
      false);
  gtk_widget_show_all(d);
  g_timeout_add(500, exitTimeout, NULL);
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
