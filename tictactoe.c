#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#ifdef __unix__
#include <unistd.h>
#elif defined _WIN32
# include <windows.h>
#define sleep(x) Sleep(1000 * x)
#endif

#define N 2

typedef struct thread
{
    pthread_t thread_id;
    int thread_num, id;
    int copy_MOVES;
    char symbol;
    char name[15];
    int condition;
} ThreadData; //Struct used to hold thread data

pthread_mutex_t game_mutex;

struct shared_data
{
    int CURRENT_TURN;
    int TOTAL_MOVES;
    int GAME_OVER;
    char BOARD[3][3];
    char WINNER[15];
} SHARED; //Struct to hold shared data

void printBoard()
{
    printf("\n");
    //Prints out 3x3 board to console
    for(int i = 0; i < 3; i++)
    {
        printf(" %c | %c | %c \n", SHARED.BOARD[i][0], SHARED.BOARD[i][1], SHARED.BOARD[i][2]);
        if(i < 2)
        	{
        	printf("-----------\n");
        	}
    }
    printf("\n");
    fflush(stdout); //Used to flush the output buffer
} //End printBoard

int checkWinner()
{
	//For loop checks for a winning condition across rows and columns
    for(int i = 0; i < 3; i++)
    {
        if(SHARED.BOARD[i][0] != ' ' && SHARED.BOARD[i][0] == SHARED.BOARD[i][1] && SHARED.BOARD[i][1] == SHARED.BOARD[i][2])
        	{
        	return 1;
        	}
        if(SHARED.BOARD[0][i] != ' ' && SHARED.BOARD[0][i] == SHARED.BOARD[1][i] && SHARED.BOARD[1][i] == SHARED.BOARD[2][i])
        	{
        	return 1;
        	}
    }
    //If statements check for a winning condition across diagonals
    if(SHARED.BOARD[0][0] != ' ' && SHARED.BOARD[0][0] == SHARED.BOARD[1][1] && SHARED.BOARD[1][1] == SHARED.BOARD[2][2])
    	{
    	return 1;
    	}
    if(SHARED.BOARD[0][2] != ' ' && SHARED.BOARD[0][2] == SHARED.BOARD[1][1] && SHARED.BOARD[1][1] == SHARED.BOARD[2][0])
    	{
    	return 1;
    	}
    return 0;
} //End checkWinner

int getRandom(int rangeLow, int rangeHigh) {
	unsigned int seed = time(NULL) ^ pthread_self(); //Used to initialize a random seed value by using the current time in seconds XOR the unique identifier of the calling thread
	srand(seed); //Initializes a random number using the variable seed
	return (rand() % (rangeHigh - rangeLow + 1)) + rangeLow; //Returns a number between rangeLow and rangeHigh

} //End getRandom

void makeMove(ThreadData *player)
{
    pthread_mutex_lock(&game_mutex);

    //Move if it is the player's turn and the game is not over
    if(SHARED.CURRENT_TURN == player->condition && !SHARED.GAME_OVER)
    {
        int valid_move = 0;
        //Loop until a valid move is made
        while(!valid_move)
        {
            int move = getRandom(1, 9);
            int row = (move-1) / 3;
            int col = (move-1) % 3;

            //Check if move is valid
            if(SHARED.BOARD[row][col] == ' ')
            {
                SHARED.BOARD[row][col] = player->symbol;
                valid_move = 1;
                printf("%s placed %c at position %d\n", player->name, player->symbol, move);
                printBoard();
                ++SHARED.TOTAL_MOVES;

                //Checks if checkWinner returns true and signals that the game is over
                if(checkWinner())
                {
                    SHARED.GAME_OVER = 1;
                    strcpy(SHARED.WINNER, player->name);
                }
                //Checks TOTAL_MOVES to see if a draw has occurred
                else if(SHARED.TOTAL_MOVES >= 9)
                {
                    SHARED.GAME_OVER = 1;
                    strcpy(SHARED.WINNER, "Draw");
                }
                //Switch CURRENT_TURN to the next player
                if(!SHARED.GAME_OVER)
                {
                	SHARED.CURRENT_TURN = (SHARED.CURRENT_TURN + 1) % 2;
                }
            }
        }
        player->copy_MOVES = SHARED.TOTAL_MOVES;
    }

    pthread_mutex_unlock(&game_mutex);
} //End makeMove


void *gameAPI(void *thread)
{
	//Generic pointer in signature is cast back to ThreadData
    ThreadData *player = (ThreadData*)thread;

    //While loop continues until game is over
    while(!SHARED.GAME_OVER)
    {
        makeMove(player);
        //Sleep used to pause for 1 second between moves
        sleep(1);
    }

    return NULL;
} //End gameAPI

void initData(ThreadData *players)
{
    memset(SHARED.BOARD, ' ', sizeof(SHARED.BOARD)); //Fill the 3x3 board with empty spaces
    //Initialize variables to start conditions
    SHARED.TOTAL_MOVES = 0;
    SHARED.GAME_OVER = 0;

    //Determine which player starts the game
    SHARED.CURRENT_TURN = getRandom(0, 1);

    //Print to console the starting player
    printf("Starting player is: ");
    if (SHARED.CURRENT_TURN == 0)
    {
        printf("X\n");
    }
    else
    {
        printf("O\n");
    }

    players[0].condition = 0;
    players[0].id = 0;
    players[0].symbol = 'X';
    players[0].copy_MOVES = 0;
    strcpy(players[0].name, "PlayerX");

    players[1].condition = 1;
    players[1].id = 1;
    players[1].symbol = 'O';
    players[1].copy_MOVES = 0;
    strcpy(players[1].name, "PlayerO");
} //End initData

int main()
{
    ThreadData players[N];
    initData(players);
    printBoard();

    pthread_mutex_init(&game_mutex, NULL);

    // Create threads by calling pthread_create
    for(int i = 0; i < N; i++)
    {
        players[i].thread_num = i;
        pthread_create(&(players[i].thread_id), NULL, gameAPI, (void*)(&players[i]));
    }

    // Call pthread_join to force main to wait for threads to finish
    for(int i = 0; i < N; i++)
    {
        pthread_join(players[i].thread_id, NULL);
    }

    //Prints the results of the game
    printf("Game Over! ");
    if (SHARED.GAME_OVER)
    {
        if (strcmp(SHARED.WINNER, "Draw") == 0)
        {
            printf("Draw!\n");
        }
        else
        {
            printf("Winner is: %s\n", SHARED.WINNER);
        }
    }

    pthread_mutex_destroy(&game_mutex);
    return 0;
}
