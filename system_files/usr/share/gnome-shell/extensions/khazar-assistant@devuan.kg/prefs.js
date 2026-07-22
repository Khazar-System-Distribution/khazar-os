import Gtk from 'gi://Gtk';
import Adw from 'gi://Adw';
import {ExtensionPreferences, gettext as _} from 'resource:///org/gnome/shell/extensions/extension.js';

export default class KhazarPreferences extends ExtensionPreferences {
    fillPreferencesWindow(window) {
        const page = new Adw.PreferencesPage({title: _('General')});
        const group = new Adw.PreferencesGroup({title: _('Khazar AI Settings')});

        const socketRow = new Adw.EntryRow({
            title: _('Orchestrator socket'),
            text: '/run/khazar/orchestrator.sock',
        });
        this._settings.bind('socket-path', socketRow, 'text', Gio.SettingsBindFlags.DEFAULT);
        group.add(socketRow);

        const shortcutRow = new Adw.SwitchRow({
            title: _('Enable Ctrl+Space shortcut'),
            subtitle: _('Quick command input from anywhere'),
        });
        this._settings.bind('enable-shortcut', shortcutRow, 'active', Gio.SettingsBindFlags.DEFAULT);
        group.add(shortcutRow);

        page.add(group);
        window.add(page);
    }
}
