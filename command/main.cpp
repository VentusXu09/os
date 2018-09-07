#include<stdio.h>
#include<string.h> 
//#include<malloc.h>
#define true 1
#define false 0
#define SIZE 7 

typedef struct StackFloat
{
	float f;
	struct StackFloat *next;
}SF;  //�洢��������ջ

typedef struct StackChar
{
	char c;
	struct StackChar *next;
}SC;       //�洢�������ջ
SC sc[100];
SF sf[100];
int number1=0;int number2=0;
SC *Push(SC *s, char c)          //�������ջ����
{  
    //SC element;
	SC *p = &sc[number1];//(SC*)malloc(sizeof(SC));
	number1++;
	p->c = c;
	p->next = s;
	return p;
}

SF *Push1(SF *s, float f)        //��������ջ���� 
{
	
	SF *p =&sf[number2];//(SF*)malloc(sizeof(SF));
	number2++;
	p->f = f;
	p->next = s;
	return p;
}
SC *Pop(SC *s)    //�������ջ
{
	SC *q = s;
	s = s->next;
	//free(q);
	return s;
}

SF *Pop1(SF *s)      //��������ջ
{
	SF *q = s;
	s = s->next;
	//free(q);
	return s;
}

char prior[7][7] =
{   // ��������ȼ���  
	// '+' '-' '*' '/' '(' ')' '#' 
	/*'+'*/ '>','>','<','<','<','>','>',
	/*'-'*/ '>','>','<','<','<','>','>',
	/*'*'*/ '>','>','>','>','<','>','>',
	/*'/'*/ '>','>','>','>','<','>','>',
	/*'('*/ '<','<','<','<','<','=',' ',
	/*')'*/ '>','>','>','>',' ','>','>',
	/*'#'*/ '<','<','<','<','<',' ','='

};


float operate(float a, unsigned char sign, float b)      //����operate�������������������ֵ  
{
		switch (sign)
		{
		case '+': return a + b; break;
		case '-': return a - b; break;
		case '*': return a*b; break;
		case '/': if (b == 0)
		{
			printf("divided by ZERO\n"); 
		}
				  return a / b; break;
		default: return 0;
		}
	
}

char order[SIZE] = { '+','-','*','/','(',')','#' };

int judge(char Test, char *Testop)  //�ж��Ƿ�Ϊ�����
{
	int Find = false;
	for (int i = 0; i< SIZE; i++)
	{
		if (Test == Testop[i])
			Find = true;
	}
	return Find;
}
int Return(char op, char *Testop)  //�����������˳��
{
	for (int i = 0; i< SIZE; i++)
	{
		if (op == Testop[i])
			return i;
	}
}
char precede(char stackin, char stackout)  //������������������ȼ��ȽϽ��
{
	return prior[Return(stackin, order)][Return(stackout, order)];
}

int is_digit(char ch)
{
	if(ch>='0'&&ch<='9')
		return 1;
	else
		return 0;
}
int is_space(char ch)
{
	if(ch==' ')
		return 1;
	else
		return 0;
}
double atof(char *s)
{
	double power,value;
	int i,sign;
	//assert(s!=NULL);//�ж��ַ����Ƿ�Ϊ��
	for(i=0;is_space(s[i]);i++);//��ȥ�ַ���ǰ�Ŀո�
	sign=(s[i]=='-')?-1:1;
	if(s[i]=='-'||s[i]=='+')//Ҫ���з���λ��ǰ��һλ
		i++;
	for(value=0.0;is_digit(s[i]);i++)//����С����Ǯ������
		value=value*10.0+(s[i]-'0');
	if(s[i]=='.')
		i++;
	for(power=1.0;is_digit(s[i]);i++)//����С����������
	{
		value=value*10.0+(s[i]-'0');
		power*=10.0;
	}
	return sign*value/power;
}

float calculate(char* MyExpression)
{
	// �������ʽ��ֵ����������㷨  
	// ��OPTR��OPND�ֱ�Ϊ�����ջ��������ջ��OPΪ���������   
	SC *OPTR = NULL;       // �����ջ���ַ�Ԫ��   
	SF *OPND = NULL;       // ������ջ��ʵ��Ԫ��   
	char data[20];
	float Data, a, b; //data���ԭʼ���ַ������֣�Data���ת����double�͵�����
	char sign, *c, end[] = { '#','\0' }; //sign����������c���ڶ�ȡ�ַ�����end�����ַ�������
	OPTR = Push(OPTR, '#');   //���������ջ�з�����ͼ���#
	c = strcat(MyExpression, end);
	strcpy(data, "\0");
	while (*c != '#' || OPTR->c != '#')  //��������ջ�ѿղ����ַ���Ҳ���꣬����ѭ��
	{
		if (!judge(*c, order))  //�������������ʹ�������data����
		{
			end[0] = *c;
			strcat(data, end);
			c++;
			if (judge(*c, order))  //��һ����������ͽ���data�Ķ�ȡ
			{
				int j = 0;
				
				if (j == 0)
				{
					Data = atof(data);
				}
				//�ַ���ת������(double)
				OPND = Push1(OPND, Data);
				strcpy(data, "\0");
			}
		}
		else    // �����������ͽ������в���
		{
			switch (precede(OPTR->c, *c))     // �Ƚ�ջ��Ԫ�����ȼ���
			{
			case '<':   OPTR = Push(OPTR, *c);

				c++;
				break;
			case '=':

				OPTR = Pop(OPTR);   // �����Ų�������һ�ַ�
				c++;
				break;
			case '>':
				sign = OPTR->c; OPTR = Pop(OPTR);    // ��������ջ��ȡ��ջ�����������뵱ǰ��������������� 
				b = OPND->f; OPND = Pop1(OPND);
				a = OPND->f; OPND = Pop1(OPND);
				OPND = Push1(OPND, operate(a, sign, b));
				break;
			}//switch
		}
	} //while   
	return OPND->f;
} //calculate����   

void simplify(char *a)//ɾ���ո񣬰���������еĸ��ź�����ת��Ϊ��0-a������ʽ
{    
    char c[1000]={0};
	for(int i=0,j=0;i<1000&&a[i]!='\0';i++) 
	{
		if(a[i]!=' ')
		{
			c[j]=a[i];j++;
		}
	}
	for(int i=0;i<1000;i++) 
	{
		a[i]=c[i];
	}
	int flag3=1;
	for(int i=0;a[i]!='\0';i++)
	{
		if(a[i]<48||a[i]>57)
		  if(a[i]!='+'&&a[i]!='-'&&a[i]!='*'&&a[i]!='/'&&a[i]!='('&&a[i]!=')'&&a[i]!='.')
		    {
		    	flag3=0;  
				printf("�������\n");
				break;
			}
			
	}
	int i; char b[1000] = { 0 }; int j; int flag = 0; int flag1 = 0; int flag2 = 0;
	for (i = 999; i >= 0; i--)
		if ((a[i] == '-' || a[i] == '+') && (i == 0 || judge(a[i - 1], order) == true && a[i - 1] != ')'))
		{
			if (a[i] == '+') flag2 = 1;
			for (j = 0; j<i; j++)
				b[j] = a[j];
			b[i] = '('; b[i + 1] = '0'; b[i + 2] = (flag2>0) ? '+' : '-'; b[i + 3] = a[i + 1];
			if (a[i + 1] != '(')
			{
				b[i + 3] = a[i + 1];
				b[i + 4] = ')';
				for (j = i + 5; a[j - 3] != '\0'; j++)
					b[j] = a[j - 3];
			}
			else
			{
				for (j = i + 4;; j++)
				{
					if (flag == -1)
					{
						b[j] = ')'; flag = 0; break;
					}
					b[j] = a[j - 2];
					if (b[j] == '(') flag++;
					if (b[j] == ')') flag--;
				}
				for (j = j + 1; a[j - 3] != '\0'; j++)
					b[j] = a[j - 3];


			}
			for (i = 0; i<1000; i++)
				a[i] = b[i];
			break;
		}
	for (i = 0; a[i] != '\0'; i++)
		if ((a[i] == '-' || a[i] == '+') && (i == 0 || judge(a[i - 1], order) == true && a[i - 1] != ')'))
			flag1 = 1;
	if (flag1 == 1)
		simplify(a);
}
void mygets(char *str)
{
	int i=0;
	scanf("%c",&str[i]);
	while(str[i++]!='\n')
	{
		scanf("%c",&str[i]);
	}
	str[i-1]=0;
	if(str[0]==0)
	  str[0]='a';
}
int main() {
	printf("lty�ļ�����\n");
	printf("������һ����ѧ���ʽ,�����пո����ż�������\n");
	printf("����exit��ʾ�˳�\n");
	while(true)
	{   
	    number1=0;number2=0;
		char c[1000]={0};
		mygets(c);
		if(strcmp(c,"exit")==0)
		break;
		simplify(c);
	    float ans=calculate(c);
	    //printf("%f\n",ans);
	    int ans1=int(ans);
	    int ans2=(ans-ans1)*1000000;
	    printf("%d",ans1);
	    printf(".");
	    printf("%d\n",ans2);
	
	 } 
	/*char c[1000]={0};
		simplify(c);
	float a=calculate(c);
	printf("%f",a);*/
	return 0;
}


