#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <mpi.h>
#include <math.h>

#define SQUARE(x)  ((x)*(x))
#define SQUARE_UL(X) SQUARE(((uint64_t) (X)))

#define MALLOC_ARRAY(var, size) ((var) = malloc((size)*sizeof(*(var))))
#define CALLOC_ARRAY(var, size) ((var) = calloc((size), sizeof(*(var))))
#define ASSERT_MSG(msg, cond) assert(  ((void)(msg), (cond)) )

#define cell( _i_, _j_ ) board[ ld_board * (_j_) + (_i_) ]
#define ngb( _i_, _j_ )  nb_neighbour[ ld_nb_neighbour * ((_j_) - 1) + ((_i_) - 1 ) ]

typedef struct cgl_proc_ cgl_proc;
struct cgl_proc_ {
    int rank;
    int group_size;
    int N;
    MPI_Comm cart_comm, line_comm, col_comm;
    int line, col;
    int prev_col, next_col;
    int prev_line, next_line;
};

int *board, *nb_neighbour;

static double cgl_timer(void)
{
    struct timeval tp;
    gettimeofday( &tp, NULL );
    return tp.tv_sec + 1e-6 * tp.tv_usec;
}

static void output_board(int N, int *board, int ld_board, int loop)
{
    printf("loop %d\n", loop);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if ( cell( i, j ) == 1 )
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
static int generate_initial_board(int N, int board[], int ld_board)
{
    int num_alive = 0;

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == N/2 || j == N/2) {
                cell(i, j) = 1;
                num_alive ++;
            } else
                cell(i, j) = 0;
        }
    }
    return num_alive;
}

static int main_loop(cgl_proc *P, int maxloop)
{
    (void) P;

    int num_alive = 0;
    for (int loop = 0; loop < maxloop; ++loop) {




        MPI_Barrier(MPI_COMM_WORLD);
    }

    return num_alive;
}

static void game_of_life(cgl_proc *P, int maxloop, int board_size)
{
    /* Leading dimension of the board array */
    int ld_board = board_size + 2;
    /* Leading dimension of the neigbour counters array */
    int ld_nb_neighbour = board_size;

    MALLOC_ARRAY(board, SQUARE(ld_board));
    MALLOC_ARRAY(nb_neighbour, SQUARE(ld_nb_neighbour));

    int num_alive;
    num_alive = generate_initial_board(board_size, &(cell(1, 1)), ld_board);

    printf("Starting number of living cells = %d\n", num_alive);
    ASSERT_MSG(
        "board size must be multiple of proc count",
        board_size % P->group_size == 0
    );

    main_loop(P, maxloop);
    printf("Final number of living cells = %d\n", num_alive);

    free(board);
    free(nb_neighbour);
}

void cgl_proc_init(cgl_proc *p)
{
    MPI_Comm_size(MPI_COMM_WORLD, &p->group_size);

    double Nd = sqrt(p->group_size);
    ASSERT_MSG(
        "processus must be perfect integer square",
        floor(Nd) == Nd
    );

    int N = p->N   = (int)Nd;
    int dims[2]    = {N, N};
    int periods[2] = {1, 1};

    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &p->cart_comm);
    MPI_Comm_rank(p->cart_comm, &p->rank);

    {
        int rem_dims[2] = {1, 0};
        MPI_Cart_sub(p->cart_comm, rem_dims, &p->line_comm);
        MPI_Comm_rank(p->line_comm, &p->col);
    }

    {
        int rem_dims[2] = {0, 1};
        MPI_Cart_sub(p->cart_comm, rem_dims, &p->col_comm);
        MPI_Comm_rank(p->col_comm, &p->line);
    }

    MPI_Cart_shift(p->col_comm, 0, 1, &p->prev_col, &p->next_col);
    MPI_Cart_shift(p->line_comm, 0, 1, &p->prev_line, &p->next_line);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: %s Nb_Iterations Board_Size\n", argv[0]);
        return EXIT_SUCCESS;
    }

    cgl_proc P;
    MPI_Init(NULL, NULL);
    cgl_proc_init(&P);

    int maxloop = atoi(argv[1]);
    int board_size = atoi(argv[2]);


    printf("Running MPI version, "
           "grid of size %d, %d iterations\n"
           "rank %d, group_size %d",
           board_size, maxloop, P.rank, P.group_size);

    double t1 = cgl_timer();
    game_of_life(&P, maxloop, board_size);
    double t2 = cgl_timer();

    double time = t2 - t1;
    printf("%gms\n", time * 1.e3);

    MPI_Finalize();
    return EXIT_SUCCESS;
}
