#include<stdio.h>
 
int main(){
    int year, month, day,sum,leap;
        char time[8]={};
    double a=1.2;
    printf("%e",a);
    printf("please input year month day:");
        read(0,time,8);
        year=(time[0]-48)*1000+(time[1]-48)*100+(time[2]-48)*10+(time[3]-48);
        month=(time[4]-48)*10+(time[5]-48);
        day=(time[6]-48)*10+(time[7]-48);
    sum = 0;
    switch (month)
    {
    case 1:sum = 0; break;
    case 2:sum = 31; break;
    case 3:sum = 59; break;
    case 4:sum = 90; break;
    case 5:sum = 120; break;
    case 6:sum = 151; break;
    case 7:sum = 181; break;
    case 8:sum = 212; break;
    case 9:sum = 243; break;
    case 10:sum = 273; break;
    case 11:sum = 304; break;
    case 12:sum = 334; break;
    default:
        printf("data error");
        return 0;
        break;
    }
    sum += day;
    if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0))
    {
        leap = 1;
    }
    else
    {
        leap = 0;
    }
    if (leap==1&&month>2)
    {
        sum += 1;
    }
    printf("It is %d day of this %d",sum,year);
    return 0;
}
