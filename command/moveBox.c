#include<stdio.h>
 
#define COL 11
#define ROW 10
 
#define PERSON 'A'
#define BOX 'X'
#define WALL '#'
#define SPACE ' '
#define ERROR '@'
 
#define UP 100
#define DOWN 101
#define LEFT 102
#define RIGHT 103
 
char map[ROW][COL] = {
    "##########",//0  
    "###     ##",//1  
    "###     ##",//2  
    "##AX  # ##",//3  
    "###  ##  #",//4  
    "#####    #",//5  
    "##       #",//6  
    "#     ####",//7  
    "###       ",//8  
    "##########" //9  
    //A:ÈË  £¬ X£ºÏä×Ó 
};
int location[2] = { 3, 2 };
 
void printAll()
{
    int i;
    for (i = 0; i < ROW; i++)
    {
        printf("%s\n", map[i]);
    }
}
int win()
{
    if (map[8][9]=='X')
    {
        return 1;
    }
    return 0;
}
char boxNear(int direction)
{
    switch (direction)
    {
    case UP:
        if (map[location[0] - 2][location[1]]==WALL)
        {
            return WALL;
        }
        else
        {
            return SPACE;
        }
    case DOWN:
        if (map[location[0] + 2][location[1]] == WALL)
        {
            return WALL;
        }
        else
        {
            return SPACE;
        }
    case LEFT:
        if (map[location[0]][location[1]-2] == WALL)
        {
            return WALL;
        }
        else
        {
            return SPACE;
        }
    case RIGHT:
        if (map[location[0]][location[1]+2] == WALL)
        {
            return WALL;
        }
        else
        {
            return SPACE;
        }
    default:
        return WALL;
        break;
    }
}
void moveUP()
{
    switch (map[location[0] -1][location[1]])
    {
    case WALL:
        break;
    case BOX:
        if (boxNear(UP)!=WALL)
        {
            map[location[0]][location[1]] = ' ';
            map[location[0] - 1][location[1]] = 'A';
            map[location[0] - 2][location[1]] = 'X';
            location[0] -= 1;
        }
        break;
    case SPACE:
        map[location[0]][location[1]] = ' ';
        map[location[0] - 1][location[1]] = 'A';
        location[0] -= 1;
        break;
    default:
        printf("UP error\n");
        return;
        break;
    }
}
void moveDOWN()
{
    switch (map[location[0] +1][location[1]])
    {
    case WALL:
        break;
    case BOX:
        if (boxNear(DOWN) != WALL)
        {
            map[location[0]][location[1]] = ' ';
            map[location[0] + 1][location[1]] = 'A';
            map[location[0] + 2][location[1]] = 'X';
            location[0] += 1;
        }
        break;
    case SPACE:
        map[location[0]][location[1]] = ' ';
        map[location[0] + 1][location[1]] = 'A';
        location[0] += 1;
        break;
    default:
        printf("DOWN error\n");
        return;
        break;
    }
}
void moveLEFT(){
    switch (map[location[0]][location[1]-1])
    {
    case WALL:
        break;
    case BOX:
        if (boxNear(LEFT) != WALL)
        {
            map[location[0]][location[1]] = ' ';
            map[location[0]][location[1]-1] = 'A';
            map[location[0]][location[1]-2] = 'X';
            location[1] -= 1;
        }
        break;
    case SPACE:
        map[location[0]][location[1]] = ' ';
        map[location[0]][location[1]-1] = 'A';
        location[1] -= 1;
        break;
    default:
        printf("LEFT error\n");
        return;
        break;
    }
}
void moveRIGHT(){
    switch (map[location[0]][location[1]+1])
    {
    case WALL:
        break;
    case BOX:
        if (boxNear(RIGHT) != WALL)
        {
            map[location[0]][location[1]] = ' ';
            map[location[0]][location[1]+1] = 'A';
            map[location[0]][location[1]+2] = 'X';
            location[1] += 1;
        }
        break;
    case SPACE:
        map[location[0]][location[1]] = ' ';
        map[location[0]][location[1]+1] = 'A';
        location[1] += 1;
        break;
    default:
        printf("RIGHT error\n");
        return;
        break;
    }
}
void run(){
    char direction = ' ';
    char dir[1]={};
    while (1)
    {
        read(0,dir,1);
	direction=dir[0];
        if (direction == 'w' || direction == 'W')
        {
            moveUP();
        }
        else if (direction == 's' || direction == 'S')
        {
            moveDOWN();
 
        }
        else if (direction == 'a' || direction == 'A')
        {
            moveLEFT();
        }
        else if (direction == 'd' || direction == 'D')
        {
            moveRIGHT();
        }
        else if (direction=='Q'||direction=='q')
        {
            printf("game over!\n");
            break;
        }
        else
        {
            printf("No such direction\n");
        }
        printAll();
        if (win())
        {
            printf("you win!\n");
            break;
        }
    }
}
void init()
{
    printf("******************************************\n");
    printf("*                                        *\n");
    printf("*          welcome to move box!!         *\n");
    printf("*              'A' is human              *\n");
    printf("*               'X' is box               *\n");
    printf("*           enter w s a d to move        *\n");
    printf("*            enter q to quit             *\n");
    printf("*  if you move the box the lower right,  *\n");
    printf("*               you will win!            *\n");
    printf("*                                        *\n");
    printf("******************************************\n");
}
int main(){
    init();
    printAll();
    run();
    return 0;
}
