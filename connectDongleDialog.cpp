#include "mobot.h"
#include "RoboMancer.h"

void showConnectDongleDialog(void)
{
  GdkColor color;
  GtkWidget *w;
  /* First, see if the dongle is connected */
  if( (g_mobotParent != NULL) && (((mobot_t*)g_mobotParent)->connected != 0)) {
    /* The dongle is connected */
    /* Make the text entry green */
    gdk_color_parse("green", &color);
    w = GTK_WIDGET(gtk_builder_get_object(g_builder, "entry_connectDongleCurrentPort"));
    gtk_widget_modify_fg(w, GTK_STATE_NORMAL, &color);
  } else {
    /* The dongle is not connected */
    gdk_color_parse("red", &color);
    w = GTK_WIDGET(gtk_builder_get_object(g_builder, "entry_connectDongleCurrentPort"));
    gtk_widget_modify_fg(w, GTK_STATE_NORMAL, &color);
    gtk_entry_set_text(GTK_ENTRY(w), "<Not Connected>");
  }
  GtkWidget* window = GTK_WIDGET(gtk_builder_get_object(g_builder, "dialog_connectDongle"));
  gtk_widget_show(window);
}

void hideConnectDongleDialog(void)
{
  GtkWidget* window = GTK_WIDGET(gtk_builder_get_object(g_builder, "dialog_connectDongle"));
  gtk_widget_hide(window);
}

void on_button_connectDongleConnect_clicked(GtkWidget *w, gpointer data)
{
  char buf[256];
  GtkRadioButton *connectAutomaticallyButton = 
    GTK_RADIO_BUTTON(gtk_builder_get_object(g_builder, "radiobutton_connectDongleAuto"));
  GtkRadioButton *connectManuallyButton = 
    GTK_RADIO_BUTTON(gtk_builder_get_object(g_builder, "radiobutton_connectDongleManual"));
  GtkEntry *manualComPort = GTK_ENTRY(gtk_builder_get_object(g_builder, "entry_connectDongleManual"));
  GtkEntry *currentComPort = GTK_ENTRY(gtk_builder_get_object(g_builder, "entry_connectDongleCurrentPort"));

  /* Check to see if the auto button is pressed */
  if(
      gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(connectAutomaticallyButton))
    )
  {
    /* If the auto-detect button is pressed */
  } else if (
      gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(connectManuallyButton))
      )
  {
    /* If the manual selection is pressed */
    /* Get the entry text */
    const char* port = gtk_entry_get_text(manualComPort);
    printf("Connecting to port %s\n", port);
    if(Mobot_connectWithTTY((mobot_t*)g_mobotParent, port)) {
      GtkLabel* errLabel = GTK_LABEL(gtk_builder_get_object(g_builder, "label_connectFailed"));
      sprintf(buf, "Error: Could not connect to dongle at %s.\n", port);
      gtk_label_set_text(errLabel, buf);
      GtkWidget* errWindow = GTK_WIDGET(gtk_builder_get_object(g_builder, "dialog_connectFailed"));
      gtk_widget_show(errWindow);
    } else {
      gtk_entry_set_text(currentComPort, port);
    }
  } else {
    /* Error */
    return;
  }
}

void on_button_connectDongleClose_clicked(GtkWidget *w, gpointer data)
{
  hideConnectDongleDialog();
  return;
}
