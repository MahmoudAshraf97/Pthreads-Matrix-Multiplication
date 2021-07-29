#include <stdio.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#define NUM_THREADS 16


//create a structure to pass the thread parameters
struct Thread {
    int     thread_id;
    int     work_start;
    int     work_end;
    int     p_f;
    int     n_f;
    double* matrix1;
    double* matrix2;
    double* result;
    
};

void* PartialSum(void* threadarg) {
    double internal_sum = 0;
    struct Thread* parameters;
    parameters = (struct Thread*)threadarg;
    printf("Start of Thread %d =  %d \n", parameters->thread_id, parameters->work_start);
    printf("End of Thread %d =  %d \n", parameters->thread_id, parameters->work_end);
    for (int z = parameters->work_start; z <= parameters->work_end ; z++) {
        for (int k = 0; k < parameters->n_f; k++) {
            int i = z / parameters->p_f;
            int j = z % parameters->p_f; 
            //printf("i = %d, j = %d", i, j);
            parameters->result[z] += parameters->matrix1[i * parameters->n_f + k] * parameters->matrix2[k * parameters->p_f + j];
        }
        //printf("Result %.0f\n", parameters->result[z]);
    }
    return 0;
}
int main() {
    
    pthread_t threads[NUM_THREADS];
 
    int rc;

    double serial_t = 0.0;
    double parallel_t = 0.0;
    
    
    // Creating and Filling the Array
 
    int m = 2048;
    int n = 2048;
    int p = 2048;
    double *matrix1;    // We uses this reference variable to access
    double* matrix2;   // dynamically created array elements
    double* result_s;
    double* result_p;

    matrix1 = (double*)calloc(m * n, sizeof(double));  // Make double array of size elements
    matrix2 = (double*)calloc(n * p, sizeof(double));  // Make double array of size elements
    result_s = (double*)calloc(m * p, sizeof(double));  // Make double array of size elements
    result_p = (double*)calloc(m * p, sizeof(double));  // Make double array of size elements

    // Fill the array with 1 
    for (int i = 0; i < m * n; i++) {
        matrix1[i] = 1;
    }
    for (int i = 0; i < n * p; i++) {
        matrix2[i] = 2;
    }
    for (int i = 0; i < m * p; i++) {
        result_s[i] = 0;
        result_p[i] = 0;
    }
    


    //serial excution
    clock_t s_start = clock();
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < p; j++) {
            for (int k = 0; k < n; k++) {
                result_s[i*p+j] += matrix1[i*n+k] * matrix2[k*p+j];
            }
        }
    }

    clock_t s_end = clock();

    //parallel excution
    clock_t p_start = clock();

    //Create parameters array
    struct Thread param[NUM_THREADS]; 

    //Setting parameters
    int batch_size = m * p / NUM_THREADS;
    for (int t = 0; t < NUM_THREADS; t++) 
    {
        param[t].thread_id = t;
        param[t].work_start = t * batch_size;
        param[t].work_end = ((t + 1) * batch_size) - 1;
        param[t].p_f = p;
        param[t].n_f = n;
        param[t].matrix1 = matrix1;
        param[t].matrix2 = matrix2;
        param[t].result = result_p;
    }

    //distribution of the remaining data among the threads
    if (m*p % NUM_THREADS) {   

        for (int i = 0; i < (m*p % NUM_THREADS); i++) {

            param[i].work_end += (i + 1);
            param[i + 1].work_start += (i + 1);
        }
        for (int i = (m*p % NUM_THREADS); i <= NUM_THREADS; i++) {

            param[i].work_end += (m*p % NUM_THREADS);

            if ((i+1) < NUM_THREADS) {

                param[i + 1].work_start += (m*p % NUM_THREADS);
            }
            
        }
    }
    
    //Creating threads and assigning tasks
    for (int t = 0; t < NUM_THREADS; t++)
    {
        rc = pthread_create(&threads[t], NULL, PartialSum, (void*) &param[t]);
        if (rc)
        {
            perror("ERROR:");
            exit(-1);


        }
    }

    
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);
    clock_t p_end = clock();

    for (int i = 0; i < m*p; i++) {
        if (result_s[i] != result_p[i]) {
            printf("Result Mismatch! %.0f %.0f %d\n", result_s[i] , result_p[i] , i);
        }
    }
    serial_t += (double)(s_end - s_start) / CLOCKS_PER_SEC;
    parallel_t += (double)(p_end - p_start) / CLOCKS_PER_SEC;

    printf("Serial Time elpased is %f seconds\n", serial_t);
    printf("Parallel Time elpased is %f seconds", parallel_t);
    
    return 0;

}
