# Khazar AI Platform — ujust reseptləri

# Khazar servislərini aktivləşdir
enable-khazar:
    #!/usr/bin/bash
    set -eux
    systemctl enable khazar.target --now
    echo "Khazar AI platform aktivləşdirildi."
    kha status

# Ollama üçün populyar modelləri endir
setup-ollama-models model="llama3.2":
    #!/usr/bin/bash
    set -eux
    systemctl start ollama
    ollama pull {{ model }}
    echo "Model {{ model }} yükləndi."

# Khazar servis statusu
khazar-status:
    #!/usr/bin/bash
    kha status

# Khazar servislərini yenidən başlat
khazar-restart:
    #!/usr/bin/bash
    set -eux
    systemctl restart khazar.target
