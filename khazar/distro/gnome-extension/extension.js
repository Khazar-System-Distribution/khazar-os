// Khazar AI Assistant — GNOME Shell Extension
// Quick Settings toggle + Ctrl+Space command bar

import St from 'gi://St';
import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import Clutter from 'gi://Clutter';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PanelMenu from 'resource:///org/gnome/shell/ui/panelMenu.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';

export default class KhazarExtension extends Extension {
    enable() {
        this._indicator = new PanelMenu.Button(0.0, 'Khazar AI', false);
        let icon = new St.Icon({
            icon_name: 'khazar-logo-symbolic',
            style_class: 'system-status-icon',
        });
        this._indicator.add_child(icon);

        let item = new PopupMenu.PopupMenuItem('Əmr daxil et...');
        item.connect('activate', () => this._showCommandBar());
        this._indicator.menu.addMenuItem(item);

        this._indicator.menu.addMenuItem(new PopupMenu.PopupSeparatorMenuItem());
        this._indicator.menu.addMenuItem(new PopupMenu.PopupMenuItem('Status: işlək'));
        this._indicator.menu.addMenuItem(new PopupMenu.PopupMenuItem('Model: Tier 0 (Rule Engine)'));

        Main.panel.addToStatusArea('khazar', this._indicator);
    }

    _showCommandBar() {
        // TODO: Open a dialog for text/voice input
        // For now, launch terminal with kha
        try {
            Gio.Subprocess.new(
                ['gnome-terminal', '--', 'kha', 'help'],
                Gio.SubprocessFlags.NONE
            );
        } catch (e) {
            log('Khazar: ' + e.message);
        }
    }

    disable() {
        this._indicator?.destroy();
        this._indicator = null;
    }
}
