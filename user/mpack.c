#include <stdio.h>
#include <ulib.h>


#define N 45
#define NB 11
#define RPT 100

static int mat1[N][N];
static int mat2[N][N];
static int matr[N][N];

#define printf cprintf

int main(){

  printf("MPACK Benchmark\n");
  printf("Speed of thu-mips\n");
  printf("Params used:\n");
  printf("===================\n");
  printf("N:\t\t\t%d\n", N);
  printf("NB(RESV):\t\t%d\n", NB);
  
  printf("Begin to fill numbers\n");
  
  
  for(int i = 0, cnt = 1; i < N; i++, cnt++){
    for(int j = 0; j < N; j++){
      mat1[i][j] = cnt;
      mat2[i][j] = cnt + 42 + 1;
    }
  }

  printf("Begin to calc\n");
  
  for(int cnt = 0; cnt < RPT; cnt++){
    time_t beginClock = gettime_msec();
    for(int i = 0; i < N; i++){
      for(int k = 0; k < N; k++){
        int a_i_k = mat1[i][k];
        for (int j = 0; j < N; j++){
          matr[i][j] += a_i_k * mat2[k][j];
        }
      }
    }
    printf("Finished %d of %d runs\n", cnt + 1, RPT);
    printf("Time elapsed: %d ms\n", gettime_msec() - beginClock);
  }

  return 0;
}
