#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

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

static void main_loop(const int BS, int maxloop)
{
    /* Leading dimension of the board array */
    int ldboard = BS + 2;
    /* Leading dimension of the neigbour counters array */
    int ldnbngb = BS;

    int *board = malloc( ldboard * ldboard * sizeof(int) );
    int *nbngb = malloc( ldnbngb * ldnbngb * sizeof(int) );
    int num_alive = generate_initial_board( BS, &(cell(1, 1)), ldboard );
    printf("Starting number of living cells = %d\n", num_alive);

    for (int loop = 1; loop <= maxloop; loop++) {

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

	#pragma omp parallel for schedule(static)
	for (int j = 1; j <= BS; j++) {
	    for (int i = 1; i <= BS; i++) {
		ngb( i, j ) =
		    cell( i-1, j-1 ) + cell( i, j-1 ) + cell( i+1, j-1 ) +
		    cell( i-1, j   ) +                  cell( i+1, j   ) +
		    cell( i-1, j+1 ) + cell( i, j+1 ) + cell( i+1, j+1 );
	    }
	}

	num_alive = 0;
	#pragma omp parallel for schedule(static) reduction(+:num_alive)
	for (int j = 1; j <= BS; j++) {
	    for (int i = 1; i <= BS; i++) {
		if ( (ngb( i, j ) < 2) || (ngb( i, j ) > 3) )
		    cell(i, j) = 0;
		else if ((ngb( i, j )) == 3)
		    cell(i, j) = 1;
		if (cell(i, j) == 1)
		    ++num_alive;
	    }
	}

	/* Avec les cellules sur les bords
           (utile pour vérifier les comm MPI) */
	/* output_board( BS+2, &(cell(0, 0)), ldboard, loop ); */

	/* Avec juste les "vraies" cellules: on commence à l'élément (1,1) */
	// output_board( BS, &(cell(1, 1)), ldboard, loop);
	// printf("%d cells are alive\n", num_alive);
    }

    printf("Final number of living cells = %d\n", num_alive);
    free(board);
    free(nbngb);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
	printf("Usage: %s nb_iterations size\n", argv[0]);
	return EXIT_SUCCESS;
    }
    int maxloop = atoi(argv[1]);
    int BS = atoi(argv[2]);

    printf("Running OMP version, "
	   "grid of size %d, %d iterations\n", BS, maxloop);

    double t1 = mytimer();
    main_loop(BS, maxloop);
    double t2 = mytimer();

    double temps = t2 - t1;
    printf("%.2lf\n",(double)temps * 1.e3);

    return EXIT_SUCCESS;
}
