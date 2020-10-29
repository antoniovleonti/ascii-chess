/*  Antonio Leonti
    10.26.2020
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


/*-MACRO DEFINITIONS----------------------------------------------------------*/

#define N_PCS   6   // number of pieces
#define PAWN    1   // piece ids
#define KNIGHT  2
#define BISHOP  3
#define ROOK    4
#define QUEEN   5
#define KING    6   // make sure king is ALWAYS 6
#define EP_FLAG 99  // en passant flag


/*-TYPE DEFINITIONS-----------------------------------------------------------*/

// playing board
typedef struct Board_
{
    int** b; // 8x8 piece map {-N_PIECES..N_PIECES}
    int** t; // 8x8 touch map {0,1}
} Board;
// type for piece movement functions
typedef int*** (*Piece_f)(Board, int*, int);


/*-FUNCTION DECLARATIONS------------------------------------------------------*/

// game / board state
Board make_move(Board, int**, int);
int play_game(Board);

// move evaluation
int is_pseudo_legal(Board, int**, int);
int is_suicide(Board, int**, int);
int is_trapped(Board, int);
int is_hit_by(Board, int*, int, int);

// handling boards
Board copy_board(Board);
void free_board(Board);

// IO
void print_board(Board, int);
int** read_move(char*);

// helper functions
void cast_dydx(Board, int[][64][2], int*, int, int, int, int[2]);
int*** malloc_from_tmp(int[][64][2], int[2]);
int* find(Board, int, int);
int signum(int);

// piece movement functions
int*** P__(Board, int*, int ); // PAWN
int*** N__(Board, int*, int ); // KNIGHT
int*** B__(Board, int*, int ); // BISHOP
int*** R__(Board, int*, int ); // ROOK
int*** Q__(Board, int*, int ); // QUEEN
int*** K__(Board, int*, int ); // KING

// array of piece movement functions
Piece_f list_moves[N_PCS] = {P__,N__,B__,R__,Q__,K__};


/*-FUNCTION DEFINITIONS-------------------------------------------------------*/

/*-helper functions-----------------------------------------------------------*/

int signum(int x)
{
    return (x>0) - (x<0);
}

void cast_dydx(Board B, int tmp[][64][2],
                int* pos, int dy, int dx,
                int player, int count[2])
{
    // go as far as possible in direction pointed to by {dy,dx}
    int y, x, cap;
    for(int i=1; 1; i++)
    {
        y = pos[0] + dy*i, x = pos[1] + dx*i;

        if(y<0 || y>=8 || x<0 || x>=8) return;

        cap = (B.b[y][x]) && (abs(B.b[y][x])!=EP_FLAG);

        if(signum(B.b[y][x]) != player)
        {
            tmp[cap][count[cap]][0] = y;
            tmp[cap][count[cap]][1] = x;

            count[cap]++;
        }
        if(cap) return;
    }
}

int*** malloc_from_tmp(int tmp[][64][2], int count[2])
{
    // create return pointer
    int*** moves = malloc(2*sizeof(int**));

    for(int i=0; i<2; i++)
    {
        // if there are no moves in this list, make it NULL
        if(!count[i])
        {
            moves[i] = NULL;
            continue;
        }
        // otherwise, create array
        moves[i] = malloc((count[i]+1) * sizeof(int*));
        // first element = number of elements
        moves[i][0] = malloc(sizeof(int));
        *moves[i][0] = count[i];

        // the rest of the elements are the moves.
        for(int j=0; j<count[i]; j++)
        {
            // make space
            moves[i][j+1] = malloc(2*sizeof(int));
            // copy values
            for(int k=0;k<2; k++) moves[i][j+1][k] = tmp[i][j][k];
        }
    }
    return moves;
}

/*-Piece movement functions---------------------------------------------------*/

int*** P__(Board B, int* pos, int player) // PAWN.......list_moves[0]
{
    int tmp[2][64][2] = {0};
    int count[2] = {0};
    int y, x;

    if(pos[0]==(int)(4+3.5f*player)) return malloc_from_tmp(tmp, count);


    // first non-captures
    y = pos[0] + player;
    x = pos[1];
    // if nothing's directly in front of the pawn
    if( y < 8 && y >= 0
     && !B.b[y][x])
    {
        // it can move forward 1
        tmp[0][count[0]][0] = y;
        tmp[0][count[0]][1] = x;
        count[0]++;

        y += player;
        // if pawn is at home & nothing is 2 squares ahead
        if(pos[0]==(int)(4-2.5*player) && !B.b[y][x])
        {
            tmp[0][count[0]][0] = y;
            tmp[0][count[0]][1] = x;
            count[0]++;
        }
    }
    y = pos[0] + player;
    // now captures
    for(int i=0; i<2; i++)
    {
        x = pos[1] + 1-2*i;
        // check if it's possible to capture in this direction
        if( x >= 0 && x < 8
         && signum(B.b[y][x]) == -player)
        {
            tmp[1][count[1]][0] = y;
            tmp[1][count[1]][1] = x;
            count[1]++;
        }
    }
    return malloc_from_tmp(tmp, count);
}

int*** N__(Board B, int* pos, int player) // KNIGHT.....list_moves[1]
{
    int tmp[2][64][2];
    int count[2] = {0};
    // candidate moves
    int dydx[8][2] =
    {
        { 2,-1},{ 2, 1},{-2,-1},{-2, 1},
        { 1,-2},{ 1, 2},{-1,-2},{-1, 2}
    };

    int x, y, cap;
    for(int i=0; i<8; i++)
    {
        y=pos[0]+dydx[i][0], x=pos[1]+dydx[i][1];

        if( y>=0 && y<8 && // within bounds
            x>=0 && x<8 && // ^^^
            signum(B.b[y][x]) != player // not occupied by friendly piece
        ){
            // cap = 1 if capture, 0 if not
            cap = B.b[y][x];
            cap = (abs(cap)!=EP_FLAG) && (cap);
            // copy over
            tmp[cap][count[cap]][0] = y;
            tmp[cap][count[cap]][1] = x;
            count[cap]++;
        }
    }
    return malloc_from_tmp(tmp, count);
}

int*** B__(Board B, int* pos, int player) // BISHOP.....list_moves[2]
{
    // a bishop on an open board can reach 13 squares from the center
    int tmp[2][64][2];
    int count[2] = {0};

    int dy, dx;
    for(int a=0; a<2; a++)
    for(int b=0; b<2; b++)
    {
        dy = 1-2*a;
        dx = 1-2*b;
        //printf("dy=%d dx=%d\n",dy,dx);
        cast_dydx(B, tmp, pos, dy, dx, player, count);
    }
    return malloc_from_tmp(tmp, count);
}

int*** R__(Board B, int* pos, int player) // ROOK.......list_moves[3]
{
    // a rook on an open board can always reach 14 squares
    int tmp[2][64][2];
    int count[2] = {0};

    int dy, dx;
    for(int a=0; a<2; a++)
    for(int b=0; b<2; b++)
    {
        dy =  a*(1-2*b);
        dx = !a*(1-2*b);
        cast_dydx(B, tmp, pos, dy, dx, player, count);
    }
    return malloc_from_tmp(tmp, count);
}

int*** Q__(Board B, int* pos, int player) // QUEEN......list_moves[4]
{
    // a queen on an open board can reach 27 squares from the center
    int tmp[2][64][2];
    int count[2] = {0};

    int dy, dx;
    for(int dy=-1; dy<2; dy++)
    for(int dx=-1; dx<2; dx++)
    {
        if(!dy && !dx) continue;

        cast_dydx(B, tmp, pos, dy, dx, player, count);
    }
    return malloc_from_tmp(tmp, count);
}

int*** K__(Board B, int* pos, int player) // KING.......list_moves[5]
{
    int tmp[2][64][2];
    int count[2] = {0};

    // first find all normal moves
    for(int dy=-1; dy<=1; dy++)
    for(int dx=-1; dx<=1; dx++)
    {
        if(dy || dx)
        {
            int y = pos[0] + dy;
            int x = pos[1] + dx;

            if( y>=0 && y<8 // within bounds
            &&  x>=0 && x<8
            &&  signum(B.b[y][x]) != player) // not occupied by friendly piece
            {
                // cap = 1 if capture, 0 if not
                int cap = B.b[y][x];
                cap = (abs(cap)!=EP_FLAG) && (cap);
                // copy over
                tmp[cap][count[cap]][0] = y;
                tmp[cap][count[cap]][1] = x;

                count[cap]++;
            }
        }
    }

    // find castling moves
    if( pos[0] == (int)(4-3.5f*player)  // correct y coord
     && pos[1] == 3                     // correct x coord
     && !B.t[pos[0]][3]                 // untouched king
     && !is_hit_by(B, pos, -33, player) // not in check
    ){
        for(int i=0; i<2; i++)
        {
            int dx = 1-2*i; // -1 or 1

            if(!B.t[pos[0]][(int)(4+3.5f*dx)]) // untouched rook
            {
                int flag = 0;

                // make sure squares are open
                for(int j=3+dx; j>0 && j<7; j+=dx)
                {
                    if(B.b[pos[0]][j])
                    {
                        flag = 1;
                        break;
                    }
                }
                if(flag) continue;

                tmp[0][count[0]][0] = pos[0];
                tmp[0][count[0]][1] = 3 + 2*dx;

                count[0]++;
            }
        }
    }
    return malloc_from_tmp(tmp, count);
}

/*-Game / board state handling functions--------------------------------------*/

// plays a chess game from starting position B
int play_chess(Board B)
{
    char input[100]; // input buffer
    int** m; // player's move


    // begin game
    for(int i=1; 1; i=-i)
    {
        print_board(B, i);

        // check for mate at the start of every move.
        if(is_trapped(B, i)) // if player has no valid moves
        {
            // find the king
            int* pos = find(B, KING, i);
            // is he in check?
            int isCheck = is_hit_by(B, pos, -1, i);
            free(pos); // (done with this)

            return isCheck * -i; // return {-1, 0, 1}
        }

        // the game goes on.
        printf("\n\t%s's turn: > ", (i>0) ? "White" : "Black");

        // get move ("g1f3 instead of Nf3" - PGN is HARD to program)
        fgets(input, 100, stdin);
        m = read_move(input); // translate input; move is in m

        if(!m) // if input is invalid
        {
            puts("\n\t (?) Invalid input! Try again.\n");
            i =- i;
            continue;
        }

        if( is_pseudo_legal(B, m, i)
         && !is_suicide(B, m, i))
        {
            B=make_move(B, m, i);
        }
        else
        {
            printf("\n\t (!) Illegal move! Try again.\n");
            i=-i;
        }
        for(int i=0;i<2;i++) free(m[i]);
        free(m);
    }
}

// checks if player has any legal moves
int is_trapped(Board B, int player)
{
    int answer = 1;

    int** move = malloc(2*sizeof(int*)); // will point to candidate dest squares
    move[0]    = malloc(2*sizeof(int)); // will be current ("from") square
    int* i = &move[0][0];
    int* j = &move[0][1]; // so I can use move[0] as iterator in for loop

    // iterate through entire board
    for(*i=0; *i<8; (*i)++)
    for(*j=0; *j<8; (*j)++)
    {
        int piece = B.b[*i][*j];

        if(signum(piece)==player)
        {
            // get the list of possible moves
            int*** list = list_moves[abs(piece)-1](B, move[0], player);
            // iterate through possible moves
            for(int k=0; k<2; k++)
            {
                if(list[k]==NULL) continue;

                for(int l=1; l<=**list[k]; l++)
                {
                    // copy move destination
                    move[1] = list[k][l];

                    // found legal move, so this is not mate.
                    if(answer && !is_suicide(B, move, player)) answer=0;

                    free(list[k][l]);
                }
                free(list[k]);
            }
            free(list);

            if(!answer) return 0; // check for flag before moving on
        }
    }
    // if it got through the entire board without finding a valid move
    return 1;
}

// makes move & handles special cases (such as moving the rook when castling)
Board make_move(Board B, int** move, int player)
{
    // helpful shortcuts
    int y[2] = {move[0][0],move[1][0]};
    int x[2] = {move[0][1],move[1][1]};
    int dy = y[1]-y[0], dx = x[1]-x[0];

    int piece = B.b[y[1]][x[0]];

    int* pos;

    // special cases go here. (en passant, castling)
    switch(abs(piece))
    {
        case KING: // castling
        {
            if(!dy && abs(dx)==2)
            {
                // take care of the rook
                B.b[y[0]][(int)(4+(3.5f*signum(dx)))] = 0;
                B.b[y[0]][x[0]+signum(dx)] = ROOK*player;
            }
        } break;
        case PAWN:
        {
            // enable en passant
            if(!dx && dy==2*player)
            {
                B.b[y[0]+player][x[0]] = player*EP_FLAG;
            }

            // enable pawn promotions
            else if(y[1]==(int)(4+(3.5*player)))
            {
                B.b[y[0]][x[0]] = QUEEN*player;
            }
        } break;
    }

    // perform desired move
    B.b[y[1]][x[1]] = B.b[y[0]][x[0]];
    B.b[y[0]][x[0]] = 0;

    // remember these squares were touched
    B.t[y[0]][x[0]] = B.t[y[1]][x[1]] = 0;

    //find & remove opponent's en passant flag(s) if it exists
    if((pos = find(B, EP_FLAG, -player)))
    {
        B.b[pos[0]][pos[1]] = 0;
        free(pos);
    }

    // return new board state (useful for when making a move from cpy_board)
    return B;
}

// finds player's piece on the board (usually the king, but can be any piece)
int* find(Board B, int piece, int player)
{
    int* pos = malloc(sizeof(int)*2);

    for(int i=0; i<8; i++)
    for(int j=0; j<8; j++)
    {
        if(B.b[i][j] == piece*player)
        {
            pos[0] = i, pos[1] = j;
            return pos;
        }
    }
    return NULL;
}

/*-Move evaluation functions--------------------------------------------------*/

// checks if destination square is available for piece at source square
int is_pseudo_legal(Board B, int** move, int player)
{
    int answer = 0;

    int y[2] = {move[0][0], move[1][0]};
    int x[2] = {move[0][1], move[1][1]};

    int p[2] = {B.b[y[0]][x[0]], B.b[y[1]][x[1]]}; // pieces in each square
    int cap = (abs(p[1]) != EP_FLAG && p[1]); // is the proposed move a capture?

    if(signum(p[0]) != player) return 0;

    // get list of pseudo-legal moves
    int*** list = list_moves[abs(p[0])-1](B, move[0], player);

    // only check captures or non-captures-- whichever is needed
    if(list[!cap])
    {
        // delete elements
        for(int i=**list[!cap]; i>=0; i--) free(list[!cap][i]);
        free(list[!cap]); // delete pointer
    }

    if(!list[cap]) return 0;

    for(int i=1; i<=**list[cap]; i++)
    {
        //grab this pointer
        int* pos = list[cap][i];

        // if we've found the desired move
        if( !answer
         && pos[0] == y[1]
         && pos[1] == x[1] )
        {
            answer = 1;
        }
        free(pos);
    }
    free(list[cap][0]);
    free(list[cap]);
    free(list);

    return answer;
}

// checks if square is hit by opposing player's pieces "set" (1) in bitmask
int is_hit_by(Board B, int* pos, int mask, int player)
{
    int answer = 0;

    for(int i=0; !answer && i<N_PCS; i++)
    {
        if(!(mask>>i & 1)) continue; // check if piece is included

        // get all moves possible of piece i from pos
        int*** list = list_moves[i](B, pos, player);

        // ignore the non-captures
        if(list[0])
        {
            for(int j=**list[0]; j>=0; j--) free(list[0][j]);
            free(list[0]);
        }
        if(!list[1]){ free(list); continue; } // if there are no captures

        // check all potential captures here
        for(int j=1; j<=**list[1]; j++)
        {
            int y = list[1][j][0];
            int x = list[1][j][1];

            printf("checking (%d,%d) for piece %d\n",y,x,i*player);

            // if this is the right piece to attack pos
            if(!answer && abs(B.b[y][x])==(i+1)) answer=1;

            free(list[1][j]); // free this element
        }
        // free stuff
        free(list[1][0]);
        free(list[1]);
        free(list);
    }
    return answer;
}

// checks if move results in the loss of the king
int is_suicide(Board B, int** move, int player)
{
    int answer = 0;

    // copy board state, and make this move on the new board.
    Board H = make_move(copy_board(B), move, player);

    // find the king in both variations
    int** pos = malloc(2*sizeof(int*));
    pos[0] = find(B, KING, player);
    pos[1] = find(H, KING, player);

    do
    {
        // inch king closer destination square (if king doesn't, do nothing)
        for(int i=0; i<2; i++)
        {
            pos[0][i] += signum(pos[1][i]-pos[0][i]);
        }
        // check if king is hit at any point during his journey
        if(is_hit_by(H, pos[0], -1, player))
        {
            answer = 1;
            break;
        }
    }
    while(  !answer
         && ( pos[0][0] != pos[1][0]
           || pos[0][1] != pos[1][1] ) );


    for(int i=0; i<2; i++) free(pos[i]);
    free(pos);
    free_board(H);

    return answer;
}

/*-Board handling functions---------------------------------------------------*/

Board copy_board(Board B)
{
    Board C;

    C.b = malloc(8*sizeof(int*));
    C.t = malloc(8*sizeof(int*));

    for(int i=0; i<8; i++)
    {
        // make space for data
        C.b[i] = malloc(8*sizeof(int));
        C.t[i] = malloc(8*sizeof(int));
        // copy data
        memcpy(C.b[i], B.b[i], 8*sizeof(int));
        memcpy(C.t[i], B.t[i], 8*sizeof(int));
    }
    return C;
}

void free_board(Board B)
{
    for(int i=0; i<8; i++)
    {
        free(B.b[i]);
        free(B.t[i]);
    }
    free(B.b);
    free(B.t);
}

/*-IO functions---------------------------------------------------------------*/

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
        if((c = strchr(abcs, str[i]))) // if youre using abc notation...
        r[i/2][1-i%2] = (int)(c-abcs);

        else if((c = strchr(nums, str[i]))) // number notation...
        r[i/2][1-i%2] = (int)(c-nums);

        else return NULL;
    }
    return r;
}

void print_board(Board B, int player)
{
    int v, p = -player;
    for(int i=7*(p<0); i<8 && i>=0; i+=p)
    {
        printf("\n\t%d |  ", i+1); // label 123
        for(int j=7*(p<0); j<8 && j>=0; j+=p)
        {
            int v = B.b[i][j];
            printf( "%2c ",
                abs(v)==EP_FLAG || !v
                    ? i%2 == j%2 ? ':' : '~'    // empty squares
                    : "kqrbnp_PNBRQK"[6+v]      // pieces
            );
        }
    }
    // bottom border
    printf("\n\n\t      ");
    for(int j=0;j<7*3+1;j++) printf("-");
    // label abc
    printf("\n\n\t    ");
    for(int i=7*(p<0);i<8&&i>=0;i+=p) printf(" %2c", "hgfedcba"[i]);
    printf("\n\n");
}
