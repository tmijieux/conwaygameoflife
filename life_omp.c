#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#define cell( _i_, _j_ ) board[ ld_board * (_j_) + (_i_) ]
#define ngb( _i_, _j_ )  nb_neighbour[ ld_nb_neighbour * ((_j_) - 1) + ((_i_) - 1 ) ]

double cgl_timer(void)
{
    struct timeval tp;
    gettimeofday( &tp, NULL );
    return tp.tv_sec + 1e-6 * tp.tv_usec;
}

void output_board(int N, int *board, int ld_board, int loop)
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
int generate_initial_board(int N, int *board, int ld_board)
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

static void main_loop(const int board_size, int maxloop)
{
    /* Leading dimension of the board array */
    int ld_board = board_size + 2;
    /* Leading dimension of the neigbour counters array */
    int ld_nb_neighbour = board_size;

    int *board = malloc( ld_board * ld_board * sizeof(int) );
    int *nb_neighbour = malloc( ld_nb_neighbour * ld_nb_neighbour * sizeof(int) );
    int num_alive = generate_initial_board( board_size, &(cell(1, 1)), ld_board );
    printf("Starting number of living cells = %d\n", num_alive);

    for (int loop = 1; loop <= maxloop; loop++) {

	cell(   0, 0   ) = cell(board_size, board_size);
	cell(   0, board_size+1) = cell(board_size,  1);
	cell(board_size+1, 0   ) = cell( 1, board_size);
	cell(board_size+1, board_size+1) = cell( 1,  1);

	for (int i = 1; i <= board_size; i++) {
	    cell(   i,    0) = cell( i, board_size);
	    cell(   i, board_size+1) = cell( i,  1);
	    cell(   0,    i) = cell(board_size,  i);
	    cell(board_size+1,    i) = cell( 1,  i);
	}

	#pragma omp parallel for schedule(static)
	for (int j = 1; j <= board_size; j++) {
	    for (int i = 1; i <= board_size; i++) {
		ngb( i, j ) =
		    cell( i-1, j-1 ) + cell( i, j-1 ) + cell( i+1, j-1 ) +
		    cell( i-1, j   ) +                  cell( i+1, j   ) +
		    cell( i-1, j+1 ) + cell( i, j+1 ) + cell( i+1, j+1 );
	    }
	}

	num_alive = 0;
	#pragma omp parallel for schedule(static) reduction(+:num_alive)
	for (int j = 1; j <= board_size; j++) {
	    for (int i = 1; i <= board_size; i++) {
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
	/* output_board( board_size+2, &(cell(0, 0)), ld_board, loop ); */

	/* Avec juste les "vraies" cellules: on commence à l'élément (1,1) */
	// output_board( board_size, &(cell(1, 1)), ld_board, loop);
	// printf("%d cells are alive\n", num_alive);
    }

    printf("Final number of living cells = %d\n", num_alive);
    free(board);
    free(nb_neighbour);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
	printf("Usage: %s nb_iterations size\n", argv[0]);
	return EXIT_SUCCESS;
    }
    int maxloop = atoi(argv[1]);
    int board_size = atoi(argv[2]);

    printf("Running OMP version, "
	   "grid of size %d, %d iterations\n", board_size, maxloop);

    double t1 = cgl_timer();
    main_loop(board_size, maxloop);
    double t2 = cgl_timer();

    double temps = t2 - t1;
    printf("%.2lf\n",(double)temps * 1.e3);

    return EXIT_SUCCESS;
}
