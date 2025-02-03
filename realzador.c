#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include "bmp.h"
#include "filter.h"
#include "filter.c"

#define SHM_NAME_ORIG "bmp_shared_memory"          // Memoria compartida original
#define SHM_NAME_NEW "processed_bmp_sharped_memory"  // Nueva memoria compartida para la imagen realzada
#define SEM_NAME "/bmp_semaphore"

int main() {
    sem_t *sem = sem_open(SEM_NAME, 0);
    sem_wait(sem);

    const char* shared_memory_name = "bmp_shared_memory";
    int shm_fd = shm_open(shared_memory_name, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    const int shared_memory_size = sizeof(BMP_Header) + 1920 * 1080 * 4;

    printf("Mapeando la memoria compartida...\n");
    void* shared_memory = mmap(0, shared_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    BMP_Image *imageOut = createBMPImageCopy(shared_memory);
    
    int edgeFilter[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};
    

    applyEdgeDetection(imageOut, edgeFilter);

    const char* new_shared_memory_name = "processed_bmp_sharped_memory"; 
    int shm_fd_new = shm_open(new_shared_memory_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd_new == -1) {
        perror("shm_open (nueva)");
        return EXIT_FAILURE;
    }

    printf("Asignando tamaño a la nueva memoria compartida...\n");
    if (ftruncate(shm_fd_new, shared_memory_size) == -1) {
        perror("Error al ajustar el tamaño de la nueva memoria compartida");
        exit(EXIT_FAILURE);
    }

    void* shm_ptr_new = mmap(0, shared_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_new, 0);
    if (shm_ptr_new == MAP_FAILED) {
        perror("mmap (nueva)");
        return EXIT_FAILURE;
    }

    memcpy(shm_ptr_new, imageOut, shared_memory_size);

    printf("Realce de bordes aplicado y guardado en nueva memoria compartida.\n");

    munmap(shared_memory, shared_memory_size);
    close(shm_fd);
    munmap(shm_ptr_new, shared_memory_size);
    close(shm_fd_new);
    sem_post(sem);
    sem_close(sem);
    free(imageOut);

    return EXIT_SUCCESS;
}