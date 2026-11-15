#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>

// ===== SHARED: settings, Product structure and helper functions =====

enum Category { CLOTH, ELECTRONIC, FOOD };
const char *const CAT_NAME[] = {"Cloth", "Electronic", "Food"};
const int CAT_BASE[] = {100, 200, 300};

const int MAX_PRODUCTS = 200;
const double PRIORITY_FEE = 50.0;
const char *const MANAGER_PASS = "admin123";
const char *const NO_EXPIRY = "NONE";

const char *const INV_FILE     = "inventory.txt";
const char *const ORDERS_FILE  = "orders.txt";
const char *const SALES_FILE   = "sales.txt";
const char *const EXPIRED_FILE = "expired.txt";

struct Product {
    int id;
    char name[32];
    int cat;
    int qty;
    double cost;
    double price;
    char expiry[11];
};

inline void trim(char *s) {
    int start = 0;
    while (s[start] != '\0' && isspace((unsigned char)s[start])) start++;
    memmove(s, s + start, strlen(s + start) + 1);
    int len = (int)strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) { s[len - 1] = '\0'; len--; }
}

inline void readLine(char *text, int size) {
    if (fgets(text, size, stdin) == NULL) {
        printf("\nInput closed. Bye!\n");
        exit(0);
    }
    if (strchr(text, '\n') == NULL) {
        int c = getchar();
        while (c != '\n' && c != EOF) c = getchar();
    }
    text[strcspn(text, "\r\n")] = '\0';
    for (int i = 0; text[i] != '\0'; i++)
        if (text[i] == '|') text[i] = '/';
    trim(text);
}

inline int readInt() {
    char buf[64];
    char *end;
    while (true) {
        readLine(buf, sizeof(buf));
        long value = strtol(buf, &end, 10);
        if (buf[0] != '\0' && *end == '\0') return (int)value;
        printf("Please type a whole number: ");
    }
}

inline double readDouble() {
    char buf[64];
    char *end;
    while (true) {
        readLine(buf, sizeof(buf));
        double value = strtod(buf, &end);
        if (buf[0] != '\0' && *end == '\0' && value >= 0) return value;
        printf("Please type a valid amount: ");
    }
}

inline bool containsNoCase(const char *text, const char *word) {
    int n = (int)strlen(text);
    int m = (int)strlen(word);
    for (int start = 0; start + m <= n; start++) {
        int j = 0;
        while (j < m && tolower((unsigned char)text[start + j]) == tolower((unsigned char)word[j])) j++;
        if (j == m) return true;
    }
    return false;
}

inline bool sameNoCase(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++; b++;
    }
    return *a == *b;
}

inline void addDays(int days, char *out) {
    time_t now = time(NULL);
    tm date = *localtime(&now);
    date.tm_mday = date.tm_mday + days;
    mktime(&date);
    strftime(out, 11, "%Y-%m-%d", &date);
}