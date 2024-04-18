#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <png.h>

int main() {
    // Ouvrir le fichier contenant les données binaires de l'image en mode binaire
    FILE *file = fopen("./test", "rb");
    if (!file) {
        fprintf(stderr, "Impossible d'ouvrir le fichier.\n");
        return 1;
    }

    // Initialiser la structure pour les erreurs
    struct jpeg_error_mgr jerr;
    struct jpeg_decompress_struct cinfo;
    cinfo.err = jpeg_std_error(&jerr);

    // Initialiser le décompresseur JPEG
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, file);

    // Lire les informations d'en-tête du fichier JPEG
    jpeg_read_header(&cinfo, TRUE);

    // Démarrer la décompression
    jpeg_start_decompress(&cinfo);

    // Allouer de la mémoire pour stocker les pixels de l'image
    unsigned long image_size = cinfo.output_width * cinfo.output_height * cinfo.output_components;
    unsigned char *image_buffer = (unsigned char*)malloc(image_size);

    // Lire les lignes de l'image
    while (cinfo.output_scanline < cinfo.output_height) {
        unsigned char *buffer_array[1];
        buffer_array[0] = image_buffer + (cinfo.output_scanline * cinfo.output_width * cinfo.output_components);
        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }

    // Terminer la décompression
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    // Fermer le fichier
    fclose(file);

    // Enregistrer les pixels de l'image dans un fichier PNG
    FILE *output_file = fopen("output.png", "wb");
    if (!output_file) {
        fprintf(stderr, "Impossible d'ouvrir le fichier de sortie.\n");
        free(image_buffer);
        return 1;
    }

    // Initialiser la structure png pour l'écriture
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(output_file);
        fprintf(stderr, "Erreur lors de la création de la structure PNG.\n");
        free(image_buffer);
        return 1;
    }

    // Initialiser la structure d'informations sur le fichier PNG
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(output_file);
        fprintf(stderr, "Erreur lors de la création de la structure d'informations PNG.\n");
        free(image_buffer);
        return 1;
    }

    // Initialiser les flux de lecture/écriture de données PNG
    png_init_io(png_ptr, output_file);

    // Configurer les paramètres de l'image PNG
    png_set_IHDR(png_ptr, info_ptr, cinfo.output_width, cinfo.output_height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // Ecrire les données de l'image dans le fichier PNG
    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, &image_buffer);
    png_write_end(png_ptr, NULL);

    // Nettoyer les structures et libérer la mémoire
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(output_file);
    free(image_buffer);

    return 0;
}
