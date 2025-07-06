#include <stdio.h>
#include <pdfio.h>
#include <zlib.h>
#include <string.h>

int main()
{
    printf("Hello PDFio %s!\n", PDFIO_VERSION);
    char a[] = "Hello";
    char *b = strdup(a);
    return 0;
}
