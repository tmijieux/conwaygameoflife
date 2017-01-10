#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#define SQUARE(x)  ((x)*(x))
#define MALLOC_ARRAY(var, size) ((var) = malloc((size)*sizeof(*(var))))
#define CALLOC_ARRAY(var, size) ((var) = calloc((size), sizeof(*(var))))
#define SWAP_POINTER(var_a_, var_b_) do {       \
        void *_tmp_ptr_macroXX_ = (var_a_);     \
        var_a_ = var_b_;                        \
        var_b_ = _tmp_ptr_macroXX_;             \
    } while(0)

#define cell( _i_, _j_ ) board[ ld_board * (_j_) + (_i_) ]
#define ncell( _i_, _j_ ) next_board[ ld_board * (_j_) + (_i_) ]

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
 * This function generates the initial board with one row and one
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
	    } else
		cell(i, j) = 0;
	}
    }

    return num_alive;
}

int board_size, maxloop, ld_board;
int *board, *next_board;

static inline void
cgl_main_loop_debug_output(int loop, int num_alive)
{
    /* Avec les cellules sur les bords
       (utile pour vérifier les comm MPI) */
    /* output_board( board_size+2, &(cell(0, 0)), ld_board, loop ); */

    /* Avec juste les "vraies" cellules: on commence à l'élément (1,1) */
    // output_board( board_size, &(cell(1, 1)), ld_board, loop);
    // printf("%d cells are alive\n", num_alive);
}

static void main_loop(void)
{
    int num_alive = 0;

    for (int loop = 0; loop < maxloop; ++loop) {
	cell(   0, 0   )                 = cell(board_size, board_size);
	cell(   0, board_size+1)         = cell(board_size,  1);
	cell(board_size+1, 0   )         = cell( 1, board_size);
	cell(board_size+1, board_size+1) = cell( 1,  1);

	for (int i = 1; i <= board_size; i++) {
	    cell(   i,    0)         = cell( i, board_size);
	    cell(   i, board_size+1) = cell( i,  1);
	    cell(   0,    i)         = cell(board_size,  i);
	    cell(board_size+1,    i) = cell( 1,  i);
	}

        num_alive = 0;
        #pragma omp parallel for schedule(static) reduction(+:num_alive)
        for (int j = 1; j <= board_size; j++) {
            for (int i = 1; i <= board_size; i++) {
                int ngb =
                    cell(i-1, j-1) + cell(i, j-1) + cell(i+1, j-1) +
                    cell(i-1, j  ) +                cell(i+1, j  ) +
                    cell(i-1, j+1) + cell(i, j+1) + cell(i+1, j+1);

                int life = cell(i,j);
                if ( ngb < 2 || ngb > 3 )
                    life = 0;
                else if ( ngb == 3 )
                    life = 1;

                if (life == 1)
                    ++num_alive;

                ncell(i, j) = life;
            }
        }
        SWAP_POINTER(board, next_board);

        #ifdef CGL_DEBUG
        cgl_main_loop_debug_output(loop, num_alive);
        #endif
    }
    printf("Final number of living cells = %d\n", num_alive);
}

static void game_of_life(void)
{
    /* Leading dimension of the board array */
    ld_board = board_size + 2;

    MALLOC_ARRAY(board, SQUARE(ld_board));
    MALLOC_ARRAY(next_board, SQUARE(ld_board));

    int num_alive;
    num_alive = generate_initial_board(board_size, &(cell(1, 1)), ld_board);
    printf("Starting number of living cells = %d\n", num_alive);

    main_loop();

    free(board);
    free(next_board);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
	printf("Usage: %s Nb_Iterations Board_Size\n", argv[0]);
	return EXIT_SUCCESS;
    }
    maxloop = atoi(argv[1]);
    board_size = atoi(argv[2]);

    printf("Running OMP version, "
           "grid of size %d, %d iterations\n", board_size, maxloop);

    double t1 = cgl_timer();
    game_of_life();
    double t2 = cgl_timer();
    double time = t2 - t1;
    printf("%.2lf ms\n", time * 1.e3);

    return EXIT_SUCCESS;
}
