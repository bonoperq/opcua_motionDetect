# Projet Client

Ce projet est un client développé dans le cadre d'un stage de fin d'étude sur l'utilisation d'un protocole de communication OPC UA couplé à de la détection de mouvement sur carte raspberry Pi 3B.

## Utilisation du Makefile

Ce projet utilise un Makefile pour simplifier les tâches courantes. Assurez-vous d'avoir installé 'make' sur votre système pour utiliser ces commandes.

### Commandes disponibles

Il est important de noter que l'ip du serveur auquel le client tente de se connecter est inscrite dans le Makefile dans la variable "SERVER_IP". Il faudra donc modifier cette variable pour tenter de se connecter à un serveur hébergé sur une autre adresse IP. Le port de connexion utilisé est le port 4080 par défaut.
Pour voir les commandes disponibles, exécutez la commande suivante dans le répertoire actuel :

```bash
make rules
```

#### Versions utilisées
OpenSSL 3.0.11 19 Sep 2023 (Library: OpenSSL 3.0.11 19 Sep 2023)
Open62541 v1.3.9 (Nov 30, 2023)
