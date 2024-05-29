# Utilisation du Makefile

Ce projet utilise un Makefile pour simplifier les tâches courantes. Assurez-vous d'avoir installé 'make' sur votre système pour utiliser ces commandes.

## Commandes disponibles

Pour voir les commandes disponibles, exécutez la commande suivante dans le répertoire actuel :

```bash
make rules
```

Le serveur dispose de plusieurs méthodes et données accessibles. Les fichiers motionDetectLog et logTail contiennent les logs de la caméra, autrement dit des dates associées à des images pour chaque détection de mouvement. Le fichier logTail reprend les 30 derniers bytes du fichier motionDetectLog.

Concernant les méthodes, il y en a deux : GetImage et GetMultipleImages qui, comme leurs noms l'indiquent, permettent de récupérer une ou plusieurs images. La fonction GetMultipleImages permet de choisir entre 1 et 5 images à récupérer en prenant en argument leurs noms.

### Versions utilisées
OpenSSL 1.1.1w 11 Sep 2023 (Library: OpenSSL 1.1.1w 11 Sep 2023)
Open62541 v1.3.9 (Nov 30, 2023)