#include "I18n.hpp"

#include <string>
#include <unordered_map>

namespace ui::i18n {
namespace {

Language g_language = Language::English;

const std::unordered_map<std::string, const char*> kGerman = {
    {"settings.unsaved", "Es gibt ungespeicherte Aenderungen!"},
    {"settings.tab.directories", "Verzeichnisse"},
    {"settings.tab.folder_structure", "Ordnerstruktur"},
    {"settings.tab.engine", "Engine"},
    {"settings.tab.plugins", "Plugins"},
    {"settings.source_path", "Quellpfad:"},
    {"settings.target_path", "Zielpfad:"},
    {"settings.tooltip.source", "Verzeichnis mit unsortierten Fotos und Videos."},
    {"settings.tooltip.target", "Stammverzeichnis fuer die organisierte Bibliothek."},
    {"settings.browse", "Durchsuchen..."},
    {"settings.source_size", "Quellgroesse: %.2f GB %s"},
    {"settings.calculating", "(wird berechnet...)"},
    {"settings.target_drive_space", "Speicherplatz Ziellaufwerk:"},
    {"settings.disk_ok", "Freier Speicher ist ausreichend."},
    {"settings.disk_critical", "KRITISCH: Nicht genug Speicher fuer Quelldateien!"},
    {"settings.disk_warning", "Warnung: Nach der Verarbeitung wird der Speicher knapp."},
    {"settings.disk_note", "Hinweis: Ziellaufwerk ist zu mehr als 75% belegt."},
    {"settings.disk_progress", "%.1f GB frei / %.1f GB gesamt (Quelle braucht %.1f GB)"},
    {"settings.folder_pattern", "Ordnerstruktur-Muster:"},
    {"settings.tooltip.pattern", "Verwendet strftime-Codes.\n%%Y=Jahr  %%m=Monat  %%d=Tag  %%B=Monatsname"},
    {"settings.migration_strategy", "Migrationsstrategie:"},
    {"settings.migration.rebuild", "Umbau (bestehende Struktur in neues Muster migrieren)"},
    {"settings.migration.merge", "Merge (erkannte Zielstruktur weiterverwenden)"},
    {"settings.migration.continue", "Weiterfuehren (erkannte Zielstruktur uebernehmen)"},
    {"settings.tooltip.migration", "Umbau: Bestehende Dateien im Ziel werden ins konfigurierte Muster verschoben.\nMerge: Neue Dateien nutzen die erkannte Zielstruktur, falls vorhanden.\nWeiterfuehren: Wie Merge, uebernimmt erkannte Struktur zusaetzlich als neues Standardmuster."},
    {"settings.example.today", "Beispiel (heute): %s"},
    {"settings.example.invalid", "Beispiel (heute): Ungueltiges Muster"},
    {"settings.presets", "Vorlagen:"},
    {"settings.preset.ymd", "Jahr/Monat/Tag"},
    {"settings.preset.ym", "Jahr/Monat"},
    {"settings.preset.yfull", "Jahr/Vollstaendiges Datum"},
    {"settings.filename_template", "Dateinamensvorlage:"},
    {"settings.tooltip.filename_template", "Verfuegbar: {original_filename}, {original_name}, {ext}, {yyyy}, {MM}, {dd}, {HH}, {mm}, {ss}"},
    {"settings.preset.original", "Originalen Dateinamen verwenden"},
    {"settings.preset.date_name", "Datum + Originalname"},
    {"settings.preset.timestamp", "Zeitstempel"},
    {"settings.language", "Sprache"},
    {"settings.language.english", "Englisch"},
    {"settings.language.german", "Deutsch"},
    {"settings.theme", "Theme"},
    {"settings.theme.dark", "Dunkel"},
    {"settings.theme.light", "Hell"},
    {"settings.theme.tooltip", "Zwischen dunklem und hellem UI-Theme waehlen."},
    {"settings.operation_mode", "Betriebsmodus:"},
    {"settings.op.copy", "Kopieren (Sicher)"},
    {"settings.op.move", "Verschieben (Effizient)"},
    {"settings.tooltip.opmode", "Kopieren: Originaldateien bleiben erhalten.\nVerschieben: Dateien werden ins neue Ziel uebertragen (Originale werden geloescht)."},
    {"settings.verification", "Verifizierung:"},
    {"settings.ver.none", "Keine (Am schnellsten)"},
    {"settings.ver.size", "Nur Groesse"},
    {"settings.ver.partial", "Teil-Hash"},
    {"settings.ver.full", "Voll-Hash (Am sichersten)"},
    {"settings.tooltip.verification", "Keine: Keine Pruefung.\nNur Groesse: Dateigroesse pruefen.\nTeil: Hash der ersten 1 MB pruefen.\nVoll: Hash der gesamten Datei pruefen."},
    {"settings.duplicate", "Duplikat-Behandlung:"},
    {"settings.dup.skip", "Ueberspringen (Sicher)"},
    {"settings.dup.overwrite", "Ueberschreiben (Gefaehrlich)"},
    {"settings.dup.rename", "Umbenennen (z. B. datei (1).jpg)"},
    {"settings.tooltip.duplicate", "Aktion, wenn eine Datei im Zielverzeichnis bereits existiert."},
    {"settings.ask_duplicate", "Bei jedem Duplikat nachfragen"},
    {"settings.tooltip.ask_duplicate", "Zeigt fuer jedes gefundene Duplikat einen Dialog zur manuellen Auswahl."},
    {"settings.dry_run", "Dry Run (Simulationsmodus)"},
    {"settings.tooltip.dry_run", "Simuliert den Ablauf, ohne Dateien zu kopieren oder zu verschieben."},
    {"settings.enable_conversion", "Formatkonvertierung aktivieren"},
    {"settings.tooltip.enable_conversion", "Konvertiert uebertragene Medien nach erfolgreicher Verifizierung in die konfigurierten Ausgabeformate."},
    {"settings.image_output", "Bild-Ausgabeformat"},
    {"settings.video_output", "Video-Ausgabeformat"},
    {"settings.tooltip.video_output", "Die Videokonvertierung nutzt FFmpeg mit H.264/AAC-Standardwerten."},
    {"settings.enable_plugins", "Plugin-System aktivieren"},
    {"settings.tooltip.enable_plugins", "Laedt externe Plugins fuer Filter, Zielanpassungen und optionale Plugin-Fenster."},
    {"settings.allow_plugin_windows", "Plugin-Fenster erlauben"},
    {"settings.tooltip.allow_plugin_windows", "Erlaubt Plugins, eigene ImGui-Fenster in der App darzustellen."},
    {"settings.plugin_directory", "Plugin-Verzeichnis"},
    {"settings.tooltip.plugin_directory", "Verzeichnis, das auf Plugin-Bibliotheken (.dylib/.so/.dll) gescannt wird."},
    {"settings.create_plugin_dir", "Plugin-Verzeichnis erstellen"},
    {"settings.open_plugin_dir", "Plugin-Verzeichnis oeffnen"},
    {"settings.reload_plugins_now", "Plugins jetzt neu laden"},
    {"settings.tooltip.reload_plugins_now", "Loest nach dem Speichern sofortiges Neuladen der Plugins aus."},
    {"settings.workflow", "Benutzerfreundlicher Ablauf:"},
    {"settings.workflow.1", "1) Plugin-Verzeichnis festlegen"},
    {"settings.workflow.2", "2) Plugin-Dateien im Plugins-Fenster hinzufuegen"},
    {"settings.workflow.3", "3) Auf \"Plugins jetzt neu laden\" klicken"},
    {"settings.save_all", "ALLE EINSTELLUNGEN SPEICHERN"},
    {"settings.discard", "AENDERUNGEN VERWERFEN"},
    {"settings.tooltip.save", "Wendet diese Einstellungen an und speichert sie auf der Festplatte."},
    {"settings.tooltip.discard", "Verwirft Aenderungen und laedt zuletzt gespeicherte Einstellungen."},
    {"menu.file", "Datei"},
    {"menu.view", "Ansicht"},
    {"menu.plugins", "Plugins"},
    {"menu.help", "Hilfe"},
    {"menu.restart", "Neustart"},
    {"menu.exit", "Beenden"},
    {"menu.dashboard", "Dashboard"},
    {"menu.preview", "Bildvorschau"},
    {"menu.settings", "Einstellungen"},
    {"menu.progress", "Fortschritt & Leistung"},
    {"menu.logs", "Logs"},
    {"menu.internal_debug", "Interner Debug"},
    {"menu.reset_layout", "Layout zuruecksetzen"},
    {"menu.plugin.search", "Suchen"},
    {"menu.plugin.add", "Hinzufuegen"},
    {"menu.plugin.manage", "Verwalten"},
    {"menu.plugin.browse_hub", "Plugin-Hub durchsuchen"},
    {"menu.plugin.open_window", "Fenster / Konfiguration oeffnen"},
    {"menu.plugin.no_window", "Keine Konfiguration/Fenster verfuegbar"},
    {"menu.plugin.version", "Version: %s"},
    {"menu.plugin.author", "Autor: %s"},
    {"menu.plugin.disabled", "(deaktiviert)"},
    {"menu.help.check_updates", "Nach Updates suchen"},
    {"menu.help.report_issue", "Problem melden"},
    {"menu.help.whats_new", "Was ist neu"},
    {"menu.help.about", "Ueber"},
    {"status.running", "Status: Aktiv"},
    {"status.idle", "Status: Leerlauf"},
    {"status.checking_updates", "Suche nach Updates..."},
    {"status.update_available", "Update verfuegbar!"},
    {"close_sorting.title", "Schliessen waehrend Sortierung"},
    {"close_sorting.line1", "Sortierung pausiert. Zum Schutz deiner Dateien bitte fortfahren oder abbrechen und schliessen."},
    {"close_sorting.line2", "Moechtest du fortfahren oder jetzt stoppen und beenden?"},
    {"close_sorting.continue", "Sortierung fortsetzen"},
    {"close_sorting.stop_exit", "Stoppen und beenden"},
    {"update.available", "Update verfuegbar"},
    {"update.new_version", "Neue Version verfuegbar: v%s"},
    {"update.blocked", "Update ist blockiert, waehrend die Sortierung laeuft."},
    {"update.install_restart", "Update installieren und neu starten"},
    {"update.install_later", "Beim Neustart aktualisieren"},
    {"update.installing", "Update wird installiert, App wird geschlossen..."},
    {"update.install_failed", "Update-Installation konnte nicht gestartet werden."},
    {"update.queued", "Update fuer den naechsten Neustart vorgemerkt."},
    {"update.no_package", "Kein installierbares Update-Paket gefunden."},
    {"update.no_direct_package", "Kein direkt installierbares Paket in diesem Release gefunden."},
    {"update.view_github", "Auf GitHub ansehen"},
    {"update.dismiss", "Schliessen"},
    {"plugin.window.title", "Plugins"},
    {"plugin.tab.search", "Suchen"},
    {"plugin.tab.add", "Hinzufuegen"},
    {"plugin.tab.manage", "Verwalten"},
    {"plugin.search.input", "GitHub-Suche"},
    {"plugin.search.tooltip", "Suchanfrage fuer Plugin-Entdeckung auf GitHub."},
    {"plugin.search.button", "Auf GitHub suchen"},
    {"plugin.search.fail", "Browser konnte fuer GitHub-Suche nicht geoeffnet werden."},
    {"plugin.search.success", "GitHub-Suche im Browser geoeffnet."},
    {"plugin.search.hub", "Plugin-Hub durchsuchen"},
    {"plugin.hub.fail", "Plugin-Hub konnte nicht geoeffnet werden."},
    {"plugin.hub.success", "Plugin-Hub im Browser geoeffnet."},
    {"plugin.add.description", "Eine kompilierte Plugin-Bibliothek in das konfigurierte Plugin-Verzeichnis hinzufuegen."},
    {"plugin.add.path", "Plugin-Dateipfad"},
    {"plugin.add.path.tooltip", "Pfad zu einer .dylib/.so/.dll-Datei."},
    {"plugin.add.button", "Plugin hinzufuegen"},
    {"plugin.add.invalid_path", "Ungueltiger Plugin-Dateipfad."},
    {"plugin.add.unsupported_ext", "Nicht unterstuetzte Plugin-Endung fuer diese Plattform."},
    {"plugin.add.added", "Plugin hinzugefuegt: "},
    {"plugin.add.fail", "Plugin konnte nicht hinzugefuegt werden: "},
    {"plugin.add.open_folder", "Plugin-Ordner oeffnen"},
    {"plugin.folder.open_fail", "Plugin-Ordner konnte nicht geoeffnet werden."},
    {"plugin.folder.open_ok", "Plugin-Ordner geoeffnet."},
    {"plugin.manage.configured_dir", "Konfiguriertes Plugin-Verzeichnis:"},
    {"plugin.manage.open_folder", "Ordner oeffnen"},
    {"plugin.manage.create_folder", "Ordner erstellen"},
    {"plugin.manage.folder_ready", "Plugin-Verzeichnis ist bereit."},
    {"plugin.manage.create_fail", "Plugin-Verzeichnis konnte nicht erstellt werden: "},
    {"plugin.manage.reload", "Plugins neu laden"},
    {"plugin.manage.reload_requested", "Plugin-Neuladen angefordert."},
    {"plugin.manage.detected_libs", "Gefundene Plugin-Bibliotheken:"},
    {"plugin.manage.no_plugins", "Keine Plugins gefunden."},
    {"plugin.manage.loaded_plugins", "Geladene Plugins (umschalten):"},
    {"plugin.manage.manager_missing", "Plugin-Manager nicht verfuegbar."},
    {"plugin.manage.no_loaded_plugins", "Keine geladenen Plugins."},
    {"plugin.manage.enabled", "Plugin aktiviert: "},
    {"plugin.manage.disabled", "Plugin deaktiviert: "},
    {"plugin.manage.toggle_fail", "Plugin-Status konnte nicht geaendert werden: "},
    {"plugin.state.enabled", "(aktiv)"},
    {"plugin.state.disabled", "(deaktiviert)"},
    {"plugin.manage.windows", "Plugin-Fenster:"},
    {"plugin.manage.no_windows", "Keine Plugin-Fenster verfuegbar."},
    {"plugin.state.open", "(offen)"},
    {"plugin.window.reopen", "Erneut oeffnen"},
    {"plugin.window.reopened", "Plugin-Fenster erneut geoeffnet: "},
    {"plugin.window.reopen_fail", "Plugin-Fenster konnte nicht erneut geoeffnet werden: "}
};

} // namespace

Language languageFromCode(std::string_view code) {
    if (code == "de" || code == "DE" || code == "de_DE" || code == "de-DE") {
        return Language::German;
    }
    return Language::English;
}

const char* languageCode(Language language) {
    return language == Language::German ? "de" : "en";
}

void setLanguage(Language language) {
    g_language = language;
}

Language getLanguage() {
    return g_language;
}

const char* tr(std::string_view key, const char* englishFallback) {
    if (g_language == Language::German) {
        auto it = kGerman.find(std::string(key));
        if (it != kGerman.end()) {
            return it->second;
        }
    }
    return englishFallback;
}

} // namespace ui::i18n
