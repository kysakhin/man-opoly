#include "../include/ui.h"
#include <stdio.h>
#include "../include/fuzzy.h"
#include "gtk/gtk.h"

#define MAX_RESULTS 10 

typedef struct {
  GtkNotebook *tab;
  GtkEntry *entry;
  GtkBox *cont;
} Widget;

typedef struct Tab {
  GtkNotebook *notebook;
  GtkWidget *child;
} Tab;

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

  const char *name = gtk_button_get_label(button);
  char *page = strchr(name, '(');
  page++;
  int num = *page - '0';
  *strchr(name, ' ') = 0;

  char command[128];
  snprintf(command, sizeof(command), "man -P cat %d %s",num, name);

  FILE *fp = popen(command, "r");
  if (!fp)
  {
    g_print("Failed to run display man page command\n");
    return;
  }

  char buffer[4096];
  size_t total_size = 0;
  char *content = NULL;

  while (fgets(buffer, sizeof(buffer), fp))
  {
    size_t len = strlen(buffer);
    content = realloc(content, total_size + len + 1);
    memcpy(content + total_size, buffer, len + 1);
    total_size += len;
  }

  pclose(fp);

  GtkWidget *text_view = gtk_text_view_new();
  GtkTextBuffer *buffer_view = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gtk_text_buffer_set_text(buffer_view, content, -1);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);

  GtkWidget *scrolled_window = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), text_view);
  gtk_widget_set_hexpand(scrolled_window, TRUE);
  gtk_widget_set_vexpand(GTK_WIDGET(scrolled_window), TRUE);
  gtk_box_append(GTK_BOX(widget->cont), scrolled_window);
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
  struct Result results[300];
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

// tabs feature
// tabs should have dynamic title setting
// it should have a close button
// it shold be easy to pass into functions.
//
// function: create_new_tab(button, notebook)
// creating the landing page right inside this function.
// this should create with the close button aswell.
// function: edit_tab_label(notebook, title)
//
// so now i gotta pass the notebook around.


void close_tab(GtkButton *button, gpointer user_data)
{
  Tab *curr = (Tab *)user_data;
  gtk_notebook_detach_tab(curr->notebook, curr->child);
}


void create_new_tab(GtkNotebook *notebook)
{
  GtkWidget *tab_label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget *label = gtk_label_new("Home");
  GtkWidget *close_button = gtk_button_new_with_label("x");
  gtk_widget_set_valign(close_button, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(tab_label), label);
  gtk_box_append(GTK_BOX(tab_label), close_button);

  

  GtkWidget *tab_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

  // search bar
  GtkWidget *entry = gtk_entry_new();
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_box_append(GTK_BOX(tab_content), entry);
  gtk_widget_set_valign(entry, GTK_ALIGN_START);

  // to display results
  GtkWidget *scrollable_int = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollable_int), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand(scrollable_int, TRUE);
  gtk_widget_set_vexpand(scrollable_int, TRUE);
  gtk_box_append(GTK_BOX(tab_content), scrollable_int);


  // sample content

  Widget *curr = g_new(Widget, 1);
  curr->entry = GTK_ENTRY(entry);
  curr->cont = GTK_BOX(tab_content);

  g_signal_connect(entry, "activate", G_CALLBACK(fetch), curr);


  GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

  GtkWidget *label_content = gtk_label_new(" man is the system's manual pager. Each page argument given to man is normally the name of a program, utility or function.\nThe manual page associated with each of these arguments is then found and displayed.\nA section, if provided, will direct man to look only in that section of the manual.\nThe default action is to search in all of the available sections following a pre-defined order (see DEFAULTS), and to show only the first page found, even if page exists in several sections.\n The table below shows the section numbers of the manual followed by the types of pages they contain.\n 1 Executable programs or shell commands\n 2 System calls (functions provided by the kernel)\n 3 Library calls (functions within program libraries)\n 4 Special files (usually found in /dev)\n 5 File formats and conventions, e.g. /etc/passwd\n 6 Games\n 7 Miscellaneous (including macro packages and conventions), e.g. man(7), groff(7), man-pages(7)\n 8 System administration commands (usually only for root)\n 9 Kernel routines [Non standard] ");
  gtk_label_set_wrap(GTK_LABEL(label_content), TRUE);
  gtk_box_append(GTK_BOX(content_box), label_content);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrollable_int), content_box);

  Tab curr_tab;
  curr_tab.notebook = notebook;
  curr_tab.child = tab_content;

  g_signal_connect(close_button, "clicked", G_CALLBACK(close_tab), &curr_tab);

  gtk_notebook_append_page(notebook, tab_content, tab_label);
}


void activate(GtkApplication* app, gpointer user_data) 
{
  GtkWidget* window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Manopoly");
  gtk_window_set_default_size(GTK_WINDOW(window), 700, 500);
  gtk_window_present(GTK_WINDOW(window));


  // root container
  GtkWidget *root_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

  // notebook
  GtkWidget *notebook = gtk_notebook_new();
  gtk_box_append(GTK_BOX(root_container), notebook);

  create_new_tab(GTK_NOTEBOOK(notebook));

  gtk_window_set_child(GTK_WINDOW(window), root_container);
}
