Name:           khazar
Version:        0.1.0
Release:        1%{?dist}
Summary:        Khazar AI Platform - Voice/Text Command Agent System

License:        GPLv3
URL:            https://github.com/Khazar-System-Distribution/khazar
Source0:        %{url}/archive/v%{version}.tar.gz

BuildRequires:  gcc, make, socat, python3
Requires:       socat, python3, systemd

%description
Khazar is an AI-powered command agent platform for Linux desktops.
It provides natural language system control: open applications,
manage packages, control network, audio, and power settings via
voice or text commands.

Components:
  - Orchestrator (Tier 0 + Tier 1 intent routing)
  - Rule Engine (deterministic command recognition)
  - Policy Engine (security policy enforcement)
  - Model Runtime (LLM inference via llama.cpp)
  - Intent Classifier (Tier 1 LLM fallback)
  - 5 Agents (desktop, package, network, power, audio)

%prep
%setup -q

%build
make all PREFIX=%{_prefix}

%install
make install DESTDIR=%{buildroot} PREFIX=%{_prefix}
mkdir -p %{buildroot}%{_sysconfdir}/khazar
cp components/*/config.toml.example %{buildroot}%{_sysconfdir}/khazar/
mkdir -p %{buildroot}%{_unitdir}
cp distro/systemd/*.service distro/systemd/khazar.target %{buildroot}%{_unitdir}/
mkdir -p %{buildroot}%{_bindir}
cp distro/cli/kha %{buildroot}%{_bindir}/
chmod +x %{buildroot}%{_bindir}/kha

%pre
getent group khazar >/dev/null || groupadd -r khazar
getent passwd khazar >/dev/null || useradd -r -s /sbin/nologin -d /var/lib/khazar -g khazar khazar

%post
%systemd_post khazar.target

%preun
%systemd_preun khazar.target

%files
%license LICENSE
%doc README.md
%{_bindir}/ai-*
%{_bindir}/kha
%{_libdir}/libai-sdk.a
%{_includedir}/ai-sdk/
%{_sysconfdir}/khazar/
%{_unitdir}/ai-*.service
%{_unitdir}/khazar.target

%changelog
* Mon Jul 21 2025 Khazar Team <dev@khazar.devuan.kg> - 0.1.0-1
- Initial RPM release
- Full Tier 0 + Tier 1 pipeline
- 5 agent types
