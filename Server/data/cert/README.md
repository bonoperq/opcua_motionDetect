# Script Bash

Ce dossier contient quelques scripts bash dont l'utilité sera expliquée ici.

## generate_ca.bash

Ce script situé dans le répertoire /ca permet de générer un CA en l'autosignant et en remplissant automatiquement les champs requis du certificats. Au préalable, il nettoie aussi le répertoire /ca.

## generate_cert.bash

Ce script est utilisé pour générer des certificats client ou serveur selon les entrées de l'utilisateur. Il utilise le fichier cert_values.txt pour remplir les champs requis du certificats

## regen_all.bash

Ce script appelle les deux précedents scripts et permet de tout regénérer pour repartir avec une nouvelle base.