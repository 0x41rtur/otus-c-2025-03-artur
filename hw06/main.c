#include <gtk/gtk.h>
#include <gio/gio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define COLUMN_NAME 0
#define NUM_COLUMNS 1

// Структура для представления файлов и директорий
typedef struct _FileItem {
    gchar *name;
    gchar *path;
    gboolean is_dir;
} FileItem;

static GType file_item_get_type(void) {
    static GType type = 0;
    if (type == 0) {
        static const GTypeInfo info = {
            sizeof(GObjectClass), NULL, NULL, NULL, NULL, NULL,
            sizeof(FileItem), 0, NULL, NULL
        };
        type = g_type_register_static(G_TYPE_OBJECT, "FileItem", &info, 0);
    }
    return type;
}

static void file_item_free(FileItem *item) {
    if (item) {
        g_free(item->name);
        g_free(item->path);
        g_free(item);
    }
}

static GListModel *get_children_cb(gpointer item_ptr, gpointer user_data) {
    FileItem *item = (FileItem *)item_ptr;
    if (!item->is_dir) return NULL;

    GListStore *store = g_list_store_new(G_TYPE_POINTER);

    DIR *dir = opendir(item->path);
    if (!dir) return G_LIST_MODEL(store);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (g_strcmp0(entry->d_name, ".") == 0 || g_strcmp0(entry->d_name, "..") == 0)
            continue;

        FileItem *child = g_new0(FileItem, 1);
        child->name = g_strdup(entry->d_name);
        child->path = g_build_filename(item->path, entry->d_name, NULL);
        struct stat st;
        if (stat(child->path, &st) == 0)
            child->is_dir = S_ISDIR(st.st_mode);
        else
            child->is_dir = FALSE;

        g_list_store_append(store, child);
    }
    closedir(dir);
    return G_LIST_MODEL(store);
}

static void setup_cb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_list_item_set_child(list_item, label);
}

static void bind_cb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    GtkWidget *label = gtk_list_item_get_child(list_item);
    if (!label) return;
    FileItem *item = (FileItem *)gtk_list_item_get_item(list_item);
    if (!item || !item->name) return;
    gtk_label_set_text(GTK_LABEL(label), item->name);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Tree");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 600);

    GListStore *root_store = g_list_store_new(G_TYPE_POINTER);
    gchar *cwd = g_get_current_dir();
    FileItem *root_item = g_new0(FileItem, 1);
    root_item->name = g_strdup(cwd);
    root_item->path = g_strdup(cwd);
    root_item->is_dir = TRUE;
    g_list_store_append(root_store, root_item);

    GtkTreeListModel *tree_model = gtk_tree_list_model_new(
        G_LIST_MODEL(root_store),
        FALSE,  // passthrough (иначе потомки не отображаются)
        TRUE,   // autoexpand
        get_children_cb,
        NULL,
        NULL
    );

    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_cb), NULL);

    GtkWidget *list_view = gtk_list_view_new(GTK_SELECTION_MODEL(gtk_single_selection_new(G_LIST_MODEL(tree_model))), factory);

    gtk_window_set_child(GTK_WINDOW(window), list_view);
    gtk_window_present(GTK_WINDOW(window));
    g_free(cwd);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.example.GtkTree", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
