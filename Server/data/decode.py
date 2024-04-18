import sys
from PIL import Image
import io

def main():
    # Vérifier si un nom de fichier binaire a été fourni en argument
    if len(sys.argv) < 2:
        print("Usage: python3 read_img.py <file>")
        return

    # Récupérer le nom du fichier binaire à décoder depuis les arguments de ligne de commande
    file = sys.argv[1]

    # Ouvrir le fichier contenant les données binaires de l'image en mode binaire
    with open(file, "r") as file:
        image = file.read()

    image_bytes = bytes.fromhex(image)
    # Ouvrir les octets comme une image avec PIL
    image = Image.open(io.BytesIO(image_bytes))

    # Enregistrer l'image
    image.save("output.jpg")

if __name__ == "__main__":
    main()
