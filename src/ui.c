#include "../include/ui.h"
#include "gdk/gdk.h"
#include "gtk/gtkcssprovider.h"
#include <stdio.h>
#include "../include/fuzzy.h"
#include "gtk/gtk.h"
#include <webkitgtk-6.0/webkit/webkit.h>

#define MAX_RESULTS 10 

void create_new_tab(GtkNotebook *notebook);
void close_tab(GtkButton *button, gpointer user_data);
void create_google_search_tab(GtkWidget *button, GtkNotebook* notebook);

typedef struct Tab {
  GtkNotebook *notebook;
  GtkWidget *child;
} Tab;

typedef struct {
  Tab *tab;
  GtkEntry *entry;
  GtkBox *cont;
} Widget;


void load_css(void) 
{
  GtkCssProvider *provider = gtk_css_provider_new();
  GFile *css_file = g_file_new_for_path("style.css");

  gtk_css_provider_load_from_file(provider, css_file);

  gtk_style_context_add_provider_for_display(
    gdk_display_get_default(),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );
}

void close_tab(GtkButton *button, gpointer user_data)
{
  Tab *curr = (Tab *)user_data;
  int total = gtk_notebook_get_n_pages(curr->notebook);
  if (total > 1) {
    int p_num = gtk_notebook_page_num(curr->notebook, curr->child);
    gtk_notebook_remove_page(curr->notebook, p_num);
  }
}


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

  // set tab label

  GtkWidget *tab_label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget *label = gtk_label_new(name);
  GtkWidget *close_button = gtk_button_new_with_label("x");
  gtk_widget_add_css_class(close_button, "flat");
  gtk_widget_set_valign(close_button, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(tab_label), label);
  gtk_box_append(GTK_BOX(tab_label), close_button);
  gtk_notebook_set_tab_label(widget->tab->notebook, widget->tab->child, tab_label);

  g_signal_connect(close_button, "clicked", G_CALLBACK(close_tab), widget->tab);

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
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
  gtk_widget_add_css_class(text_view, "text-view");

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

  // set tab label
  char new_text[128];
  snprintf(new_text, sizeof(new_text), "Search Results for: %s", text);

  GtkWidget *tab_label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget *label = gtk_label_new(new_text);
  GtkWidget *close_button = gtk_button_new_with_label("x");
  gtk_widget_add_css_class(close_button, "flat");
  gtk_widget_set_valign(close_button, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(tab_label), label);
  gtk_box_append(GTK_BOX(tab_label), close_button);
  gtk_notebook_set_tab_label(widget->tab->notebook, widget->tab->child, tab_label);

  g_signal_connect(close_button, "clicked", G_CALLBACK(close_tab), widget->tab);

  char command[256];
  snprintf(command, sizeof(command), "man -k \"%s\"", text);
  FILE *fp = popen(command, "r");

  if (fp == NULL) {
    perror("Failed to run command");
    return;
  }

  clear_children(GTK_WIDGET(widget->cont));

  struct Result {
    char line[128];
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

  gtk_widget_set_vexpand(GTK_WIDGET(widget->cont), TRUE);
  gtk_widget_set_hexpand(GTK_WIDGET(widget->cont), TRUE);
  for (int i = 0; i < result_count && i < MAX_RESULTS; i++) {
    GtkWidget *btn = gtk_button_new_with_label(results[i].line);
    g_signal_connect(btn, "clicked", G_CALLBACK(display_man_page), widget);
    gtk_box_append(GTK_BOX(widget->cont), btn);
    gtk_widget_add_css_class(btn, "button");
  }


  gtk_entry_buffer_set_text(buf, "", 0);
}


gboolean on_key_press(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) 
{
  Tab *curr = (Tab*)user_data;
  gint total = gtk_notebook_get_n_pages(curr->notebook);
  gint current_page = gtk_notebook_get_current_page(curr->notebook);
  if ((state & GDK_CONTROL_MASK))
  {
    switch (keyval)
    {
      case GDK_KEY_t:
        create_new_tab(curr->notebook);
        return GDK_EVENT_STOP;

      case GDK_KEY_w:
        close_tab(NULL, user_data);
        return GDK_EVENT_STOP;

      case GDK_KEY_Tab:
        int next_page = (current_page + 1) % total;
        gtk_notebook_set_current_page(curr->notebook, next_page);
        return GDK_EVENT_STOP;

      case GDK_KEY_ISO_Left_Tab:
        int previous_page = (current_page + 1) % total;
        gtk_notebook_set_current_page(curr->notebook, previous_page);
        return GDK_EVENT_STOP;

    }
  }
  return GDK_EVENT_PROPAGATE;
}

void create_google_search_tab(GtkWidget *button, GtkNotebook* notebook)
{
  GtkWidget *tab_label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget *label = gtk_label_new("Google");
  GtkWidget *close_button = gtk_button_new_with_label("x");
  gtk_widget_add_css_class(close_button, "flat");
  gtk_widget_set_valign(close_button, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(tab_label), label);
  gtk_box_append(GTK_BOX(tab_label), close_button);
  GtkWidget *webview = webkit_web_view_new();

  // Create scrollable container for WebView
  GtkWidget *scrolled_window = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), webview);
  gtk_widget_set_vexpand(scrolled_window, TRUE);
  gtk_widget_set_hexpand(scrolled_window, TRUE);

  // link
  Tab *curr = g_new(Tab, 1);
  curr->notebook = notebook;
  curr->child = scrolled_window;

  g_signal_connect(close_button, "clicked", G_CALLBACK(close_tab), curr);

  // Load Google search
  webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), "https://www.google.com");

  // Append the tab
  int page_num = gtk_notebook_append_page(notebook, scrolled_window, tab_label);
  gtk_notebook_set_current_page(notebook, page_num);
}


void create_new_tab(GtkNotebook *notebook)
{
  GtkWidget *tab_label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget *label = gtk_label_new("Home");
  GtkWidget *close_button = gtk_button_new_with_label("x");
  gtk_widget_add_css_class(close_button, "flat");
  gtk_widget_set_valign(close_button, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(tab_label), label);
  gtk_box_append(GTK_BOX(tab_label), close_button);

  GtkWidget *tab_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

  // search bar
  GtkWidget *entry = gtk_entry_new();
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_box_append(GTK_BOX(tab_content), entry);
  gtk_widget_set_valign(entry, GTK_ALIGN_START);
  gtk_widget_add_css_class(entry, "entry");

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

  GtkWidget *label_content = gtk_label_new("`man` is the system's manual pager. Each page argument given to man is normally the name of a program, utility or function.\nThe manual page associated with each of these arguments is then found and displayed.\nA section, if provided, will direct man to look only in that section of the manual.\nThe default action is to search in all of the available sections following a pre-defined order (see DEFAULTS), and to show only the first page found, even if page exists in several sections.\n The table below shows the section numbers of the manual followed by the types of pages they contain.\n 1 Executable programs or shell commands\n 2 System calls (functions provided by the kernel)\n 3 Library calls (functions within program libraries)\n 4 Special files (usually found in /dev)\n 5 File formats and conventions, e.g. /etc/passwd\n 6 Games\n 7 Miscellaneous (including macro packages and conventions), e.g. man(7), groff(7), man-pages(7)\n 8 System administration commands (usually only for root)\n 9 Kernel routines [Non standard] ");
  gtk_label_set_wrap(GTK_LABEL(label_content), TRUE);
  gtk_box_append(GTK_BOX(content_box), label_content);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrollable_int), content_box);

  Tab *curr_tab = g_new(Tab, 1);
  curr_tab->notebook = notebook;
  curr_tab->child = tab_content;

  curr->tab = curr_tab;

  // reorder. fix this.
  /*gtk_notebook_set_tab_reorderable(curr_tab->notebook, curr_tab->child, TRUE);*/

  g_signal_connect(close_button, "clicked", G_CALLBACK(close_tab), curr_tab);

  // keypress events
  GtkEventController *controller = gtk_event_controller_key_new();
  g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_press), curr_tab);
  gtk_widget_add_controller(GTK_WIDGET(notebook), controller);

  int new_page_index = gtk_notebook_append_page(notebook, tab_content, tab_label);

  gtk_notebook_set_current_page(notebook, new_page_index);
}

GtkWidget* create_new_tab_button(GtkNotebook *notebook)
{
  GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget *new_tab_button = gtk_button_new_with_label("+");

  gtk_widget_add_css_class(new_tab_button, "flat");
  gtk_widget_set_margin_start(new_tab_button, 2);
  gtk_widget_set_margin_end(new_tab_button, 2);

  gtk_box_append(GTK_BOX(button_box), new_tab_button);

  g_signal_connect_swapped(new_tab_button, "clicked", G_CALLBACK(create_new_tab), notebook);

  return button_box;
}


void activate(GtkApplication* app, gpointer user_data) 
{

  load_css();

  GtkWidget* window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Manopoly");
  gtk_window_set_default_size(GTK_WINDOW(window), 900, 700);
  gtk_window_present(GTK_WINDOW(window));
  gtk_widget_add_css_class(window, "window");

  // root container
  GtkWidget *root_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

  gtk_widget_add_css_class(root_container, "content-box");

  gtk_widget_set_hexpand(root_container, TRUE);
  gtk_widget_set_vexpand(root_container, TRUE);

  // notebook
  GtkWidget *notebook = gtk_notebook_new();

  create_new_tab(GTK_NOTEBOOK(notebook));

  GtkWidget *action_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  // button for new tab
  GtkWidget *new_tab_button = create_new_tab_button(GTK_NOTEBOOK(notebook));
  gtk_box_append(GTK_BOX(action_box), new_tab_button);

  // button for create google tab
  GtkWidget *gtab_create = gtk_button_new_with_label("Google");
  gtk_box_append(GTK_BOX(action_box), gtab_create);
  gtk_notebook_set_action_widget(GTK_NOTEBOOK(notebook), action_box, GTK_PACK_END);
  g_signal_connect(gtab_create, "clicked", G_CALLBACK(create_google_search_tab), GTK_NOTEBOOK(notebook));


  // tabs box?
  GtkWidget *tabs_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_append(GTK_BOX(root_container), tabs_box);
  gtk_box_append(GTK_BOX(tabs_box), notebook);

  gtk_widget_set_hexpand(tabs_box, TRUE);
  gtk_widget_set_vexpand(tabs_box, TRUE);

  gtk_window_set_child(GTK_WINDOW(window), root_container);
}
