#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define N_pcs   6 // number of pieces
#define EP_FLAG 99
#define PAWN    1 // piece ids
#define KNIGHT  2
#define BISHOP  3
#define ROOK    4
#define QUEEN   5
#define KING    6


typedef int** (*Piece_f)(int**, int*, Player);
typedef struct Player_
{
    int id; // = {-1, 1}
    int canCastle; // {-1, 0, 1, 2}->(Qside only, no, Kside only, both)
} Player;


int play_game(int[8][8], int[2]);
int** make_move(int**, int**, Player*);
int is_legal(int**, int**, Player);
int is_hit(int**, int*, Player);
int can_move(int**, Player);

// piece-specific functions
int** P__(int**, int*, Player);
int** N__(int**, int*, Player);
int** B__(int**, int*, Player);
int** R__(int**, int*, Player);
int** K__(int**, int*, Player);
int** Q__(int**, int*, Player);
// QOL fns for the above fns^^
int** malloc_from_tmp(int[][3], int);
void trace_dydx(int**, int[][3], int*, int, int, Player, int*);

// utility functions
int* find(int**, int, Player);
int signum(int);
// handling boards
int** cpy_board(int**);
void free_board(int**);
// IO
void print_board(int**, Player);
int** read_move(char*); // input -> move

// array of piece-specific functions
Piece_f list_moves[N_pcs] = {P__,N__,B__,R__,Q__,K__};

int main(void)
{
    int result;
    int canCastle[2] = {2,2};

    int B[8][8] =
    // test starting position
    {
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 6, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 1, 0},
        { 0,-6, 0, 0, 0, 0, 0, 0}
    };
    // 1=P, 2=N, 3=B, 4=R, 5=Q, 6=K
    // "classic" starting position
    // {
    //     { 4, 2, 3, 6, 5, 3, 2, 4},
    //     { 1, 1, 1, 1, 1, 1, 1, 1},
    //     { 0, 0, 0, 0, 0, 0, 0, 0},
    //     { 0, 0, 0, 0, 0, 0, 0, 0},
    //     { 0, 0, 0, 0, 0, 0, 0, 0},
    //     { 0, 0, 0, 0, 0, 0, 0, 0},
    //     {-1,-1,-1,-1,-1,-1,-1,-1},
    //     {-4,-2,-3,-6,-5,-3,-2,-4}
    // };

    // play the game
    result = play_game(B, canCastle);

    // print results
    for(int i=0; i<=35; i++) putchar('*');
    if(result) printf("\nCHECKMATE! Player %d wins!\n", result);
    else printf("\nSTALEMATE! It's a draw!\n");

    return 0;
}

// core functions

// play a game and return the winner, or 0 if a draw.
int play_game(int start[8][8], int canCastle[2])
{
    char input[100]; // input buffer

    // copy board position
    int** B = malloc(sizeof(int*)*8);
    for(int i=0; i<8; i++)
    {
        B[i] = malloc(sizeof(int)*8); // make space
        for(int j=0; j<8; j++) B[i][j] = start[i][j]; // populate
    }

    // create players
    
    Player player[2];
    for(int i=0; i<2; i++)
    {
        player[i].id = 1-2*i;
        player[i].canCastle = canCastle[i];
    }

    // begin game
    for(int i=0; 1; i=!i)
    {
        print_board(B, player[i]);

        // check for mate at the start of every move.
        if(!can_move(B, player[i])) // if player has no valid moves
        {
            int* pos = find(B, KING, player[i]); // find the king

            int isCheck = is_hit(B, pos, player[i]); // is he in check?
            free(pos); // (done with this)

            return isCheck * -player[i].id; // return {-1, 0, 1}
        }
        else
        {
            // otherwise, the game goes on.
            printf("\nPlayer %d's turn:\n\t> ",player[i].id);
            // get move ("g1f3 instead of Nf3" - PGN is HARD to program)
            fgets(input, 100, stdin);
            int** m = read_move(input); // translate input; move is in m

            if(is_legal(B, m, player[i]))
            {
                B = make_move(B, m, &player[i]);
                //remove opponent's en passant flag if it exists
                int* pos = find(B, EP_FLAG, player[!i]);
                if(pos)
                {
                    B[pos[0]][pos[1]] = 0;
                    free(pos);
                }
            } else
            {
                printf("\n (!) Illegal move! Try again.\n");
                i=!i;
            }
            free(m);
        }
    }
}

// plays a move on the board & handles special moves
int** make_move(int** B, int** m, Player* player)
{
    // helpful shortcuts
    int y0 = m[0][0],   y1 = m[1][0];
    int x0 = m[0][1],   x1 = m[1][1];
    int dy = y1 - y0,   dx = x1 - x0;
    int piece = B[y0][x0];
    //printf("piece = %d\n", piece);
    // special cases go here. (en passant, castling)
    switch(abs(piece))
    {
        case KING: // castling
        {
            if(!dy && abs(dx)==2)
            {
                // take care of the rook
                B[y0][(int)(4+(3.5f*signum(dx)))] = 0;
                B[y0][x0+signum(dx)] = ROOK*player->id;

                player->canCastle = 0;
            }
        } break;
        case ROOK: // more castling
        {
            player->canCastle = !!(player->canCastle) * -signum(x0-4);
        } break;
        case PAWN:
        {
            // enable en passant
            if(!dx && dy==2*player->id)
                B[y0+player->id][x0] = player->id*EP_FLAG;

            // enable pawn promotions
            else if(y1==(int)(4+(3.5*player->id)))
                B[y0][x0] = QUEEN*player->id;
        } break;
    }
    // perform desired move
    B[y1][x1] = B[y0][x0];
    B[y0][x0] = 0;

    return B;
}

// checks validity of a move (Da Rules)
int is_legal(int** B, int** move, Player player)
{

    // helpful shortcuts
    int y0  = move[0][0], y1 = move[1][0];
    int x0  = move[0][1], x1 = move[1][1];
    int dy  = y1 - y0,    dx = x1 - x0;
    int piece = B[y0][x0];

    int isClear = 0;
    int isCheck = 1;

    // a player can only move their pieces
    if(signum(piece) != player.id) return 0;

    // castling logic
    if(abs(piece)==KING && abs(dx)==2 && !dy)
    {
        // does the player have castling privileges?
        if(player.canCastle==2 || player.canCastle==dx/2)
        {
            // check if squares are empty
            for(int i=y0; i>=1 && i<7; i+=dx/2)
                if(B[y0][i]) return 0;

            // check if king is in / moves through check
            int* pos_K = malloc(sizeof(int)*2);
            for(int i=0; i<=2; i++)
            {
                pos_K[0] = y0;
                pos_K[1] = x0 + i*(dx/2);
                // see if this square is attacked
                if(is_hit(B, pos_K, player))
                {
                    free(pos_K);
                    return 0;
                }
            }
            // if the above tests are passed, the player may castle.
            return 1;
        }
    }
    // normal move logic
    else
    {
        // 1. IS THE SQUARE AVAILABLE FOR THIS PIECE?
        // generate list of available squares
        int** sqrs = list_moves[abs(piece)-1](B, move[0], player);

        // if there are no valid moves w this piece, you're done
        if(!sqrs) return 0;

        // iterate through list
        for(int i=1; i<=**sqrs; i++)
        {
            // if selected element square is desired square,
            if(y1==sqrs[i][0] && x1==sqrs[i][1])
            {
                // free remaining sqrs
                for(; i<=**sqrs; i++) free(sqrs[i]);

                isClear = 1;
                break;
            }
            // otherwise, free this element
            free(sqrs[i]);
        }
        // if that square wasn't in the list, it can't be a legal move
        if(!isClear) return 0;

        // 2. DOES THIS MOVE RESULT IN CHECK FOR THE PLAYER WHO PLAYED IT?
        // copy the board and make the move on this copy.
        int** H = make_move(cpy_board(B), move, &player);

        // locate the king.
        int* pos_K = find(H, KING, player);

        // (if you found the king,) see if he is in check.
        if(pos_K) isCheck = is_hit(H, pos_K, player);

        // free the copied board and the king's position
        free(pos_K); free_board(H);

        return !isCheck;
    }
    return 0;
}

// checks for check (or potential check) of player
int is_hit(int** B, int* pos, Player player)
{
    int x, y;
    int** sqrs;

    // check all potential attacker squares (max = 36)
    for(int i=0; i<N_pcs; i++)
    {
        // list of squares where this piece could be used to attack (pos)
        int** sqrs = list_moves[i](B, pos, player);

        // if there are no squares, go to next piece type
        if(!sqrs) continue;

        // iterate through squares
        for(int j=1; j<=**sqrs; j++)
        {
            y = sqrs[j][0], x = sqrs[j][1];

            if( B[y][x]==(i+1)*(-player.id)  // is it the right piece type?
                && sqrs[j][2]              ) // is this a capture?
            {
                for(; j<=**sqrs; j++) free(sqrs[j]);
                free(sqrs); // first free this

                return 1;
            }
            free(sqrs[j]);
        }
        free(sqrs);
    }
    return 0;
}

// checks for stalemate / checkmate
int can_move(int** B, Player player)
{
    //printf("can_move(player %d)\n",player.id);
    int piece;
    int** sqrs; // will store list of valid moves

    int** move = malloc(sizeof(int*)*2); // for list_moves and is_legal
    move[0] = malloc(sizeof(int)*2); // second position comes from sqrs[]

    for(int i=0; i<8; i++) // go through entire board
    for(int j=0; j<8; j++)
    {
        piece = B[i][j];

        if(signum(piece) == player.id) // if piece belongs to player
        {
            // friendly piece was found at (i,j) moves from (i,j)
            move[0][0] = i, move[0][1] = j;
            // list of candidate moves from (i,j)
            sqrs = list_moves[abs(piece)-1](B,move[0],player);

            // if there are no candidate moves, onto the next square.
            if(!sqrs) continue;

            // otherwise search for valid move in candidate moves.
            for(int k=1; k<=**sqrs; k++)
            {
                // get destination from sqrs
                move[1] = sqrs[k];
                if(is_legal(B, move, player)) // if the move is legal
                {
                    for(;k<=**sqrs;k++) free(sqrs[k]); // free candidate moves
                    free(sqrs); // pointer to candidate moves
                    free(move[0]); free(move);

                    return 1;
                }
                free(sqrs[k]); // free illegal moves as you go
            }
        }
    }
    free(move[0]); free(move);
    free(sqrs);

    return 0;
}

/*
PIECE MOVEMENT FUNCTIONS

Each generates a list of all squares a piece at position 'pos' can reach given board state 'B' (REGARDLESS of whether there is actually that piece at pos). This list is EQUIVALENT to the list of all squares from which the same piece (of the opposite color) could reach position p:

"To where could a white bishop on e4 move?"
    ...is equivalent to...
"From where could a black bishop move _to_ e4?"

This equivalence is very useful as it allows calculating potential checks, legal moves, etc for each piece using just one function.

return structure = {n, {y_1, x_1, isCapture_1},...,{y_n, x_n, isCapture_n}}
*/
int** P__(int** B, int* pos, Player player)
{

    // an unmoved pawn w/ no obstructions & two pieces to take has 4 moves
    int tmp[100][3];
    int y, x, count = 0;

    // if nothing's directly in front of the pawn
    y = pos[0]+player.id;
    x = pos[1];
    if(!B[y][x])
    {
        // it can move forward 1
        tmp[count][0] = y;
        tmp[count][1] = x;
        tmp[count][2] = 0; // is never a capture
        count++;

        y = pos[0]+2*player.id;
        if( pos[0] == (int)(4-player.id*2.5f) // if the pawn is at home
            && !B[y][x] )                // if nothing is 2 squares ahead
        {
            // it can move forward 2
            tmp[count][0] = y;
            tmp[count][1] = x;
            tmp[count][2] = 0; // is never a capture
            count++;
        }
    }
    int dx;
    for(int i=0; i<2; i++)
    {
        dx = 1-2*i;
        // check if it's possible to capture in this direction
        if(signum(B[pos[0]+player.id][pos[1]+dx]) == -player.id)
        {
            tmp[count][0] = pos[0]+player.id;
            tmp[count][1] = pos[1]+dx;
            tmp[count][2] = 1; // is always a capture
            count++;
        }
    }
    return malloc_from_tmp(tmp, count);
}

int** N__(int** B, int* pos, Player player)
{
    int tmp[100][3] = // initialize to all possible {dx, dy} pairs
    {               // will later hold final coordinates
        {-2,-1, 0}, {-2, 1, 0}, { 2,-1, 0}, { 2, 1, 0},
        {-1,-2, 0}, {-1, 2, 0}, { 1,-2, 0}, { 1, 2, 0}
    };

    int x,y, count = 0;
    for(int i=0; i<8; i++)
    {
        y=pos[0]+tmp[i][0], x=pos[1]+tmp[i][1];

        if( y>=0 && y<8 && // within bounds
            x>=0 && x<8 && // ^^^
            signum(B[y][x]) != player.id // not occupied by friendly piece
        ){
            tmp[count][0] = y, tmp[count][1] = x;
            tmp[count][2] = !!B[y][x];
            count++;
        }
    }
    return malloc_from_tmp(tmp, count);
}

int** B__(int** B, int* pos, Player player)
{
    // a bishop on an open board can reach 13 squares from the center
    int tmp[100][3];
    int dy, dx, count = 0;


    for(int a=0; a<2; a++)
    for(int b=0; b<2; b++)
    {
        dy = 1-2*a;
        dx = 1-2*b;
        //printf("dy=%d dx=%d\n",dy,dx);
        trace_dydx(B, tmp, pos, dy, dx, player, &count);
    }
    return malloc_from_tmp(tmp, count);
}

int** R__(int** B, int* pos, Player player)
{
    // a rook on an open board can always reach 14 squares
    int tmp[100][3];
    int dy, dx, count = 0;

    for(int a=0; a<2; a++)
    for(int b=0; b<2; b++)
    {
        dy =  a*(1-2*b);
        dx = !a*(1-2*b);
        trace_dydx(B, tmp, pos, dy, dx, player, &count);
    }
    return malloc_from_tmp(tmp, count);
}

int** Q__(int** B, int* pos, Player player)
{
    // a queen on an open board can reach 27 squares from the center
    int tmp[100][3];
    int dy, dx, count = 0;

    for(int dy=-1; dy<2; dy++)
    for(int dx=-1; dx<2; dx++)
    {
        if(!dy && !dx) continue;

        trace_dydx(B, tmp, pos, dy, dx, player, &count);
    }
    return malloc_from_tmp(tmp, count);
}

int** K__(int** B, int* pos, Player player)
{
    // a king on an open board can reach 8 squares anywhere but the edge
    int tmp[100][3];
    int y, x;
    int piece;
    int count = 0;

    //printf("K__((%d,%d),player %d)\n",pos[0],pos[1],player.id);
    for(int dy=-1; dy<=1; dy++)
    for(int dx=-1; dx<=1; dx++)
    {
        if(dy || dx)
        {
            y = pos[0] + dy;
            x = pos[1] + dx;
            if(y>=0 && y<8 && x>=0 && x<8)
            {
                piece = B[y][x];
                if(abs(piece)==EP_FLAG || signum(piece)!=player.id)
                {
                    tmp[count][0] = y;
                    tmp[count][1] = x;
                    tmp[count][2] = signum(piece) == -player.id \
                                    && abs(piece) != EP_FLAG;

                    count++;
                }
            }
        }
    }
    malloc_from_tmp(tmp, count);
}

// UTILITY FUNCTIONS

void trace_dydx(int** B, int tmp[][3],
                int* pos, int dy, int dx,
                Player player, int* count )
{
    //printf("trace_dydx: dy=%d, dx=%d\n", dy, dx);
    // go as far as possible in direction pointed to by {dy,dx}
    int y, x;
    for(int i=1; 1; i++)
    {
        y = pos[0] + dy*i, x = pos[1] + dx*i;

        if(y<0 || y>=8 || x<0 || x>=8) return;

        if(abs(B[y][x]) != EP_FLAG && signum(B[y][x]) != player.id)
        {
            tmp[*count][0] = y;
            tmp[*count][1] = x;
            tmp[*count][2] = B[y][x] && abs(B[y][x])!=EP_FLAG; //capture?

            (*count)++;
        }
        if(B[y][x] && abs(B[y][x]) != EP_FLAG) return;
    }
}

int** malloc_from_tmp(int tmp[][3], int count)
{
    if(!count) return NULL;

    int** moves = malloc((count+1)*sizeof(int*)); // create return pointer
    // |count|{m_1},...,{m_count}|
    *moves = malloc(sizeof(int));
    **moves = count; // first element = number of elements

    for(int i=0; i<count; i++)
    {
        moves[i+1] = malloc(3*sizeof(int)); // create space
        for(int j=0; j<3; j++)
        {
            moves[i+1][j] = tmp[i][j]; // populate from tmp
        }
    }
    return moves;
}

int** read_move(char* str)
{
    //if(strlen(str)!=4) return NULL;
    const char abcs[8] = "hgfedcba";
    const char nums[8] = "12345678";

    int** r = malloc(sizeof(int*)*2); // return array
    for(int i=0;i<2;i++) r[i] = malloc(sizeof(int)*2);

    char* c;
    for(int i=0; i<4; i++)
    {
        if(c = strchr(abcs, str[i])) // if youre using abc notation...
        r[i/2][1-i%2] = (int)(c-abcs);

        else if(c = strchr(nums, str[i])) // number notation...
        r[i/2][1-i%2] = (int)(c-nums);

        else return NULL;
    }
    return r;
}

int** cpy_board(int** B)
{
    // create return pointer
    int** C = malloc(sizeof(int*)*8);

    for(int i=0; i<8; i++)
    {
        // create space
        C[i] = malloc(sizeof(int)*8);
        // copy all values
        for(int j=0; j<8; j++) C[i][j] = B[i][j];
    }
    return C;
}

int* find(int** B, int piece, Player player)
{
    int* pos = malloc(sizeof(int)*2);

    for(int i=0; i<8; i++)
    for(int j=0; j<8; j++)
    {
        if(B[i][j] == piece*player.id)
        {
            pos[0] = i, pos[1] = j;
            return pos;
        }
    }
    return NULL;
}

void print_board(int** B, Player player)
{
    char out[5];
    for(int i=0; i<8; i++)
    {
        printf("\n\t%d |",i+1); // label 123
        for(int j=0; j<8; j++)
        {
            int v = B[i][j];
            sprintf(out,"%d",v);
            printf( "%2s ",
                    abs(v)==EP_FLAG || !v
                        ? i%2 == j%2 ? "~" : ":"    // empty squares
                        : out                       // pieces
            );
        }
    }
    printf("\n\n\t    ");
    for(int j=0; j<8*3-1; j++) printf("-"); // bottom border
    printf("\n\n\t  ");
    for(int j=0; j<8; j++) printf(" %2c", "hgfedcba"[j]); // label abc
    printf("\n\n");
}

v
{
    for(int i=0; i<8; i++)
        free(B[i]);

    free(B);
}

int signum(int x)
{
    return (x>0) - (x<0);
}
