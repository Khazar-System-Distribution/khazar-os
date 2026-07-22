#!/bin/bash
# Install Khazar GNOME Shell + GTK theme

THEME="Khazar-dark"
DEST="$HOME/.themes/$THEME"
SYSDEST="/usr/share/themes/$THEME"

echo "=== Installing Khazar Dark Theme ==="

# User install
mkdir -p "$DEST/gnome-shell/assets" "$DEST/gtk-4.0"
cp -r "$(dirname "$0")/gnome-shell/"* "$DEST/gnome-shell/"
cp -r "$(dirname "$0")/gtk-4.0/"* "$DEST/gtk-4.0/"

# System install
if [ "$EUID" -eq 0 ]; then
    mkdir -p "$SYSDEST/gnome-shell/assets" "$SYSDEST/gtk-4.0"
    cp -r "$(dirname "$0")/gnome-shell/"* "$SYSDEST/gnome-shell/"
    cp -r "$(dirname "$0")/gtk-4.0/"* "$SYSDEST/gtk-4.0/"
fi

# Activate
gsettings set org.gnome.desktop.interface gtk-theme "$THEME" 2>/dev/null || true
gsettings set org.gnome.shell.extensions.user-theme name "$THEME" 2>/dev/null || true

echo "[+] Theme installed. Log out and back in to see changes."
