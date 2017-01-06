#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#define cell( _i_, _j_ ) board[ ldboard * (_j_) + (_i_) ]
#define ngb( _i_, _j_ )  nbngb[ ldnbngb * ((_j_) - 1) + ((_i_) - 1 ) ]

double mytimer(void)
{
    struct timeval tp;
    gettimeofday( &tp, NULL );
    return tp.tv_sec + 1e-6 * tp.tv_usec;
}

void output_board(int N, int *board, int ldboard, int loop)
{

    printf("loop %d\n", loop);
    for (int i=0; i<N; i++) {
	for (int j=0; j<N; j++) {
	    if ( cell( i, j ) == 1)
		printf("X");
	    else
		printf(".");
	}
	printf("\n");
    }
}

/**
 * This function generates the iniatl board with one row and one
 * column of living cells in the middle of the board
 */
int generate_initial_board(int N, int *board, int ldboard)
{
    int num_alive = 0;

    for (int i = 0; i < N; i++) {
	for (int j = 0; j < N; j++) {
	    if (i == N/2 || j == N/2) {
		cell(i, j) = 1;
		num_alive ++;
	    }
	    else {
		cell(i, j) = 0;
	    }
	}
    }

    return num_alive;
}

void barrier_STOP(int nb_threads)
{
    static int barrier = 0;    
    static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    static pthread_mutex_t mut_barrier = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&mut_barrier);

    barrier++;

    if (barrier == nb_threads) {
	barrier = 0;
	pthread_cond_broadcast(&cond);
    } else
	pthread_cond_wait(&cond, &mut_barrier);
    
    pthread_mutex_unlock(&mut_barrier);
}

int BS, maxloop;
int ldboard, ldnbngb, num_alive;
int *board, *nbngb;
int nb_threads;

static void *main_loop(void *args)
{
    int num_alive;
    int rank = (uintptr_t)args;

    for (int loop = 0; loop <= maxloop; loop++) {
	if(rank == 0){
	    cell(   0, 0   ) = cell(BS, BS);
	    cell(   0, BS+1) = cell(BS,  1);
	    cell(BS+1, 0   ) = cell( 1, BS);
	    cell(BS+1, BS+1) = cell( 1,  1);

	    for (int i = 1; i <= BS; i++) {
		cell(   i,    0) = cell( i, BS);
		cell(   i, BS+1) = cell( i,  1);
		cell(   0,    i) = cell(BS,  i);
		cell(BS+1,    i) = cell( 1,  i);
	    }
	}
	barrier_STOP(nb_threads);

	int block_size = BS/nb_threads;
	/* printf("j start : %d , j end : %d\n",  */
	/*        block_size*rank+1 , block_size*(rank+1)); */

	// 1 - Lire les cellules des threads voisins 
	int B = block_size*rank+1, C = block_size*(rank+1);
	for (int i = 1; i <= BS; i++) {
	    ngb( i, B ) =
		cell( i-1, B-1 ) + cell( i, B-1 ) + cell( i+1, B-1 );
	    ngb( i, C ) =
		cell( i-1, C+1 ) + cell( i, C+1 ) + cell( i+1, C+1 );
	}
	// 2 - Informer les voisins de la lecture
	
	// 3 - Calcul Interne
	for (int j = block_size*rank+1; j <= block_size*(rank+1) ; j++) {
	    for (int i = 1; i <= BS; i++) {
		ngb( i, j ) = ((i==1 || i==BS)?ngb(i, j): 0) +
		    (i>1) ? (cell( i-1, j-1 ) + cell( i, j-1 ) + cell( i+1, j-1 )) : 0
		    cell( i-1, j   ) +                  cell( i+1, j   ) +
		    (i<BS) ? (cell( i-1, j+1 ) + cell( i, j+1 ) + cell( i+1, j+1 )) : 0;
	    }
	}
	barrier_STOP(nb_threads);
	num_alive = 0;
	
	for (int j = block_size*rank+1; j <= block_size*(rank+1) ; j++) {
	    for (int i = 1; i <= bs; i++) {
		if ( (ngb( i, j ) < 2) || (ngb( i, j ) > 3) )
		    cell(i, j) = 0;
		else if ((ngb( i, j )) == 3)
		    cell(i, j) = 1;
		if (cell(i, j) == 1)
		    ++num_alive;
	    }
	}

	// 4 - Vérifier que les voisins ont lus les cellules aux bords


	// 5 - Mise à jour aux bords




	barrier_STOP(nb_threads);
	if(!rank) printf("iterations %d\n", loop);
	barrier_STOP(nb_threads);
	/* Avec les cellules sur les bords (utile pour vérifier les comm MPI) */
	/* output_board( BS+2, &(cell(0, 0)), ldboard, loop ); */

	/* Avec juste les "vraies" cellules: on commence à l'élément (1,1) */
	// output_board( BS, &(cell(1, 1)), ldboard, loop);
	// printf("%d cells are alive\n", num_alive);
    }

    printf("Final number of living cells = %d\n", num_alive);
    return NULL;
}

void game_of_life(void)
{
   /* Leading dimension of the board array */
    ldboard = BS + 2;
    /* Leading dimension of the neigbour counters array */
    ldnbngb = BS;

    board = malloc( ldboard * ldboard * sizeof(int) );
    nbngb = malloc( ldnbngb * ldnbngb * sizeof(int) );
    num_alive = generate_initial_board( BS, &(cell(1, 1)), ldboard );

    printf("Starting number of living cells = %d\n", num_alive);    
    nb_threads = 5;// get_nprocs();
    pthread_t t[nb_threads];

    for (int i=0; i<nb_threads; ++i)
	pthread_create(t+i, NULL, main_loop, (void*)(uintptr_t)i);
    for (int i=0; i<nb_threads; ++i)
	pthread_join(t[i], NULL);
    free(board);
    free(nbngb);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
	printf("Usage: %s nb_iterations size\n", argv[0]);
	return EXIT_SUCCESS;
    }
    maxloop = atoi(argv[1]);
    BS = atoi(argv[2]);

    printf("Running OMP version, "
	   "grid of size %d, %d iterations\n", BS, maxloop);

    double t1 = mytimer();
    game_of_life();
    double t2 = mytimer();

    double temps = t2 - t1;
    printf("%.2lf\n",(double)temps * 1.e3);

    return EXIT_SUCCESS;
}
