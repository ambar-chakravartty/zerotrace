#include "include/gui.hpp"
#include "include/wipe.hpp"
#include <gtk/gtk.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <thread>

// --- Utilities ---

std::string formatSize(uint64_t bytes) {
    const char* suffixes[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int i = 0;
    double dblBytes = bytes;
    if (bytes > 1024) {
        for (i = 0; (bytes / 1024) > 0 && i < 5; i++, bytes /= 1024)
            dblBytes = bytes / 1024.0;
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << dblBytes << " " << suffixes[i];
    return ss.str();
}

// --- Data Structures ---

struct WipeDialogData {
    Device device;
    GtkWidget *dialog;
    GtkWidget *method_combo;
    GtkWidget *confirm_check;
};

// --- Callbacks ---

static void on_wipe_start(GtkButton* btn, gpointer user_data) {
    WipeDialogData* data = (WipeDialogData*)user_data;
    
    // Check confirmation
    if (!gtk_check_button_get_active(GTK_CHECK_BUTTON(data->confirm_check))) {
        GtkWidget *msg = gtk_message_dialog_new(
            GTK_WINDOW(data->dialog),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "Please confirm that you understand this action will permanently erase data."
        );
        gtk_window_present(GTK_WINDOW(msg));
        g_signal_connect(msg, "response", G_CALLBACK(gtk_window_destroy), NULL);
        return;
    }
    
    // Get selected wipe method
    int method_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(data->method_combo));
    WipeMethod method = WipeMethod::PLAIN_OVERWRITE;
    
    switch(method_idx) {
        case 0: method = WipeMethod::PLAIN_OVERWRITE; break;
        case 1: method = WipeMethod::ATA_SECURE_ERASE; break;
        case 2: method = WipeMethod::ENCRYPTED_OVERWRITE; break;
    }
    
    // Show confirmation message
    std::string msg_text = "You are about to wipe the ENTIRE device:\n\n";
    msg_text += "• " + data->device.path + " (" + data->device.name + ")\n";
    msg_text += "• Size: " + formatSize(data->device.sizeBytes) + "\n";
    msg_text += "\nThis action CANNOT be undone. Continue?";
    
    GtkWidget *confirm_dialog = gtk_message_dialog_new(
        GTK_WINDOW(data->dialog),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_YES_NO,
        "%s", msg_text.c_str()
    );
    
    int response = gtk_dialog_run(GTK_DIALOG(confirm_dialog));
    gtk_window_destroy(GTK_WINDOW(confirm_dialog));
    
    if (response == GTK_RESPONSE_YES) {
        // TODO: Start wipe in background thread
        std::cout << "Starting wipe operation on: " << data->device.path << std::endl;
        // In real implementation: spawn thread and call wipeDisk(data->device.path, method)
        
        gtk_window_close(GTK_WINDOW(data->dialog));
    }
}

static void show_wipe_config_dialog(GtkWindow* parent, const Device& device) {
    WipeDialogData* data = new WipeDialogData();
    data->device = device;
    
    // Create dialog
    data->dialog = gtk_window_new();
    gtk_window_set_transient_for(GTK_WINDOW(data->dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(data->dialog), TRUE);
    gtk_window_set_title(GTK_WINDOW(data->dialog), "Wipe Raw Storage Device");
    gtk_window_set_default_size(GTK_WINDOW(data->dialog), 600, 450);
    
    // Main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_top(main_box, 20);
    gtk_widget_set_margin_bottom(main_box, 20);
    gtk_widget_set_margin_start(main_box, 30);
    gtk_widget_set_margin_end(main_box, 30);
    gtk_window_set_child(GTK_WINDOW(data->dialog), main_box);
    
    // Header
    GtkWidget *header = gtk_label_new(NULL);
    std::string header_markup = "<span size='xx-large' weight='bold' color='#40a4ff'>Wipe Raw Storage Device</span>";
    gtk_label_set_markup(GTK_LABEL(header), header_markup.c_str());
    gtk_box_append(GTK_BOX(main_box), header);
    
    // Device Info Card
    GtkWidget *dev_frame = gtk_frame_new("Device Information");
    gtk_widget_set_margin_bottom(dev_frame, 10);
    GtkWidget *dev_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(dev_box, 10);
    gtk_widget_set_margin_bottom(dev_box, 10);
    gtk_widget_set_margin_start(dev_box, 10);
    gtk_widget_set_margin_end(dev_box, 10);
    gtk_frame_set_child(GTK_FRAME(dev_frame), dev_box);
    
    std::string dev_name = "<b>Device:</b> " + device.name + " (" + device.path + ")";
    GtkWidget *name_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(name_label), dev_name.c_str());
    gtk_label_set_xalign(GTK_LABEL(name_label), 0);
    gtk_box_append(GTK_BOX(dev_box), name_label);
    
    std::string dev_model = "<b>Model:</b> " + (device.model.empty() ? "Unknown" : device.model);
    GtkWidget *model_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(model_label), dev_model.c_str());
    gtk_label_set_xalign(GTK_LABEL(model_label), 0);
    gtk_box_append(GTK_BOX(dev_box), model_label);
    
    std::string dev_size = "<b>Size:</b> " + formatSize(device.sizeBytes);
    GtkWidget *size_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(size_label), dev_size.c_str());
    gtk_label_set_xalign(GTK_LABEL(size_label), 0);
    gtk_box_append(GTK_BOX(dev_box), size_label);
    
    gtk_box_append(GTK_BOX(main_box), dev_frame);
    
    // Warning about entire device
    GtkWidget *info_frame = gtk_frame_new(NULL);
    gtk_widget_set_margin_bottom(info_frame, 10);
    GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(info_box, 10);
    gtk_widget_set_margin_bottom(info_box, 10);
    gtk_widget_set_margin_start(info_box, 10);
    gtk_widget_set_margin_end(info_box, 10);
    gtk_frame_set_child(GTK_FRAME(info_frame), info_box);
    
    GtkWidget *info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info_label), 
        "<span weight='bold' size='large'>This will wipe the ENTIRE raw storage device</span>\n\n"
        "All partitions, filesystems, and data on this device will be permanently erased.");
    gtk_label_set_wrap(GTK_LABEL(info_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(info_label), 0);
    gtk_box_append(GTK_BOX(info_box), info_label);
    
    gtk_box_append(GTK_BOX(main_box), info_frame);
    
    // Wipe Method
    GtkWidget *method_frame = gtk_frame_new("Wipe Method");
    gtk_widget_set_margin_bottom(method_frame, 10);
    GtkWidget *method_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(method_box, 10);
    gtk_widget_set_margin_bottom(method_box, 10);
    gtk_widget_set_margin_start(method_box, 10);
    gtk_widget_set_margin_end(method_box, 10);
    gtk_frame_set_child(GTK_FRAME(method_frame), method_box);
    
    const char* methods[] = {"Plain Overwrite (3-pass)", "ATA Secure Erase", "Encrypted Overwrite", NULL};
    GtkStringList *method_list = gtk_string_list_new(methods);
    data->method_combo = gtk_drop_down_new(G_LIST_MODEL(method_list), NULL);
    gtk_box_append(GTK_BOX(method_box), data->method_combo);
    
    gtk_box_append(GTK_BOX(main_box), method_frame);
    
    // Confirmation
    data->confirm_check = gtk_check_button_new_with_label(
        "I understand this will permanently erase all data and cannot be undone"
    );
    gtk_box_append(GTK_BOX(main_box), data->confirm_check);
    
    // Warning
    GtkWidget *warning = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(warning), 
        "<span color='#ff6464' size='large' weight='bold'>⚠ WARNING: This action is IRREVERSIBLE!</span>");
    gtk_box_append(GTK_BOX(main_box), warning);
    
    // Buttons
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_close), data->dialog);
    gtk_box_append(GTK_BOX(btn_box), cancel_btn);
    
    GtkWidget *start_btn = gtk_button_new_with_label("Start Wipe");
    gtk_widget_add_css_class(start_btn, "destructive-action");
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_wipe_start), data);
    gtk_box_append(GTK_BOX(btn_box), start_btn);
    
    gtk_box_append(GTK_BOX(main_box), btn_box);
    
    // Cleanup on dialog close
    g_signal_connect_swapped(data->dialog, "close-request", G_CALLBACK(g_free), data);
    
    gtk_window_present(GTK_WINDOW(data->dialog));
}

static void on_wipe_clicked(GtkButton* btn, gpointer user_data) {
    Device* device = (Device*)user_data;
    GtkWidget *toplevel = gtk_widget_get_root(GTK_WIDGET(btn));
    show_wipe_config_dialog(GTK_WINDOW(toplevel), *device);
}

static void free_device(gpointer data, GClosure* closure) {
    (void)closure;
    delete (Device*)data;
}

static void refresh_device_list(GtkWidget* container_box) {
    // Remove all children
    GtkWidget *child = gtk_widget_get_first_child(container_box);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(container_box), child);
        child = next;
    }

    std::vector<Device> devices = getDevices();
    
    if (devices.empty()) {
        GtkWidget *label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), "<span color='red' size='large'>No devices found.</span>\n(Try running as root/sudo if drives are missing)");
        gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(label, 20);
        gtk_box_append(GTK_BOX(container_box), label);
        return;
    }

    for (const auto& dev : devices) {
        // Card Container (Frame)
        GtkWidget *frame = gtk_frame_new(NULL);
        gtk_widget_set_margin_bottom(frame, 10);
        
        // Main Box inside Frame
        GtkWidget *card_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_set_margin_top(card_box, 12);
        gtk_widget_set_margin_bottom(card_box, 12);
        gtk_widget_set_margin_start(card_box, 12);
        gtk_widget_set_margin_end(card_box, 12);
        gtk_frame_set_child(GTK_FRAME(frame), card_box);

        // -- Top Row: Type & Name --
        GtkWidget *top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        
        // Type Badge
        GtkWidget *type_label = gtk_label_new(dev.type.c_str());
        gtk_widget_add_css_class(type_label, "badge"); // We can add CSS later if we want, or use markup
        char* type_markup = g_strdup_printf("<span background='#40a4ff' color='white' weight='bold'>  %s  </span>", dev.type.c_str());
        gtk_label_set_markup(GTK_LABEL(type_label), type_markup);
        g_free(type_markup);
        gtk_box_append(GTK_BOX(top_row), type_label);

        // Name
        GtkWidget *name_label = gtk_label_new(dev.name.c_str());
        char* name_markup = g_strdup_printf("<span size='large' weight='bold'>%s</span>", dev.name.c_str());
        gtk_label_set_markup(GTK_LABEL(name_label), name_markup);
        g_free(name_markup);
        gtk_box_append(GTK_BOX(top_row), name_label);

        gtk_box_append(GTK_BOX(card_box), top_row);

        // -- Details Row --
        GtkWidget *details_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
        
        // Model
        std::string modelStr = "Model: " + (dev.model.empty() ? "Unknown" : dev.model);
        GtkWidget *model_label = gtk_label_new(modelStr.c_str());
        gtk_widget_add_css_class(model_label, "dim-label");
        gtk_box_append(GTK_BOX(details_box), model_label);

        // Size
        std::string sizeStr = "Size: " + formatSize(dev.sizeBytes);
        GtkWidget *size_label = gtk_label_new(sizeStr.c_str());
        gtk_widget_add_css_class(size_label, "dim-label");
        gtk_box_append(GTK_BOX(details_box), size_label);

        // Flags
        if (dev.isReadOnly) {
            GtkWidget *flag = gtk_label_new(NULL);
            gtk_label_set_markup(GTK_LABEL(flag), "<span color='#ff6464'>[READ-ONLY]</span>");
            gtk_box_append(GTK_BOX(details_box), flag);
        }
        if (dev.isRemovable) {
            GtkWidget *flag = gtk_label_new(NULL);
            gtk_label_set_markup(GTK_LABEL(flag), "<span color='#64ff64'>[REMOVABLE]</span>");
            gtk_box_append(GTK_BOX(details_box), flag);
        }

        gtk_box_append(GTK_BOX(card_box), details_box);

        // -- Actions Row --
        GtkWidget *actions_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_top(actions_box, 8);

        GtkWidget *wipe_btn = gtk_button_new_with_label("Wipe Drive");
        gtk_widget_add_css_class(wipe_btn, "destructive-action");
        
        // Copy device for callback
        Device* dev_copy = new Device(dev);
        g_signal_connect_data(wipe_btn, "clicked", G_CALLBACK(on_wipe_clicked), dev_copy, free_device, (GConnectFlags)0);

        gtk_box_append(GTK_BOX(actions_box), wipe_btn);
        gtk_box_append(GTK_BOX(card_box), actions_box);

        gtk_box_append(GTK_BOX(container_box), frame);
    }
}

static void on_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "ZeroTrace");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 700);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    // --- Header ---
    GtkWidget *header_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_top(header_bar, 10);
    gtk_widget_set_margin_bottom(header_bar, 10);
    gtk_widget_set_margin_start(header_bar, 20);
    gtk_widget_set_margin_end(header_bar, 20);
    
    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), "<span size='xx-large' weight='bold' color='#40a4ff'>ZeroTrace</span> <span size='large' color='grey'>| Secure Data Cleanup</span>");
    gtk_box_append(GTK_BOX(header_bar), title_label);

    // Spacer to push button to right
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(header_bar), spacer);

    GtkWidget *refresh_btn = gtk_button_new_with_label("Refresh Devices");
    gtk_box_append(GTK_BOX(header_bar), refresh_btn);

    gtk_box_append(GTK_BOX(main_box), header_bar);
    gtk_box_append(GTK_BOX(main_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // --- Content ---
    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_box_append(GTK_BOX(main_box), scrolled_window);

    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(content_box, 20);
    gtk_widget_set_margin_bottom(content_box, 20);
    gtk_widget_set_margin_start(content_box, 30);
    gtk_widget_set_margin_end(content_box, 30);
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), content_box);

    // Wire up refresh
    g_signal_connect_swapped(refresh_btn, "clicked", G_CALLBACK(refresh_device_list), content_box);

    // Initial load
    refresh_device_list(content_box);

    gtk_window_present(GTK_WINDOW(window));
}

void runGui() {
    // We use G_APPLICATION_DEFAULT_FLAGS. 
    // ID must be valid domain style.
    GtkApplication *app = gtk_application_new("com.zerotrace.client", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    
    // We pass 0 argc here for simplicity, but main could pass args if we changed signature
    g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
}
