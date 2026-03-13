#include "../PluginAPI.hpp"

extern "C" int pholio_plugin_api_version() {
    return PHOLIO_PLUGIN_API_VERSION;
}

extern "C" const char* pholio_plugin_name() {
    return "Hello Window";
}

extern "C" const char* pholio_plugin_version() {
    return "1.0.0";
}

extern "C" const char* pholio_plugin_author() {
    return "Pholio Team";
}

extern "C" bool pholio_plugin_process(const PholioPluginTask*, PholioPluginDecision*) {
    return true;
}

extern "C" void pholio_plugin_render_window(const PholioPluginUiApi* ui, bool* open) {
    if (!ui || !open || !*open) {
        return;
    }
    if (!ui->beginWindow("Plugin: Hello Window", open)) {
        ui->endWindow();
        return;
    }
    ui->textWrapped("This window is rendered by a plugin.");
    ui->text("Use this as a template for custom plugin UIs.");
    ui->separator();
    ui->button("Plugin UI Button");
    ui->endWindow();
}
