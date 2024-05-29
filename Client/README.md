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

Lorsque le client se connecte, 4 options s'offrent à lui :
1. Get log file : Permet de mettre à jour le fichier motionDetectLog du serveur jusqu'au Client.
2. Get Images : En entrant le nombre d'images ainsi que leurs noms séparés par des espaces, ce fichier vient récupérer les images voulues si elles existent sur le serveur, pour les mettre dans le repertoire data/images/.
3. Start monitoring : Après cette commande, le client passe en mode monitoring et affiche en continue les dernières lignes du fichier de log du server pour informer des dernièrs mouvements détectés. Pour en sortir, il suffit d'appuyer sur Ctrl-C.
4. Disconnect : Cette touche permet de déconnecter le client.

Après les commandes 1 ou 2, si des mouvements ont été detectés, le client voit les nouveaux logs sur son terminal. C'est parceque le monitoring du fichier de log a lieu en continue mais ne s'affiche pas quand le client est dans le menu pour ne pas perturber l'affichage.

#### Versions utilisées
OpenSSL 3.0.11 19 Sep 2023 (Library: OpenSSL 3.0.11 19 Sep 2023)
Open62541 v1.3.9 (Nov 30, 2023)
parler de jpeg lib
