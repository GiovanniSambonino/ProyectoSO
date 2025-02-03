#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
    uint32_t dib_header_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_in_color_table;
    uint32_t important_color_count;
} BMPHeader;
#pragma pack(pop)

void readBMP(const char* filePath, BMPHeader* header, uint8_t** pixelArray) {
    FILE* file = fopen(filePath, "rb");
    if (file == NULL) {
        perror("Error al abrir el archivo BMP");
        exit(EXIT_FAILURE);
    }

    fread(header, sizeof(BMPHeader), 1, file);
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, header->offset, SEEK_SET);

    *pixelArray = (uint8_t*)malloc(fileSize - header->offset);
    fread(*pixelArray, 1, fileSize - header->offset, file);

    fclose(file);
}

int main() {
    const char* shared_memory_name = "bmp_shared_memory";
    const int shared_memory_size = sizeof(BMPHeader) + 1920 * 1080 * 4; 

    int shm_fd = shm_open(shared_memory_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }
   
    ftruncate(shm_fd, shared_memory_size);

    
    void* shared_memory = mmap(0, shared_memory_size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        exit(EXIT_FAILURE);
    }

    BMPHeader header;
    uint8_t* pixelArray = NULL;
    char filePath[256];

    while (1) {
        printf("Por Favor, ingrese la ruta de la imagen BMP: ");
        scanf("%s", filePath);

        readBMP(filePath, &header, &pixelArray);

        memcpy(shared_memory, &header, sizeof(BMPHeader));
        memcpy((char*)shared_memory + sizeof(BMPHeader), pixelArray, header.size - header.offset);

        printf("La Imagen cargada correctamente en la memoria compartida.\n");

        free(pixelArray);
    }

    return 0;
}