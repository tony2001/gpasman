/* Gpasman, a password manager
   (C) 1998, 1999 Olivier Sessink <olivier@lx.student.wau.nl>
   (C) 2003 T. Bugra Uytun <t.bugra@uytun.com>
   http://gpasman.sourceforge.net

   (C) 2007 hildonization & misc code cleanup by Antony Dovgal <tony at daylessday dot org>

   Other code contributors:
   Dave Rudder, Chris Halverson, Matthew Palmer, Guide Berning, Jimmy Mason

   This program is free software; you can redistribute it and/or modify it 
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along 
   with this program; if not, write to the Free Software Foundation, Inc., 
   59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */


#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "i18n-support.h"
#include "config.h"

#include "gpasman.h"
#include "file.h"

#include <hildon/hildon-program.h>

/*  SOME GLOBAL VARIABLES, DEFINITIONS, STRUCTS, ETC.... {{{ */
GtkWidget *main_window, *statusbar, *clipb_entry;
HildonProgram   *program;
GtkWidget       *container;

gchar *main_window_title;
gint context_id;

G_CONST_RETURN gchar *tmp_gchar_const;
GtkWidget *pwd_entry;
GtkItemFactory *item_factory;
GtkTextBuffer *buf = NULL;


GtkListStore *list_store;
GtkTreeIter selected_row;
GtkTreeViewColumn *passwd_column;
GtkTreeViewColumn *comments_column;

/*
*   Clipboard management
*/
/* Clipboard widget, to be set later */
GtkClipboard *clipboard = NULL;
/* to know if nothing, password or username is in clipboard */
typedef enum {
	GPASMAN_CLIPB_EMPTY,
	GPASMAN_CLIPB_USER,
	GPASMAN_CLIPB_PASSWD
} GpasmanClipbType;
GpasmanClipbType clipboard_used = GPASMAN_CLIPB_EMPTY; /* empty by default */
/* should passwords be visible in clipboard? */
gboolean clipboard_show_passwd = FALSE ;  /* No by default */
	
/* this struct is used a couple of times, for changing the master password
   and adding or updating an entry */
typedef struct {
	GtkWidget *window;
	GtkWidget *entry1;
	GtkWidget *entry2;
	GtkWidget *entry3;
	GtkWidget *entry4;
	gint type;
} gtkentry_struct;

/* this is the struct for global variable current_set */
typedef struct {
	gboolean password;		/* if TRUE the the password is expanded internally
							   in rc2 library */
	gint modified;
	gchar *filename;
	gboolean fileopen;
} current_struct;
current_struct current_set;

typedef enum {
	GPASMAN_ENTRY_DIALOG_ADD,
	GPASMAN_ENTRY_DIALOG_MODIFY
} GpasmanEntryDialogType;

/* }}} */

/* prototypes {{{ */
gint main(int argc, gchar * argv[]);
void make_interface(void);
static char *menu_translate(const char* path, gpointer data);
void tree_selection_changed(GtkTreeSelection *selection, GtkEntry *clipb_entry);
void tree_selection_activated(GtkTreeView *treeview, GtkTreePath *path,
					GtkTreeViewColumn *column, GtkEntry *clipb_entry);
gboolean tree_selection_popup_menu(GtkWidget *widget, GdkEventButton *event);

void toggle_show_passwords(gpointer callback_data, guint callback_action,
						   GtkWidget *toggle);
void toggle_show_comments(gpointer callback_data, guint callback_action,
						   GtkWidget *toggle);
void return_password();
void password_ok(GtkWidget *w, GtkWidget *dialog);
void load_file();
void save_file();
void file_close(GtkWidget *w, gpointer data);
void clear_entries(void);
void file_open(GtkWidget *w, gpointer data);
void file_save(GtkWidget *w, gpointer data);
void file_save_as(GtkWidget *w, gpointer data);
void entry_new(GtkWidget *w, gpointer data);
void entry_change(GtkWidget *w, gpointer data);
void entry_remove(GtkWidget *w, gpointer data);
void entry_dialog(gint type);
void entry_dialog_ok(GtkWidget *w, gtkentry_struct * data);
void return_file();
void change_password(GtkWidget *button, gpointer data);
void check_password(gtkentry_struct *data);
void statusbar_message(gpointer message, gint time);
gint statusbar_remove(gpointer message_id);

void gpasman_exit(GtkWidget *button, gtkentry_struct *data);
gboolean gpasman_quit(GtkWidget *widget, GdkEvent *event, gpointer data);
void gpasman_about(GtkWidget *button, gtkentry_struct *data);
void gpasman_help(GtkWidget *button, gtkentry_struct *data);
void info_dialog(gchar *message, gchar *title);

void set_widgets_sensitive(gboolean open_value, gboolean modified_value);
gint modified_check(gchar *message);
/* }}} */

/********************************************************************
 * PROGRAM BEGIN - MAIN
 ********************************************************************/
gint main(int argc, gchar * argv[]) /* {{{ */
{
	/* localisation support */
#ifdef ENABLE_NLS
	bindtextdomain(PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);
#endif
	
	gtk_init(&argc, &argv);
current_set.password = FALSE;
current_set.filename = NULL;
/* i have to set this to true at the beginning so that i can set the
 * widget to false with set_widgets_sensitive() */
current_set.fileopen = TRUE;
main_window_title = PACKAGE_STRING;
	

	make_interface();
	set_widgets_sensitive(FALSE, FALSE);
	gtk_widget_show(main_window);
	
	tmp_gchar_const = g_get_home_dir();
	current_set.filename = g_build_filename(tmp_gchar_const, ".gpasman", NULL);
	tmp_gchar_const = NULL;
#ifdef DEBUG
	gpasman_debug("current_set.filename = %s", current_set.filename);
#endif
	if(g_file_test(current_set.filename, G_FILE_TEST_IS_REGULAR))	{
		load_file(current_set.filename);
	}
	else	{
		statusbar_message(_("Could not open default file."), 8000);
	}
	gtk_main();
	return 0;
}
/* }}} */

/********************************************************************
 * USER INTERFACE STUFF
 ********************************************************************/

/* menu factory */
static GtkItemFactoryEntry menu_items[] = { /* {{{ */
	{N_("/_File"), NULL, NULL, 0, "<Branch>"},
	{N_("/File/_Open"), "<control>O", file_open, 0, "<StockItem>", GTK_STOCK_OPEN},
	{N_("/File/sep1"), NULL, NULL, 0, "<Separator>"},
	{N_("/File/_Save"), "<control>S", file_save, 0, "<StockItem>", GTK_STOCK_SAVE},
	{N_("/File/Save _As..."), "<shift><control>S", file_save_as, 0, "<StockItem>", GTK_STOCK_SAVE_AS},
	{N_("/File/sep1"), NULL, NULL, 0, "<Separator>"},
	{N_("/File/Close"), NULL, file_close, 0, "<StockItem>", GTK_STOCK_CLOSE},
	{N_("/File/Quit"), "<control>Q", gpasman_exit, 0, "<StockItem>", GTK_STOCK_QUIT},
	{N_("/_Edit"), NULL, NULL, 0, "<Branch>"},
	{N_("/Edit/New entry"), "<control>N", entry_new, 0, "<StockItem>", GTK_STOCK_NEW},
	{N_("/Edit/Edit entry"), "<control>E", entry_change, 0, NULL},
	{N_("/Edit/Delete entry"), NULL, entry_remove, 0, "<StockItem>", GTK_STOCK_DELETE},
	{N_("/Edit/sep1"), NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/Change password"), NULL, change_password, 0, NULL},
	{N_("/_View"), NULL, NULL, 0, "<Branch>"},
	{N_("/View/Show comments"), NULL, toggle_show_comments, 0, "<ToggleItem>"},
	{N_("/View/Show passwords"), NULL, toggle_show_passwords, 0, "<ToggleItem>"},
	{N_("/_Help"), NULL, NULL, 0, "<Branch>"},
	{N_("/_Help/Help"), NULL, gpasman_help, 0, "<StockItem>", GTK_STOCK_HELP},
	{N_("/_Help/About"), NULL, gpasman_about, 0, NULL},
};
/* }}} */

static GtkItemFactoryEntry popup_items[] = { /* {{{ */
	{N_("/_Edit"), NULL, entry_change, 0, NULL},
/*	{N_("/_New entry"), NULL, entry_new, 0, NULL}, */
	{N_("/_Delete"), NULL, entry_remove, 0, NULL}
};
/* }}} */

/*
 * creates the main window with all the gadgets
 */
void make_interface(void) /* {{{ */
{
	GtkWidget *vbox, *hbox, *menubar, *label, *menu;
	GList *iter;
	GtkTooltips *tooltips;
	GtkWidget *scroll_win;
	gint nmenu_items;

	GtkAccelGroup *accel_group;
	
	GtkTreeModel *tree_model;
	GtkWidget *tree_view;
	GtkCellRenderer *cell_renderer;
	GtkTreeViewColumn *tree_column;
	GtkTreeSelection *tree_selection;

	
	program = HILDON_PROGRAM(hildon_program_get_instance());
	g_set_application_name("gpasman");

	main_window = hildon_window_new();

	hildon_program_add_window(program, HILDON_WINDOW(main_window));

	gtk_window_set_default_size(GTK_WINDOW(main_window), 600, 300);
	gtk_window_set_title(GTK_WINDOW(main_window), main_window_title);
	g_signal_connect(GTK_OBJECT(main_window), "delete_event",
					 G_CALLBACK(gpasman_quit), NULL);
	g_signal_connect(GTK_OBJECT(main_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	gtk_window_set_wmclass(GTK_WINDOW(main_window), "Gpasman", "gpasman");

	vbox = gtk_vbox_new(FALSE, 0);
	/* gtk_container_border_width (GTK_CONTAINER (vbox), 5); */
	gtk_container_add(GTK_CONTAINER(main_window), vbox);


	/* menubar creation */
	nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
								"<main>", accel_group);
	gtk_item_factory_set_translate_func(item_factory, menu_translate,
										NULL, NULL);
	gtk_item_factory_create_items(item_factory,
								nmenu_items, menu_items,
								NULL);
	gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
	
	menubar = gtk_item_factory_get_widget(item_factory, "<main>");

	/* convert menubar to menu */
	menu = gtk_menu_new();
	iter = gtk_container_get_children(GTK_CONTAINER (menubar));

	while (iter) {
		GtkWidget *submenu;

		submenu = GTK_WIDGET (iter->data);
		gtk_widget_reparent(submenu, menu);

		iter = g_list_next (iter);
	}

	/* remove the usual menu, use the hildonized one */
	/* gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0); */ 
	hildon_window_set_menu(HILDON_WINDOW(main_window), GTK_MENU(menu));

	/* GtkTreeView */
	list_store = gtk_list_store_new(NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
									G_TYPE_STRING, G_TYPE_STRING);
	tree_model = GTK_TREE_MODEL(list_store);
	tree_view = gtk_tree_view_new_with_model(tree_model);
	g_object_unref(tree_model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree_view), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree_view), COLUMN_HOST);
	cell_renderer = gtk_cell_renderer_text_new();
	tree_column = gtk_tree_view_column_new_with_attributes(_("Host"),
								cell_renderer, "text", COLUMN_HOST, NULL);
	gtk_tree_view_column_set_sort_column_id(tree_column, COLUMN_HOST);
	gtk_tree_view_column_set_sort_indicator(tree_column, TRUE);
	gtk_tree_view_column_set_sizing(tree_column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(tree_column, 175);
	gtk_tree_view_column_set_resizable(tree_column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), tree_column);

	cell_renderer = gtk_cell_renderer_text_new();
	tree_column = gtk_tree_view_column_new_with_attributes(_("User"), cell_renderer,
											"text", COLUMN_USER, NULL);
	gtk_tree_view_column_set_sort_column_id(tree_column, COLUMN_USER);
	gtk_tree_view_column_set_sizing(tree_column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(tree_column, 150);
	gtk_tree_view_column_set_resizable(tree_column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), tree_column);

	cell_renderer = gtk_cell_renderer_text_new();
	passwd_column = gtk_tree_view_column_new_with_attributes(_("Password"), cell_renderer,
											"text", COLUMN_PASSWD, NULL);
	gtk_tree_view_column_set_sizing(passwd_column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(passwd_column, 150);
	gtk_tree_view_column_set_resizable(passwd_column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), passwd_column);
	gtk_tree_view_column_set_visible(passwd_column, FALSE);

	cell_renderer = gtk_cell_renderer_text_new();
	comments_column = gtk_tree_view_column_new_with_attributes(_("Comments"), cell_renderer,
											"text", COLUMN_COMMENT, NULL);
	gtk_tree_view_column_set_fixed_width(comments_column, 200);
	gtk_tree_view_column_set_resizable(comments_column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), comments_column);
	tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	gtk_tree_selection_set_mode(tree_selection, GTK_SELECTION_BROWSE);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), TRUE);
	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tree_view), GTK_TREE_VIEW_GRID_LINES_VERTICAL);

	/* scrolled window where GtkTreeView is inside */
	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll_win),
										GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(scroll_win), tree_view);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_win, TRUE, TRUE, 0);
	gtk_widget_show_all(scroll_win);
	

	/* status bar */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gtk_label_new(_(" Clipboard: "));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	clipb_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), clipb_entry, FALSE, FALSE, 0);
	gtk_widget_show(clipb_entry);
	gtk_entry_set_editable(GTK_ENTRY(clipb_entry), 0);

	tooltips = gtk_tooltips_new();
	gtk_tooltips_set_tip(tooltips, clipb_entry,
						 _("Doubleclick an entry to copy the password"),
						 NULL);

	statusbar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(hbox), statusbar, TRUE, TRUE, 0);
	gtk_widget_show(statusbar);

	context_id =
		gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),
									 "Gpasman: ");
	statusbar_message(_("Initialising..."), 1000);



	g_signal_connect(tree_view, "row_activated",
						G_CALLBACK(tree_selection_activated), clipb_entry);
	g_signal_connect(tree_selection, "changed",
						G_CALLBACK(tree_selection_changed), GTK_ENTRY(clipb_entry));
	g_signal_connect(tree_view, "button_press_event",
						G_CALLBACK(tree_selection_popup_menu), NULL);

	

	/* finaly main window gadgets */
	gtk_widget_realize(main_window);
	gtk_widget_show_all(main_window);
	
	/* Set "Show comments" menu item to on, which they are */
	gtk_check_menu_item_set_active(
			GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory,
													   "/View/Show comments")),
			TRUE);
			
	/* set clipboard */
	clipboard = gtk_widget_get_clipboard(GTK_WIDGET(main_window), 
											GDK_SELECTION_PRIMARY);
	/* Hildon/Maemo specific lines to make clipboard working */
	gtk_clipboard_set_can_store (clipboard, NULL, 0);
	gtk_clipboard_store(clipboard);
}
/* }}} */

static char *menu_translate(const char *path, gpointer data) /* {{{ */
{
    return (char *)gettext (path);
}
/* }}} */

/* 
 * makes menu entries sensible
 * ONLY this function modifies the current_set.fileopen and 
 * current_set.modified variables!
 * I can NOT use here inttool definition _(string) because the translation
 * GtkItemFactory doesn't allow direct translation, that is also the reason
 * why we have the menu_translate() function.
 */
void set_widgets_sensitive(gboolean open_value, gboolean modified_value) /* {{{ */
{
	if(open_value || !modified_value)	{
		gtk_widget_set_sensitive(
			gtk_item_factory_get_item(item_factory, "/File/Save"),
			modified_value);
	}
	gtk_widget_set_sensitive(
		gtk_item_factory_get_item(item_factory, "/File/Save As..."),
		open_value);
	gtk_widget_set_sensitive(
		gtk_item_factory_get_item(item_factory, "/File/Close"),
		open_value);
	gtk_widget_set_sensitive(
		gtk_item_factory_get_item(item_factory, "/Edit/Edit entry"),
		open_value);
	gtk_widget_set_sensitive(
		gtk_item_factory_get_item(item_factory, "/Edit/Delete entry"),
		open_value);
	gtk_widget_set_sensitive(
		gtk_item_factory_get_item(item_factory, "/Edit/Change password"),
		open_value);
	gtk_widget_set_sensitive(
		gtk_item_factory_get_item(item_factory, "/View/Show passwords"),
		open_value);

	current_set.fileopen = open_value;
	current_set.modified = modified_value;
}
/* }}} */

/*
 * on selection change clears the clipboard entry content
 */
void tree_selection_changed(GtkTreeSelection *selection, GtkEntry *clipb_entry) /* {{{ */
{
	GtkTreeModel *model;
	gchar *clipb_user = NULL;  /* username; to set into clipboard */
	
	if(clipboard_used)	{
		gtk_entry_set_text(clipb_entry, "\0");
		gtk_editable_select_region(GTK_EDITABLE(clipb_entry), 0, -1);
		gtk_editable_copy_clipboard(GTK_EDITABLE(clipb_entry));
		clipboard_used = GPASMAN_CLIPB_EMPTY;
	}
	
	if (gtk_tree_selection_get_selected(selection, &model, &selected_row)) {
		gtk_tree_model_get(model, &selected_row, COLUMN_USER, &clipb_user, -1);
	}

	if(clipb_user)	{
		gtk_entry_set_text(GTK_ENTRY(clipb_entry), clipb_user);
		gtk_editable_select_region(GTK_EDITABLE(clipb_entry), 0, -1);
		gtk_editable_copy_clipboard(GTK_EDITABLE(clipb_entry));
		gtk_entry_set_visibility(clipb_entry, TRUE);
		clipboard_used = GPASMAN_CLIPB_USER;
		statusbar_message(_("Username copied to clipboard"), 2000);
	}
		
}
/* }}} */

/*
 * on a double click copies the passwd to clipboard (which is visible)
 */
void tree_selection_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, GtkEntry *clipb_entry) /* {{{ */
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *clipb_passwd;		/* password; to set into clipboard */

	model = gtk_tree_view_get_model(treeview);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter,
					COLUMN_PASSWD, &clipb_passwd,
					-1);
	if(clipb_passwd)	{
		gtk_entry_set_text(GTK_ENTRY(clipb_entry), clipb_passwd);
		gtk_editable_select_region(GTK_EDITABLE(clipb_entry), 0, -1);
		gtk_editable_copy_clipboard(GTK_EDITABLE(clipb_entry));
		if(clipboard_show_passwd)
			gtk_entry_set_visibility(clipb_entry, TRUE);
		else
			gtk_entry_set_visibility(clipb_entry, FALSE);
		clipboard_used = GPASMAN_CLIPB_PASSWD;
		statusbar_message(_("Password copied to clipboard"), 2000);
	}
	selected_row = iter;

}
/* }}} */

/*
 * on right mouse (mouse button 3) click creates popup menu
 */
gboolean tree_selection_popup_menu(GtkWidget *widget, GdkEventButton *event) /* {{{ */
{
	gint npopup_items = 0;
	GtkItemFactory *popup_item_factory = NULL;
	GtkWidget *popup_menu = NULL;
	
	if(gtk_list_store_iter_is_valid(list_store, &selected_row))	{
		if(event->button == 3)	{
			/* popup_menu creation */
			npopup_items = sizeof(popup_items) / sizeof(popup_items[0]);
			popup_item_factory = gtk_item_factory_new(GTK_TYPE_MENU,
										"<popup>", NULL);
			gtk_item_factory_create_items(popup_item_factory, npopup_items,
										popup_items, NULL);
			popup_menu = gtk_item_factory_get_widget(popup_item_factory,
										"<popup>");
			gtk_menu_popup(GTK_MENU(popup_menu), NULL, NULL, NULL, NULL,
										event->button, event->time);
			return TRUE;
		}
	}
	return FALSE;
}
/* }}} */

/********************************************************************
 * DIALOG WINDOWS
 ********************************************************************/

/*
 * helper for return_password()
 */
void password_ok(GtkWidget *w, GtkWidget *dialog) /* {{{ */
{
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}
/* }}} */

/*
 * widget that asks for the main passwd 
 * helper for open_file()
 */
void return_password(void) /* {{{ */
{
	GtkWidget *dialog, *label, *table;
	G_CONST_RETURN gchar *tmp;
	gchar *label_str, *tmp2 = NULL;
	gint dialog_response;

	dialog = gtk_dialog_new_with_buttons(_("Master password"),
						GTK_WINDOW(main_window), GTK_DIALOG_MODAL,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	
	table = gtk_table_new(2, 1, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_container_set_border_width(GTK_CONTAINER(table), 6);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table,
						TRUE, TRUE, 0);
	
	label_str = g_strdup_printf(_("Enter master password for\n%s:"),
							current_set.filename);
	label = gtk_label_new(label_str);
	g_free(label_str);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	
	pwd_entry = gtk_entry_new_with_max_length(100);
	gtk_entry_set_visibility(GTK_ENTRY(pwd_entry), 0);
	/* turn off auto-caps-first */
	hildon_gtk_entry_set_input_mode(GTK_ENTRY(pwd_entry), HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_INVISIBLE);

	g_signal_connect(GTK_OBJECT(pwd_entry), "activate",
					 G_CALLBACK(password_ok), dialog);
	gtk_table_attach_defaults(GTK_TABLE(table), pwd_entry, 0, 1, 1, 2);
	gtk_widget_show_all(dialog);
	
	dialog_response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(dialog_response == GTK_RESPONSE_OK)	{
		tmp = gtk_entry_get_text(GTK_ENTRY(pwd_entry));
		if(strlen(tmp) < GPASMAN_MIN_PWD_LENGTH) {
			gchar *str1;
			str1 = g_strdup_printf(_("Password has to be > %d characters"), GPASMAN_MIN_PWD_LENGTH);
			statusbar_message(str1, 8000);
			g_free(str1);
			gtk_widget_destroy(dialog);
			return;
		}
#ifdef DEBUG
		gpasman_debug("tmp=%s", tmp);
#endif
		tmp2 = g_malloc(strlen(tmp) + 1);
		strncpy(tmp2, tmp, strlen(tmp) + 1);
		expand_key(tmp2);
		current_set.password = TRUE;
	}
	gtk_widget_destroy(dialog);

}
/* }}} */

/*
 * file selector widget
 * helper for file_open() and file_save_as()
 */
void return_file(void) /* {{{ */
{
	GtkWidget *fs;
	gint fs_result;

	fs = gtk_file_selection_new(_("Select file"));
	gtk_widget_show(fs);
	gtk_grab_add(GTK_WIDGET(fs));
	fs_result = gtk_dialog_run(GTK_DIALOG(fs));
	switch(fs_result)	{
		case GTK_RESPONSE_OK:
			tmp_gchar_const =
					gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
			break;
		default:
			break;
	}
	gtk_widget_destroy(fs);
}
/* }}} */

/*
 * opens the file and creates the TreeView
 */
void load_file(void) /* {{{ */
{
	gchar *entry[NUM_COLUMNS];
	gint retval, count;

	GtkTreeIter iter;

	if (current_set.password) {
		current_set.password = FALSE;
	}
	
	/* ask user for passwd */
	return_password();
	if (!current_set.password) {
		return;
	}
#ifdef DEBUG
	gpasman_debug("password is set=%d", current_set.password);
#endif

	retval = load_init((char *)current_set.filename);
#ifdef DEBUG
	gpasman_debug("load_init retval=%d", retval);
#endif
	switch (retval) {
	case GPASMAN_FILE_INIT_SUCCESS:
		statusbar_message(_("Load initialised"), 3000);
		break;
	case GPASMAN_FILE_INIT_ERROR:
		statusbar_message(_("Can't open file"), 8000);
		return;
	case GPASMAN_FILE_INIT_BAD_PERMISSION:
		statusbar_message(_("File permissions bad, should be -rw-------"), 8000);
		return;
	case GPASMAN_FILE_INIT_SYMLINK:
		statusbar_message(_("File is a symlink"), 8000);
		return;
	case GPASMAN_FILE_INIT_BAD_STATUS:
		statusbar_message(_("Can't get file status"), 8000);
		return;
	default:
		statusbar_message(_("Internal program error"), 8000);
		return;
	}
	
	
	while ((retval = load_entry(entry)) == GPASMAN_FILE_INIT_SUCCESS) {
#ifdef DEBUG
		gpasman_debug("entry[0]=%s, entry[1]=%s, entry[2]=%s, entry[3]=%s",
						 entry[0], entry[1], entry[2], entry[3]);
#endif
		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter,
					COLUMN_HOST, entry[COLUMN_HOST],
					COLUMN_USER, entry[COLUMN_USER],
					COLUMN_PASSWD, entry[COLUMN_PASSWD],
					COLUMN_COMMENT, entry[COLUMN_COMMENT],
					-1);
		for (count = 0; count < NUM_COLUMNS; count++) {
			g_free(entry[count]);
#ifdef DEBUG
			gpasman_debug("freeiing entry[%d]", count);
#endif
		}
	}
	if(retval == GPASMAN_FILE_INIT_LAST_LOADED) {
		statusbar_message(_("Last entry loaded"), 3000);
	}
	else	{
		g_assert_not_reached();
	}
	
	retval = load_finalize();
	tmp_gchar_const = g_strdup_printf("%s - %s", 
								current_set.filename, main_window_title);
	gtk_window_set_title(GTK_WINDOW(main_window), tmp_gchar_const);
	tmp_gchar_const = NULL;
	set_widgets_sensitive(TRUE, FALSE);
	current_set.modified = 0;
#ifdef DEBUGload
	gpasman_debug("load_finalize retval=%d", retval);
#endif
}
/* }}} */

/*
 * helper for file_save and file_save_as
 */
void save_file(void) /* {{{ */
{
	gint retval;
	char *entry[NUM_COLUMNS];

	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean loopHelper = FALSE;

	if (!current_set.filename)
		file_save_as(NULL, NULL);
	if (!current_set.password) {
		return_password();
		if (!current_set.password) {
			return;
		}
	}

	retval = save_init((char *)current_set.filename);
	switch (retval) {
	case 1:
		statusbar_message(_("Save initialised"), 3000);
		break;
	case 0:
		statusbar_message(_("Can't open/create file"), 8000);
		return;
	case -1:
		statusbar_message(_("File permissions bad, should be -rw-------"),
						  8000);
		return;
	case -2:
		statusbar_message(_("File is a symlink"), 8000);
		return;
	case -3:
		statusbar_message(_("Can't get file status"), 8000);
		return;
	default:
		statusbar_message(_("Internal program error"), 8000);
		return;
	}
	
	model = GTK_TREE_MODEL(list_store);
	loopHelper = gtk_tree_model_get_iter_first(model, &iter);
	while(loopHelper)	{
		gtk_tree_model_get(model, &iter,
				COLUMN_HOST, &entry[COLUMN_HOST],
				COLUMN_USER, &entry[COLUMN_USER],
				COLUMN_PASSWD, &entry[COLUMN_PASSWD],
				COLUMN_COMMENT, &entry[COLUMN_COMMENT],
				-1);
		if (save_entry(entry) == GPASMAN_FILE_INIT_EOF) {
			statusbar_message(_("Error writing file, contents not saved"),
							  8000);
			save_finalize();
			return;
		}
		loopHelper = gtk_tree_model_iter_next(model, &iter);
	}
	if (save_finalize() == GPASMAN_FILE_INIT_EOF) {
		statusbar_message(_("Error writing file, contents not saved"), 8000);
	}
	else {
		statusbar_message(_("Save finished"), 3000);
		set_widgets_sensitive(TRUE, FALSE);
	}
	tmp_gchar_const = g_strdup_printf("%s - %s", 
								current_set.filename, main_window_title);
	gtk_window_set_title(GTK_WINDOW(main_window), tmp_gchar_const);
	tmp_gchar_const = NULL;
}
/* }}} */

void file_close(GtkWidget *w, gpointer data) /* {{{ */
{
	if(modified_check(_("Do you want to save the changes you made to "
						"your passwords before closing?\n\n"
						"Your changes will be lost if you don't save them."))
			== 1)
	{
		clear_entries();
	}
}
/* }}} */

/* helper for file_close() and file_open() */
void clear_entries(void) /* {{{ */
{
	gtk_list_store_clear(list_store);
	gtk_entry_set_text(GTK_ENTRY(clipb_entry), "");
	set_widgets_sensitive(FALSE, FALSE);
	current_set.password = FALSE;
	g_free(current_set.filename);
	current_set.filename = NULL;
}
/* }}} */

/* 
 * helper for almost everything!
 * checks if the file is modified, if yes it ask:
 * 1) to continue without saving
 * 2) to continue with saving
 * 3) cancel last operation request
 */
gint modified_check(gchar *message) /* {{{ */
{
	GtkWidget *dialog;
	gint dialog_response;

	if (current_set.modified != 0) {
		dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
						GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_NONE, 
						message);
		gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
		gtk_dialog_add_buttons(GTK_DIALOG(dialog),
								GTK_STOCK_NO, GTK_RESPONSE_NO,
								GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
								GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
								NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
		dialog_response = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		switch (dialog_response) {
		case GTK_RESPONSE_ACCEPT:
			save_file();
			return 1;
			break;
		case GTK_RESPONSE_NO:
			return 1;
			break;
		default:
			return 0;
			break;
		}
	}
	else {
		return 1;
	}
}
/* }}} */

/*
 * dialog for editing or modifying an entry
 */
void entry_dialog(gint type) /* {{{ */
{
	gtkentry_struct *dialog = NULL;
	GtkWidget *table, *label;
	gchar *window_title, *button_title;
	gint dialog_response;
	HildonGtkInputMode password_mode = HILDON_GTK_INPUT_MODE_FULL;

	GtkTreeModel *model;
	gchar *str_host, *str_user, *str_passwd, *str_comment;

	if(type == GPASMAN_ENTRY_DIALOG_ADD)	{
		window_title = _("New entry");
		button_title = _("_Add");
		/* make the password invisible only when entering it first */
		password_mode |= HILDON_GTK_INPUT_MODE_INVISIBLE;
	}
	else	{
		window_title = _("Edit entry");
		button_title = _("_Edit");
		if(!gtk_list_store_iter_is_valid(list_store, &selected_row)) {
			statusbar_message(_("You have to select a host to update one..."), 8000);
			g_free(dialog);
			return;
		}
	}
	
	dialog = g_malloc(sizeof(gtkentry_struct));
	dialog->type = type;
	dialog->window = gtk_dialog_new_with_buttons(
						window_title, GTK_WINDOW(main_window),
						GTK_DIALOG_MODAL,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
						NULL);
	gtk_window_position(GTK_WINDOW(dialog->window), GTK_WIN_POS_MOUSE);

	table = gtk_table_new(4, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog->window)->vbox), table);

	/* creating the labels and entries */
	label = gtk_label_new(_("Host:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	dialog->entry1 = gtk_entry_new_with_max_length(100);
	gtk_table_attach_defaults(GTK_TABLE(table), dialog->entry1, 1, 2, 0, 1);
	/* turn off auto-caps-first */
	hildon_gtk_entry_set_input_mode(GTK_ENTRY(dialog->entry1), HILDON_GTK_INPUT_MODE_FULL);
	
	label = gtk_label_new(_("User:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	dialog->entry2 = gtk_entry_new_with_max_length(50);
	gtk_table_attach_defaults(GTK_TABLE(table), dialog->entry2, 1, 2, 1, 2);
	/* turn off auto-caps-first */
	hildon_gtk_entry_set_input_mode(GTK_ENTRY(dialog->entry2), HILDON_GTK_INPUT_MODE_FULL);
	
	label = gtk_label_new(_("Password:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
	dialog->entry3 = gtk_entry_new_with_max_length(50);
	gtk_table_attach_defaults(GTK_TABLE(table), dialog->entry3, 1, 2, 2, 3);
	/* turn off auto-caps-first */
	hildon_gtk_entry_set_input_mode(GTK_ENTRY(dialog->entry3), password_mode);
	
	label = gtk_label_new(_("Comment:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);
	dialog->entry4 = gtk_entry_new_with_max_length(300);
	gtk_table_attach_defaults(GTK_TABLE(table), dialog->entry4, 1, 2, 3, 4);

	/* if modify; filling in the current values */
	if(dialog->type == GPASMAN_ENTRY_DIALOG_MODIFY)	{
		model = GTK_TREE_MODEL(list_store);
		gtk_tree_model_get(model, &selected_row,
					COLUMN_HOST, &str_host,
					COLUMN_USER, &str_user,
					COLUMN_PASSWD, &str_passwd,
					COLUMN_COMMENT, &str_comment,
					-1);
		gtk_entry_set_text(GTK_ENTRY(dialog->entry1), str_host);
		gtk_entry_set_text(GTK_ENTRY(dialog->entry2), str_user);
		gtk_entry_set_text(GTK_ENTRY(dialog->entry3), str_passwd);
		gtk_entry_set_text(GTK_ENTRY(dialog->entry4), str_comment);
	}
	
	
	gtk_window_set_focus(GTK_WINDOW(dialog->window), dialog->entry1);
	gtk_widget_show_all(dialog->window);
	dialog_response = gtk_dialog_run(GTK_DIALOG(dialog->window));
	switch(dialog_response)	{
		case GTK_RESPONSE_ACCEPT:
			entry_dialog_ok(NULL, dialog);
			break;
		default:
			break;
	}
	gtk_widget_destroy(dialog->window);
	g_free(dialog);
}
/* }}} */

/*
 * entry_dialog helper
 */
void entry_dialog_ok(GtkWidget * w, gtkentry_struct * data) /* {{{ */
{
	gchar *entry_text[4];
	gint count;
	
	GtkTreeIter iter;

	entry_text[0] =
		gtk_editable_get_chars(GTK_EDITABLE(data->entry1), 0, -1);
	entry_text[1] =
		gtk_editable_get_chars(GTK_EDITABLE(data->entry2), 0, -1);
	entry_text[2] =
		gtk_editable_get_chars(GTK_EDITABLE(data->entry3), 0, -1);
	entry_text[3] =
		gtk_editable_get_chars(GTK_EDITABLE(data->entry4), 0, -1);
	for (count = 0; count < 4; count++) {
		if (strlen(entry_text[count]) == 0) {
			strcpy(entry_text[count], " ");
		}
	}
	if (data->type == GPASMAN_ENTRY_DIALOG_ADD) {
		gtk_list_store_append(list_store, &iter);
	}
	else	{
		iter = selected_row;
	}
	gtk_list_store_set(list_store, &iter,
				COLUMN_HOST, entry_text[0],
				COLUMN_USER, entry_text[1],
				COLUMN_PASSWD, entry_text[2],
				COLUMN_COMMENT, entry_text[3],
				-1);
	
	
	set_widgets_sensitive(TRUE, TRUE);
}
/* }}} */

void change_password(GtkWidget * button, gpointer data) /* {{{ */
{
	gtkentry_struct *dialog;
	GtkWidget *label, *table;

	GtkWidget *modified_dialog;
	gint modified_dialog_result, dialog_result;

	/* we don't use here modified_check() function because we need here only
	 * two button (ok and cancel). */
	if (current_set.modified != 0) {
		modified_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
												 GTK_DIALOG_MODAL,
												 GTK_MESSAGE_QUESTION,
												 GTK_BUTTONS_OK_CANCEL,
												 _("To change the master password "
												 "I have to save the modifications.\n\n"
												 "Save modifications to continue?"));
		modified_dialog_result =
			gtk_dialog_run(GTK_DIALOG(modified_dialog));
		gtk_widget_destroy(modified_dialog);
		switch (modified_dialog_result) {
		case GTK_RESPONSE_OK:
			save_file();
			break;
		default:
			return;
		}
	}

	dialog = g_malloc(sizeof(gtkentry_struct));

	dialog->window = gtk_dialog_new_with_buttons(
						_("Change master password"),
						GTK_WINDOW(main_window),
						GTK_DIALOG_MODAL,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						GTK_STOCK_OK, GTK_RESPONSE_OK,
						NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog->window), GTK_RESPONSE_CANCEL);
	gtk_grab_add(dialog->window);
	gtk_window_position(GTK_WINDOW(dialog->window), GTK_WIN_POS_MOUSE);

	table = gtk_table_new(3, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog->window)->vbox), table);

/*	label = gtk_label_new(_("Old password:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	dialog->entry1 = gtk_entry_new_with_max_length(25);
	gtk_entry_set_visibility(GTK_ENTRY(dialog->entry1), 0);
	gtk_entry_set_text(GTK_ENTRY(dialog->entry1), "");
	gtk_table_attach_defaults(GTK_TABLE(table), dialog->entry1, 1, 2, 0, 1);
*/	
	label = gtk_label_new(_("New password:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	dialog->entry2 = gtk_entry_new_with_max_length(25);
	gtk_entry_set_visibility(GTK_ENTRY(dialog->entry2), 0);
	gtk_entry_set_text(GTK_ENTRY(dialog->entry2), "");
	/* turn off auto-caps-first */
	hildon_gtk_entry_set_input_mode(GTK_ENTRY(dialog->entry2), HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_INVISIBLE);
		
	gtk_table_attach_defaults(GTK_TABLE(table), dialog->entry2, 1, 2, 1, 2);

	label = gtk_label_new(_("Confirm password:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
	dialog->entry3 = gtk_entry_new_with_max_length(25);
	gtk_entry_set_text(GTK_ENTRY(dialog->entry3), "");
	/* turn off auto-caps-first */
	hildon_gtk_entry_set_input_mode(GTK_ENTRY(dialog->entry3), HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_INVISIBLE);

	gtk_entry_set_visibility(GTK_ENTRY(dialog->entry3), 0);
	gtk_table_attach_defaults(GTK_TABLE(table), dialog->entry3, 1, 2, 2, 3);

	
	gtk_widget_show_all(dialog->window);
	gtk_window_set_focus(GTK_WINDOW(dialog->window), dialog->entry2);
	dialog_result = gtk_dialog_run(GTK_DIALOG(dialog->window));
	switch(dialog_result)	{
		case GTK_RESPONSE_OK:
			/* the first argument is an artifact that has to be cleaned */
			check_password(dialog);
			break;
		default:
			break;
	}
	gtk_widget_destroy(dialog->window);
}
/* }}} */

/*
 * change_password helper
 */
void check_password(gtkentry_struct * data) /* {{{ */
{
	gchar *tmppass;

#ifdef DEBUG
	gpasman_debug("window=%p", data->window);
#endif

#define GSTRNDUP(entry)	g_strndup(gtk_entry_get_text(GTK_ENTRY(entry)), GPASMAN_MAX_PWD_LENGTH)
	if (current_set.password) {
		tmppass = GSTRNDUP(data->entry2);
		if(strlen(tmppass) < GPASMAN_MIN_PWD_LENGTH)	{
			gchar *str1;
			str1 = g_strdup_printf(_("Master password has to be > %d characters"),
									GPASMAN_MIN_PWD_LENGTH);
			statusbar_message(str1, 6000);
			g_free(str1);
			g_free(tmppass);
			return;
		}
		if(g_strncasecmp(
				tmppass, gtk_entry_get_text(GTK_ENTRY(data->entry3)),
				GPASMAN_MAX_PWD_LENGTH) != 0)
		{
			statusbar_message(_("New master passwords not identical"), 6000);
			g_free(tmppass);
			return;
		}
	}
	else {
		tmppass = GSTRNDUP(data->entry2);
		if (strlen(tmppass) < GPASMAN_MIN_PWD_LENGTH) {
			gchar *str1;
			str1 = g_strdup_printf(_("Master password has to be > %d characters"),
									GPASMAN_MIN_PWD_LENGTH);
			statusbar_message(str1, 6000);
			g_free(str1);
			g_free(tmppass);
			return;
		}
	}
	expand_key(tmppass);
	current_set.password = TRUE;
	statusbar_message(_("Master password changed"), 8000);

	save_file();
	set_widgets_sensitive(TRUE, FALSE);
}
/* }}} */

void statusbar_message(gpointer message, gint time) /* {{{ */
{
	gint count;

	count = gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, (gchar *) message);
	gtk_timeout_add(time, statusbar_remove, GINT_TO_POINTER(count));
}
/* }}} */

gint statusbar_remove(gpointer message_id) /* {{{ */
{
	gtk_statusbar_remove(GTK_STATUSBAR(statusbar), context_id, GPOINTER_TO_INT(message_id));
	return 0;
}
/* }}} */

/********************************************************************
 * MENUBAR CALLBACKS
 ********************************************************************/
void file_open(GtkWidget * w, gpointer data) /* {{{ */
{
	if(modified_check(_("Do you want to save the changes you made to "
						"your passwords before opening a new file?\n\n"
						"Your changes will be lost if you don't save them."))
			== 0)
	{
		return;
	}
	return_file();
	if (tmp_gchar_const	&& g_file_test(tmp_gchar_const, G_FILE_TEST_IS_REGULAR))	{
		clear_entries();	/* clears also current_set.filename */
		current_set.filename = g_strdup(tmp_gchar_const);
		load_file();
	}
	else {
		if(!tmp_gchar_const)	{
			statusbar_message(_("No file selected"), 4000);
		}
		else	{
			statusbar_message(_("Could not open file"), 4000);
		}
	}
	tmp_gchar_const = NULL;
}
/* }}} */

void file_save(GtkWidget * w, gpointer data) /* {{{ */
{
	if (current_set.filename) {
		save_file();
	}
	else {
		file_save_as(w, data);
	}
}
/* }}} */

void file_save_as(GtkWidget * w, gpointer data) /* {{{ */
{
	return_file();
	if(tmp_gchar_const) {
		if(current_set.filename) g_free(current_set.filename);
		current_set.filename = g_strdup(tmp_gchar_const);
		tmp_gchar_const = NULL;
		save_file();
	}
}
/* }}} */

void entry_new(GtkWidget * w, gpointer data) /* {{{ */
{
	entry_dialog(GPASMAN_ENTRY_DIALOG_ADD);
}
/* }}} */

void entry_change(GtkWidget * w, gpointer data) /* {{{ */
{
	entry_dialog(GPASMAN_ENTRY_DIALOG_MODIFY);
}
/* }}} */

void entry_remove(GtkWidget * w, gpointer data) /* {{{ */
{
	if(gtk_list_store_iter_is_valid(list_store, &selected_row))	{
		gtk_list_store_remove(list_store, &selected_row);
		set_widgets_sensitive(TRUE, TRUE);
		statusbar_message(_("Successfully removed."), 8000);
	}
	else	{
		statusbar_message(_("You have to select a host to remove one..."), 8000);
	}
}
/* }}} */

void toggle_show_passwords(gpointer callback_data, guint callback_action, GtkWidget * toggle) /* {{{ */
{
	if ((GTK_CHECK_MENU_ITEM(toggle)->active)) {
		gtk_tree_view_column_set_visible(passwd_column, TRUE);
		gtk_entry_set_visibility(GTK_ENTRY(clipb_entry), TRUE);
		clipboard_show_passwd = TRUE;
	}
	else {
		gtk_tree_view_column_set_visible(passwd_column, FALSE);
		if (clipboard_used == GPASMAN_CLIPB_PASSWD) 
			gtk_entry_set_visibility(GTK_ENTRY(clipb_entry), FALSE);
		clipboard_show_passwd = FALSE;
	}
}
/* }}} */

void toggle_show_comments(gpointer callback_data, guint callback_action, GtkWidget * toggle) /* {{{ */
{
	if ((GTK_CHECK_MENU_ITEM(toggle)->active)) {
		gtk_tree_view_column_set_visible(comments_column, TRUE);
	}
	else {
		gtk_tree_view_column_set_visible(comments_column, FALSE);
	}
}
/* }}} */

/********************************************************************
 * MISC CALLBACKS FROM MENUBAR
 ********************************************************************/

void gpasman_exit(GtkWidget *widget, gtkentry_struct *data) /* {{{ */
{
	if(modified_check(_("Do you want to save the changes you made to "
						"your passwords before quitting?\n\n"
						"Your changes will be lost if you don't save them."))
			== 1)
	{
		gtk_main_quit();
	}
}
/* }}} */

gboolean gpasman_quit(GtkWidget *widget, GdkEvent *event, gpointer data) /* {{{ */
{
	gpasman_exit(widget, NULL);
	return TRUE;
}
/* }}} */

void gpasman_about(GtkWidget *button, gtkentry_struct *data) /* {{{ */
{
	info_dialog
		(_("Gpasman\nOriginally written by Olivier Sessink\n"
		 "because he forgot a password\n\n"
		 "Hildonized by Antony Dovgal <tony@daylessday.org>\n\n"
		 "(C) 1998-1999 - Olivier Sessink <gpasman@nl.linux.org>\n"
		 "(C) 2003 - T. Bugra Uytun <t.bugra@uytun.com>\n"
		 "http://gpasman.sourceforge.net\n\n"
		 "the rc2 encryption is based on the rc2\n"
		 "library by Matthew Palmer"),
		 _("About gpasman"));
}
/* }}} */

void gpasman_help(GtkWidget *button, gtkentry_struct *data) /* {{{ */
{
	info_dialog(_("Gpasman - a password manager\n\n"
				"Gpasman encrypts a lot of passwords using one master\n"
				"password. Every time you load or save a new file a\n"
				"master password is asked to (un)lock the contents.\n\n"
				"You can change this master password using \"change password\".\n"
				"The contents can be edited using \"new entry\", \"update entry\"\n"
				"and \"remove entry\". If you have a lot of entries, sort them by\n"
				"clicking on the titles of the list.\n\n"
				"Doubleclicking an entry puts the password on the clipboard\n"
				"so you can simply paste it into your\n"
				"password asking application using the middle mouse\n"
				"button. Errors messages are put only on the statusbar,\n"
				"no annoying dialogs. If present, the file ~/.gpasman will be\n"
				"opened at startup. The rest should be pretty self\n"
				"explaining stuff."),
				_("Help"));
}
/* }}} */

void info_dialog(gchar *message, gchar *title) /* {{{ */
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
								GTK_DIALOG_DESTROY_WITH_PARENT,
								GTK_MESSAGE_INFO,
								GTK_BUTTONS_OK, message);
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

	g_signal_connect_swapped(GTK_OBJECT(dialog), "response",
								G_CALLBACK(gtk_widget_destroy),
								GTK_OBJECT(dialog));

	gtk_widget_show_all(dialog);
}
/* }}} */

/*
 * vim600: sw=4 ts=4 fdm=marker
 */

