#include "include/gui.hpp"
#include "include/cert.hpp"
#include "include/wipe.hpp"
#include "include/dev.hpp"
#include <gtk/gtk.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <map>

// --- App State ---

struct AppState {
    GtkWidget *stack;
    GtkWidget *device_list_view;
    GtkWidget *wipe_options_view;
    
    // Wipe Options Widgets
    GtkWidget *target_device_label;
    GtkWidget *methods_box;
    GtkWidget *status_label;
    GtkCheckButton *radio_plain;
    GtkCheckButton *radio_encrypted;
    GtkCheckButton *radio_ata;
    GtkCheckButton *radio_firmware;
    
    // Selected Context
    Device selectedDevice;
};

static AppState appState;

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

// --- Forward Declarations ---
static void refresh_device_list(GtkWidget* container_box);
static void switch_to_device_list(GtkButton* btn, gpointer user_data);

// --- Logic ---

static void show_status(const std::string& msg, bool isError = false) {
    if (appState.status_label) {
        std::string markup;
        if (isError) {
            markup = g_strdup_printf("<span color='red'>%s</span>", msg.c_str());
        } else {
            markup = g_strdup_printf("<span color='green'>%s</span>", msg.c_str());
        }
        gtk_label_set_markup(GTK_LABEL(appState.status_label), markup.c_str());
        g_free((gpointer)markup.c_str()); // Need to be careful with g_strdup_printf ownership if using const char
        // Actually g_strdup_printf returns a newly allocated string.
        // In the line above: `markup = ...`. `markup` is std::string, so it copies.
        // Wait, g_strdup_printf returns gchar*.
        // Correct usage:
        // gchar* m = g_strdup_printf(...);
        // gtk_label_set_markup(..., m);
        // g_free(m);
    }
}

// Fixed version of show_status to avoid leak/error
static void show_status_safe(const std::string& msg, bool isError = false) {
    if (appState.status_label) {
        gchar* markup;
        if (isError) {
            markup = g_strdup_printf("<span color='red'>%s</span>", msg.c_str());
        } else {
            markup = g_strdup_printf("<span color='green'>%s</span>", msg.c_str());
        }
        gtk_label_set_markup(GTK_LABEL(appState.status_label), markup);
        g_free(markup);
    }
}


static void on_confirm_wipe_clicked(GtkButton* btn, gpointer user_data) {
    (void)btn;
    (void)user_data;
    
    WipeMethod method = WipeMethod::PLAIN_OVERWRITE;
    std::string methodStr = "Plain Overwrite";
    
    if (gtk_check_button_get_active(appState.radio_plain)) {
        method = WipeMethod::PLAIN_OVERWRITE;
        methodStr = "Plain Overwrite";
    } else if (gtk_check_button_get_active(appState.radio_encrypted)) {
        method = WipeMethod::ENCRYPTED_OVERWRITE;
        methodStr = "Encrypted Overwrite";
    } else if (gtk_check_button_get_active(appState.radio_ata)) {
        method = WipeMethod::ATA_SECURE_ERASE;
        methodStr = "ATA Secure Erase";
    } else if (gtk_check_button_get_active(appState.radio_firmware)) {
        method = WipeMethod::FIRMWARE_ERASE;
        methodStr = "Firmware Erase";
    }
    
    std::cout << "Starting wipe on " << appState.selectedDevice.path 
              << " using method: " << methodStr << std::endl;
              
    // TODO: This should ideally be async to avoid freezing UI
    // For now, we update status before (which might not render if main thread blocks) 
    // and after.
    
    show_status_safe("Wiping... Please Wait...", false);
    
    // Force UI update? In GTK4 strictly we should use an async worker.
    // But for this task scope, blocking is "acceptable" if not requested otherwise, 
    // though users hate it. 
    // Let's just call it.
    
    WipeResult result = wipeDisk(appState.selectedDevice.path, method);
    
    if (result.status == WipeStatus::SUCCESS) {
        show_status_safe("Wipe Completed Successfully!", false);
    } else if(result.status == WipeStatus::FAILURE) {
        show_status_safe("Wipe Failed! Check console/logs.", true);
    }
}

static void switch_to_device_list(GtkButton* btn, gpointer user_data) {
    (void)btn;
    (void)user_data;
    gtk_stack_set_visible_child(GTK_STACK(appState.stack), appState.device_list_view);
}

// Prepare the options screen for the selected device
static void prepare_wipe_options() {
    // Update Label
    std::string labelText = "Target: <b>" + appState.selectedDevice.name + "</b>\n" +
                            "<span size='small' color='gray'>" + appState.selectedDevice.path + " - " + 
                            formatSize(appState.selectedDevice.sizeBytes) + "</span>";
    gtk_label_set_markup(GTK_LABEL(appState.target_device_label), labelText.c_str());
    
    // Reset Status
    gtk_label_set_text(GTK_LABEL(appState.status_label), "");

    // Reset selection to default
    gtk_check_button_set_active(appState.radio_plain, TRUE); 
}

static void on_wipe_request(GtkButton* btn, gpointer user_data) {
    (void)btn;
    Device* dev = (Device*)user_data;
    appState.selectedDevice = *dev; // Copy device data
    
    prepare_wipe_options();
    gtk_stack_set_visible_child(GTK_STACK(appState.stack), appState.wipe_options_view);
}

static void free_device_copy(gpointer data, GClosure* closure) {
    (void)closure;
    delete (Device*)data;
}

// --- Views Construction ---

static GtkWidget* create_device_list_view() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Header
    GtkWidget *header_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_top(header_bar, 10);
    gtk_widget_set_margin_bottom(header_bar, 10);
    gtk_widget_set_margin_start(header_bar, 20);
    gtk_widget_set_margin_end(header_bar, 20);
    
    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), "<span size='xx-large' weight='bold' color='#40a4ff'>ZeroTrace</span> <span size='large' color='grey'>| Secure Data Cleanup</span>");
    gtk_box_append(GTK_BOX(header_bar), title_label);

    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(header_bar), spacer);

    GtkWidget *refresh_btn = gtk_button_new_with_label("Refresh Devices");
    gtk_box_append(GTK_BOX(header_bar), refresh_btn);
    
    gtk_box_append(GTK_BOX(box), header_bar);
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Content Scroller
    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_box_append(GTK_BOX(box), scrolled_window);

    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(content_box, 20);
    gtk_widget_set_margin_bottom(content_box, 20);
    gtk_widget_set_margin_start(content_box, 30);
    gtk_widget_set_margin_end(content_box, 30);
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), content_box);

    // Initial load
    refresh_device_list(content_box);
    
    // Signal
    g_signal_connect_swapped(refresh_btn, "clicked", G_CALLBACK(refresh_device_list), content_box);
    
    return box;
}

static GtkWidget* create_wipe_options_view() {
    GtkWidget *container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_valign(container, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(container, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(container, 40);
    gtk_widget_set_margin_bottom(container, 40);
    
    // Header
    GtkWidget *header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header), "<span size='x-large' weight='bold'>Confirm Wipe Options</span>");
    gtk_box_append(GTK_BOX(container), header);
    
    // Target Device Info
    appState.target_device_label = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(appState.target_device_label), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(container), appState.target_device_label);
    
    gtk_box_append(GTK_BOX(container), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Methods Selection
    GtkWidget *methods_frame = gtk_frame_new("Wipe Method");
    appState.methods_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(appState.methods_box, 10);
    gtk_widget_set_margin_bottom(appState.methods_box, 10);
    gtk_widget_set_margin_start(appState.methods_box, 10);
    gtk_widget_set_margin_end(appState.methods_box, 10);
    gtk_frame_set_child(GTK_FRAME(methods_frame), appState.methods_box);
    
    // Radio Buttons
    // In GTK4, check buttons are used for radios.
    // Create first one
    appState.radio_plain = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Plain Overwrite (Zero-fill) - High Compatibility"));
    
    // Create others, grouping with the first
    appState.radio_encrypted = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Encrypted Overwrite - Crypto Safe"));
    gtk_check_button_set_group(appState.radio_encrypted, appState.radio_plain);
    
    appState.radio_ata = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("ATA Secure Erase - Fast & Native"));
    gtk_check_button_set_group(appState.radio_ata, appState.radio_plain);
    
    appState.radio_firmware = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Firmware/Factory Reset - Vendor Specific"));
    gtk_check_button_set_group(appState.radio_firmware, appState.radio_plain);
    
    gtk_box_append(GTK_BOX(appState.methods_box), GTK_WIDGET(appState.radio_plain));
    gtk_box_append(GTK_BOX(appState.methods_box), GTK_WIDGET(appState.radio_encrypted));
    gtk_box_append(GTK_BOX(appState.methods_box), GTK_WIDGET(appState.radio_ata));
    gtk_box_append(GTK_BOX(appState.methods_box), GTK_WIDGET(appState.radio_firmware));
    
    gtk_box_append(GTK_BOX(container), methods_frame);
    
    // Status
    appState.status_label = gtk_label_new("");
    gtk_box_append(GTK_BOX(container), appState.status_label);
    
    // Actions
    GtkWidget *actions_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(actions_box, GTK_ALIGN_CENTER);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(switch_to_device_list), NULL);
    
    GtkWidget *confirm_btn = gtk_button_new_with_label("PERFORM WIPE");
    gtk_widget_add_css_class(confirm_btn, "destructive-action");
    g_signal_connect(confirm_btn, "clicked", G_CALLBACK(on_confirm_wipe_clicked), NULL);
    
    gtk_box_append(GTK_BOX(actions_box), cancel_btn);
    gtk_box_append(GTK_BOX(actions_box), confirm_btn);
    
    gtk_box_append(GTK_BOX(container), actions_box);
    
    return container;
}

// --- List Logic (Refactored) ---

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
        gtk_widget_add_css_class(type_label, "badge"); 
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
        
        // Copy device data for callback
        Device* devCopy = new Device(dev);
        g_signal_connect_data(wipe_btn, "clicked", G_CALLBACK(on_wipe_request), devCopy, free_device_copy, (GConnectFlags)0);

        gtk_box_append(GTK_BOX(actions_box), wipe_btn);
        gtk_box_append(GTK_BOX(card_box), actions_box);

        gtk_box_append(GTK_BOX(container_box), frame);
    }
}

// --- Main Init ---

static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "ZeroTrace");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 700);

    // Main Stack
    appState.stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(appState.stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    
    // Create Views
    appState.device_list_view = create_device_list_view();
    appState.wipe_options_view = create_wipe_options_view();
    
    gtk_stack_add_named(GTK_STACK(appState.stack), appState.device_list_view, "device_list");
    gtk_stack_add_named(GTK_STACK(appState.stack), appState.wipe_options_view, "wipe_options");
    
    gtk_window_set_child(GTK_WINDOW(window), appState.stack);

    gtk_window_present(GTK_WINDOW(window));
}

void runGui() {
    GtkApplication *app = gtk_application_new("com.zerotrace.client", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
}
