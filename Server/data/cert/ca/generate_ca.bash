#!/bin/bash

# Définir le répertoire de destination
destination="/home/bonoperq/Documents/StageQuentin/Server/data/cert/ca"

# Nettoyer le répertoire de destination en enlevant tous les fichiers sauf les .conf, .md et .bash
find "$destination" -type f ! -name '*.conf' ! -name '*.md' ! -name '*.bash' -exec rm -f {} +

# Générer le certificat de l'autorité de certification (CA) dans le répertoire de destination
# Utilisez "$destination" pour spécifier le répertoire de destination
openssl req -x509 -nodes -newkey rsa:4096 -keyout "$destination/ca.key" -out "$destination/ca.crt" -days 7300 -subj "/CN=MyCA/OU=Elsys/O=France/L=Antibes/ST=France/C=FR" -addext "keyUsage = digitalSignature, keyEncipherment, cRLSign, keyCertSign" -addext "extendedKeyUsage = serverAuth, clientAuth" -addext "subjectAltName = DNS:WFRPF4JGG3BL"

# Convertir le certificat au format DER
openssl x509 -in "$destination/ca.crt" -outform der -out "$destination/ca.der"

# Compter l'index
touch "$destination/certindex"
echo 01 > "$destination/certserial"
echo 01 > "$destination/crlnumber"

# Générer la liste de révocation de certificats (CRL)
openssl ca -config "$destination/ca.conf" -gencrl -keyfile "$destination/ca.key" -cert "$destination/ca.crt" -out "$destination/root.crl.pem"
openssl crl -inform PEM -in "$destination/root.crl.pem" -outform DER -out "$destination/root.crl"
rm "$destination/root.crl.pem"

echo "Le certificat de l'autorité de certification a été généré avec succès dans le répertoire $destination."
