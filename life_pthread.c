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
#define ASSERT_MSG(msg, cond) assert(  ((void)(msg), (cond)) )

#define cell( _i_, _j_ ) board[ ld_board * (_j_) + (_i_) ]
#define ngb( _i_, _j_ )  nb_neighbour[ ld_nb_neighbour * ((_j_) - 1) + ((_i_) - 1 ) ]


typedef struct thread_info_ cgl_thread_info;
struct thread_info_ {
    pthread_mutex_t m;
    pthread_cond_t cond_left;
    pthread_cond_t cond_right;
    int read_left;
    int read_right;
    int num_alive;
};


int board_size, maxloop, nb_threads;
int ld_board, ld_nb_neighbour;
int *board, *nb_neighbour;
cgl_thread_info *thread_infos;

static double cgl_timer(void)
{
    struct timeval tp;
    gettimeofday( &tp, NULL );
    return tp.tv_sec + 1e-6 * tp.tv_usec;
}

static void check_board(int N, int *board, int ld_board)
{
    for (int j = 0; j < N; ++j)
        for (int i = 0; i < N; ++i)
            assert( (cell(i, j) == 1 || cell(i, j) == 0) );
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

static inline void read_border_extern_neighbour(int B, int C)
{
    for (int i = 1; i <= board_size; ++i)
        ngb(i, B) = cell(i-1, B-1) + cell(i, B-1) + cell(i+1, B-1);

    for (int i = 1; i <= board_size; ++i)
        ngb(i, C) = cell(i-1, C+1) + cell(i, C+1) + cell(i+1, C+1);
}

static inline void read_left_border_intern(int B)
{
    for (int i = 1; i <= board_size; ++i) {
        ngb(i, B) +=
            cell(i-1, B)   +                cell(i+1, B) +
            cell(i-1, B+1) + cell(i, B+1) + cell(i+1, B+1);
    }
}

static inline void read_right_border_intern(int C)
{
    for (int i = 1; i <= board_size; ++i) {
        ngb(i, C) +=
            cell(i-1, C-1) + cell(i, C-1) + cell(i+1, C-1) +
            cell(i-1, C)   +                cell(i+1, C);
    }
}

static inline void read_middle_intern(int B, int C)
{
    for (int j = B+1; j <= C-1; ++j) {
        for (int i = 1; i <= board_size; ++i) {
            ngb(i, j) =
                cell(i-1, j-1) + cell(i, j-1) + cell(i+1, j-1) +
                cell(i-1, j)   +                cell(i+1, j) +
                cell(i-1, j+1) + cell(i, j+1) + cell(i+1, j+1);
        }
    }
}

static inline void read_intern_neighbour(int B, int C)
{
    read_left_border_intern(B);
    read_middle_intern(B, C);
    read_right_border_intern(C);
}

static inline void compute_intern_cells(int B, int C, int *num_alive)
{
    for (int j = B+1; j <= C-1; ++j) {
        for (int i = 1; i <= board_size; ++i) {

            if ( (ngb( i, j ) < 2) || (ngb( i, j ) > 3) )
                cell(i, j) = 0;
            else if ((ngb(i, j)) == 3)
                cell(i, j) = 1;
            if (cell(i, j) == 1)
                ++(*num_alive);
        }
    }
}

static inline void compute_extern_cells(int B, int C, int *num_alive)
{
    {
        int j = B;
        for (int i = 1; i <= board_size; ++i) {
            if ( (ngb(i, j) < 2) || (ngb(i, j) > 3) )
                cell(i, j) = 0;
            else if ((ngb(i, j)) == 3)
                cell(i, j) = 1;
            if (cell(i, j) == 1)
                ++(*num_alive);
        }
    }

    {
        int j = C;
        for (int i = 1; i <= board_size; ++i) {
            if ( (ngb(i, j) < 2) || (ngb(i, j) > 3) )
                cell(i, j) = 0;
            else if ((ngb(i, j)) == 3)
                cell(i, j) = 1;
            if (cell(i, j) == 1)
                ++(*num_alive);
        }
    }
}

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

static void notify_read_to_neighbours(int rank, int loop)
{
    { // prev
        int prev_rank = rank-1<0 ? nb_threads-1 : rank-1;
        cgl_thread_info *ti = thread_infos+prev_rank;
        pthread_mutex_lock(&ti->m);

        assert(ti->read_right == loop-1);

        ti->read_right = loop;
        pthread_cond_signal(&ti->cond_right);
        pthread_mutex_unlock(&ti->m);
    }

    { // next
        int next_rank = (rank+1) % nb_threads;
        cgl_thread_info *ti = thread_infos+next_rank;
        pthread_mutex_lock(&ti->m);

        assert(ti->read_left == loop-1);

        ti->read_left = loop;
        pthread_cond_signal(&ti->cond_left);
        pthread_mutex_unlock(&ti->m);
    }
}

static void check_neighbours_read(int rank, int loop)
{
    cgl_thread_info *ti = thread_infos+rank;

    pthread_mutex_lock(&ti->m);

    while (ti->read_right != loop)
        pthread_cond_wait(&ti->cond_right, &ti->m);

    while (ti->read_left != loop)
        pthread_cond_wait(&ti->cond_left, &ti->m);

    pthread_mutex_unlock(&ti->m);
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

static inline void global_border_ghost_zone_recopy(void)
{
    cell(   0, 0   )                 = cell(board_size, board_size);
    cell(   0, board_size+1)         = cell(board_size,  1);
    cell(board_size+1, 0   )         = cell( 1, board_size);
    cell(board_size+1, board_size+1) = cell( 1,  1);

    for (int i = 1; i <= board_size; ++i) {
        cell(   i,    0)         = cell( i, board_size);
        cell(   i, board_size+1) = cell( i,  1);
        cell(   0,    i)         = cell(board_size,  i);
        cell(board_size+1,    i) = cell( 1,  i);
    }
}

static void *main_loop(void *args)
{
    int num_alive = 0;
    int rank = (uintptr_t)args;
    int block_size = board_size/nb_threads;
    int B = block_size*rank+1;
    int C = block_size*(rank+1);

    for (int loop = 0; loop < maxloop; ++loop) {
        if (rank == 0)
            global_border_ghost_zone_recopy();

        barrier_STOP(nb_threads);

        // 1 - Lire les cellules des threads voisins
        read_border_extern_neighbour(B, C);

        // 2 - Informer les voisins de la lecture
        notify_read_to_neighbours(rank, loop);

        // 3 - Calcul Interne
        read_intern_neighbour(B, C);
        num_alive = 0;
        compute_intern_cells(B, C, &num_alive);

        // 4 - Vérifier que les voisins ont lus les cellules aux bords
        check_neighbours_read(rank, loop);

        // 5 - Mise à jour aux bords
        compute_extern_cells(B, C, &num_alive);
        barrier_STOP(nb_threads);

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
    /* Leading dimension of the neigbour counters array */
    ld_nb_neighbour = board_size;

    MALLOC_ARRAY(board, SQUARE(ld_board));
    MALLOC_ARRAY(nb_neighbour, SQUARE(ld_nb_neighbour));

    int num_alive;
    num_alive = generate_initial_board(board_size, &(cell(1, 1)), ld_board);

    printf("Starting number of living cells = %d\n", num_alive);
    nb_threads = get_nprocs();
    ASSERT_MSG(
        "board size must be multiple of thread count",
        board_size % nb_threads == 0
    );

    pthread_t t[nb_threads];

    CALLOC_ARRAY(thread_infos, nb_threads);

    for (int i = 0; i < nb_threads; ++i) {
        cgl_thread_info *ti = thread_infos+i;
        pthread_mutex_init(&ti->m, NULL);
        pthread_cond_init(&ti->cond_left, NULL);
        pthread_cond_init(&ti->cond_right, NULL);
        ti->read_right = -1;
        ti->read_left = -1;
    }

    for (int i = 0; i < nb_threads; ++i)
        pthread_create(t+i, NULL, main_loop, (void*)(uintptr_t)i);

    num_alive = 0;
    for (int i = 0; i < nb_threads; ++i) {
        pthread_join(t[i], NULL);
        num_alive += thread_infos[i].num_alive;
    }
    printf("Final number of living cells = %d\n", num_alive);

    free(board);
    free(nb_neighbour);
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
