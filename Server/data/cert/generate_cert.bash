#!/bin/bash

# Demander à l'utilisateur de saisir les noms de fichiers et le répertoire
read -p "Entrez le répertoire où enregistrer les fichiers (./client ou ./server) : " directory
read -p "Entrez le nom de la clé privée (sans extension) : " private_key_name
read -p "Entrez le nom du certificat (sans extension) : " file_name

# Vérifier si le répertoire existe
if [[ ! -d "$directory" ]]; then
    echo "Répertoire invalide."
    exit 1
fi

# Générer une clé privée
openssl genrsa -out "${directory}/encryption/${private_key_name}.key" 2048

# Créer une demande de signature de certificat (CSR) en remplissant automatiquement les champs
subj_values=$(cat cert_values.txt)
openssl req -new -key "${directory}/encryption/${private_key_name}.key" -out "${directory}/encryption/${file_name}.csr" \
    -subj "$subj_values"

# Signer le certificat avec le certificat de l'autorité de certification (CA)
openssl ca -batch -config "${directory}/config/ca.conf" -notext -in "${directory}/encryption/${file_name}.csr" -out "${directory}/encryption/${file_name}.crt"

# Convertir le certificat en format DER
openssl x509 -outform der -in "${directory}/encryption/${file_name}.crt" -out "${directory}/encryption/${file_name}.der"

echo "Le certificat a été généré avec succès dans le répertoire ${directory}."
