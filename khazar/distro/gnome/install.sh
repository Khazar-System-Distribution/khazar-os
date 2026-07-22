#!/bin/bash
# Install KhazarOS GNOME customization — run from within the distro

KH_SHARE="/usr/share/khazar"

echo "╔══════════════════════════════════════╗"
echo "║   KhazarOS — GNOME Özəlləşdirmə     ║"
echo "╚══════════════════════════════════════╝"
echo ""

# 1. Theme
echo "[1/5] Quraşdırılır: Khazar Dark tema..."
mkdir -p /usr/share/themes/Khazar-dark/{gnome-shell/assets,gtk-4.0}
cp -r "$KH_SHARE/gnome/theme/Khazar-dark/"* /usr/share/themes/Khazar-dark/
mkdir -p ~/.themes && cp -r /usr/share/themes/Khazar-dark ~/.themes/ 2>/dev/null || true

# 2. Extension
echo "[2/5] Quraşdırılır: Khazar Assistant extension..."
mkdir -p /usr/share/gnome-shell/extensions/khazar-assistant@devuan.kg
cp -r "$KH_SHARE/gnome/extensions/khazar-assistant@devuan.kg/"* \
      /usr/share/gnome-shell/extensions/khazar-assistant@devuan.kg/

# 3. GSettings
echo "[3/5] Tətbiq olunur: Sistem ayarları..."
cp "$KH_SHARE/gnome/settings/00-khazar-defaults.gschema.override" \
   /usr/share/glib-2.0/schemas/
glib-compile-schemas /usr/share/glib-2.0/schemas/

# 4. Branding
echo "[4/5] Quraşdırılır: Divar kağızı + ikonlar..."
mkdir -p /usr/share/backgrounds/khazar \
         /usr/share/icons/hicolor/scalable/apps \
         /usr/share/icons/hicolor/symbolic/apps
cp "$KH_SHARE/branding/khazar-logo.png" /usr/share/backgrounds/khazar/khazar-default.png
cp "$KH_SHARE/branding/khazar-logo.png" /usr/share/backgrounds/khazar/khazar-dark.png
cp "$KH_SHARE/branding/khazar-logo.svg" /usr/share/icons/hicolor/scalable/apps/khazar-logo.svg 2>/dev/null || true

# 5. Enable
echo "[5/5] Aktivləşdirilir..."
gsettings set org.gnome.desktop.interface gtk-theme "Khazar-dark" 2>/dev/null || true
gsettings set org.gnome.desktop.interface icon-theme "Papirus-Dark" 2>/dev/null || true
gsettings set org.gnome.shell enabled-extensions "['khazar-assistant@devuan.kg']" 2>/dev/null || true
gsettings set org.gnome.desktop.interface color-scheme "prefer-dark" 2>/dev/null || true

echo ""
echo "=== Bitdi ==="
echo "Yenidən daxil olun (logout/login) və ya Alt+F2 → r"
