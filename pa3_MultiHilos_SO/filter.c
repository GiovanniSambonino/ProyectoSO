#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"
typedef struct
{
    BMP_Image *imageIn;
    BMP_Image *imageOut;
    int **boxFilter;
    int startRow;
    int endRow;
} ThreadData;

void apply(BMP_Image *imageIn, BMP_Image *imageOut)
{

    int boxFilter[3][3] = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}};

    for (int i = 1; i < imageIn->norm_height - 1; i++)
    {
        for (int j = 1; j < imageIn->header.width_px - 1; j++)
        {
            int r = 0, g = 0, b = 0, a = 0;
            for (int k = -1; k <= 1; k++)
            {
                for (int l = -1; l <= 1; l++)
                {
                    r += imageIn->pixels[i + k][j + l].red * boxFilter[k + 1][l + 1];
                    g += imageIn->pixels[i + k][j + l].green * boxFilter[k + 1][l + 1];
                    b += imageIn->pixels[i + k][j + l].blue * boxFilter[k + 1][l + 1];
                    a += imageIn->pixels[i + k][j + l].alpha * boxFilter[k + 1][l + 1];
                }
            }

            imageOut->pixels[i][j].red = r / 9;
            imageOut->pixels[i][j].green = g / 9;
            imageOut->pixels[i][j].blue = b / 9;
            imageOut->pixels[i][j].alpha = a / 9;
        }
    }
}

void *filterThreadWorker(void *args)
{
    ThreadData *data = (ThreadData *)args;

    for (int i = data->startRow; i < data->endRow; i++)
    {
        for (int j = 1; j < data->imageIn->header.width_px - 1; j++)
        {
            int r = 0, g = 0, b = 0;
            for (int k = -1; k <= 1; k++)
            {
                for (int l = -1; l <= 1; l++)
                {
                    r += data->imageIn->pixels[i + k][j + l].red * data->boxFilter[k + 1][l + 1];
                    g += data->imageIn->pixels[i + k][j + l].green * data->boxFilter[k + 1][l + 1];
                    b += data->imageIn->pixels[i + k][j + l].blue * data->boxFilter[k + 1][l + 1];
                }
            }

            data->imageOut->pixels[i][j].red = r / 9;
            data->imageOut->pixels[i][j].green = g / 9;
            data->imageOut->pixels[i][j].blue = b / 9;
        }
    }

    for (int i = 0; i < 3; i++)
    {
        free(data->boxFilter[i]);
    }
    free(data->boxFilter);

    return NULL;
}

void applyParallel(BMP_Image *imageIn, BMP_Image *imageOut, int boxFilter[3][3], int numThreads)
{

    pthread_t threads[numThreads];
    int rowsPerThread = imageIn->norm_height / numThreads;

    for (int i = 0; i < numThreads; i++)
    {
        ThreadData *data = malloc(sizeof(ThreadData));
        data->imageIn = imageIn;
        data->imageOut = imageOut;
        data->startRow = i * rowsPerThread;
        data->endRow = (i == numThreads - 1) ? imageIn->norm_height : (i + 1) * rowsPerThread;
        data->boxFilter = malloc(3 * sizeof(int *));
        for (int j = 0; j < 3; j++)
        {
            data->boxFilter[j] = malloc(3 * sizeof(int));
            for (int k = 0; k < 3; k++)
            {
                data->boxFilter[j][k] = boxFilter[j][k];
            }
        }
        pthread_create(&threads[i], NULL, filterThreadWorker, (void *)data);
    }

    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }
}
