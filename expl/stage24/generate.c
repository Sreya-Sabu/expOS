#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE* fptr;
    fptr = fopen("num.dat", "w");
    int i = 1;
    for (i = 1; i <= 20; i++) {
        fprintf(fptr, "%d\n", i);
    }
    fclose(fptr);
    return 0;
}