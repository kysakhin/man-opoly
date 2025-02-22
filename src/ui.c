#include "../include/ui.h"
#include <stdio.h>
#include "../include/fuzzy.h"
#include "gtk/gtk.h"

#define MAX_RESULTS 7

typedef struct {
  GtkEntry *entry;
  GtkBox *cont;
} Widget;

void clear_children(GtkWidget *widget) 
{
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(widget)) != NULL)
  {
    gtk_widget_unparent(child);
  }
}

void display_man_page(GtkButton *button, gpointer user_data)
{
  Widget *widget = (Widget *)user_data;

  clear_children(GTK_WIDGET(widget->cont));

  const char *cmd = gtk_button_get_label(button);
  *strchr(cmd, ' ') = 0;

  g_print("%s", cmd);
}

void fetch(GtkButton *button, gpointer user_data)
{
  Widget *widget = (Widget *)user_data;
  GtkEntryBuffer *buf = gtk_entry_get_buffer(widget->entry);
  const char *text = gtk_entry_buffer_get_text(buf);

  char command[256];
  snprintf(command, sizeof(command), "man -k \"%s\"", text);
  FILE *fp = popen(command, "r");
  g_print("%s", command);

  if (fp == NULL) {
    perror("Failed to run command");
    return;
  }

  clear_children(GTK_WIDGET(widget->cont));

  struct Result {
    char line[1024];
    int score;
  };
  struct Result results[100];
  int result_count = 0;

  char line[1024];
  while (fgets(line, sizeof(line), fp) != NULL) 
  {
    int score = fuzzy_match(text, line);
    if (score > 0) {
      strncpy(results[result_count].line, line, sizeof(results[result_count].line));
      results[result_count].score = score;
      result_count++;
    }
  }

  pclose(fp);

  for (int i = 0; i < result_count - 1; i++) {
    for (int j = i + 1; j < result_count; j++) {
      if (results[i].score < results[j].score) {
        struct Result temp = results[i];
        results[i] = results[j];
        results[j] = temp;
      }
    }
  }

  for (int i = 0; i < result_count && i < MAX_RESULTS; i++) {
    GtkWidget *btn = gtk_button_new_with_label(results[i].line);
    g_signal_connect(btn, "clicked", G_CALLBACK(display_man_page), widget);
    gtk_box_append(GTK_BOX(widget->cont), btn);
  }


  gtk_entry_buffer_set_text(buf, "", 0);
}

void activate(GtkApplication* app, gpointer user_data) 
{
  GtkWidget* window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Manopoly");
  gtk_window_set_default_size(GTK_WINDOW(window), 700, 500);
  gtk_window_present(GTK_WINDOW(window));


  // root container
  GtkWidget *root_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

  // search bar
  GtkWidget *entry = gtk_entry_new();
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_box_append(GTK_BOX(root_container), entry);
  gtk_widget_set_valign(entry, GTK_ALIGN_START);

  // to display results
  GtkWidget *scrollable_int = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollable_int), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand(scrollable_int, TRUE);
  gtk_widget_set_vexpand(scrollable_int, TRUE);
  gtk_box_append(GTK_BOX(root_container), scrollable_int);


  // sample content
  GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

  GtkWidget *label = gtk_label_new(" man is the system's manual pager. Each page argument given to man is normally the name of a program, utility or function.\nThe manual page associated with each of these arguments is then found and displayed.\nA section, if provided, will direct man to look only in that section of the manual.\nThe default action is to search in all of the available sections following a pre-defined order (see DEFAULTS), and to show only the first page found, even if page exists in several sections.\n The table below shows the section numbers of the manual followed by the types of pages they contain.\n 1 Executable programs or shell commands\n 2 System calls (functions provided by the kernel)\n 3 Library calls (functions within program libraries)\n 4 Special files (usually found in /dev)\n 5 File formats and conventions, e.g. /etc/passwd\n 6 Games\n 7 Miscellaneous (including macro packages and conventions), e.g. man(7), groff(7), man-pages(7)\n 8 System administration commands (usually only for root)\n 9 Kernel routines [Non standard] ");
  gtk_label_set_wrap(GTK_LABEL(label), TRUE);
  gtk_box_append(GTK_BOX(content_box), label);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrollable_int), content_box);

  Widget *curr = g_new(Widget, 1);
  curr->entry = GTK_ENTRY(entry);
  curr->cont = GTK_BOX(content_box);

  g_signal_connect(entry, "activate", G_CALLBACK(fetch), curr);

  gtk_window_set_child(GTK_WINDOW(window), root_container);
}
