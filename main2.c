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

typedef struct Player_s
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
// array of piece-specific functions
typedef int** (*Piece_f)(int**, int*, Player);
Piece_f piece_moves[N_pcs] = {P__,N__,B__,R__,Q__,K__};
// utility functions
int** malloc_from_tmp__(int[][3], int); // creating move lists
int trace_dydx(int**, int[][3], int*, int, int, Player, int);
int* find(int**, int pc, Player);
int** cpy_board(int**); // handling boards
void free_board(int**);
void print_board(int**, Player);
int** read_move(char*); // input -> move
int signum(int);

int main(void)
{
    // int B[8][8] = // initial board state
    // { // 1=P, 2=N, 3=B, 4=R, 5=Q, 6=K; -1..-6=black
    //     { 4, 2, 3, 6, 5, 3, 2, 4},
    //     { 1, 1, 1, 1, 1, 1, 1, 1},
    //     { 0, 0, 0, 0, 0, 0, 0, 0},
    //     { 0, 0, 0, 0, 0, 0, 0, 0},
    //     { 0, 0, 0, 0, 0, 0, 0, 0},
    //     { 0, 0, 0, 0, 0, 0, 0, 0},
    //     {-1,-1,-1,-1,-1,-1,-1,-1},
    //     {-4,-2,-3,-6,-5,-3,-2,-4}
    // };
    int B[8][8] = // initial board state
    { // 1=P, 2=N, 3=B, 4=R, 5=Q, 6=K; -1..-6=black
        { 0, 0, 0, 0, 0, 0, 0, 4},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 6, 0, 0, 0, 0, 0, 0},
        { 0, 0, 3, 3, 0, 0, 0, 0},
        {-6, 0, 0, 0, 0, 0, 0, 0}
    };
    int canCastle[2] = {2,2};

    // play the game
    int result = play_game(B, canCastle);

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
        player[i].canCastle = 2;
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
            free(pos); // done with this

            return isCheck * -player[i].id; // return {-1, 0, 1}
        } else
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
    int pc0= B[y0][x0], pc1= B[y1][x1];
    //printf("pc0 = %d\n", pc0);
    // special cases go here. (en passant, castling)
    switch(abs(pc0))
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
        case PAWN: // enable en passant
        {
            if(!dx && dy==2*player->id)
            {
                // add "flag" behind pawn which other pawns can capture
                B[y0+player->id][x0] = player->id*EP_FLAG;
            }
        } break;
    }
    // perform desired move
    B[y1][x1] = B[y0][x0];
    B[y0][x0] = 0;

    return B;
}

// checks validity of a move (Da Rules)
int is_legal(int** B, int** m, Player player)
{
    // helpful shortcuts
    int y0 = m[0][0],   y1 = m[1][0];
    int x0 = m[0][1],   x1 = m[1][1];
    int dy = y1 - y0,   dx = x1 - x0;
    int pc0= B[y0][x0], pc1= B[y1][x1];

    int isClear = 0,    isCheck = 1;
    int isCastles = (abs(pc0)==KING && abs(dx)==2 && !dy);

    // a player can only move their pieces
    if(signum(pc0) != player.id) return 0;

    if(pc0 && !isCastles) // normal move logic
    {
        // First, see if the move is clear. start w list of available squares
        int** sqrs = piece_moves[abs(pc0)-1](B, m[0], player);

        if(!sqrs) return 0; // if there are no valid moves w this piece

        // iterate through list
        for(int i=1; i<=**sqrs; i++)
        {
            // if desired square is current element,
            if(y1==sqrs[i][0] && x1==sqrs[i][1])
                isClear = 1; // green light.
        }
        if(!isClear) return 0;

        // Now check for check. Begin by "hypothetically" making the move
        int** H = make_move(cpy_board(B), m, &player);

        // locate the king.
        int* pos_K = find(H, KING, player);
        if(pos_K) // make sure there's a king
        {
            // now check to see if the king is attacked in this variation.
            isCheck = is_hit(H, pos_K, player);
        }

        free(pos_K); free_board(H);

        return !isCheck;
    }
    else if(isCastles) // castling logic
    {
        // is player allowed to castle in this direction?
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
                pos_K[1] = x0+i*(dx/2);
                // see if this square is attacked
                if(is_hit(B, pos_K, player)) return 0;
            }
            // if the above tests are passed, the player may castle.
            return 1;
        }
    }
    return 0;
}

// checks for check (or potential check)
int is_hit(int** B, int* pos, Player player)
{
    //
    int x, y, isCapture;

    // check all potential attacker squares (max = 36)
    for(int i=0; i<N_pcs; i++)
    {
        // list of squares where piece[i] could be to attack the square
        int** sqrs = piece_moves[i](B, pos, player);

        if(!sqrs) continue; // if there are no squares, go to next piece type

        // iterate through squares
        for(int j=1; j<=**sqrs; j++)
        {
            y = sqrs[j][0], x = sqrs[j][1];
            // a piece_i of opponent's at [y][x] will always attack pos
            if( sqrs[j][2] && // is there a piece there?
                B[y][x]==(i+1)*(-player.id)) // is this the right piece type?
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
    int** move = malloc(sizeof(int*)*2); // for piece_moves and is_legal
    move[0] = malloc(sizeof(int)*2); // second position comes from sqrs[]

    int** sqrs; // will store list of valid moves

    for(int i=0; i<8; i++) // go through entire board
    for(int j=0; j<8; j++)
    {
        if(signum(B[i][j]) == player.id) // if piece belongs to player
        {
            move[0][0] = i, move[0][1] = j; // start square
            sqrs = piece_moves[abs(B[i][j])-1](B,move[0],player); // end square
            // search for a legal move
            if(!sqrs) continue;

            for(int k=1; k<=**sqrs; k++)
            {
                // get destination from sqrs
                move[1] = sqrs[k];
                if(is_legal(B, move, player)) // if the move is legal
                {
                    free(move[0]); free(move);
                    for(; k<=**sqrs; k++) free(sqrs[k]);
                    free(sqrs);  // free everything

                    return 1;
                }
                free(sqrs[k]); // otherwise free as you go
            }
        }
    }
    free(move[0]); free(move);
    free(sqrs); // free everything (sqrs elements have already been freed)

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
    int tmp[4][3];
    int count = 0;

    // if nothing's directly in front of the pawn
    int b = B[pos[0]+player.id][pos[1]];
    if(b==0)
    {
        // it can move forward 1
        tmp[count][0] = pos[0]+player.id;
        tmp[count][1] = pos[1];
        tmp[count][2] = 0; // is never a capture
        count++;

        if( pos[0] == (int)(4-player.id*2.5f) &&  // if the pawn is at home
            !B[pos[0]+2*player.id][pos[1]]   ) // if nothing is 2 squares ahead
        {
            // it can move forward 2
            tmp[count][0] = pos[0]+2*player.id;
            tmp[count][1] = pos[1];
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
    return malloc_from_tmp__(tmp, count);
}

int** N__(int** B, int* pos, Player player)
{
    int tmp[8][3] = // initialize to all possible {dx, dy} pairs
    {               // will later hold final coordinates
        {-2,-1, 0}, {-2, 1, 0}, { 2,-1, 0}, { 2, 1, 0},
        {-1,-2, 0}, {-1, 2, 0}, { 1,-2, 0}, { 1, 2, 0},
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
    return malloc_from_tmp__(tmp, count);
}

int** B__(int** B, int* pos, Player player)
{
    // a bishop on an open board can reach 13 squares from the center
    int tmp[13][3];
    int dy, dx, count = 0;

    for(int a=0; a<2; a++)
    for(int b=0; b<2; b++)
    {
        dy = 1-2*a;
        dx = 1-2*b;
        //printf("B__: dy=%d dx=%d\n", dy, dx);
        count = trace_dydx(B, tmp, pos, dy, dx, player, count);
        //printf("count = %d\n",count);
    }
    return malloc_from_tmp__(tmp, count);
}

int** R__(int** B, int* pos, Player player)
{
    // a rook on an open board can always reach 14 squares
    int tmp[14][3];
    int dy, dx, count = 0;

    for(int a=0; a<2; a++)
    for(int b=0; b<2; b++)
    {
        dy =  a*1-2*a;
        dx = !a*1-2*b;
        //printf("dy = %d dx = %d\n", dy, dx);
        count = trace_dydx(B, tmp, pos, dy, dx, player, count);
    }
    return malloc_from_tmp__(tmp, count);
}

int** Q__(int** B, int* pos, Player player)
{
    // a queen on an open board can reach 27 squares from the center
    int tmp[27][3];
    int dy, dx, count = 0;

    for(int a=-1; a<2; a++)
    for(int b=-1; b<2; b++)
    {
        if(!a && !b) continue;

        dy = a;
        dx = b;
        //printf("Q__: dy=%d, dx=%d\n",dy, dx);
        count = trace_dydx(B, tmp, pos, dy, dx, player, count);
    }
    return malloc_from_tmp__(tmp, count);
}

int** K__(int** B, int* pos, Player player)
{
    // a king on an open board can reach 5 squares anywhere but the edge
    int tmp[5][3];
    int y, x, count = 0;

    for(int i=-1; i<2; i++)
    for(int j=-1; j<2; j++)
    {
        if(i || j)
        {
            y=pos[0]+i, x=pos[1]+j;

            //
            //printf("K__: y=%d x=%d\n",y,x);

            if( y>=0 && y<8 && // within bounds
                x>=0 && x<8 && // ^^^
                ( signum(B[y][x]) != player.id ||
                  abs(B[y][x]) == EP_FLAG) // not occupied by friendly piece
            ){
                tmp[count][0] = y, tmp[count][1] = x;
                tmp[count][2] = B[y][x] && abs(B[y][x])!=EP_FLAG;
                count++;
            }
        }
    }//printf("END K__;\n\n");
    return malloc_from_tmp__(tmp, count);
}

// UTILITY FUNCTIONS

int trace_dydx(int** B, int tmp[][3],
                int* pos, int dy, int dx,
                Player player, int count )
{
    //printf("trace_dydx: dy=%d, dx=%d\n", dy, dx);
    int y, x;
    // go in direction pointed to by {dy,dx}
    for(int i=1; 1; i++)
    {
        y = pos[0]+dy*i, x = pos[1]+dx*i;
        //printf( "\ttrace_dydx: y=%d, x=%d\n", y,x);

        if(y<0 || y>=8 || x<0 || x>=8) return count;

        //printf("\t\ttrace_dydx: checking\n",y,x);
        if(abs(B[y][x]) != EP_FLAG && signum(B[y][x]) != player.id)
        {
            //printf("\t\t\t1\n");
            tmp[count][0] = y, tmp[count][1] = x;
            tmp[count][2] = B[y][x] && abs(B[y][x])!=EP_FLAG; //capture?
            //printf("\t\t\ttmp[count]={%d %d %d}\n",tmp[count][0],tmp[count][1],tmp[count][2]);
            count++;
            //printf("\tcount = %d",count);
        }
        if(B[y][x] && abs(B[y][x]) != EP_FLAG)
            return count;
    }
}

int** malloc_from_tmp__(int tmp[][3], int count)
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
        printf("\n\t%d | ",i+1); // label 123
        for(int j=0; j<8; j++)
        {
            int v = B[i][j];
            sprintf(out,"%d",v);
            printf( " %2s",
                    abs(v)==EP_FLAG || !v
                        ? i%2 == j%2 ? "~" : "+"    // empty squares
                        : out                       // pieces
            );
        }
    }
    printf("\n\n\t     ");
    for(int j=0; j<8*3-1; j++) printf("_"); // bottom border
    printf("\n\n\t    ");
    for(int j=0; j<8; j++) printf(" %2c", "hgfedcba"[j]); // label abc
    printf("\n\n");
}

void free_board(int** B)
{
    for(int i=0; i<8; i++)
        free(B[i]);

    free(B);
}

int signum(int x)
{
    return (x>0) - (x<0);
}
