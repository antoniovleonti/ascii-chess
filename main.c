#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include "pieces.h"

int** read_move(int[8][8], char*);
int is_attacked(int[8][8], int, int, int);
int is_blocked(int[8][8], int**);
void make_move(int[8][8], int**);
void print_board(int[8][8], int);
int sign(int);

int main(void)
{
    char input[100]; // input buffer

    int B[8][8] = // initial board state
    { // 1=P, 2=N, 3=B, 4=R, 5=Q, 6=K; -1..-6=black
        { 4, 2, 3, 6, 5, 3, 2, 4},
        { 1, 1, 1, 1, 1, 1, 1, 1},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        {-1,-1,-1,-1,-1,-1,-1,-1},
        {-4,-2,-3,-6,-5,-3,-2,-4}
    };
    // gen board & pieces

    for(int player=1; 1; player*=-1)
    {
        int tmp = is_attacked(B,player,3,3);
        printf("e4 IS %sattacked by player %d",
            tmp ?"":"NOT ", player
        );
        print_board(B, player);
        // get move ("g1f3 instead of Nf3" - PGN is HARD to program)
        fgets(input, 100, stdin);
        int** m = read_move(B, input);
        printf("m={{%d %d},{%d %d}}\n",m[0][0],m[0][1],m[1][0],m[1][1]);

        // if move is valid
        if(m && is_blocked(B, m)<1)
        {
            // check for check / checkmate / stalemate
            make_move(B, m);
        }
    }
}
// returns -1 if a capture, 0 if no capture, 1 if blocked
int is_blocked(int B[8][8], int** m)
{
    int y0 = m[0][0], y1 = m[1][0];
    int x0 = m[0][1], x1 = m[1][1];
    int dy = y1-y0, dx = x1-x0;
    int p0 = B[y0][x0], p1 = B[y1][x1];

    //printf("dy = %d; dx = %d\npiece = %d\n", dy, dx, piece);
    switch(abs(p0))
    {   case 1: // pawn
        {
            // "normal" moves
            if(!dx && dy==sign(p0)) return !!p1;

            // special first move
            if(!dx && dy==2*sign(p0) && y0==(p0>0?1:6))
            {
                for(int i=1; i<=2; i++) // check for blocking pieces
                    if(B[y0+i*sign(dy)][x0]) return 1;

                return 0; // if the above check does not find an obstruction
            }
            // captures
            if(abs(dx)==1 && dy==sign(p1))
            {
                printf("HERE!\n");
                return !sign(p0)+sign(p1) ? -1 : 1;
            }
        } break;
        case 6: // king (castling)
        {
            if(!dy && abs(dx)==2)
            {   for(int i=1; i<=2+(dx>0); i++)
                    if(B[y0][x0+sign(dx)*i]) return 1;

                return 0; // if the above check does not find an obstruction
            }
        }
        default: // "geometric" (normal) pieces
        {
            int pi = abs(p0)-2; // piece index in piece movement lookup table
            int i=0, j=0;
            int is_capt = !(sign(p0) + sign(p1)); // is this a capture?

            if( moveMask[pi][7+dy][7+dx] &&
                sign(p0) != sign(p1)
            ){
                while(1)
                {   // ==1 is the base case in the piece ADT
                    if(moveMask[pi][7+dy+i][7+dx+j]==1) return 0-!(sign(p0)+sign(p1));
                    // if there's another piece in the way
                    if((i||j) && B[y1+i][x1+j]) return 1;
                    // move on to next square
                    i -= sign(dy), j -= sign(dx);
                }
            }
        } break;
    }
    return 1;
}

int is_attacked(int B[8][8], int player, int y, int x)
{
    int** m = malloc(sizeof(int*)*2);
    for(int i=0; i<2; i++) m[i] = malloc(sizeof(int)*2);

    m[1][0]=y; m[1][1]=x;

    for(int i=m[0][0]=0; i<8; i++,m[0][0]++)
        for(int j=m[0][1]=0; j<8; j++, m[0][1]++)
            if(sign(B[i][j])==player && is_blocked(B, m)==-1)
                return 1;

    return 0;
}

void make_move(int B[8][8], int** m)
{
    int y0 = m[0][0], y1 = m[1][0];
    int x0 = m[0][1], x1 = m[1][1];
    int dy = y1-y0, dx = x1-x0;
    int p = B[y0][x0];

    // take care of special cases first
    switch(abs(p))
    {   case 1:
        {   // special first move
            if(abs(dy)==2) B[y0+sign(p)][x0] = p>0 ? INT_MAX : INT_MIN;
            // en passant
            if( dx==1 && dy==1 &&
                B[y1][x1]==INT_MAX || B[y1][x1]==INT_MIN
            ){
                B[y0][x1] = 0; // get rid of the pawn
            }
        } break;
        case 6:
        {   if(!dy && abs(dx)==2)
            {
                B[y0][x0+sign(dx)*3+(dx>0)] = 0;
                B[y0][x0+sign(dx)] = 4*sign(p);
            }
        } break;
    }
    // always make the intended move
    B[y1][x1] = B[y0][x0];
    B[y0][x0] = 0;
}

int** read_move(int board[8][8], char* str)
{
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

void print_board(int B[8][8], int player)
{
    for(int i=0; i<8; i++)
    {
        printf("\n\t%d  ",i+1); // label 123
        for(int j=0; j<8; j++) printf(" %2d ",
            B[i][j] == INT_MAX || B[i][j] == INT_MIN ? 0 : B[i][j]
        ); // nums
    }
    printf("\n\n\t   ");
    for(int j=0; j<8; j++) printf(" %2c ", "hgfedcba"[j]); // label abc
    putchar('\n');
}

int sign(int x)
{
    return (x>0) - (x<0);
}
