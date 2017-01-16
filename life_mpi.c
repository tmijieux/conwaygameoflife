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

#define cell( _i_, _j_ ) B->board[ B->ld_board * (_j_) + (_i_) ]
#define ngb( _i_, _j_ )  B->nb_neighbour[ B->ld_nb_neighbour * ((_j_) - 1) + ((_i_) - 1 ) ]

typedef struct cgl_proc_ cgl_proc;
struct cgl_proc_ {
    int rank;
    int group_size;
    int group_length;

    MPI_Comm cart_comm, line_comm, col_comm;
    int line, col;
    int prev_col, next_col;
    int prev_line, next_line;
};

typedef struct cgl_board_ cgl_board;
struct cgl_board_ {
    int n;
    char *board;
    char *nb_neighbour;
    int ld_board;
    int ld_nb_neighbour;
};

static double cgl_timer(void)
{
    struct timeval tp;
    gettimeofday( &tp, NULL );
    return tp.tv_sec + 1e-6 * tp.tv_usec;
}

static void output_board(cgl_board *B, int loop)
{
    int N = B->n;

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


#define GLOBAL_TO_LOCAL_ABSCISSA(x_) ((x_) - (P->col*B->n))
#define GLOBAL_TO_LOCAL_ORDINATE(y_) ((y_) - (P->line*B->n))
#define IS_LOCAL_ABSCISSA(x_) ((x_) >= 0 && (x_) < B->n)
#define IS_LOCAL_ORDINATE(y_) IS_LOCAL_ABSCISSA(y_)

static int
cgl_board_generate(cgl_board *B, cgl_proc *P)
{

    int num_alive = 0;
    int n = B->n;
    int N = n * P->group_length;
    int i = GLOBAL_TO_LOCAL_ABSCISSA(N/2);
    int j = GLOBAL_TO_LOCAL_ORDINATE(N/2);

    if (IS_LOCAL_ABSCISSA(i)) {
        for (int k = 0; k < n; ++k) {
            cell(i, k) = 1;
            ++ num_alive;
        }
    }
    if (IS_LOCAL_ORDINATE(j)) {
        for (int k = 0; k < n; ++k) {
            cell(k, j) = 1;
            ++ num_alive;
        }
    }
    if (IS_LOCAL_ABSCISSA(i) && IS_LOCAL_ORDINATE(j))
        -- num_alive;

    return num_alive;
}

static void (Write)(cgl_board *B, cgl_proc *P)
{
    int n = B->n;

    MPI_Send( &(cell(1, 1)), n, MPI_INT, P->prev_col, 0, P->line_comm);
    MPI_Send( &(cell(1, n)), n, MPI_INT, P->next_col, 0, P->line_comm);
    MPI_Send( &(cell(1, 1)), n, MPI_INT, P->prev_line, 0, P->col_comm);
    MPI_Send( &(cell(n, 1)), n, MPI_INT, P->next_line, 0, P->col_comm);

}

static void (CompleteWrite)(cgl_board *B, cgl_proc *P)
{
    MPI_Send( &(cell(0, 0)), n, MPI_INT, P->prev_col, 0, P->line_comm);
    MPI_Send( &(cell(0, 0)), n, MPI_INT, P->next_col, 0, P->line_comm);
    MPI_Send( &(cell(0, 0)), n, MPI_INT, P->prev_line, 0, P->col_comm);
    MPI_Send( &(cell(0, 0)), n, MPI_INT, P->next_line, 0, P->col_comm);
}

static void (Receive)(cgl_board *B, cgl_proc *P)
{
    int n = B->n;

    // left
    MPI_Recv( &(cell(1,   0)), n, MPI_INT, P->prev_col, 0, P->line_comm);

    //right
    MPI_Recv( &(cell(1, n+1)), n, MPI_INT, P->next_col, 0, P->line_comm);

    //top
    MPI_Recv( &(cell(0,   1)), n, MPI_INT, P->prev_line, 0, P->col_comm);

    //bot
    MPI_Recv( &(cell(n+1, 1)), n, MPI_INT, P->next_line, 0, P->col_comm);
}

static int
cgl_board_main_loop(cgl_board *B, cgl_proc *P, int maxloop)
{
    (void) P;

    int num_alive = 0;
    int board_size = B->n;

    (Write());

    for (int loop = 0; loop < maxloop; ++loop) {

        (CompleteWrite());
        (Receive());

        for (int j = 1; j <= board_size; j++) {
            for (int i = 1; i <= board_size; i++) {
                ngb( i, j ) =
                    cell( i-1, j-1 ) + cell( i, j-1 ) + cell( i+1, j-1 ) +
                    cell( i-1, j   ) +                  cell( i+1, j   ) +
                    cell( i-1, j+1 ) + cell( i, j+1 ) + cell( i+1, j+1 );
            }
        }

        (Write());

        MPI_Barrier(MPI_COMM_WORLD);
    }

    return num_alive;
}

void cgl_proc_init(cgl_proc *p)
{
    MPI_Comm_size(MPI_COMM_WORLD, &p->group_size);

    double Nd = sqrt(p->group_size);
    ASSERT_MSG(
        "processus must be perfect integer square",
        floor(Nd) == Nd
    );

    int N = p->group_length = (int)Nd;
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

    MPI_Cart_shift(p->line_comm, 0, 1, &p->prev_col, &p->next_col);
    MPI_Cart_shift(p->col_comm, 0, 1, &p->prev_line, &p->next_line);

}

static void
cgl_board_init(cgl_board *B, cgl_proc *P, int64_t board_size)
{
    ASSERT_MSG(
        "board size must be a multiple of group length",
        board_size % P->group_length == 0
    );
    int n = B->n = board_size / P->group_length;

    /* Leading dimension of the board array */
    B->ld_board = n + 2;
    /* Leading dimension of the neigbour counters array */
    B->ld_nb_neighbour = n;

    CALLOC_ARRAY(B->board, SQUARE(B->ld_board));
    MALLOC_ARRAY(B->nb_neighbour, SQUARE(B->ld_nb_neighbour));

    int num_alive = cgl_board_generate(B, P);

    printf("Starting number of living cells = %d\n", num_alive);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: %s Nb_Iterations Board_Size\n", argv[0]);
        return EXIT_SUCCESS;
    }

    cgl_proc P;
    cgl_board B;
    MPI_Init(NULL, NULL);
    cgl_proc_init(&P);

    int maxloop = atoi(argv[1]);
    int board_size = atoi(argv[2]);

    printf("Running MPI version, "
           "grid of size %d, %d iterations\n"
           "rank %d, group_size %d",
           board_size, maxloop, P.rank, P.group_size);

    cgl_board_init(&B, &P, board_size);

    double t1 = cgl_timer();

    cgl_board_main_loop(&B, &P, maxloop);
    double t2 = cgl_timer();

    double time = t2 - t1;
    printf("%gms\n", time * 1.e3);

    MPI_Finalize();
    return EXIT_SUCCESS;
}
