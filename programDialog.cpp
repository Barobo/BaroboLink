#include <gtk/gtk.h>
#include <string.h>
#define PLAT_GTK 1
#define GTK
#include <Scintilla.h>
#include <SciLexer.h>
#include <ScintillaWidget.h>
#include "RoboMancer.h"
#ifdef _MSYS
#include <windows.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#ifndef _MSYS
#define MAX_PATH 512
#endif

char *g_curFileName = NULL;
bool g_dirty = false;

void initProgramDialog(void)
{
  /* Attach signal handlers */
  g_signal_connect(G_OBJECT(g_scieditor), SCINTILLA_NOTIFY, G_CALLBACK(on_scintilla_notify), NULL);
  /* Set a monospace font */
  scintilla_send_message(
      g_sci,
      SCI_STYLESETFONT,
      STYLE_DEFAULT,
      (sptr_t)"!Courier");
  scintilla_send_message(
      g_sci,
      SCI_STYLESETSIZE,
      STYLE_DEFAULT,
      (sptr_t)10);
  on_imagemenuitem_new_activate(NULL, NULL);
}

void on_imagemenuitem_new_activate(GtkWidget* widget, gpointer data)
{
  if(g_curFileName != NULL) {
    free(g_curFileName);
    g_curFileName = NULL;
  }
  /* Clear the document */
  scintilla_send_message(g_sci, SCI_CLEARALL, 0, 0);
  /* Set up the initial code stub */
  scintilla_send_message(g_sci, SCI_INSERTTEXT, 0, (sptr_t)
      "#include <stdio.h>\n"
      "#include <mobot.h>\n"
      "\n"
      "int main()\n"
      "{\n"
      "    /* Declare a new robot variable */\n"
      "    CMobot robot;\n"
      "    /* Link the new variable with a physical robot */\n"
      "    robot.connect();\n"
      "    /* Move the robot to its zero position */\n"
      "    robot.moveToZero();\n"
      "\n"
      "    /* The next lines of code make the program pause until the user presses a\n"
      "     * key. This is useful in Windows especially to keep the console window \n"
      "     * open after the program has finished. */\n"
      "    printf(\"Program Ended. Press a key to continue.\\n\");\n"
      "    getchar();\n"
      "    return 0;\n"
      "}\n" );
}

void on_imagemenuitem_open_activate(GtkWidget* widget, gpointer data)
{
  GtkWidget *dialog;
  dialog = gtk_file_chooser_dialog_new ("Open File",
      GTK_WINDOW(g_window),
      GTK_FILE_CHOOSER_ACTION_OPEN,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
      NULL);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename;
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    /* Load the contents of the file into the scintilla window. First, load the
     * whole file contents into a character buffer. */
    char *contents;
    struct stat s;
    stat(filename, &s);
    FILE *fp;
    fp = fopen(filename, "r");
    if(fp == NULL) {
      /* FIXME: Should pop up a warning dialog or something */
      return;
    }
    contents = (char*)malloc(s.st_size+1);
    fread(contents, s.st_size, 1, fp);
    scintilla_send_message(g_sci, SCI_SETTEXT, 0, (sptr_t)contents);
    fclose(fp);
    free(contents);
    
    g_curFileName = strdup(filename);
    g_free (filename);
  }
  gtk_widget_destroy (dialog);
}

void save_to_file(const char* filename)
{
  /* Open the file for writing */
  FILE* fp;
  fp = fopen(filename, "w");
  if(fp == NULL) {
    /* Could not open the file. Pop up an error message */
    GtkWidget* dialog;
    dialog = gtk_message_dialog_new (GTK_WINDOW(g_window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "Error saving to file '%s': %s",
        filename, g_strerror (errno));

    /* Destroy the dialog when the user responds to it (e.g. clicks a button) */
    g_signal_connect_swapped (dialog, "response",
        G_CALLBACK (gtk_widget_destroy),
        dialog);
    return;
  }
  int sourceCodeSize;
  char* sourceCode;
  sourceCodeSize = scintilla_send_message(g_sci, SCI_GETLENGTH, 0, 0) + 1;
  sourceCode = (char*)malloc(sourceCodeSize);
  scintilla_send_message(g_sci, SCI_GETTEXT, sourceCodeSize, (sptr_t)sourceCode);
  /* Write it to the file */
  fwrite(sourceCode, sourceCodeSize, sizeof(char), fp); 
  /* Close the file */
  fclose(fp);
  free(sourceCode);
  /* Set the scintilla save point */
  scintilla_send_message(g_sci, SCI_SETSAVEPOINT, 0, 0);
  return;
}

void on_imagemenuitem_save_activate(GtkWidget* widget, gpointer data)
{
  /* if there is a current file name, just save there */
  if(g_curFileName != NULL) {
    save_to_file(g_curFileName);
  } else {
    on_imagemenuitem_saveAs_activate(widget, data);
  }
}

void on_imagemenuitem_saveAs_activate(GtkWidget* widget, gpointer data)
{
  /* Open a save file dialog */
  GtkWidget *dialog;
  dialog = gtk_file_chooser_dialog_new ("Save File",
      GTK_WINDOW(g_window),
      GTK_FILE_CHOOSER_ACTION_SAVE,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
      NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  if (g_curFileName == NULL)
  {
    //gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), default_folder_for_saving);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "Untitled document");
  }
  else
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), g_curFileName);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename;
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    save_to_file (filename);
    if(g_curFileName != NULL) {
      free(g_curFileName);
      g_curFileName = strdup(filename);
    }
    g_free (filename);
  }
  gtk_widget_destroy (dialog);
}

void on_imagemenuitem_cut_activate(GtkWidget* widget, gpointer data)
{
  scintilla_send_message(g_sci, SCI_CUT, 0, 0);
}

void on_imagemenuitem_copy_activate(GtkWidget* widget, gpointer data)
{
  scintilla_send_message(g_sci, SCI_COPY, 0, 0);
}

void on_imagemenuitem_paste_activate(GtkWidget* widget, gpointer data)
{
  scintilla_send_message(g_sci, SCI_PASTE, 0, 0);
}

void on_button_exportExe_clicked(GtkWidget* widget, gpointer data)
{
#ifdef _MSYS
  static bool path_set = false;
  static char lastFilename[256] = "";
  printf("Hello there\n");
  /* Pop up a "save file" dialog and get the filename */
  GtkWidget *dialog;
  dialog = gtk_file_chooser_dialog_new ("Save File",
      GTK_WINDOW(g_window),
      GTK_FILE_CHOOSER_ACTION_SAVE,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
      NULL);
  gtk_file_chooser_set_do_overwrite_confirmation( GTK_FILE_CHOOSER(dialog), TRUE);
  if(lastFilename[0] != '\0') {
    /* A file has been saved before */
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), lastFilename);
  } else {
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "program.exe");
  }
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename;
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    //open_file (filename);
    wchar_t *tmpdirname = new wchar_t[MAX_PATH];
    wchar_t tmpdirpfx[256] = L"RoboMancerTmp";
    wchar_t* dirname;
    GetTempPathW(256, tmpdirname);
    dirname = _wtempnam(tmpdirname, tmpdirpfx);
    /* Create the directory */
    _wmkdir(dirname);
    /* Copy the source code over there */
    char* sourceCode;
    int sourceCodeSize;
    sourceCodeSize = scintilla_send_message(g_sci, SCI_GETLENGTH, 0, 0) + 1;
    sourceCode = (char*)malloc(sourceCodeSize);
    scintilla_send_message(g_sci, SCI_GETTEXT, sourceCodeSize, (sptr_t)sourceCode);
    wchar_t sourceFileName[256];
    swprintf(sourceFileName, L"%s/source.cpp", dirname);
    wprintf(L"%s\n", sourceFileName);
    FILE *fp;
    fp = _wfopen(sourceFileName, L"w");
    if(fp == NULL) {
      fprintf(stderr, "Error writing temporary source code file\n.");
      free(sourceCode);
      g_free(filename);
      gtk_widget_destroy(dialog);
      return;
    }
    fprintf(fp, "%s\n", sourceCode);
    fclose(fp);
    free(sourceCode);
    /* Get the current working directory */
    wchar_t *cwd = new wchar_t[MAX_PATH];
    _wgetcwd(cwd, MAX_PATH);
    /* Modify the PATH environment variables so that necessary libraries can be
     * found */
    if(!path_set) {
      wchar_t* pathenv;
      wchar_t* newpathenv;
      int pathenvSize;
      pathenvSize = GetEnvironmentVariableW(L"PATH", NULL, 0);
      pathenvSize += MAX_PATH*2;
      pathenv = (wchar_t*)malloc(pathenvSize+1);
      newpathenv = (wchar_t*)malloc(pathenvSize+1);
      GetEnvironmentVariableW(L"PATH", pathenv, pathenvSize);
      swprintf(newpathenv, L"%s;%s\\mingw\\bin", pathenv, cwd);
      SetEnvironmentVariableW(L"PATH", newpathenv);
      path_set = true;
      free(pathenv);
      free(newpathenv);
    }
    /* Set up the compile command */
    wchar_t *compileCommand= new wchar_t[MAX_PATH];
    swprintf(
        compileCommand, 
        L"%s\\mingw\\bin\\gcc.exe -U_WIN32 -lmobot %s -o %s\\program.exe > %s\\log.txt 2>&1", 
        cwd, sourceFileName, dirname, dirname);
    _wsystem(compileCommand);
    /* Copy the compiled file to the requested location */
    wchar_t* fileOrigin = new wchar_t[MAX_PATH];
    swprintf(fileOrigin, L"%s\\program.exe", dirname);
    char* fileOriginA = new char[MAX_PATH];
    wcstombs(fileOriginA, fileOrigin, MAX_PATH);
    CopyFileA(fileOriginA, filename, false);
    /* Display the log contents */
    /* Open the log file */
    FILE *logfile;
    wchar_t* logfileName = new wchar_t[MAX_PATH];
    swprintf(logfileName, L"%s\\log.txt", dirname);
    logfile = _wfopen(logfileName, L"r");
    if(logfile == NULL) {
      fprintf(stderr, "Could not open compile-log file for reading!\n");
      return;
    }
    GtkWidget *w = GTK_WIDGET(gtk_builder_get_object(g_builder, "textview_programMessages"));
    w = GTK_WIDGET(gtk_text_view_get_buffer(GTK_TEXT_VIEW(w)));
    char line[512];
    /* Get the file line by line, inserting it into the text buffer */
    while(fgets(line, 512, logfile) != NULL) {
      gtk_text_buffer_insert_at_cursor(
          GTK_TEXT_BUFFER(w),
          line,
          -1);
    }
      gtk_text_buffer_insert_at_cursor(
          GTK_TEXT_BUFFER(w),
          "Helloooo!!!",
          -1);
    fclose(logfile);

    g_free (filename);

    /* FRee eevveryythinggg!!! */
    delete[] logfileName;
    delete[] tmpdirname;
    delete[] cwd;
    delete[] compileCommand;
    delete[] fileOrigin;
    delete[] fileOriginA;
  } else {
    printf("Gtk response was not accept!\n");
  }
  gtk_widget_destroy (dialog);
#endif
}

void on_button_runExe_clicked(GtkWidget* widget, gpointer data)
{
}

void on_scintilla_notify(GObject *gobject, GParamSpec *pspec, struct SCNotification* scn)
{
  GtkWidget *w;
  switch(scn->nmhdr.code) {
    case SCN_SAVEPOINTREACHED:
      /* gray out "save" file menu item */
      w = GTK_WIDGET(gtk_builder_get_object(g_builder, "imagemenuitem_save"));
      gtk_widget_set_sensitive(w, false);
      g_dirty = false;
      break;
    case SCN_SAVEPOINTLEFT:
      /* Ungray "save" menu item */
      w = GTK_WIDGET(gtk_builder_get_object(g_builder, "imagemenuitem_save"));
      gtk_widget_set_sensitive(w, true);
      g_dirty = true;
      break;
  }
}
