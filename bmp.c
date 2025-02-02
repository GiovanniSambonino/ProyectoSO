#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "bmp.h"

// Estructura para pasar argumentos a cada hilo
typedef struct {
    BMP_Image* image;
    int start_row;
    int end_row;
    int blur_radius;
} ThreadArgs;

// Función que será ejecutada por cada hilo
void* applyBlurThread(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    BMP_Image* image = threadArgs->image;
    int start_row = threadArgs->start_row;
    int end_row = threadArgs->end_row;
    int blurRadius = threadArgs->blur_radius;

    int width = image->header.width_px;

    for (int i = start_row; i < end_row; i++) {
        for (int j = 0; j < width; j++) {
            int sumBlue = 0, sumGreen = 0, sumRed = 0;
            int count = 0;

            // Aplicar el desenfoque promediando los píxeles dentro del radio
            for (int di = -blurRadius; di <= blurRadius; di++) {
                for (int dj = -blurRadius; dj <= blurRadius; dj++) {
                    int ni = i + di;
                    int nj = j + dj;
                    if (ni >= 0 && ni < end_row && nj >= 0 && nj < width) {
                        sumBlue += image->pixels[ni][nj].blue;
                        sumGreen += image->pixels[ni][nj].green;
                        sumRed += image->pixels[ni][nj].red;
                        count++;
                    }
                }
            }

            // Asignar los valores promediados al píxel actual
            image->pixels[i][j].blue = sumBlue / count;
            image->pixels[i][j].green = sumGreen / count;
            image->pixels[i][j].red = sumRed / count;
        }
    }

    return NULL;
}

// Función para aplicar desenfoque utilizando múltiples hilos
void applyBlurMultiThreaded(BMP_Image* image, int num_threads) {
    int height = image->norm_height / 2;  // Limitar a la mitad superior
    int rows_per_thread = height / num_threads;
    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];

    // Crear hilos para aplicar el desenfoque en paralelo
    for (int i = 0; i < num_threads; i++) {
        int start_row = i * rows_per_thread;
        int end_row = (i == num_threads - 1) ? height : (i + 1) * rows_per_thread;  // El último hilo toma el resto

        args[i].image = image;
        args[i].start_row = start_row;
        args[i].end_row = end_row;
        args[i].blur_radius = 1;  // Puedes ajustar el radio del desenfoque si lo deseas

        pthread_create(&threads[i], NULL, applyBlurThread, &args[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}

/* USE THIS FUNCTION TO PRINT ERROR MESSAGES
   DO NOT MODIFY THIS FUNCTION
*/
void printError(int error)
{
  switch (error)
  {
  case ARGUMENT_ERROR:
    printf("Usage:ex5 <source> <destination>\n");
    break;
  case FILE_ERROR:
    printf("Unable to open file!\n");
    break;
  case MEMORY_ERROR:
    printf("Unable to allocate memory!\n");
    break;
  case VALID_ERROR:
    printf("BMP file not valid!\n");
    break;
  default:
    break;
  }
}

/* The input argument is the source file pointer. The function will first construct a BMP_Image image by allocating memory to it.
 * Then the function read the header from source image to the image's header.
 * Compute data size, width, height, and bytes_per_pixel of the image and stores them as image's attributes.
 * Finally, allocate menory for image's data according to the image size.
 * Return image;
 */
BMP_Image *createBMPImage(FILE *fptr)
{
  printf("Creando imagen\n");
  BMP_Image *image = (BMP_Image *)malloc(sizeof(BMP_Image));
  if (!image)
  {
    printError(MEMORY_ERROR);
    return NULL;
  }

  fread(&image->header, sizeof(BMP_Header), 1, fptr);

  if (ferror(fptr))
  {
    printError(VALID_ERROR);
    free(image);
    return NULL;
  }

  image->bytes_per_pixel = image->header.bits_per_pixel / 8;
  image->norm_height = abs(image->header.height_px);

  image->pixels = (Pixel **)malloc(image->norm_height * sizeof(Pixel *));
  if (!image->pixels)
  {
    printError(MEMORY_ERROR);
    free(image);
    return NULL;
  }

  for (int i = 0; i < image->norm_height; i++)
  {
    image->pixels[i] = malloc(image->header.width_px * sizeof(Pixel));
    if (!image->pixels[i])
    {
      printError(MEMORY_ERROR);
      for (int j = 0; j < i; j++)
      {
        free(image->pixels[j]);
      }
      free(image->pixels);
      free(image);
      return NULL;
    }
  }
  printf("Imagen creada\n");
  return image;
}

BMP_Image *createBMPImageCopy(const BMP_Image *template)
{
  BMP_Image *image = (BMP_Image *)malloc(sizeof(BMP_Image));
  if (image == NULL)
  {
    printError(MEMORY_ERROR);
    return NULL;
  }

  image->header = template->header;
  image->norm_height = template->norm_height;
  image->bytes_per_pixel = template->bytes_per_pixel;

  image->pixels = (Pixel **)malloc(image->norm_height * sizeof(Pixel *));
  if (image->pixels == NULL)
  {
    printError(MEMORY_ERROR);
    free(image);
    return NULL;
  }

  for (int i = 0; i < image->norm_height; i++)
  {
    image->pixels[i] = (Pixel *)malloc(template->header.width_px * sizeof(Pixel));
    if (image->pixels[i] == NULL)
    {
      printError(MEMORY_ERROR);
      for (int j = 0; j < i; j++)
      {
        free(image->pixels[j]);
      }
      free(image->pixels);
      free(image);
      return NULL;
    }
  }

  return image;
}

/* The input arguments are the source file pointer, the image data pointer, and the size of image data.
 * The functions reads data from the source into the image data matriz of pixels.
 */
void readImageData(FILE *srcFile, BMP_Image *image, int dataSize)
{
  printf("Leyendo imagen\n");
  fseek(srcFile, image->header.offset, SEEK_SET);
  if (ferror(srcFile))
  {
    printError(FILE_ERROR);
    return;
  }

  for (int i = 0; i < image->norm_height; i++)
  {
    fread(image->pixels[i], image->bytes_per_pixel, image->header.width_px, srcFile);
    if (ferror(srcFile))
    {
      printError(FILE_ERROR);
      return;
    }
  }
  printf("Datos leidos\n");
}

/* The input arguments are the pointer of the binary file, and the image data pointer.
 * The functions open the source file and call to CreateBMPImage to load de data image.
 */
void readImage(FILE *srcFile, BMP_Image **dataImage)
{
  *dataImage = createBMPImage(srcFile);
  if (dataImage == NULL)
  {
    printError(FILE_ERROR);
    return;
  }

  readImageData(srcFile, *dataImage, (*dataImage)->header.imagesize);
}

/* The input arguments are the destination file name, and BMP_Image pointer.
 * The function write the header and image data into the destination file.
 */
void writeImage(char *destFileName, BMP_Image *dataImage)
{

  FILE *destFile = fopen(destFileName, "wb");
  if (!destFile)
  {
    printError(FILE_ERROR);
    return;
  }

  fwrite(&dataImage->header, sizeof(BMP_Header), 1, destFile);
  fseek(destFile, dataImage->header.offset, SEEK_SET);

  for (int i = 0; i < dataImage->norm_height; i++)
  {
    fwrite(dataImage->pixels[i], sizeof(Pixel), dataImage->header.width_px, destFile);
  }

  fclose(destFile);
}

/* The input argument is the BMP_Image pointer. The function frees memory of the BMP_Image.
 */
void freeImage(BMP_Image *image)
{
  for (int i = 0; i < image->norm_height; i++)
  {
    free(image->pixels[i]);
  }
  free(image->pixels);
  free(image);
}

/* The functions checks if the source image has a valid format.
 * It returns TRUE if the image is valid, and returns FASLE if the image is not valid.
 * DO NOT MODIFY THIS FUNCTION
 */
int checkBMPValid(BMP_Header *header)
{
  // Make sure this is a BMP file
  if (header->type != 0x4d42)
  {
    return FALSE;
  }
  // Make sure we are getting 24 bits per pixel
  if (header->bits_per_pixel != 24 && header->bits_per_pixel != 32)
  {
    return FALSE;
  }
  // Make sure there is only one image plane
  if (header->planes != 1)
  {
    return FALSE;
  }
  // Make sure there is no compression
  if (header->compression != 0)
  {
    return FALSE;
  }
  return TRUE;
}

/* The function prints all information of the BMP_Header.
   DO NOT MODIFY THIS FUNCTION
*/
void printBMPHeader(BMP_Header *header)
{
  printf("file type (should be 0x4d42): %x\n", header->type);
  printf("file size: %d\n", header->size);
  printf("offset to image data: %d\n", header->offset);
  printf("header size: %d\n", header->header_size);
  printf("width_px: %d\n", header->width_px);
  printf("height_px: %d\n", header->height_px);
  printf("planes: %d\n", header->planes);
  printf("bits: %d\n", header->bits_per_pixel);
}

/* The function prints information of the BMP_Image.
   DO NOT MODIFY THIS FUNCTION
*/
void printBMPImage(BMP_Image *image)
{
  printf("data size is %lu\n", image->norm_height * image->header.width_px * sizeof(Pixel));
  printf("norm_height size is %d\n", image->norm_height);
  printf("bytes per pixel is %d\n", image->bytes_per_pixel);
}
