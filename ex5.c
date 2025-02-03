#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "bmp.h"
#include "filter.h" 

int main(int argc, char **argv) {
    // Verificar si se ha especificado el número de hilos
    if (argc < 2) {
        printf("Uso: %s <numero_de_hilos> [imagen_de_salida]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_threads = atoi(argv[1]);  // Obtener el número de hilos del argumento

    if (num_threads <= 0) {
        printf("El número de hilos debe ser mayor a 0\n");
        exit(EXIT_FAILURE);
    }

    // Asignar un nombre de archivo predeterminado si no se proporciona
    const char* output_image = (argc == 3) ? argv[2] : "output.bmp";

    const char* shared_memory_name = "bmp_shared_memory";
    const char* new_shared_memory_name = "processed_bmp_blurred_memory";
    const int shared_memory_size = sizeof(BMP_Header) + 1920 * 1080 * 4; // Ajusta según el tamaño de la imagen

    printf("-----------------------------------------------\n");

    printf("Abriendo la memoria compartida '%s'...\n", shared_memory_name);

    // Abrir la memoria compartida para leer la imagen
    int shm_fd = shm_open(shared_memory_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error al abrir la memoria compartida");
        exit(EXIT_FAILURE);
    }
    printf("Memoria compartida '%s' abierta con éxito.\n", shared_memory_name);

    // Mapear la memoria compartida
    printf("Mapeando la memoria compartida...\n");
    void* shared_memory = mmap(0, shared_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    printf("Memoria compartida mapeada con éxito.\n");

    // Acceder a la cabecera y los datos de la imagen desde la memoria compartida
    printf("Accediendo a los datos de la imagen...\n");
    BMP_Image image;
    image.header = *(BMP_Header*)shared_memory;
    image.norm_height = abs(image.header.height_px);
    image.bytes_per_pixel = image.header.bits_per_pixel / 8;

    printf("Imagen cargada. Dimensiones: %dx%d, Bytes por píxel: %d\n", image.header.width_px, image.norm_height, image.bytes_per_pixel);

    // Configurar los píxeles de la imagen para que apunten a la memoria compartida
    image.pixels = (Pixel**)(malloc(image.norm_height * sizeof(Pixel*)));
    uint8_t* pixelArray = (uint8_t*)shared_memory + sizeof(BMP_Header);
    for (int i = 0; i < image.norm_height; i++) {
        image.pixels[i] = (Pixel*)(pixelArray + i * image.header.width_px * image.bytes_per_pixel);
    }
    printf("Píxeles configurados correctamente.\n");

    // Aplicar el filtro de desenfoque con múltiples hilos
    printf("Aplicando filtro de desenfoque con %d hilos...\n", num_threads);
    applyDesenfoque(&image, num_threads);
    printf("Filtro de desenfoque aplicado con éxito.\n");

    // Guardar la imagen desenfocada de vuelta al archivo si se proporciona un nombre de archivo
    printf("Guardando la imagen desenfocada en el archivo '%s'...\n", output_image);
    writeImage(output_image, &image);
    printf("Imagen guardada en '%s'.\n", output_image);

    // Crear nueva memoria compartida para la imagen desenfocada
    printf("Creando nueva memoria compartida '%s'...\n", new_shared_memory_name);
    int new_shm_fd = shm_open(new_shared_memory_name, O_CREAT | O_RDWR, 0666);
    if (new_shm_fd == -1) {
        perror("Error al crear la nueva memoria compartida");
        free(image.pixels);
        munmap(shared_memory, shared_memory_size);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    printf("Nueva memoria compartida '%s' creada con éxito.\n", new_shared_memory_name);

    // Asignar tamaño a la nueva memoria compartida
    printf("Asignando tamaño a la nueva memoria compartida...\n");
    if (ftruncate(new_shm_fd, shared_memory_size) == -1) {
        perror("Error al ajustar el tamaño de la nueva memoria compartida");
        close(new_shm_fd);
        free(image.pixels);
        munmap(shared_memory, shared_memory_size);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    printf("Tamaño de la nueva memoria compartida asignado con éxito.\n");

    // Mapear la nueva memoria compartida
    printf("Mapeando la nueva memoria compartida...\n");
    void* new_shared_memory = mmap(0, shared_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED, new_shm_fd, 0);
    if (new_shared_memory == MAP_FAILED) {
        perror("Error al mapear la nueva memoria compartida");
        close(new_shm_fd);
        free(image.pixels);
        munmap(shared_memory, shared_memory_size);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    printf("Nueva memoria compartida mapeada con éxito.\n");

    // Copiar la imagen desenfocada a la nueva memoria compartida
    printf("Copiando la imagen desenfocada a la nueva memoria compartida...\n");
    memcpy(new_shared_memory, shared_memory, shared_memory_size);
    printf("Imagen desenfocada copiada a la nueva memoria compartida con éxito.\n");

    // Liberar la memoria utilizada para los punteros de píxeles
    printf("Liberando memoria...\n");
    free(image.pixels);

    // Desmapear y cerrar las memorias compartidas
    printf("Desmapeando y cerrando las memorias compartidas...\n");
    munmap(shared_memory, shared_memory_size);
    munmap(new_shared_memory, shared_memory_size);
    close(shm_fd);
    close(new_shm_fd);
    printf("Memorias compartidas cerradas correctamente. Proceso completado.\n");

    printf("-----------------------------------------------\n");

    return 0;
}