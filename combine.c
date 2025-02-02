#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "bmp.h"

int main() {
    const char* blurred_shared_memory_name = "processed_bmp_blurred_memory";
    const char* sharpened_shared_memory_name = "processed_bmp_sharped_memory";
    const int shared_memory_size = sizeof(BMP_Header) + 1920 * 1080 * 4; // Ajusta según el tamaño de la imagen

    // Abrir la memoria compartida de la imagen desenfocada
    int shm_fd_blurred = shm_open(blurred_shared_memory_name, O_RDONLY, 0666);
    if (shm_fd_blurred == -1) {
        perror("Error al abrir la memoria compartida desenfocada");
        exit(EXIT_FAILURE);
    }

    // Abrir la memoria compartida de la imagen realzada
    int shm_fd_sharpened = shm_open(sharpened_shared_memory_name, O_RDONLY, 0666);
    if (shm_fd_sharpened == -1) {
        perror("Error al abrir la memoria compartida realzada");
        exit(EXIT_FAILURE);
    }

    // Mapear las memorias compartidas
    void* shared_memory_blurred = mmap(0, shared_memory_size, PROT_READ, MAP_SHARED, shm_fd_blurred, 0);
    if (shared_memory_blurred == MAP_FAILED) {
        perror("Error al mapear la memoria desenfocada");
        exit(EXIT_FAILURE);
    }

    void* shared_memory_sharpened = mmap(0, shared_memory_size, PROT_READ, MAP_SHARED, shm_fd_sharpened, 0);
    if (shared_memory_sharpened == MAP_FAILED) {
        perror("Error al mapear la memoria realzada");
        exit(EXIT_FAILURE);
    }

    // Leer la cabecera de la imagen desde la memoria desenfocada (asumimos que ambas imágenes tienen la misma cabecera)
    BMP_Image final_image;
    final_image.header = *(BMP_Header*)shared_memory_blurred;
    final_image.norm_height = abs(final_image.header.height_px);
    final_image.bytes_per_pixel = final_image.header.bits_per_pixel / 8;

    // Asegurarse de que las filas están alineadas a 4 bytes
    int padding = (4 - (final_image.header.width_px * final_image.bytes_per_pixel) % 4) % 4;
    int row_size = final_image.header.width_px * final_image.bytes_per_pixel + padding;

    // Configurar los píxeles de la imagen combinada (sin usar memoria compartida, se va a guardar localmente)
    final_image.pixels = (Pixel**)(malloc(final_image.norm_height * sizeof(Pixel*)));
    for (int i = 0; i < final_image.norm_height; i++) {
        final_image.pixels[i] = (Pixel*)(malloc(final_image.header.width_px * sizeof(Pixel)));
    }

    // Acceder a los píxeles desenfocados y realzados
    uint8_t* blurred_pixelArray = (uint8_t*)shared_memory_blurred + sizeof(BMP_Header);
    uint8_t* sharpened_pixelArray = (uint8_t*)shared_memory_sharpened + sizeof(BMP_Header);

    // Copiar la mitad superior de la imagen desenfocada
    for (int i = 0; i < final_image.norm_height / 2; i++) {
        for (int j = 0; j < final_image.header.width_px; j++) {
            final_image.pixels[i][j] = ((Pixel*)(blurred_pixelArray + i * row_size))[j];
        }
    }

    // Copiar la mitad inferior de la imagen realzada
    for (int i = final_image.norm_height / 2; i < final_image.norm_height; i++) {
        for (int j = 0; j < final_image.header.width_px; j++) {
            final_image.pixels[i][j] = ((Pixel*)(sharpened_pixelArray + i * row_size))[j];
        }
    }

    // Guardar la imagen combinada en un archivo local
    if (access("outputs", F_OK) == -1) {
        // Crear directorio 'outputs' si no existe
        mkdir("outputs", 0755);
    }

    // Calcular correctamente el tamaño de la imagen con el padding incluido
    final_image.header.imagesize = final_image.norm_height * row_size;
    final_image.header.size = sizeof(BMP_Header) + final_image.header.imagesize;

    // Guardar la imagen combinada
    writeImage("outputs/img_mixed.bmp", &final_image);

    // Liberar la memoria utilizada para los píxeles de la imagen combinada
    for (int i = 0; i < final_image.norm_height; i++) {
        free(final_image.pixels[i]);
    }
    free(final_image.pixels);

    // Desmapear las memorias compartidas
    munmap(shared_memory_blurred, shared_memory_size);
    munmap(shared_memory_sharpened, shared_memory_size);

    // Cerrar los descriptores de memoria compartida
    close(shm_fd_blurred);
    close(shm_fd_sharpened);

    return 0;
}
