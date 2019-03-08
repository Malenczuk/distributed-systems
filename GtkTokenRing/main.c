#include "TokenRingTCP.c"

GObject *window;
GObject *login_dialog;
GtkEntry *id_input;
GtkEntry *local_port_input;
GtkEntry *out_ip_input;
GtkEntry *out_port_input;
GtkCheckButton *token_check_button;
GtkTextView *msg_content;
GtkTextView *contents;
GtkEntry *msg_destination;
GObject *msg_send_button;
GError *error = NULL;

void closeApp(GtkWidget *window, gpointer data) { gtk_main_quit(); }

void toggleToken(GtkWidget *widget, gpointer data) {
    TOKEN ^= 1;
    g_print("TOKEN: %d\n", TOKEN);
}

void content_message_append(char* name, char *text, char* style){
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(contents);
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(text_buffer, &end_iter);

    gtk_text_buffer_insert_with_tags_by_name(text_buffer,&end_iter, name, -1, style, NULL);
    gtk_text_buffer_insert_with_tags_by_name(text_buffer,&end_iter, ": ", -1, style, NULL);

    gtk_text_buffer_insert (text_buffer,&end_iter, text, -1);
    gtk_text_buffer_insert (text_buffer,&end_iter, "\n", -1);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW (contents), &end_iter, 0, 0, 0, 0);
}


static gboolean print_message(gpointer data){
    token *token = data;
    content_message_append((*token).source, (*token).data, "sender");
    free(token);
    return FALSE;
}

static void send_message(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(msg_content);
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_start_iter(text_buffer, &start_iter);
    gtk_text_buffer_get_end_iter(text_buffer, &end_iter);
    const char *destination = gtk_entry_get_text(msg_destination);
    char *content = gtk_text_buffer_get_text(text_buffer, &start_iter, &end_iter, FALSE);

    if (!strcmp(destination, "") || !strcmp(content, ""))
        return;

    g_print("destination: '%s'\n", destination);
    g_print("contents: '%s'\n", content);
    content_message_append(ID, content, "my_login");
    enQueue(queue, create_token(DATA, 2, ID, destination, content, strlen(content)));
    gtk_text_buffer_set_text(text_buffer, "", 0);
    gtk_widget_grab_focus((GtkWidget *) msg_content);
}

void *thread(void *arg){
    init();

    connect_tokenring();
    if(TOKEN) send_token(empty_token());

    struct epoll_event event;
    while (1) {
        if (epoll_wait(epoll, &event, 1, -1) == -1) FAILURE_EXIT(1, "epoll_wait", "\nError : epoll_wait failed\n")

        if (event.data.fd < 0)
            handle_connection(-event.data.fd);
        else
            handle_token(event.data.fd);

    }
}

static void conn(GtkWidget *widget, gpointer data) {
    strcpy(ID, gtk_entry_get_text(id_input));
    LOCAL_PORT = strtoul(gtk_entry_get_text(local_port_input), 0, 10);
    strcpy(OUT_IP, gtk_entry_get_text(out_ip_input));
    OUT_PORT = strtoul(gtk_entry_get_text(out_port_input), 0, 10);

    g_print("ID: %s\n", ID);
    g_print("LOCAL_PORT: %hu\n", LOCAL_PORT);
    g_print("OUT_IP: %s\n", OUT_IP);
    g_print("OUT_PORT: %hu\n", OUT_PORT);

    g_signal_handlers_disconnect_by_func(login_dialog, G_CALLBACK(closeApp), NULL);
    gtk_window_close((GtkWindow *) login_dialog);
    gtk_widget_show_all((GtkWidget *) window);
    g_thread_new("thread", thread, NULL);
}

int main(int argc, char *argv[]) {
    GtkBuilder *builder;
    GObject *button;

    gtk_init(&argc, &argv);

    /* Construct a GtkBuilder instance and load our UI description */
    builder = gtk_builder_new();
    if (gtk_builder_add_from_file(builder, "builder.ui", &error) == 0) {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    login_dialog = gtk_builder_get_object(builder, "login_dialog");
    window = gtk_builder_get_object(builder, "window");
    contents = (GtkTextView *) gtk_builder_get_object(builder, "contents");
    id_input = (GtkEntry *) gtk_builder_get_object(builder, "id_input");
    local_port_input = (GtkEntry *) gtk_builder_get_object(builder, "local_port_input");
    out_ip_input = (GtkEntry *) gtk_builder_get_object(builder, "out_ip_input");
    out_port_input = (GtkEntry *) gtk_builder_get_object(builder, "out_port_input");
    token_check_button = (GtkCheckButton *) gtk_builder_get_object(builder, "token_check_button");
    msg_destination = (GtkEntry *) gtk_builder_get_object(builder, "msg_destination");
    msg_content = (GtkTextView *) gtk_builder_get_object(builder, "msg_content");
    msg_send_button = gtk_builder_get_object(builder, "msg_send_button");

    //CREATE TAGS
    gtk_text_buffer_create_tag(gtk_text_view_get_buffer(contents), "my_login",
                               "foreground", "red", "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_buffer_create_tag(gtk_text_view_get_buffer(contents), "sender",
                               "foreground", "blue", "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_buffer_create_tag(gtk_text_view_get_buffer(contents), "italic",
                               "style", PANGO_STYLE_ITALIC, NULL);

    /* Connect signal handlers to the constructed widgets. */

    g_signal_connect (window, "destroy", G_CALLBACK(closeApp), NULL);
    g_signal_connect (login_dialog, "destroy", G_CALLBACK(closeApp), NULL);
    g_signal_connect (msg_send_button, "clicked", G_CALLBACK(send_message), NULL);
    g_signal_connect (token_check_button, "toggled", G_CALLBACK(toggleToken), NULL);


    button = gtk_builder_get_object(builder, "login_dialog_exit");
    g_signal_connect (button, "clicked", G_CALLBACK(closeApp), NULL);

    button = gtk_builder_get_object(builder, "login_dialog_connect");
    g_signal_connect (button, "clicked", G_CALLBACK(conn), NULL);

    gtk_main();

    return 0;
}