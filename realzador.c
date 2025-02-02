#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include "bmp.h"
#include "filter.h"

#define SHM_NAME "/bmp_shared"
#define SEM_NAME "/bmp_semaphore"

int main() {
    sem_t *sem = sem_open(SEM_NAME, 0);
    sem_wait(sem);

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    struct stat shm_stat;
    fstat(shm_fd, &shm_stat);
    BMP_Image *shm_ptr = mmap(0, shm_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }

    BMP_Image *imageOut = clone_bmp(shm_ptr);
    int edgeFilter[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};
    int numThreads = 4;
    applyParallel(shm_ptr, imageOut, edgeFilter, numThreads);

    memcpy(shm_ptr, imageOut, shm_stat.st_size);

    printf("Realce de bordes aplicado a la segunda mitad de la imagen.\n");

    munmap(shm_ptr, shm_stat.st_size);
    close(shm_fd);
    sem_post(sem);
    sem_close(sem);
    free_bmp(imageOut);

    return EXIT_SUCCESS;
}
