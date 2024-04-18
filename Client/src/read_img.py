from PIL import Image
import os
import io
import sys

def main():

    if len(sys.argv) < 2 :
        print("Usage: python3 read_img.py <fileName>")
        return

    fileName = sys.argv[1]
    fileRoot, fileExtension = os.path.splitext(fileName)
    inputFilePath = f"./data/images/{fileRoot}.bin"

    # Vérifier si le fichier existe
    if not os.path.exists(inputFilePath):
        print(f"File {fileRoot}.bin not found.")
        return

    with open(inputFilePath, "rb") as file:
        image_bytes = file.read()

    image = Image.open(io.BytesIO(image_bytes))

    # Enregistrer l'image
    outputFileName = f"./data/images/{fileName}"
    image.save(outputFileName)

    os.remove(inputFilePath)

if __name__ == "__main__":
    main()