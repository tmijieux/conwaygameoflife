#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define SQUARE(x)  ((x)*(x))
#define SQUARE_UL(X) SQUARE(((uint64_t) (X)))

#define MALLOC_ARRAY(var, size) ((var) = malloc((size)*sizeof(*(var))))
#define CALLOC_ARRAY(var, size) ((var) = calloc((size), sizeof(*(var))))
#define SWAP_POINTER(var_a_, var_b_) do {       \
        void *_tmp_ptr_macroXX_ = (var_a_);     \
        var_a_ = var_b_;                        \
        var_b_ = _tmp_ptr_macroXX_;             \
    } while(0)
#define ASSERT_MSG(msg, cond) assert(  ((void)(msg), (cond)) )

#define cell( _i_, _j_ ) board[ ld_board * (_j_) + (_i_) ]
#define ncell( _i_, _j_ ) next_board[ ld_board * (_j_) + (_i_) ]


#define s_cell( _i_, _j_ ) s_board[ ld_board * (_j_) + (_i_) ]
#define s_ncell( _i_, _j_ ) s_next_board[ ld_board * (_j_) + (_i_) ]

typedef struct thread_info_ cgl_thread_info;
struct thread_info_ {
    int num_alive;
};


int board_size, maxloop, nb_threads, ld_board;
int *s_board, *s_next_board;
cgl_thread_info *thread_infos;

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

#define compute_cells() do {                                            \
        for (int j = B; j <= C; j++) {                                  \
            for (int i = 1; i <= board_size; i++) {                     \
                int ngb =                                               \
                    cell(i-1, j-1) + cell(i, j-1) + cell(i+1, j-1) +    \
                    cell(i-1, j  ) +                cell(i+1, j  ) +    \
                    cell(i-1, j+1) + cell(i, j+1) + cell(i+1, j+1);     \
                                                                        \
                int life = cell(i,j);                                   \
                if ( ngb < 2 || ngb > 3 )                               \
                    life = 0;                                           \
                else if ( ngb == 3 )                                    \
                    life = 1;                                           \
                                                                        \
                if (life == 1)                                          \
                    ++num_alive;                                        \
                                                                        \
                ncell(i, j) = life;                                     \
            }                                                           \
        }                                                               \
    } while(0)                                                          \


static void barrier_STOP(int nb_threads)
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

static inline void cgl_main_loop_debug_output(
    int loop, int rank, int num_alive, int B, int C, int block_size)
{
    barrier_STOP(nb_threads);
    if(!rank) printf("iterations %d\n", loop);
    barrier_STOP(nb_threads);
    printf("%d cells are alive (rank %d)\n", num_alive, rank);
    barrier_STOP(nb_threads);
    printf("B=%d, C=%d rank=%d, block_size=%d\n",
           B, C, rank, block_size);
    barrier_STOP(nb_threads);
    if(!rank) puts("");
    barrier_STOP(nb_threads);

/* Avec les cellules sur les bords
   (utile pour vérifier les comm MPI) */
/* output_board( board_size+2, &(cell(0, 0)), ld_board, loop ); */

/* Avec juste les "vraies" cellules: on commence à l'élément (1,1) */
// output_board( board_size, &(cell(1, 1)), ld_board, loop);
// printf("%d cells are alive\n", num_alive);
}

#define global_border_ghost_zone_recopy()                               \
    do {                                                                \
        cell(   0, 0   )                 = cell(board_size, board_size); \
        cell(   0, board_size+1)         = cell(board_size,  1);        \
        cell(board_size+1, 0   )         = cell( 1, board_size);        \
        cell(board_size+1, board_size+1) = cell( 1,  1);                \
        for (int i = 1; i <= board_size; ++i) {                         \
            cell(   i,    0)         = cell( i, board_size);            \
            cell(   i, board_size+1) = cell( i,  1);                    \
            cell(   0,    i)         = cell(board_size,  i);            \
            cell(board_size+1,    i) = cell( 1,  i);                    \
        }                                                               \
    } while(0)                                                          \

static void *main_loop(void *args)
{
    int num_alive = 0;
    int rank = (uintptr_t)args;
    int block_size = board_size/nb_threads;
    int B = block_size*rank+1;
    int C = block_size*(rank+1);
    int *board = s_board;
    int *next_board = s_next_board;

    for (int loop = 0; loop < maxloop; ++loop) {
        if (rank == 0)
            global_border_ghost_zone_recopy();

        barrier_STOP(nb_threads);

        num_alive = 0;
        compute_cells();

        barrier_STOP(nb_threads);
            SWAP_POINTER(board, next_board);

        #ifdef CGL_DEBUG
        cgl_main_loop_debug_output(loop, rank, num_alive, B, C, block_size);
        #endif
    }

    thread_infos[rank].num_alive = num_alive;
    return NULL;
}

static void game_of_life(void)
{
    /* Leading dimension of the board array */
    ld_board = board_size + 2;

    MALLOC_ARRAY(s_board, SQUARE(ld_board));
    MALLOC_ARRAY(s_next_board, SQUARE(ld_board));

    int num_alive;
    num_alive = generate_initial_board(board_size, &(s_cell(1, 1)), ld_board);

    printf("Starting number of living cells = %d\n", num_alive);
    nb_threads = get_nprocs();
    ASSERT_MSG(
        "board size must be multiple of thread count",
        board_size % nb_threads == 0
    );

    pthread_t t[nb_threads];
    CALLOC_ARRAY(thread_infos, nb_threads);

    for (int i = 0; i < nb_threads; ++i)
        pthread_create(t+i, NULL, main_loop, (void*)(uintptr_t)i);

    num_alive = 0;
    for (int i = 0; i < nb_threads; ++i) {
        pthread_join(t[i], NULL);
        num_alive += thread_infos[i].num_alive;
    }
    printf("Final number of living cells = %d\n", num_alive);

    free(s_board);
    free(s_next_board);
    free(thread_infos);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: %s Nb_Iterations Board_Size\n", argv[0]);
        return EXIT_SUCCESS;
    }
    maxloop = atoi(argv[1]);
    board_size = atoi(argv[2]);

    printf("Running PTHREAD version, "
           "grid of size %d, %d iterations\n", board_size, maxloop);

    double t1 = cgl_timer();
    game_of_life();
    double t2 = cgl_timer();

    double time = t2 - t1;
    printf("%gms\n", time * 1.e3);

    return EXIT_SUCCESS;
}
