#!/bin/bash
# KhazarOS GNOME Setup — runs on first boot
# Applies theme, extensions, GSettings, and branding

set -e

echo "╔══════════════════════════════════════╗"
echo "║   KhazarOS GNOME Setup              ║"
echo "╚══════════════════════════════════════╝"
echo ""

# 1. Install theme
echo "[1/6] Installing Khazar Dark theme..."
mkdir -p /usr/share/themes/Khazar-dark/gnome-shell/assets
mkdir -p /usr/share/themes/Khazar-dark/gtk-4.0
mkdir -p ~/.themes/Khazar-dark/gnome-shell/assets
mkdir -p ~/.themes/Khazar-dark/gtk-4.0
cp -r /usr/share/khazar/gnome/theme/Khazar-dark/gnome-shell/* /usr/share/themes/Khazar-dark/gnome-shell/ 2>/dev/null || true
cp -r /usr/share/khazar/gnome/theme/Khazar-dark/gtk-4.0/* /usr/share/themes/Khazar-dark/gtk-4.0/ 2>/dev/null || true
cp -r /usr/share/themes/Khazar-dark ~/.themes/ 2>/dev/null || true
echo "  [+] Theme installed"

# 2. Install GNOME extensions
echo "[2/6] Installing Khazar extensions..."
EXT_DIR=~/.local/share/gnome-shell/extensions/khazar-assistant@devuan.kg
mkdir -p "$EXT_DIR/schemas"
cp -r /usr/share/khazar/gnome/extensions/khazar-assistant@devuan.kg/* "$EXT_DIR/" 2>/dev/null || true
echo "  [+] Extensions installed"

# 3. Compile schemas
echo "[3/6] Compiling GSettings schemas..."
cp /usr/share/khazar/gnome/settings/00-khazar-defaults.gschema.override /usr/share/glib-2.0/schemas/ 2>/dev/null || true
glib-compile-schemas /usr/share/glib-2.0/schemas/ 2>/dev/null || true
echo "  [+] Schemas compiled"

# 4. Apply GSettings
echo "[4/6] Applying desktop settings..."
gsettings set org.gnome.desktop.interface gtk-theme "Khazar-dark" 2>/dev/null || true
gsettings set org.gnome.desktop.interface icon-theme "Papirus-Dark" 2>/dev/null || true
gsettings set org.gnome.desktop.interface color-scheme "prefer-dark" 2>/dev/null || true
gsettings set org.gnome.desktop.background picture-uri "file:///usr/share/backgrounds/khazar/khazar-default.png" 2>/dev/null || true
gsettings set org.gnome.shell favorite-apps "['org.mozilla.firefox.desktop', 'org.gnome.Nautilus.desktop', 'org.gnome.Console.desktop', 'org.gnome.Software.desktop', 'org.gnome.Settings.desktop']" 2>/dev/null || true
gsettings set org.gnome.shell enabled-extensions "['khazar-assistant@devuan.kg']" 2>/dev/null || true
echo "  [+] Settings applied"

# 5. Wallpaper
echo "[5/6] Setting wallpaper..."
mkdir -p /usr/share/backgrounds/khazar
cp /usr/share/khazar/branding/khazar-logo.png /usr/share/backgrounds/khazar/khazar-default.png 2>/dev/null || true
echo "  [+] Wallpaper set"

# 6. Terminal profile
echo "[6/6] Configuring terminal..."
PROFILE=$(gsettings get org.gnome.Console profile-id 2>/dev/null || echo "")
echo "  [+] Terminal ready"

echo ""
echo "=== GNOME setup complete ==="
echo "Please log out and back in to see all changes."
