#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include "bmp.h"
#include "filter.h"
#include "filter.c"

#define SHM_NAME "/bmp_shared"
#define SEM_NAME "/bmp_semaphore"

int main() {
    sem_t *semaforo = sem_open(SEM_NAME, 0);
    sem_wait(semaforo);

    int memocom = shm_open(SHM_NAME, O_RDWR, 0666);
    if (memocom == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    struct stat memocom_stat;
    fstat(memocom, &memocom_stat);
    BMP_Image *memocom_ptr = mmap(0, memocom_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, memocom, 0);
    if (memocom_ptr == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }

    BMP_Image *imageOut = createBMPImageCopy(memocom_ptr);
    int edgeFilter[3][3] = {{-1, -1, -1}, 
                            {-1, 8, -1}, 
                            {-1, -1, -1}};
    int numThreads = 4;
    applyParallel(memocom_ptr, imageOut, edgeFilter, numThreads);

    memcpy(memocom_ptr, imageOut, memocom_stat.st_size);

    printf("Aplicado a la segunda mitad de la imagen un realce de bordes\n");

    munmap(memocom_ptr, memocom_stat.st_size);
    close(memocom);
    sem_post(semaforo);
    sem_close(semaforo);
    free(imageOut);

    return EXIT_SUCCESS;
}
