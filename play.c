/*  Antonio Leonti
    10.25.2020
    ASCII Chess
*/
#include "chess.h"

int main(void)
{
    int r;
    Board B;

    int start[8][8] =
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
    {
        { 4, 0, 0, 6, 0, 0, 0, 4},
        { 0, 0, 0, 1, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0,-5, 0, 0, 0, 0},
        { 0, 0, 0,-6, 0,-4, 0,-4}
    };

    B.b = malloc(8*sizeof(int*));
    B.t = malloc(8*sizeof(int*));

    for(int i=0; i<8; i++)
    {
        // copy starting position
        B.b[i] = malloc(8*sizeof(int));
        memcpy(B.b[i], start[i], 8*sizeof(int));
        // set up touch board
        B.t[i] = calloc(8,sizeof(int));
    }

    printf
    (   "\n%s%s%s%s%s\n",

        "\tASCII-Chess by Antonio Leonti (Oct 25, 2020)\n\n",

        "\tNotes:\n",
        "\t - Enter your moves as \"e2e4\", \"e7e5\", etc.\n",
        "\t - This program follows all the normal rules of\n\t   chess EXCEPT for underpromotion.\n",
        "\t - White's pieces are capital letters, while\n\t   those of Black are lowercase.\n"
        "\t - The board is flipped every move to face the\n\t   player to move.\n"
    );

    // play the game
    r = play_chess(B);

    // print results
    for(int i=0; i<=35; i++) putchar('*');
    if(r) printf("\n\n\tCHECKMATE! %s wins!\n\n", r>0?"White":"Black");
    else printf("\n\n\tSTALEMATE! It's a draw!\n\n");

    return 0;
}

// core functions
