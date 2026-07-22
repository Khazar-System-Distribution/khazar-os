// Khazar AI Assistant — GNOME Shell Extension
// Panel icon + Ctrl+Space command bar + voice input

import St from 'gi://St';
import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import Clutter from 'gi://Clutter';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PanelMenu from 'resource:///org/gnome/shell/ui/panelMenu.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import {Extension, gettext as _} from 'resource:///org/gnome/shell/extensions/extension.js';

const KH_SOCKET = '/run/khazar/orchestrator.sock';

export default class KhazarExtension extends Extension {
    enable() {
        this._settings = this.getSettings();

        // Panel indicator
        this._indicator = new PanelMenu.Button(0.0, 'Khazar AI', false);

        let box = new St.BoxLayout({style_class: 'khazar-panel-box'});
        let icon = new St.Icon({
            icon_name: 'khazar-logo-symbolic',
            style_class: 'system-status-icon khazar-icon',
        });
        box.add_child(icon);

        let label = new St.Label({
            text: 'AI',
            style_class: 'khazar-label',
            y_align: Clutter.ActorAlign.CENTER,
        });
        box.add_child(label);
        this._indicator.add_child(box);

        // Menu items
        let cmdItem = new PopupMenu.PopupMenuItem(_('Əmr daxil et...'));
        cmdItem.connect('activate', () => this._showCommandBar());
        this._indicator.menu.addMenuItem(cmdItem);

        this._indicator.menu.addMenuItem(new PopupMenu.PopupSeparatorMenuItem());

        let statusItem = new PopupMenu.PopupMenuItem(_('Status: işlək'));
        statusItem.setSensitive(false);
        this._indicator.menu.addMenuItem(statusItem);

        let modelItem = new PopupMenu.PopupMenuItem(_('Model: Tier 0 (Rule Engine)'));
        modelItem.setSensitive(false);
        this._indicator.menu.addMenuItem(modelItem);

        this._indicator.menu.addMenuItem(new PopupMenu.PopupSeparatorMenuItem());

        let restartItem = new PopupMenu.PopupMenuItem(_('Khazar-ı yenidən başlat'));
        restartItem.connect('activate', () => this._restartKhazar());
        this._indicator.menu.addMenuItem(restartItem);

        Main.panel.addToStatusArea('khazar', this._indicator, 1);

        // Keyboard shortcut: Ctrl + Space
        Main.wm.addKeybinding(
            'khazar-command',
            this._settings,
            Clutter.ModifierType.CONTROL_MASK,
            Clutter.KEY_space,
            () => this._showCommandBar()
        );
    }

    _showCommandBar() {
        // Open a small terminal for input
        try {
            let proc = Gio.Subprocess.new(
                ['gnome-terminal', '--window', '--wait', '--',
                 'bash', '-c',
                 'echo "╔════════════════════════════╗";' +
                 'echo "║   Khazar AI Assistant     ║";' +
                 'echo "║   Əmri yazın:            ║";' +
                 'echo "╚════════════════════════════╝";' +
                 'echo "";' +
                 'read -p "> " cmd;' +
                 'kha "$cmd";' +
                 'read -p "Davam etmək üçün Enter..."'],
                Gio.SubprocessFlags.NONE
            );
        } catch (e) {
            log('Khazar: ' + e.message);
        }
    }

    _restartKhazar() {
        try {
            Gio.Subprocess.new(
                ['pkexec', 'systemctl', 'restart', 'khazar.target'],
                Gio.SubprocessFlags.NONE
            );
        } catch (e) {
            log('Khazar restart: ' + e.message);
        }
    }

    disable() {
        Main.wm.removeKeybinding('khazar-command');
        this._indicator?.destroy();
        this._indicator = null;
        this._settings = null;
    }
}
