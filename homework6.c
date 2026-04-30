#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_RZ_LEN 1000

typedef struct {
    int cameraId;
    int monthIdx;   // 0..11
    int day;        // 1..31
    int hour;       // 0..23
    int minute;     // 0..59
    int timeKey;    // minutes since start of year (non-leap)
    char *rz;       // malloc'ed, null-terminated
} Report;

typedef struct {
    Report *data;
    size_t size;
    size_t cap;
} ReportVector;

typedef struct {
	char *rz;
	size_t start; // inclusive
	size_t end;   // exclusive
} RZGroup;

static const char *MONTHS[12] = {
    "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};
static const int DAYS_IN_MONTH[12] = {
    31,28,31,30,31,30,31,31,30,31,30,31
};
static const int DAYS_BEFORE_MONTH[12] = {
    0,31,59,90,120,151,181,212,243,273,304,334
};

static void initReportVector(ReportVector *v) {
    v->data = NULL;
    v->size = 0;
    v->cap = 0;
}

static int growReportVector(ReportVector *v) {
    size_t newCap = v->cap ? v->cap * 2 : 16;
    Report *nd = (Report *)realloc(v->data, newCap * sizeof(Report));
    if (!nd) return 0;
    v->data = nd;
    v->cap = newCap;
    return 1;
}

static int pushReport(ReportVector *v, const Report *r) {
    if (v->size == v->cap) {
        if (!growReportVector(v)) return 0;
    }
    v->data[v->size++] = *r;
    return 1;
}

static void freeReports(ReportVector *v) {
    for (size_t i = 0; i < v->size; ++i) {
        free(v->data[i].rz);
    }
    free(v->data);
    v->data = NULL;
    v->size = v->cap = 0;
}

static int monthIndex(const char *tok) {
    if (strlen(tok) != 3) return -1;
    if (!isupper((unsigned char)tok[0]) || !islower((unsigned char)tok[1]) || !islower((unsigned char)tok[2]))
        return -1;
    for (int i = 0; i < 12; ++i) {
        if (tok[0] == MONTHS[i][0] && tok[1] == MONTHS[i][1] && tok[2] == MONTHS[i][2])
            return i;
    }
    return -1;
}

static int validDay(int monthIdxVal, int day) {
    if (monthIdxVal < 0 || monthIdxVal >= 12) return 0;
    return day >= 1 && day <= DAYS_IN_MONTH[monthIdxVal];
}

static int computeTimeKey(int monthIdxVal, int day, int hour, int minute) {
    int daysBefore = DAYS_BEFORE_MONTH[monthIdxVal];
    int dayOffset = daysBefore + (day - 1);
    return dayOffset * 24 * 60 + hour * 60 + minute;
}

// Sorting and indexing helpers
static int cmpReportRZTimeCam(const void *a, const void *b) {
	const Report *ra = (const Report *)a;
	const Report *rb = (const Report *)b;
	int s = strcmp(ra->rz, rb->rz);
	if (s != 0) return s;
	if (ra->timeKey < rb->timeKey) return -1;
	if (ra->timeKey > rb->timeKey) return 1;
	if (ra->cameraId < rb->cameraId) return -1;
	if (ra->cameraId > rb->cameraId) return 1;
	return 0;
}

static RZGroup *buildGroups(const ReportVector *reports, size_t *outCount) {
	*outCount = 0;
	if (reports->size == 0) return NULL;
	RZGroup *groups = (RZGroup *)malloc(reports->size * sizeof(RZGroup));
	if (!groups) return NULL;
	size_t g = 0;
	size_t i = 0;
	while (i < reports->size) {
		size_t j = i + 1;
		while (j < reports->size && strcmp(reports->data[j].rz, reports->data[i].rz) == 0) {
			++j;
		}
		groups[g].rz = reports->data[i].rz;
		groups[g].start = i;
		groups[g].end = j;
		++g;
		i = j;
	}
	*outCount = g;
	return groups;
}

static int findGroupIndex(const RZGroup *groups, size_t n, const char *rz) {
	size_t lo = 0, hi = n;
	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		int s = strcmp(rz, groups[mid].rz);
		if (s == 0) return (int)mid;
		if (s < 0) hi = mid;
		else lo = mid + 1;
	}
	return -1;
}

static size_t lowerBoundTimeKey(const Report *data, size_t start, size_t end, int key) {
	size_t lo = start, hi = end;
	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		if (data[mid].timeKey < key) lo = mid + 1;
		else hi = mid;
	}
	return lo;
}

static size_t rangeLeftEq(const Report *data, size_t start, size_t idx) {
	int key = data[idx].timeKey;
	while (idx > start && data[idx - 1].timeKey == key) --idx;
	return idx;
}

static size_t rangeRightEq(const Report *data, size_t end, size_t idx) {
	int key = data[idx].timeKey;
	size_t j = idx + 1;
	while (j < end && data[j].timeKey == key) ++j;
	return j;
}

// Input helpers
static void skipSpaces(FILE *f) {
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (!isspace(c)) {
            ungetc(c, f);
            break;
        }
    }
}

static int readExpectedChar(FILE *f, char expected) {
    skipSpaces(f);
    int c = fgetc(f);
    if (c == EOF) return 0;
    if (c != expected) return 0;
    return 1;
}

static int peekChar(FILE *f) {
    int c = fgetc(f);
    if (c != EOF) ungetc(c, f);
    return c;
}

static int readIntToken(FILE *f, long *outVal) {
    skipSpaces(f);
    int c = fgetc(f);
    if (c == EOF) return 0;
    int neg = 0;
    if (c == '+' || c == '-') {
        neg = (c == '-');
        int next = fgetc(f);
        if (next == EOF) return 0;
        if (!isdigit(next)) {
            return 0;
        }
        long v = next - '0';
        while ((c = fgetc(f)) != EOF && isdigit(c)) {
            int d = c - '0';
            if (v > (LONG_MAX - d) / 10) {
                return 0; // overflow
            }
            v = v * 10 + d;
        }
        if (c != EOF) ungetc(c, f);
        if (neg) {
            if (-v > 0 && v != 0) return 0; // overflow to positive (paranoia)
            *outVal = -v;
        } else {
            *outVal = v;
        }
        return 1;
    } else if (isdigit(c)) {
        long v = c - '0';
        while ((c = fgetc(f)) != EOF && isdigit(c)) {
            int d = c - '0';
            if (v > (LONG_MAX - d) / 10) {
                return 0; // overflow
            }
            v = v * 10 + d;
        }
        if (c != EOF) ungetc(c, f);
        *outVal = v;
        return 1;
    } else {
        return 0;
    }
}

static int readNonSpaceToken(FILE *f, char *buf, size_t bufSize) {
    skipSpaces(f);
    int c = fgetc(f);
    if (c == EOF) return 0;
    size_t n = 0;
    int truncated = 0;
    if (isspace(c)) return 0;
    buf[n++] = (char)c;
    while ((c = fgetc(f)) != EOF && !isspace(c)) {
        if (n + 1 < bufSize) {
            buf[n++] = (char)c;
        } else {
            truncated = 1; /* keep consuming but remember overflow */
        }
    }
    if (c != EOF) ungetc(c, f);
    if (truncated) return -1;
    buf[n] = '\0';
    return 1;
}

static int readUnsignedIntToken(FILE *f, long *outVal) {
    skipSpaces(f);
    int c = fgetc(f);
    if (c == EOF) return 0;
    if (c == '+' || c == '-') {
        // explicit sign not allowed for day/hour/min
        return 0;
    }
    if (!isdigit(c)) {
        return 0;
    }
    long v = c - '0';
    while ((c = fgetc(f)) != EOF && isdigit(c)) {
        int d = c - '0';
        if (v > (LONG_MAX - d) / 10) {
            return 0; // overflow
        }
        v = v * 10 + d;
    }
    if (c != EOF) ungetc(c, f);
    *outVal = v;
    return 1;
}

static int parseReport(FILE *f, Report *out) {
    long cam;
    if (!readIntToken(f, &cam)) return 0;
    if (cam < 0 || cam > INT_MAX) return 0;
    if (!readExpectedChar(f, ':')) return 0;
    char rzTok[MAX_RZ_LEN + 1];
    int rzRes = readNonSpaceToken(f, rzTok, sizeof(rzTok));
    if (rzRes <= 0) return 0;
    if ((int)strlen(rzTok) > MAX_RZ_LEN) return 0;
    char monTok[16];
    int monRes = readNonSpaceToken(f, monTok, sizeof(monTok));
    if (monRes <= 0) return 0;
    int mIdx = monthIndex(monTok);
    if (mIdx < 0) return 0;
    long dayL;
    if (!readUnsignedIntToken(f, &dayL)) return 0;
    if (dayL < INT_MIN || dayL > INT_MAX) return 0;
    int day = (int)dayL;
    long hourL;
    if (!readUnsignedIntToken(f, &hourL)) return 0;
    if (hourL < 0 || hourL > 23) return 0;
    if (!readExpectedChar(f, ':')) return 0;
    long minL;
    if (!readUnsignedIntToken(f, &minL)) return 0;
    if (minL < 0 || minL > 59) return 0;
    if (!validDay(mIdx, day)) return 0;
    out->cameraId = (int)cam;
    out->monthIdx = mIdx;
    out->day = day;
    out->hour = (int)hourL;
    out->minute = (int)minL;
    out->timeKey = computeTimeKey(mIdx, day, (int)hourL, (int)minL);
    size_t rzLen = strlen(rzTok);
    char *rz = (char *)malloc(rzLen + 1);
    if (!rz) return 0;
    memcpy(rz, rzTok, rzLen + 1);
    out->rz = rz;
    return 1;
}

static int parseReportsList(FILE *f, ReportVector *reports) {
    if (!readExpectedChar(f, '{')) return 0;
    skipSpaces(f);
    int c = peekChar(f);
    if (c == '}') {
        fgetc(f);
        return -1;
    }
    for (;;) {
        Report r;
        if (!parseReport(f, &r)) return 0;
        if (!pushReport(reports, &r)) return 0;
        skipSpaces(f);
        int ch = fgetc(f);
        if (ch == ',') {
            continue;
        } else if (ch == '}') {
            break;
        } else if (ch == EOF) {
            return 0;
        } else {
            return 0;
        }
    }
    return reports->size > 0 ? 1 : -1;
}

static void printTime(int monthIdxVal, int day, int hour, int minute) {
    printf("%s %d %02d:%02d", MONTHS[monthIdxVal], day, hour, minute);
}

// we optimized the query handler using binary search over pre-sorted data
static void handleQueryFast(const ReportVector *reports, const RZGroup *groups, size_t numGroups, const char *rzTok, int mIdx, int day, int hour, int minute) {
	int gi = findGroupIndex(groups, numGroups, rzTok);
	if (gi < 0) {
		printf("> Car not found.\n");
		return;
	}
	const size_t start = groups[gi].start;
	const size_t end = groups[gi].end;
	const Report *data = reports->data;
	const int queryKey = computeTimeKey(mIdx, day, hour, minute);

	size_t pos = lowerBoundTimeKey(data, start, end, queryKey);
	if (pos < end && data[pos].timeKey == queryKey) {
		size_t l = rangeLeftEq(data, start, pos);
		size_t r = rangeRightEq(data, end, pos);
		size_t count = r - l;
		printf("> Exact: ");
		printTime(mIdx, day, hour, minute);
		printf(", %zux [", count);
		for (size_t i = l; i < r; ++i) {
			if (i > l) printf(", ");
			printf("%d", data[i].cameraId);
		}
		printf("]\n");
		return;
	}

	// we found the previous report
	if (pos == start) {
		printf("> Previous: N/A\n");
	} else {
		size_t pidx = pos - 1;
		size_t l = rangeLeftEq(data, start, pidx);
		size_t r = rangeRightEq(data, end, pidx);
		const Report *t = &data[l];
		printf("> Previous: ");
		printTime(t->monthIdx, t->day, t->hour, t->minute);
		printf(", %zux [", r - l);
		for (size_t i = l; i < r; ++i) {
			if (i > l) printf(", ");
			printf("%d", data[i].cameraId);
		}
		printf("]\n");
	}

	// we found the next report
	if (pos == end) {
		printf("> Next: N/A\n");
	} else {
		size_t nidx = pos;
		size_t l = rangeLeftEq(data, start, nidx);
		size_t r = rangeRightEq(data, end, nidx);
		const Report *t = &data[l];
		printf("> Next: ");
		printTime(t->monthIdx, t->day, t->hour, t->minute);
		printf(", %zux [", r - l);
		for (size_t i = l; i < r; ++i) {
			if (i > l) printf(", ");
			printf("%d", data[i].cameraId);
		}
		printf("]\n");
	}
}

int main(void) {
    ReportVector reports;
    initReportVector(&reports);

	// we set larger stdio buffers for faster I/O
	setvbuf(stdin, NULL, _IOFBF, 1 << 20);
	setvbuf(stdout, NULL, _IOFBF, 1 << 20);

    printf("Camera reports:\n");
    int pr = parseReportsList(stdin, &reports);
    if (pr <= 0) {
        printf("Invalid input.\n");
        freeReports(&reports);
        return 0;
    }

	// just sort and build index for fast queries
	qsort(reports.data, reports.size, sizeof(Report), cmpReportRZTimeCam);
	size_t numGroups = 0;
	RZGroup *groups = buildGroups(&reports, &numGroups);
	if (reports.size > 0 && groups == NULL) {
		printf("Invalid input.\n");
		freeReports(&reports);
		return 0;
	}

    printf("Search:\n");

    for (;;) {
        skipSpaces(stdin);
        int c = peekChar(stdin);
        if (c == EOF) break;
        char rzTok[MAX_RZ_LEN + 1];
        int rzRes = readNonSpaceToken(stdin, rzTok, sizeof(rzTok));
        if (rzRes <= 0) {
            printf("Invalid input.\n");
			free(groups);
			freeReports(&reports);
            return 0;
        }
        if ((int)strlen(rzTok) > MAX_RZ_LEN) {
            printf("Invalid input.\n");
			free(groups);
			freeReports(&reports);
            return 0;
        }
        char monTok[16];
        int monRes = readNonSpaceToken(stdin, monTok, sizeof(monTok));
        if (monRes <= 0) {
            printf("Invalid input.\n");
			free(groups);
			freeReports(&reports);
            return 0;
        }
        int mIdx = monthIndex(monTok);
        if (mIdx < 0) {
            printf("Invalid input.\n");
			free(groups);
			freeReports(&reports);
            return 0;
        }
        long dayL;
        if (!readUnsignedIntToken(stdin, &dayL)) {
            printf("Invalid input.\n");
			free(groups);
			freeReports(&reports);
            return 0;
        }
        if (dayL < INT_MIN || dayL > INT_MAX) {
            printf("Invalid input.\n");
			free(groups);
			freeReports(&reports);
            return 0;
        }
        int day = (int)dayL;
        long hourL;
        if (!readUnsignedIntToken(stdin, &hourL) || hourL < 0 || hourL > 23) {
            printf("Invalid input.\n");
			free(groups);
			freeReports(&reports);
            return 0;
        }
        if (!readExpectedChar(stdin, ':')) {
            printf("Invalid input.\n");
			free(groups);
			freeReports(&reports);
            return 0;
        }
        long minL;
        if (!readUnsignedIntToken(stdin, &minL) || minL < 0 || minL > 59) {
            printf("Invalid input.\n");
			free(groups);
			freeReports(&reports);
            return 0;
        }
        if (!validDay(mIdx, day)) {
            printf("Invalid input.\n");
			free(groups);
			freeReports(&reports);
            return 0;
        }
		handleQueryFast(&reports, groups, numGroups, rzTok, mIdx, day, (int)hourL, (int)minL);
    }

	free(groups);
    freeReports(&reports);
    return 0;
}
