#include <iostream>
#include <iomanip>
#include <omp.h>
#include <cmath>

using namespace std;
#define N 10000000
#define CHUNK_SIZE (10*331118)

double calculate_pi(int num_threads){
    double pi = 0.0;
    double sum = 0.0;
    double h = 1.0/N;

    omp_set_num_threads(num_threads);

    #pragma omp parallel for reduction(+:sum) schedule(dynamic, CHUNK_SIZE)
    for (int i = 0; i < N; i++){
        double x = (i+0.5)*h;
        sum += 4.0 / (1.0 + x * x);
    }

    pi = sum * h;

    return pi;
}

int main(){
    int thread_counts[] = {1, 2, 4, 8, 12, 16};
    const int num_tests = sizeof(thread_counts) / sizeof(thread_counts[0]);

    cout << "N = " << N << ", Chunk size = " << CHUNK_SIZE << endl;
    cout << "Threads\tTime (sec)\tPi Value" << endl;
    cout << "------\t------\t------" << endl;

    double reference_pi = 0.0;
    double min_time = 1e9;
    int best_threads = 0;

    for (int t = 0; t < num_tests; t++){
        int num_threads = thread_counts[t];
        double start_time = omp_get_wtime();
        double pi = calculate_pi(num_threads);
        double end_time = omp_get_wtime();

        double elapsed = end_time - start_time;

        if (t == 0) reference_pi = pi;

        if(abs(pi - reference_pi) > 1e-6){
            cerr << "Error: Pi mismatch"<< endl;
            return 1;
        }

        cout << num_threads << "\t"<< fixed << setprecision(4) << elapsed <<"\t\t" << setprecision(10) << pi << endl;

        if (elapsed < min_time){
            min_time = elapsed;
            best_threads = num_threads;
        }
    }

    cout << "Best perfomance at " << best_threads << " threads (time = " << min_time << " sec)" << endl;
    return 0;
}