# KhazarOS GNOME — Build & Install
# make gnome → hamısını qurar /usr/share/khazar/gnome/-ə

KH_GNOME := /usr/share/khazar/gnome

.PHONY: theme extension settings branding install clean

all: theme extension settings

theme:
	@echo "=== Khazar Dark Theme ==="
	@mkdir -p $(KH_GNOME)/theme/Khazar-dark/{gnome-shell/assets,gtk-4.0}
	@cp -r theme/Khazar-dark/* $(KH_GNOME)/theme/Khazar-dark/

extension:
	@echo "=== Khazar Assistant Extension ==="
	@mkdir -p $(KH_GNOME)/extensions/khazar-assistant@devuan.kg/schemas
	@cp -r extensions/khazar-assistant@devuan.kg/* $(KH_GNOME)/extensions/khazar-assistant@devuan.kg/
	@mkdir -p /usr/share/gnome-shell/extensions/khazar-assistant@devuan.kg
	@cp -r extensions/khazar-assistant@devuan.kg/* /usr/share/gnome-shell/extensions/khazar-assistant@devuan.kg/ 2>/dev/null || true

settings:
	@echo "=== GSettings Defaults ==="
	@mkdir -p $(KH_GNOME)/settings
	@cp settings/00-khazar-defaults.gschema.override $(KH_GNOME)/settings/
	@cp settings/00-khazar-defaults.gschema.override /usr/share/glib-2.0/schemas/ 2>/dev/null || true
	@glib-compile-schemas /usr/share/glib-2.0/schemas/ 2>/dev/null || true

branding:
	@echo "=== Branding ==="
	@mkdir -p /usr/share/backgrounds/khazar /usr/share/icons/hicolor/scalable/apps
	@cp ../branding/khazar-logo.png /usr/share/backgrounds/khazar/khazar-default.png
	@cp ../branding/khazar-logo.svg /usr/share/icons/hicolor/scalable/apps/khazar-logo.svg 2>/dev/null || true

install: all
	@echo "=== Installing GNOME customizations ==="
	@gsettings set org.gnome.desktop.interface gtk-theme "Khazar-dark" 2>/dev/null || true
	@gsettings set org.gnome.desktop.interface color-scheme "prefer-dark" 2>/dev/null || true
	@gsettings set org.gnome.desktop.background picture-uri "file:///usr/share/backgrounds/khazar/khazar-default.png" 2>/dev/null || true
	@gsettings set org.gnome.shell enabled-extensions "['khazar-assistant@devuan.kg']" 2>/dev/null || true
	@echo "  [+] Applied. Log out and back in."

clean:
	@rm -rf $(KH_GNOME)
