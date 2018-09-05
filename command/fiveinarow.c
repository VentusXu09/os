#include<stdio.h>

char map[10][10] = {
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
	'0', '0', '0', '0', '0', '0', '0', '0', '0', '0'
};
int judge(int candidate){
	int i, j,k;
	char ch = ' ';
	if (candidate == 0)
	{
		ch = 'x';
	}
	else
	{
		ch = 'v';
	}
		for (i = 0; i < 10; i++)
		{
			for (j = 0; j < 10; j++)
			{
				if (map[i][j] == ch)
				{
					if (9 - j >= 4)
					{
						for (k = 1; k <= 4; k++)
						{
							if (map[i][j + k] != ch)
							{
								break;
							}
							if (k == 4)
							{
								return 1;
							}
						}
					}
					if (9 - i >= 4)
					{
						for (k = 1; k <= 4; k++)
						{
							if (map[i + k][j] != ch)
							{
								break;
							}
							if (k == 4)
							{
								return 1;
							}
						}
					}
					if (9 - j >= 4 && 9 - i >= 4)
					{
						for (k = 1; k <= 4; k++)
						{
							if (map[i + k][j + k] != ch)
							{
								break;
							}
							if (k == 4)
							{
								return 1;
							}
						}
					}
					if (i >= 4 && 9-j>=4)
					{
						for (k = 1; k <= 4; k++)
						{
							if (map[i - k][j + k] != ch)
							{
								break;
							}
							if (k==4)
							{
								return 1;
							}
						}
					}
				}
			}
		}
		return 0;
}
int main(void)
{
	printf("*************************************************************\n");
	printf("*                                                           *\n");
	printf("*              Welcome to five-in-a-row!                    *\n");
	printf("*         The first hand is the candidate of 'x'.           *\n");
	printf("*         The second hand is the candidate of 'v'.          *\n");
	printf("*  You can input coordinate like this 'XX'(double integer). *\n");
	printf("*   The coordinate is beginning of '00',and end of '99'.    *\n");
	printf("*            you can end the game by enter '??'               *");
	printf("*                  Now let's begin!                         *\n");
	printf("*                                                           *\n");
	printf("*************************************************************\n");
	int x, y;
	int candidate = 0;
	int win = 0;
        char coordinate[2]={};
	for (int i = 0; i < 10; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				printf("%c ", map[i][j]);
			}
			printf("\n");
		}
	while (1)
	{
		if (candidate == 0)
		{
			printf("'x'candicate will set:");
		}
		else
		{
			printf("'v'candicate will set:");
		}
		read(0,coordinate,2);
		if(coordinate[0]=='?'||coordinate[1]=='?')
		{
			printf("game over!");
			break;
		}
                x=coordinate[0]-48;
                y=coordinate[1]-48;
		if (candidate==0)
		{
			map[x][y] = 'x';
			win = judge(candidate);
			candidate = 1;
		}
		else
		{
			map[x][y] = 'v';
			win = judge(candidate);
			candidate = 0;
		}
		for (int i = 0; i < 10; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				printf("%c ", map[i][j]);
			}
			printf("\n");
		}
		if (win == 1)
		{
			if (candidate == 1)
			{
				printf("'x' win!\n");
				break;
			}
			else
			{
				printf("'v' win!\n");
				break;
			}
		}
	}
	return 0;
}
