#ifndef _ROBOMANCER_H_
#define _ROBOMANCER_H_

#include "RobotManager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Global Funcs */
void initialize();

/* Global Vars */
extern GtkBuilder *g_builder;
extern CRobotManager *g_robotManager;

/* Connect Dialog */
G_MODULE_EXPORT void on_button_connect_addRobot_clicked(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_button_connect_remove_clicked(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_button_connect_moveUpAvailable_clicked(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_button_connect_moveDownAvailable_clicked(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_button_connect_connect_clicked(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_button_connect_moveUpConnected_clicked(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_button_connect_moveDownConnected_clicked(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_button_connect_disconnect_clicked(GtkWidget* widget, gpointer data);
void refreshConnectDialog();

#ifdef __cplusplus
}
#endif



#endif
