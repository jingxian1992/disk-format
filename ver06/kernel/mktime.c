#include <stdio.h>
#include <asm/io.h>
#include <time.h>

#define MINUTE 60
#define HOUR 3600
#define DAY 86400
#define YEAR 31536000

/* interestingly, we assume leap-years */
int month[12];

void time_init(void)
{
	struct tm time;
	
	month[0] = 0;
	month[1] = DAY*(31);
	month[2] = DAY*(31+29);
	month[3] = DAY*(31+29+31);
	month[4] = DAY*(31+29+31+30);
	month[5] = DAY*(31+29+31+30+31);
	month[6] = DAY*(31+29+31+30+31+30);
	month[7] = DAY*(31+29+31+30+31+30+31);
	month[8] = DAY*(31+29+31+30+31+30+31+31);	
	month[9] = DAY*(31+29+31+30+31+30+31+31+30);	
	month[10] = DAY*(31+29+31+30+31+30+31+31+30+31);	
	month[11] = DAY*(31+29+31+30+31+30+31+31+30+31+30);
	
	do {
		time.tm_sec = CMOS_READ(0);
		time.tm_min = CMOS_READ(2);
		time.tm_hour = CMOS_READ(4);
		time.tm_mday = CMOS_READ(7);
		time.tm_mon = CMOS_READ(8);
		time.tm_year = CMOS_READ(9);
	} while (time.tm_sec != CMOS_READ(0));
	BCD_TO_BIN(&(time.tm_sec));
	BCD_TO_BIN(&(time.tm_min));
	BCD_TO_BIN(&(time.tm_hour));
	BCD_TO_BIN(&(time.tm_mday));
	BCD_TO_BIN(&(time.tm_mon));
	BCD_TO_BIN(&(time.tm_year));
	
	time.tm_mon--;
	startup_time = kernel_mktime(&time);
	printk("\tthe opening computer time stamp is 0x%08X seconds.\n", startup_time);
	
}

int CMOS_READ(unsigned char addr)
{
	int tmp;
	outb_p(0x80|addr,0x70);
	tmp = inb_p(0x71);
	return tmp;
}

void BCD_TO_BIN(int * val_ptr)
{
	*val_ptr = ((*val_ptr)&15) + (((*val_ptr)>>4)*10);
}

int __isleap(int year)
{
	if (0 == (year % 4))
	{
		if (0 == (year % 100))
		{
			if (0 == (year % 400))
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 0;
	}
}

unsigned int kernel_mktime(struct tm * tm)
{
	unsigned int res;
	int year;

	year = tm->tm_year + 2000 - 1970;
/* magic offsets (y+1) needed to get leapyears right.*/
	res = YEAR*year + DAY*((year+1)/4);
	res += month[tm->tm_mon];
/* and (y+2) here. If it wasn't a leap-year, we have to adjust */
	if (tm->tm_mon>1 && ((year+2)%4))
		res -= DAY;
	res += DAY*(tm->tm_mday-1);
	res += HOUR*tm->tm_hour;
	res += MINUTE*tm->tm_min;
	res += tm->tm_sec;
	return res;
}


