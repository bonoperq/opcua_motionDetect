#!/bin/bash

# Fonction pour afficher un message d'erreur et quitter le script
exit_with_error() {
    echo "Erreur : $1"
    exit 1
}

# Modifier la première ligne du fichier cert_values.txt
sed -i '1s@.*@/C=FR/ST=France/L=Antibes/O=Elsys/OU=Advans/CN=RaspberryServer@' ./cert_values.txt || exit_with_error "Impossible de modifier la première ligne de cert_values.txt"

# Nettoyer le répertoire ./ca en enlevant tous les fichiers sauf les .conf, .md et .bash
find ./ca -type f ! -name '*.conf' ! -name '*.md' ! -name '*.bash' -exec rm -f {} +

# Appeler le script generate_ca.bash
./ca/generate_ca.bash || exit_with_error "Erreur lors de l'exécution de generate_ca.bash"

echo "CA généré"

# Nettoyer le répertoire ./server/encryption en enlevant tout
rm -rf ./server/encryption/* || exit_with_error "Erreur lors de la suppression du contenu de server/encryption"

# Nettoyer le répertoire ./client/encryption en enlevant tout
rm -rf ./client/encryption/* || exit_with_error "Erreur lors de la suppression du contenu de client/encryption"

# Appeler le script generate_cert.bash en entrant les entrées "./client" "client" et "client"
./generate_cert.bash << EOF || exit_with_error "Erreur lors de l'exécution de generate_cert.bash pour le client"
./client
client
client
EOF

echo "Client généré"

# Modifier le fichier openssl_answer.txt
sed -i '1s/Client/RaspberryServer/' ./cert_values.txt || exit_with_error "Impossible de modifier le fichier openssl_answer.txt"

# Appeler le script generate_cert.bash en entrant les entrées "./server" "server" et "server"
./generate_cert.bash << EOF || exit_with_error "Erreur lors de l'exécution de generate_cert.bash pour le serveur"
./server
server
server
EOF

echo "Server généré"

# Modifier le fichier openssl_answer.txt
sed -i '1s/RaspberryServer/Client/' ./cert_values.txt || exit_with_error "Impossible de modifier le fichier openssl_answer.txt"

echo "Toutes les commandes ont été exécutées avec succès."
