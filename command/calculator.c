#include<stdio.h>
#include<string.h>
//#include<malloc.h>
#define true 1
#define false 0
#define SIZE 7
#define NULL 0
typedef struct StackFloat
{
 float f;
 struct StackFloat *next;
}SF;  //存储浮点数的栈
typedef struct StackChar
{
 char c;
 struct StackChar *next;
}SC;       //存储运算符的栈
SC sc[100];
SF sf[100];
int number1=0;int number2=0;
SC *Push(SC *s, char c)          //运算符进栈功能
{ 
    //SC element;
 SC *p = &sc[number1];//(SC*)malloc(sizeof(SC));
 number1++;
 p->c = c;
 p->next = s;
 return p;
}
SF *Push1(SF *s, float f)        //运算数进栈功能
{
 
 SF *p =&sf[number2];//(SF*)malloc(sizeof(SF));
 number2++;
 p->f = f;
 p->next = s;
 return p;
}
SC *Pop(SC *s)    //运算符出栈
{
 SC *q = s;
 s = s->next;
 //free(q);
 return s;
}
SF *Pop1(SF *s)      //运算数出栈
{
 SF *q = s;
 s = s->next;
 //free(q);
 return s;
}
char prior[7][7] =
{   // 运算符优先级表 
 // '+' '-' '*' '/' '(' ')' '#'
 /*'+'*/ '>','>','<','<','<','>','>',
 /*'-'*/ '>','>','<','<','<','>','>',
 /*'*'*/ '>','>','>','>','<','>','>',
 /*'/'*/ '>','>','>','>','<','>','>',
 /*'('*/ '<','<','<','<','<','=',' ',
 /*')'*/ '>','>','>','>',' ','>','>',
 /*'#'*/ '<','<','<','<','<',' ','='
};

float operate(float a, unsigned char sign, float b)      //函数operate，对两个数和运算符求值 
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
int judge(char Test, char *Testop)  //判断是否为运算符
{
 int Find = false;
 for (int i = 0; i< SIZE; i++)
 {
  if (Test == Testop[i])
   Find = true;
 }
 return Find;
}
int Return(char op, char *Testop)  //返回运算符的顺序
{
 for (int i = 0; i< SIZE; i++)
 {
  if (op == Testop[i])
   return i;
 }
}
char precede(char stackin, char stackout)  //返回两个运算符的优先级比较结果
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
 //assert(s!=NULL);//判断字符串是否为空
 for(i=0;is_space(s[i]);i++);//除去字符串前的空格
 sign=(s[i]=='-')?-1:1;
 if(s[i]=='-'||s[i]=='+')//要是有符号位就前进一位
  i++;
 for(value=0.0;is_digit(s[i]);i++)//计算小数点钱的数字
  value=value*10.0+(s[i]-'0');
 if(s[i]=='.')
  i++;
 for(power=1.0;is_digit(s[i]);i++)//计算小数点后的数字
 {
  value=value*10.0+(s[i]-'0');
  power*=10.0;
 }
 return sign*value/power;
}
float calculate(char* MyExpression)
{
 // 算术表达式求值的算符优先算法 
 // 设OPTR和OPND分别为运算符栈和运算数栈，OP为运算符集合  
 SC *OPTR = NULL;       // 运算符栈，字符元素  
 SF *OPND = NULL;       // 运算数栈，实数元素  
 char data[20];
 float Data, a, b; //data存放原始的字符型数字，Data存放转换成double型的数字
 char sign, *c, end[] = { '#','\0' }; //sign存放运算符，c用于读取字符串，end用于字符串连接
 OPTR = Push(OPTR, '#');   //先在运算符栈中放入最低级的#
 c = strcat(MyExpression, end);
 strcpy(data, "\0");
 while (*c != '#' || OPTR->c != '#')  //如果运算符栈已空并且字符串也读完，结束循环
 {
  if (!judge(*c, order))  //如果不是运算符就存在数组data里面
  {
   end[0] = *c;
   strcat(data, end);
   c++;
   if (judge(*c, order))  //下一个是运算符就结束data的读取
   {
    int j = 0;
   
    if (j == 0)
    {
     Data = atof(data);
    }
    //字符串转换函数(double)
    OPND = Push1(OPND, Data);
    strcpy(data, "\0");
   }
  }
  else    // 如果是运算符就进行下列操作
  {
   switch (precede(OPTR->c, *c))     // 比较栈顶元素优先级低
   {
   case '<':   OPTR = Push(OPTR, *c);
    c++;
    break;
   case '=':
    OPTR = Pop(OPTR);   // 脱括号并接收下一字符
    c++;
    break;
   case '>':
    sign = OPTR->c; OPTR = Pop(OPTR);    // 从运算数栈中取出栈顶两个数，与当前的运算符进行运算
    b = OPND->f; OPND = Pop1(OPND);
    a = OPND->f; OPND = Pop1(OPND);
    OPND = Push1(OPND, operate(a, sign, b));
    break;
   }//switch
  }
 } //while  
 return OPND->f;
} //calculate函数  
void simplify(char *a)//删除空格，把运算语句中的负号和正号转换为（0-a）的形式
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
/*void mygets(char *str)
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
}*/
int main() {
 printf("lty's calculator\n");
 printf("please input \n");
 printf("exit means out\n");
 while(true)
 {  
     number1=0;number2=0;
  char c[1000]={0};
  read(0,c,1000);
  if(c[0]=='e'&&c[1]=='x'&&c[2]=='i'&&c[3]=='t')
  break;
  simplify(c);
     float ans=calculate(c);
     int ans1=ans;
     int ans2=(ans-ans1)*1000000;
     int size=0;int n=ans2;
  while(n>0)
{size++;
n=n/10;
}
if(size==0) size=1;
     printf("%d",ans1);
     printf(".");
     for(int i=size;i<6;i++)
     {printf("0");}
     printf("%d\n",ans2);
 
  }
 /*char c[1000]={0};
  simplify(c);
 float a=calculate(c);
 printf("%f",a);*/
 return 0;
}


