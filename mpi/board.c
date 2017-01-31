
#include "util.h"
#include "board.h"

static void output_board(cgl_board *B)
{
    int N = B->n;
    for (int i = 0; i <= N+1; ++i) {
        for (int j = 0; j <= N+1; ++j) {
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

#define GLOBAL_TO_LOCAL_LINE(i_) (((i_) - (P->line*B->n))+1)
#define GLOBAL_TO_LOCAL_COL(j_) (((j_) - (P->col*B->n))+1)
#define IS_LOCAL_LINE(i_) ((i_) >= 1 && (i_) <= B->n)
#define IS_LOCAL_COL(j_) IS_LOCAL_LINE(j_)

static int
cgl_board_generate(cgl_board *B, cgl_proc *P)
{
    int num_alive = 0;
    int n = B->n;
    int N = n * P->group_length;
    int i = GLOBAL_TO_LOCAL_LINE(N/2);
    int j = GLOBAL_TO_LOCAL_COL(N/2);

    if (IS_LOCAL_LINE(i)) {
        //printf("bla rank=%d i=%d N/2=%d col=%d line=%d\n", P->rank, i, N/2, P->col, P->line);
        for (int k = 1; k <= n; ++k) {
            cell(i, k) = 1;
            ++ num_alive;
        }
    }
    if (IS_LOCAL_COL(j)) {
        //printf("ble rank=%d j=%d N/2=%d col=%d line=%d\n", P->rank, j, N/2, P->col, P->line);
        for (int k = 1; k <= n; ++k) {
            cell(k, j) = 1;
            ++ num_alive;
        }
    }
    if (IS_LOCAL_LINE(i) && IS_LOCAL_COL(j))
        -- num_alive;

    return num_alive;
}

static MPI_Datatype line_type;

void cgl_board_setup_mpi_type(cgl_board *B)
{
    static bool ran_once = false;

    if (ran_once)
        return;
    ran_once = true;

    MPI_Type_vector(B->ld_board, 1, B->ld_board, CGL_BOARD_MPI_TYPE, &line_type);
    MPI_Type_commit(&line_type);
}

void cgl_board_exchange_right_left(cgl_board *B, cgl_proc *P)
{
    MPI_Request r[4] = {
        MPI_REQUEST_NULL, MPI_REQUEST_NULL,
        MPI_REQUEST_NULL, MPI_REQUEST_NULL };
    MPI_Status st[4];
    int n = B->n;

    //right
    MPI_Isend( &(cell(1, n)), n, CGL_BOARD_MPI_TYPE,
               P->next_col, 2, P->line_comm, r);
    MPI_Irecv( &(cell(1, n+1)), n, CGL_BOARD_MPI_TYPE,
               P->next_col, 3, P->line_comm, r+1);

    // left
    MPI_Isend( &(cell(1, 1)), n, CGL_BOARD_MPI_TYPE,
               P->prev_col, 3, P->line_comm, r+2);
    MPI_Irecv( &(cell(1, 0)), n, CGL_BOARD_MPI_TYPE,
               P->prev_col, 2, P->line_comm, r+3);

    MPI_Waitall(4, r, st);
}

void cgl_board_exchange_top_bot(cgl_board *B, cgl_proc *P)
{
    MPI_Request r[4] = {
        MPI_REQUEST_NULL, MPI_REQUEST_NULL,
        MPI_REQUEST_NULL, MPI_REQUEST_NULL };
    MPI_Status st[4];
    int n = B->n;

    //top
    MPI_Isend( &(cell(1, 0)), 1, line_type,
               P->prev_line, 0, P->col_comm, r);
    MPI_Irecv( &(cell(0,   0)), 1, line_type,
               P->prev_line, 1, P->col_comm, r+1);

    // bot
    MPI_Isend( &(cell(n, 0)), 1, line_type,
               P->next_line, 1, P->col_comm, r+2);
    MPI_Irecv( &(cell(n+1, 0)), 1, line_type,
               P->next_line, 0, P->col_comm, r+3);

    MPI_Waitall(4, r, st);
}

void cgl_board_distributed_output(cgl_board *B, cgl_proc *P, int loop)
{
    MPI_Barrier(MPI_COMM_WORLD);
    if (P->rank == 0)
        printf("\n_______________________________\nloop %d\n", loop);
    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < P->group_size; ++i) {
        MPI_Barrier(MPI_COMM_WORLD);
        if (P->rank == i) {
            printf("[(%d, %d), %d]\n", P->line, P->col, loop);
            output_board(B);
            puts("");
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

int cgl_board_main_loop(cgl_board *B, cgl_proc *P, int maxloop)
{
    int num_alive = 0;
    int board_size = B->n;

    //cgl_board_distributed_output(B, P, -1);

    for (int loop = 0; loop < maxloop; ++loop)
    {
        cgl_board_exchange_right_left(B, P);
        cgl_board_exchange_top_bot(B, P);
        //cgl_board_distributed_output(B, P, loop);

        for (int j = 1; j <= board_size; j++) {
            for (int i = 1; i <= board_size; i++) {
                ngb( i, j ) =
                    cell( i-1, j-1 ) + cell( i, j-1 ) + cell( i+1, j-1 ) +
                    cell( i-1, j   ) +                  cell( i+1, j   ) +
                    cell( i-1, j+1 ) + cell( i, j+1 ) + cell( i+1, j+1 );
            }
        }

	num_alive = 0;
	for (int j = 1; j <= board_size; j++) {
	    for (int i = 1; i <= board_size; i++) {
		if ( (ngb( i, j ) < 2) ||(ngb( i, j ) > 3) )
		    cell(i, j) = 0;
                else if ((ngb( i, j )) == 3)
                    cell(i, j) = 1;
		if (cell(i, j) == 1)
		    num_alive ++;
	    }
	}
    }
    //cgl_board_distributed_output(B, P, -2);

    return num_alive;
}

void cgl_board_init(cgl_board *B, cgl_proc *P, int64_t board_size)
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
    //printf("Starting number of living cells = %d\n", num_alive);

    cgl_board_setup_mpi_type(B);
}

void cgl_board_fini(cgl_board *B)
{
    free(B->board);
    free(B->nb_neighbour);
    memset(B, 0, sizeof*B);
}
