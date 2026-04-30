#ifndef __PROGTEST__
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
typedef struct TDate
{
  unsigned       m_Year;
  unsigned short m_Month;
  unsigned short m_Day;
} TDATE;
TDATE makeDate ( unsigned       y,
                 unsigned short m,
                 unsigned short d )
{
  TDATE res = { y, m, d };
  return res;
}
bool  equalDate ( TDATE a,
                  TDATE b )
{
  return a . m_Year == b . m_Year
         && a . m_Month == b . m_Month
         && a . m_Day == b . m_Day;
}
#endif /* __PROGTEST__ */

// check if leap year
bool isLeapYear(unsigned year) {
  if (year % 4000 == 0) 
    return false;
  if (year % 400 == 0) 
    return true;
  if (year % 100 == 0) 
    return false;
  if (year % 4 == 0) 
    return true;
  return false;
}

// fast days since 1900-01-01 (1900-01-01 -> 1)
static inline long long fastDaysSince1900(unsigned year, unsigned short month, unsigned short day) {
  long long y = (long long)year;
  long long yearsSince1900 = y - 1900LL;
  
  // count leap years in [1900, year-1] with 4000-year exception
  long long a4    = ( (y - 1) / 4    ) - ( 1899 / 4    );
  long long a100  = ( (y - 1) / 100  ) - ( 1899 / 100  );
  long long a400  = ( (y - 1) / 400  ) - ( 1899 / 400  );
  long long a4000 = ( (y - 1) / 4000 ) - ( 1899 / 4000 );
  long long leapYears = a4 - a100 + a400 - a4000;
  long long days = yearsSince1900 * 365LL + leapYears;

  // month offsets (non-leap)
  static const int monthCumulative[13] = {
    0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
  };

  days += monthCumulative[month];
  if (month > 2 && isLeapYear(year)) days += 1;
  days += (long long)day;
  return days;
}

// convert date to days since 1900
long long dateToDay(TDATE date) {
  return fastDaysSince1900(date.m_Year, date.m_Month, date.m_Day);
}

// get days in month
int getDaysInMonth(unsigned year, unsigned short month) {
  int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2) {
    if (isLeapYear(year))
      return 29;
    else
      return 28;
  }
  return days[month - 1];
}

// check if date is valid
bool isValidDate(TDATE date) {
  if (date.m_Year < 1900) 
    return false;
  if (date.m_Month < 1 || date.m_Month > 12) 
    return false;
  if (date.m_Day < 1) 
    return false;
  if (date.m_Day > getDaysInMonth(date.m_Year, date.m_Month)) 
    return false;
  return true;
}


// convert days back to date
TDATE dayToDate(long long days) {
  TDATE result;
  result.m_Year = 1900;
  
  while (days > 0) {
    int yearDays;
    if (isLeapYear(result.m_Year))
      yearDays = 366;
    else
      yearDays = 365;
    
    if (days <= yearDays)
      break;
    
    days -= yearDays;
    result.m_Year++;
  }
  
  result.m_Month = 1;
  while (days > 0) {
    int monthDays = getDaysInMonth(result.m_Year, result.m_Month);
    if (days <= monthDays)
      break;
    
    days -= monthDays;
    result.m_Month++;
  }
  
  result.m_Day = (unsigned short)days;
  
  return result;
}

// First, get day of week from days since 1900-01-01
// and then map it to 0=Mon, 1=Tue, ..., 4=Fri
// with using modulo operation
int getDayOfWeek(long long daysSince1900) {
  return (int)((daysSince1900 - 1) % 7);
}

// check if friday 13th
bool isFriday13(TDATE date) {
  if (date.m_Day != 13) 
    return false;
  long long days = dateToDay(date);
  return getDayOfWeek(days) == 4;
}

bool countFriday13 ( TDATE from, TDATE to, long long int * cnt )
{
  if (!isValidDate(from)) 
    return false;
  if (!isValidDate(to)) 
    return false;
  
  long long fromDays = dateToDay(from);
  long long toDays = dateToDay(to);
  if (fromDays > toDays) 
    return false;
  
  // Determine first and last months in range that include a 13th within [from, to]
  unsigned fy = from.m_Year;
  unsigned short fm = from.m_Month;
  if (from.m_Day > 13) {
    fm++;
    if (fm > 12) { fm = 1; fy++; }
  }
  unsigned ty = to.m_Year;
  unsigned short tm = to.m_Month;
  if (to.m_Day < 13) {
    if (tm == 1) { tm = 12; ty--; } else tm--;
  }
  
  if ( (long long)ty < (long long)fy || (ty == fy && tm < fm) ) {
    *cnt = 0;
    return true;
  }
  
  *cnt = 0;
  unsigned cy = fy;
  unsigned short cm = fm;
  
  // Helpers
  #define MONTH_IS_FRIDAY_13(y,m) ( getDayOfWeek(fastDaysSince1900((y),(m),13)) == 4 )
  #define ADVANCE_ONE_MONTH(y,m) do { (m)++; if ((m) > 12) { (m) = 1; (y)++; } } while (0)
  
  // Align to January to enable 400-year jumps
  while ( (cy < ty || (cy == ty && cm <= tm)) && cm != 1 ) {
    if (MONTH_IS_FRIDAY_13(cy, cm)) (*cnt)++;
    ADVANCE_ONE_MONTH(cy, cm);
  }
  
  // Well we will jump by full 400-year Gregorian cycles without crossing 4000-year boundaries
  while ( cy + 400 <= ty || (cy + 400 == ty && 1 <= tm) ) {
    unsigned next4000 = (cy / 4000) * 4000 + 4000;
    if (cy + 400 > next4000) break; // would cross 4000 boundary -> stop jumping
    if (cm != 1) break; // only from January
    unsigned maxYearsToTy = (ty > cy) ? (ty - cy) : 0;
    unsigned maxYearsToBoundary = next4000 - cy;
    unsigned cycles = maxYearsToTy / 400;
    unsigned boundaryCycles = maxYearsToBoundary / 400;
    if (boundaryCycles < cycles) cycles = boundaryCycles;
    if (cycles == 0) break;
    *cnt += 688LL * (long long)cycles; // 688 Friday-13ths per 400 years
    cy += 400 * cycles;
    cm = 1;
  }
  
  // Well we will process remaining months
  while ( cy < ty || (cy == ty && cm <= tm) ) {
    if (MONTH_IS_FRIDAY_13(cy, cm)) (*cnt)++;
    ADVANCE_ONE_MONTH(cy, cm);
  }
  
  #undef MONTH_IS_FRIDAY_13
  #undef ADVANCE_ONE_MONTH
  
  return true;
}

bool prevFriday13 ( TDATE * date )
{
  if (!isValidDate(*date)) 
    return false;
  
  // We will start from 13th of current month
  TDATE current = *date;
  current.m_Day = 13;
  
  long long currentDays = dateToDay(current);
  long long inputDays = dateToDay(*date);
  
  // If current month's 13th is at or after input date, we go to previous month
  if (currentDays >= inputDays) {
    unsigned prevMonth = (current.m_Month == 1) ? 12 : (unsigned)current.m_Month - 1;
    unsigned prevYear  = (current.m_Month == 1) ? current.m_Year - 1 : current.m_Year;
    int daysInPreviousMonth = getDaysInMonth(prevYear, prevMonth);
    current.m_Month = (unsigned short)prevMonth;
    current.m_Year  = (unsigned)prevYear;
    currentDays -= daysInPreviousMonth;
  }
  
  // We will search backwards month by month
  while (currentDays >= 1) {
    if (current.m_Year < 1900) 
      return false;
    
    if (getDayOfWeek(currentDays) == 4) {
      *date = current;
      return true;
    }
    
    // Well, basically I want to move to previous month's 13th
    unsigned prevMonth = (current.m_Month == 1) ? 12 : (unsigned)current.m_Month - 1;
    unsigned prevYear  = (current.m_Month == 1) ? current.m_Year - 1 : current.m_Year;
    int daysInPreviousMonth = getDaysInMonth(prevYear, prevMonth);
    current.m_Month = (unsigned short)prevMonth;
    current.m_Year  = (unsigned)prevYear;
    currentDays -= daysInPreviousMonth;
  }
  
  return false;
}

bool nextFriday13 ( TDATE * date )
{
  if (!isValidDate(*date)) 
    return false;
  
  // Start from 13th of current month
  TDATE current = *date;
  current.m_Day = 13;
  
  long long currentDays = dateToDay(current);
  long long inputDays = dateToDay(*date);
  
  // If current month's 13th is at or before input date, go to next month
  if (currentDays <= inputDays) {
    int daysInCurrentMonth = getDaysInMonth(current.m_Year, current.m_Month);
    currentDays += daysInCurrentMonth;
    current.m_Month++;
    if (current.m_Month > 12) {
      current.m_Month = 1;
      current.m_Year++;
    }
  }
  
  // Search forwards month by month
  while (1) {
    if (getDayOfWeek(currentDays) == 4) {
      *date = current;
      return true;
    }
    
    // Move to next month's 13th
    int daysInCurrentMonth = getDaysInMonth(current.m_Year, current.m_Month);
    currentDays += daysInCurrentMonth;
    current.m_Month++;
    if (current.m_Month > 12) {
      current.m_Month = 1;
      current.m_Year++;
    }
  }
  
  return false;
}

#ifndef __PROGTEST__
int main ()
{
  long long int cnt;
  TDATE x;
   assert ( countFriday13 ( makeDate ( 1900,  1,  1 ), makeDate ( 2025,  5,  1 ), &cnt )
            && cnt == 215LL );
   assert ( countFriday13 ( makeDate ( 1900,  1,  1 ), makeDate ( 2025,  6,  1 ), &cnt )
            && cnt == 215LL );
   assert ( countFriday13 ( makeDate ( 1900,  1,  1 ), makeDate ( 2025,  5, 13 ), &cnt )
            && cnt == 215LL );
   assert ( countFriday13 ( makeDate ( 1900,  1,  1 ), makeDate ( 2025,  6, 13 ), &cnt )
            && cnt == 216LL );
   assert ( countFriday13 ( makeDate ( 1904,  1,  1 ), makeDate ( 2025,  5,  1 ), &cnt )
            && cnt == 207LL );
   assert ( countFriday13 ( makeDate ( 1904,  1,  1 ), makeDate ( 2025,  6,  1 ), &cnt )
            && cnt == 207LL );
   assert ( countFriday13 ( makeDate ( 1904,  1,  1 ), makeDate ( 2025,  5, 13 ), &cnt )
            && cnt == 207LL );
   assert ( countFriday13 ( makeDate ( 1904,  1,  1 ), makeDate ( 2025,  6, 13 ), &cnt )
            && cnt == 208LL );
   assert ( countFriday13 ( makeDate ( 1905,  2, 13 ), makeDate ( 2025,  5,  1 ), &cnt )
            && cnt == 205LL );
   assert ( countFriday13 ( makeDate ( 1905,  2, 13 ), makeDate ( 2025,  6,  1 ), &cnt )
            && cnt == 205LL );
   assert ( countFriday13 ( makeDate ( 1905,  2, 13 ), makeDate ( 2025,  5, 13 ), &cnt )
            && cnt == 205LL );
   assert ( countFriday13 ( makeDate ( 1905,  2, 13 ), makeDate ( 2025,  6, 13 ), &cnt )
            && cnt == 206LL );
   assert ( countFriday13 ( makeDate ( 1905,  1, 13 ), makeDate ( 2025,  5,  1 ), &cnt )
            && cnt == 206LL );
   assert ( countFriday13 ( makeDate ( 1905,  1, 13 ), makeDate ( 2025,  6,  1 ), &cnt )
            && cnt == 206LL );
   assert ( countFriday13 ( makeDate ( 1905,  1, 13 ), makeDate ( 2025,  5, 13 ), &cnt )
            && cnt == 206LL );
   assert ( countFriday13 ( makeDate ( 1905,  1, 13 ), makeDate ( 2025,  6, 13 ), &cnt )
            && cnt == 207LL );
   assert ( countFriday13 ( makeDate ( 2025,  5, 13 ), makeDate ( 2025,  5, 13 ), &cnt )
            && cnt == 0LL );
   assert ( countFriday13 ( makeDate ( 2025,  6, 13 ), makeDate ( 2025,  6, 13 ), &cnt )
            && cnt == 1LL );
   assert ( ! countFriday13 ( makeDate ( 2025, 11,  1 ), makeDate ( 2025, 10,  1 ), &cnt ) );
   assert ( ! countFriday13 ( makeDate ( 2025, 10, 32 ), makeDate ( 2025, 11, 10 ), &cnt ) );
   assert ( ! countFriday13 ( makeDate ( 2090,  2, 29 ), makeDate ( 2090,  2, 29 ), &cnt ) );
   assert ( countFriday13 ( makeDate ( 2096,  2, 29 ), makeDate ( 2096,  2, 29 ), &cnt )
            && cnt == 0LL );
   assert ( ! countFriday13 ( makeDate ( 2100,  2, 29 ), makeDate ( 2100,  2, 29 ), &cnt ) );
   assert ( countFriday13 ( makeDate ( 2000,  2, 29 ), makeDate ( 2000,  2, 29 ), &cnt )
            && cnt == 0LL );
   x = makeDate ( 2025, 6, 12 );
   assert ( prevFriday13 ( &x )
            && equalDate ( x, makeDate ( 2024, 12, 13 ) ) );
   x = makeDate ( 2025, 6, 12 );
   assert ( nextFriday13 ( &x )
            && equalDate ( x, makeDate ( 2025, 6, 13 ) ) );
   x = makeDate ( 2025, 6, 13 );
   assert ( prevFriday13 ( &x )
            && equalDate ( x, makeDate ( 2024, 12, 13 ) ) );
   x = makeDate ( 2025, 6, 13 );
   assert ( nextFriday13 ( &x )
            && equalDate ( x, makeDate ( 2026, 2, 13 ) ) );
   x = makeDate ( 2025, 6, 14 );
   assert ( prevFriday13 ( &x )
            && equalDate ( x, makeDate ( 2025, 6, 13 ) ) );
   x = makeDate ( 2025, 6, 14 );
   assert ( nextFriday13 ( &x )
            && equalDate ( x, makeDate ( 2026, 2, 13 ) ) );
   x = makeDate ( 2025, 2, 29 );
   assert ( ! prevFriday13 ( &x ) );
   x = makeDate ( 2025, 2, 29 );
   assert ( ! nextFriday13 ( &x ) );
   x = makeDate ( 1900, 3, 18 );
   assert ( ! prevFriday13 ( &x ) );
   x = makeDate ( 1900, 3, 18 );
   assert ( nextFriday13 ( &x )
            && equalDate ( x, makeDate ( 1900, 4, 13 ) ) );
  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */

