#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define MAX_SECTIONS 10000
#define MAX_UPDATES 300000

//we define the date
typedef struct {
    unsigned year;
    unsigned short month;
    unsigned short day;
} Date;

//we define the cost update
typedef struct {
    long long days;
    int section;
    int cost;
} CostUpdate;

//we define the partition
typedef struct {
    int start1, end1;
    int start2, end2;
} Partition;

//we check if the year is a leap year
static bool isLeapYear(unsigned year) {
    if (year % 4000 == 0) return false;
    if (year % 400 == 0) return true;
    if (year % 100 == 0) return false;
    if (year % 4 == 0) return true;
    return false;
}
//we get the days in the month
static int getDaysInMonth(unsigned year, unsigned short month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) return 29;
    return days[month - 1];
}
//we convert the date to days
static long long dateToDays(Date d) {
    long long y = (long long)d.year;
    long long yearsSince1900 = y - 1900LL;
    long long a4 = ((y - 1) / 4) - (1899 / 4);
    long long a100 = ((y - 1) / 100) - (1899 / 100);
    long long a400 = ((y - 1) / 400) - (1899 / 400);
    long long a4000 = ((y - 1) / 4000) - (1899 / 4000);
    long long leapCount = a4 - a100 + a400 - a4000;
    long long total = yearsSince1900 * 365LL + leapCount;
    static const int monthDays[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    total += monthDays[d.month];
    if (d.month > 2 && isLeapYear(d.year)) total++;
    total += (long long)d.day;
    return total;
}
//we check if the date is valid
static bool isValidDate(Date d) {
    if (d.year < 1900) return false;
    if (d.month < 1 || d.month > 12) return false;
    if (d.day < 1) return false;
    if (d.day > getDaysInMonth(d.year, d.month)) return false;
    return true;
}
//we parse the date
static bool parseDate(const char *str, Date *out) {
    if (!str || !*str) return false;
    int y, m, d;
    char extra;
    int result = sscanf(str, "%d-%d-%d%c", &y, &m, &d, &extra);
    if (result == 4) return false;
    if (result != 3) return false;
    if (strlen(str) != 10) return false;
    if (str[4] != '-' || str[7] != '-') return false;
    out->year = (unsigned)y;
    out->month = (unsigned short)m;
    out->day = (unsigned short)d;
    return isValidDate(*out);
}
//we compare the dates
static int compareDates(Date a, Date b) {
    long long da = dateToDays(a);
    long long db = dateToDays(b);
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}
//well, we did this in the previous homework
static bool parseInt(const char **str, int *val) {
    while (**str && isspace(**str)) (*str)++;
    if (!**str) return false;
    char *end;
    long v = strtol(*str, &end, 10);
    if (v < 0 || v > 2147483647) return false;
    if (end == *str) return false;
    *val = (int)v;
    *str = end;
    return true;
}
//we try to parse the input and parse the cost list
static bool parsePositiveInt(const char **str, int *val) {
    while (**str && isspace(**str)) (*str)++;
    if (!**str) return false;
    char *end;
    long v = strtol(*str, &end, 10);
    if (v <= 0 || v > 2147483647) return false;
    if (end == *str) return false;
    *val = (int)v;
    *str = end;
    return true;
}
//we parse the cost list
static bool parseCostList(int *sections, int *n) {
    char buffer[200000];
    buffer[0] = '\0';
    bool foundClose = false;
    while (!foundClose && fgets(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), stdin)) {
        for (const char *q = buffer; *q; q++) {
            if (*q == '}') { foundClose = true; break; }
        }
        if (foundClose) break;
        if (strlen(buffer) > sizeof(buffer) - 1000) return false;
    }
    const char *p = buffer;
    while (*p && *p != '{') p++;
    if (*p != '{') return false;
    p++;
    *n = 0;
    bool firstItem = true;
    while (*n < MAX_SECTIONS) {
        while (*p && isspace(*p)) p++;
        if (*p == '}') break;
        if (!*p) return false;
        if (!firstItem) {
            if (*p != ',') return false;
            p++;
            while (*p && isspace(*p)) p++;
        }
        firstItem = false;
        if (*p == '}') break;
        int cost;
        if (!parsePositiveInt(&p, &cost)) return false;
        sections[(*n)++] = cost;
        while (*p && isspace(*p)) p++;
    }
    while (*p && *p != '}') {
        if (!isspace(*p) && *p != ',') return false;
        p++;
    }
    return (*p == '}') && (*n >= 2) && (*n <= MAX_SECTIONS);
}
//we read the update and parse the date and the section
static bool readUpdate(const char *line, CostUpdate *upd, int maxSection, Date *lastDate, Date *updDate) {
    const char *p = line;
    while (*p && isspace(*p)) p++;
    if (*p != '=') return false;
    p++;
    
    Date date;
    char dateStr[20];
    int i = 0;
    while (*p && isspace(*p)) p++;
    while (*p && i < 19 && *p != ' ') {
        dateStr[i++] = *p++;
    }
    dateStr[i] = '\0';
    if (i == 0) return false;
    if (!parseDate(dateStr, &date)) return false;
    
    if (lastDate && compareDates(date, *lastDate) <= 0) return false;
    
    while (*p && isspace(*p)) p++;
    if (!*p) return false;
    int section;
    if (!parseInt(&p, &section)) return false;
    if (section < 0 || section >= maxSection) return false;
    
    while (*p && isspace(*p)) p++;
    if (*p != ':') return false;
    p++;
    
    while (*p && isspace(*p)) p++;
    if (!*p) return false;
    int cost;
    if (!parsePositiveInt(&p, &cost)) return false;
    
    while (*p && isspace(*p)) p++;
    if (*p && *p != '\n' && *p != '\0' && *p != '\r') return false;
    
    upd->days = dateToDays(date);
    upd->section = section;
    upd->cost = cost;
    if (lastDate) *lastDate = date;
    if (updDate) *updDate = date;
    return true;
}
//we calculate the costs for the given time interval
static void calculateCosts(int *initialCosts, int n, CostUpdate *updates, int numUpdates,
                           long long fromDays, long long toDays, long long *costs) {
    long long sectionCosts[MAX_SECTIONS];
    long long sectionLastDay[MAX_SECTIONS];
    int i, j;
    
    for (i = 0; i < n; i++) {
        costs[i] = 0;
        sectionCosts[i] = (long long)initialCosts[i];
        sectionLastDay[i] = fromDays;
    }
    
    // single pass through updates, process all sections at once
    // this is faster than checking each section separately
    for (j = 0; j < numUpdates; j++) {
        long long updDays = updates[j].days;
        if (updDays > toDays) break; // updates are sorted, can stop early
        
        int sec = updates[j].section;
        if (sec < 0 || sec >= n) continue;
        
        if (updDays <= fromDays) {
            sectionCosts[sec] = updates[j].cost;
        } else {
            // calculate cost for days between updates
            long long prevDay = sectionLastDay[sec];
            if (updDays > prevDay) {
                costs[sec] += sectionCosts[sec] * (updDays - prevDay);
            }
            sectionCosts[sec] = updates[j].cost;
            sectionLastDay[sec] = updDays;
        }
    }
    
    // add remaining days after last update
    for (i = 0; i < n; i++) {
        costs[i] += sectionCosts[i] * (toDays - sectionLastDay[i] + 1);
    }
}
//we find the optimal partitions and calculate the difference
static void findOptimalPartitions(long long *costs, int n, Partition *results, int *numResults, long long *minDiff) {
    long long totalCost = 0;
    int i;
    for (i = 0; i < n; i++) {
        totalCost += costs[i];
    }
    
    *minDiff = totalCost + 1;
    *numResults = 0;
    
    // try all possible partitions
    for (int start = 0; start < n; start++) {
        long long companyA = 0;
        int end = (start - 1 + n) % n;
        
        for (int len = 1; len < n; len++) {
            end = (end + 1) % n;
            companyA += costs[end];
            
            long long companyB = totalCost - companyA;
            long long diff = (companyA > companyB) ? (companyA - companyB) : (companyB - companyA);
            
            if (diff < *minDiff) {
                *minDiff = diff;
                *numResults = 0; // found better solution, reset
            }
            
            if (diff == *minDiff) {
                // calculate second partition
                int start2, end2;
                if (start <= end) {
                    if (end == n - 1) {
                        start2 = 0;
                        end2 = start - 1;
                    } else {
                        start2 = end + 1;
                        end2 = (start == 0) ? (n - 1) : (start - 1);
                    }
                } else {
                    start2 = end + 1;
                    end2 = start - 1;
                }
                
                // normalize to avoid duplicates
                int s1, e1, s2, e2;
                if (start > start2 || (start == start2 && end > end2)) {
                    s1 = start2;
                    e1 = end2;
                    s2 = start;
                    e2 = end;
                } else {
                    s1 = start;
                    e1 = end;
                    s2 = start2;
                    e2 = end2;
                }
                
                // check if duplicate
                int r;
                for (r = 0; r < *numResults; r++) {
                    if (results[r].start1 == s1 && results[r].end1 == e1 &&
                        results[r].start2 == s2 && results[r].end2 == e2) {
                        break;
                    }
                }
                
                if (r == *numResults && *numResults < 1000) {
                    results[*numResults].start1 = s1;
                    results[*numResults].end1 = e1;
                    results[*numResults].start2 = s2;
                    results[*numResults].end2 = e2;
                    (*numResults)++;
                }
            }
        }
    }
}
//we process the query and calculate the costs
static bool processQuery(const char *line, int *initialCosts, int n, CostUpdate *updates, int numUpdates) {
    const char *p = line;
    while (*p && isspace(*p)) p++;
    if (*p != '?') return false;
    p++;
    Date date1, date2;
    char dateStr1[20], dateStr2[20];
    int i = 0;
    while (*p && isspace(*p)) p++;
    
    while (*p && i < 19 && *p != ' ' && *p != '\n') dateStr1[i++] = *p++;
    
    dateStr1[i] = '\0';
    
    if (i == 0 || !parseDate(dateStr1, &date1)) return false;
    
    i = 0;
    
    while (*p && isspace(*p)) p++;
    
    while (*p && i < 19 && *p != ' ' && *p != '\n' && *p != '\0') dateStr2[i++] = *p++;
    
    dateStr2[i] = '\0';
    
    if (i == 0 || !parseDate(dateStr2, &date2)) return false;
    
    long long fromDays = dateToDays(date1);
    long long toDays = dateToDays(date2);
    // I tried tocompare days directly instead of calling compareDates (faster)
    if (fromDays > toDays) return false;
    
    while (*p && isspace(*p)) p++;
    
    if (*p && *p != '\n' && *p != '\0' && *p != '\r') return false;
    
    long long costs[MAX_SECTIONS];
    
    calculateCosts(initialCosts, n, updates, numUpdates, fromDays, toDays, costs);
    
    Partition results[1000];
    int numResults;
    long long minDiff;
    
    findOptimalPartitions(costs, n, results, &numResults, &minDiff);
    
    printf("Difference: %lld, options: %d\n", minDiff, numResults);
    
    for (int i = 0; i < numResults; i++) {
        printf("* %d - %d, %d - %d\n", results[i].start1, results[i].end1, results[i].start2, results[i].end2);
    }
    return true;
}

int main(void) {
    printf("Daily cost:\n");
    int initialCosts[MAX_SECTIONS];
    int n;
    if (!parseCostList(initialCosts, &n)) {
        printf("Invalid input.\n");
        return 1;
    }
    CostUpdate updates[MAX_UPDATES];
    int numUpdates = 0;
    Date lastUpdateDate = {0, 0, 0};
    bool hasLastDate = false;
    char line[1000];
    while (fgets(line, sizeof(line), stdin)) {
        bool isEmpty = true;
        for (const char *p = line; *p; p++) {
            if (!isspace(*p)) { isEmpty = false; break; }
        }
        if (isEmpty) continue;
        const char *p = line;
        while (*p && isspace(*p)) p++;
        if (*p == '=') {
            CostUpdate upd;
            Date last;
            if (hasLastDate) {
                last = lastUpdateDate;
            } else {
                last.year = 1900;
                last.month = 1;
                last.day = 1;
            }
            Date updateDate;
            if (!readUpdate(line, &upd, n, hasLastDate ? &last : NULL, &updateDate)) {
                printf("Invalid input.\n");
                return 1;
            }
            if (numUpdates >= MAX_UPDATES) {
                printf("Invalid input.\n");
                return 1;
            }
            updates[numUpdates++] = upd;
            lastUpdateDate = updateDate;
            hasLastDate = true;
        } else if (*p == '?') {
            if (!processQuery(line, initialCosts, n, updates, numUpdates)) {
                printf("Invalid input.\n");
                return 1;
            }
        } else {
            printf("Invalid input.\n");
            return 1;
        }
    }
    return 0;
}
