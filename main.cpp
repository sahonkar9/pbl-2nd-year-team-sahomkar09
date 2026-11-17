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
const char *const USERS_FILE   = "userpass.txt";

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

inline bool validDate(const char *text) {
    int y, m, d;
    if (strlen(text) != 10 || text[4] != '-' || text[7] != '-') return false;
    if (sscanf(text, "%d-%d-%d", &y, &m, &d) != 3) return false;
    tm date = tm();
    date.tm_year = y - 1900; date.tm_mon = m - 1; date.tm_mday = d; date.tm_hour = 12;
    mktime(&date);
    return y >= 2000 && date.tm_mon == m - 1 && date.tm_mday == d;
}

inline int daysLeft(const char *text) {
    int y, m, d;
    sscanf(text, "%d-%d-%d", &y, &m, &d);
    tm expiry = tm();
    expiry.tm_year = y - 1900; expiry.tm_mon = m - 1; expiry.tm_mday = d; expiry.tm_hour = 12;

    time_t now = time(NULL);
    tm today = *localtime(&now);
    today.tm_hour = 12; today.tm_min = 0; today.tm_sec = 0;

    double days = difftime(mktime(&expiry), mktime(&today)) / (60.0 * 60.0 * 24.0);
    if (days < 0) return (int)(days - 0.5);
    return (int)(days + 0.5);
}
// ===== AYUSHMAN: AVL TREE - insert, search, delete and rotations =====
// DSA: Self-balancing BST. AVL keeps product IDs sorted and tree height balanced.
struct Node {
    Product data;
    Node *left;
    Node *right;
    int height;
};

// OOP: Encapsulation - tree details are private; public methods provide access.
class AVL {
    Node *root;

    int heightOf(Node *n) {
        if (n == NULL) return 0;
        return n->height;
    }
    void updateHeight(Node *n) {
        int l = heightOf(n->left);
        int r = heightOf(n->right);
        n->height = 1 + (l > r ? l : r);
    }

    // Positive balance: left-heavy; negative: right-heavy.
    int balanceOf(Node *n) {
        return heightOf(n->left) - heightOf(n->right);
    }

    // AVL rotations restore balance after insertion or deletion.
    Node *rotateRight(Node *y) {
        Node *x = y->left;
        y->left = x->right;
        x->right = y;
        updateHeight(y);
        updateHeight(x);
        return x;
    }

    Node *rotateLeft(Node *x) {
        Node *y = x->right;
        x->right = y->left;
        y->left = x;
        updateHeight(x);
        updateHeight(y);
        return y;
    }

    Node *rebalance(Node *n) {
        updateHeight(n);
        int balance = balanceOf(n);

        if (balance > 1) {
            if (balanceOf(n->left) < 0)
                n->left = rotateLeft(n->left);
            return rotateRight(n);
        }
        if (balance < -1) {
            if (balanceOf(n->right) > 0)
                n->right = rotateRight(n->right);
            return rotateLeft(n);
        }
        return n;
    }

    // Insert by ID, then rebalance while returning up the tree.
    Node *insertNode(Node *n, const Product &p, bool &added) {
        if (n == NULL) {
            Node *box = new Node;
            box->data = p;
            box->left = NULL;
            box->right = NULL;
            box->height = 1;
            added = true;
            return box;
        }
        if (p.id < n->data.id)      n->left  = insertNode(n->left, p, added);
        else if (p.id > n->data.id) n->right = insertNode(n->right, p, added);
        else return n;
        return rebalance(n);
    }

    // Delete by ID and rebalance the affected path.
    Node *deleteNode(Node *n, int id, bool &deleted) {
        if (n == NULL) return NULL;

        if (id < n->data.id)      n->left  = deleteNode(n->left, id, deleted);
        else if (id > n->data.id) n->right = deleteNode(n->right, id, deleted);
        else {
            deleted = true;
            if (n->left == NULL || n->right == NULL) {
                Node *child = (n->left != NULL) ? n->left : n->right;
                delete n;
                return child;
            }

            Node *next = n->right;
            while (next->left != NULL) next = next->left;
            n->data = next->data;
            bool dummy = false;
            n->right = deleteNode(n->right, next->data.id, dummy);
        }
        return rebalance(n);
    }

    void fillArray(Node *n, Product list[], int &count) {
        if (n == NULL || count >= MAX_PRODUCTS) return;
        fillArray(n->left, list, count);
        if (count < MAX_PRODUCTS) { list[count] = n->data; count++; }
        fillArray(n->right, list, count);
    }

    void freeTree(Node *n) {
        if (n == NULL) return;
        freeTree(n->left);
        freeTree(n->right);
        delete n;
    }

public:
    AVL()  { root = NULL; }
    ~AVL() { freeTree(root); }

    bool insert(const Product &p) {
        bool added = false;
        root = insertNode(root, p, added);
        return added;
    }

    bool remove(int id) {
        bool deleted = false;
        root = deleteNode(root, id, deleted);
        return deleted;
    }

    Product *find(int id) {
        Node *n = root;
        while (n != NULL) {
            if (id == n->data.id) return &n->data;
            if (id < n->data.id) n = n->left;
            else                 n = n->right;
        }
        return NULL;
    }

    int toArray(Product list[]) {
        int count = 0;
        fillArray(root, list, count);
        return count;
    }
};

// ===== AYUSHMAN: INVENTORY - add, restock, search and delete =====
// OOP: Encapsulation - inventory data and operations are grouped inside a class.
class Inventory {
    AVL tree;

    void printHeader(bool manager) {
        if (manager) printf("  %-5s%-14s%-6s%-8s%-8s%s\n", "ID", "Name", "Qty", "Cost", "Price", "Expiry");
        else         printf("  %-5s%-14s%-6s%-8s%s\n",     "ID", "Name", "Qty", "Price", "Expiry");
    }
    void printRow(const Product &p, bool manager) {
        if (manager) printf("  %-5d%-14s%-6d%-8.0f%-8.0f%s\n", p.id, p.name, p.qty, p.cost, p.price, p.expiry);
        else         printf("  %-5d%-14s%-6d%-8.0f%s\n",       p.id, p.name, p.qty, p.price, p.expiry);
    }

    int nextId(int cat) {
        Product list[MAX_PRODUCTS];
        int n = tree.toArray(list);
        int biggest = CAT_BASE[cat];
        for (int i = 0; i < n; i++)
            if (list[i].cat == cat && list[i].id > biggest) biggest = list[i].id;
        return biggest + 1;
    }

    void addDefault(const char *name, int cat, int qty, double cost, double price, int daysToExpire) {
        Product p = Product();
        p.id = nextId(cat);
        snprintf(p.name, sizeof(p.name), "%s", name);
        p.cat = cat;
        p.qty = qty;
        p.cost = cost;
        p.price = price;
        if (daysToExpire > 0) addDays(daysToExpire, p.expiry);
        else                  snprintf(p.expiry, sizeof(p.expiry), "%s", NO_EXPIRY);
        tree.insert(p);
    }
    void loadDefaultShop() {

        addDefault("Socks",   CLOTH,       10,   40,     60,    0);
        addDefault("Pant",    CLOTH,       15,   400,    600,   0);
        addDefault("Shirt",   CLOTH,       5,    300,    500,   0);
        addDefault("Shoes",   CLOTH,       8,    800,    1200,  0);
        addDefault("Coat",    CLOTH,       6,    1200,   1800,  0);

        addDefault("Phone",   ELECTRONIC,  10,   12000,  15000, 0);
        addDefault("Camera",  ELECTRONIC,  5,    20000,  25000, 0);
        addDefault("Watch",   ELECTRONIC,  12,   1500,   2500,  0);
        addDefault("Laptop",  ELECTRONIC,  7,    35000,  45000, 0);

        addDefault("Atta",    FOOD,        20,   30,     40,    30);
        addDefault("Biscuit", FOOD,        40,   8,      10,    10);
        addDefault("Dal",     FOOD,        25,   90,     120,   60);
        save();
    }

    void restockOld(int id) {
        Product *p = tree.find(id);
        if (p == NULL) { printf("Product not found.\n"); return; }
        printf("%s has %d. How many to add? ", p->name, p->qty);
        int extra = readInt();
        if (extra <= 0) { printf("Quantity must be more than 0.\n"); return; }
        p->qty = p->qty + extra;
        save();
        printf("Done. %s now has %d.\n", p->name, p->qty);
    }
    void addNew() {
        Product list[MAX_PRODUCTS];
        if (tree.toArray(list) >= MAX_PRODUCTS) { printf("Shop is full.\n"); return; }

        Product p = Product();
        printf("Category (1=Cloth 2=Electronic 3=Food): ");
        int c = readInt();
        if (c < 1 || c > 3) { printf("Invalid category.\n"); return; }
        p.cat = c - 1;
        p.id = nextId(p.cat);

        printf("Name: ");          readLine(p.name, sizeof(p.name));
        printf("Quantity: ");      p.qty = readInt();
        printf("Cost price: ");    p.cost = readDouble();
        printf("Selling price: "); p.price = readDouble();
        if (p.name[0] == '\0' || p.qty <= 0) { printf("Name and quantity are needed.\n"); return; }

        if (p.cat == FOOD) {
            char date[64];
            printf("Expiry date (YYYY-MM-DD): ");
            readLine(date, sizeof(date));
            if (!validDate(date)) { printf("That is not a valid date.\n"); return; }
            memcpy(p.expiry, date, 11);
        } else {
            snprintf(p.expiry, sizeof(p.expiry), "%s", NO_EXPIRY);
        }

        tree.insert(p);
        save();
        printf("Added %s with ID %d.\n", p.name, p.id);
    }

public:

    Inventory() {
        FILE *f = fopen(INV_FILE, "r");
        if (f == NULL) { loadDefaultShop(); return; }

        char line[256];
        while (fgets(line, sizeof(line), f) != NULL) {
            Product p = Product();

            int got = sscanf(line, "%d|%31[^|]|%d|%d|%lf|%lf|%10s",
                             &p.id, p.name, &p.cat, &p.qty, &p.cost, &p.price, p.expiry);
            if (got == 7) tree.insert(p);
        }
        fclose(f);
    }

    void save() {
        Product list[MAX_PRODUCTS];
        int n = tree.toArray(list);
        FILE *f = fopen(INV_FILE, "w");
        if (f == NULL) { printf("Warning: could not save inventory.\n"); return; }
        for (int i = 0; i < n; i++)
            fprintf(f, "%d|%s|%d|%d|%.2f|%.2f|%s\n", list[i].id, list[i].name, list[i].cat,
                    list[i].qty, list[i].cost, list[i].price, list[i].expiry);
        fclose(f);
    }

    Product *find(int id)           { return tree.find(id); }
    bool remove(int id)             { return tree.remove(id); }
    int getAll(Product list[])      { return tree.toArray(list); }

    void showAll(bool manager) {
        Product list[MAX_PRODUCTS];
        int n = tree.toArray(list);
        printf("\n========== TOTAL INVENTORY ==========\n");
        int lastCat = -1;
        for (int i = 0; i < n; i++) {
            if (list[i].cat != lastCat) {
                printf("\n[%s]\n", CAT_NAME[list[i].cat]);
                printHeader(manager);
                lastCat = list[i].cat;
            }
            printRow(list[i], manager);
        }
    }
    void showOne(const Product &p, bool manager) {
        printHeader(manager);
        printRow(p, manager);
    }
    // Prints several products under one header (used by recommendations).
    void showList(const Product list[], int n, bool manager) {
        printHeader(manager);
        for (int i = 0; i < n; i++) printRow(list[i], manager);
    }

    void searchByName(const char *word, bool manager) {
        Product list[MAX_PRODUCTS];
        int n = tree.toArray(list);
        int found = 0;
        for (int i = 0; i < n; i++) {
            if (containsNoCase(list[i].name, word) || containsNoCase(CAT_NAME[list[i].cat], word)) {
                if (found == 0) printHeader(manager);
                printRow(list[i], manager);
                found++;
            }
        }
        if (found == 0) printf("No matching product.\n");
    }

    void addProduct() {
        printf("Enter an existing product ID to restock (or 0 for a NEW product): ");
        int id = readInt();
        if (id != 0) restockOld(id);
        else         addNew();
    }
    void deleteProduct() {
        printf("Product ID to delete: ");
        int id = readInt();
        Product *p = tree.find(id);
        if (p == NULL) { printf("Product not found.\n"); return; }
        char name[32];
        snprintf(name, sizeof(name), "%s", p->name);
        tree.remove(id);
        save();
        printf("Deleted %s (ID %d).\n", name, id);
    }
};


// ===== AMAN: SALES AND PROFIT =====
// OOP: Encapsulation - sales totals and report functions belong to one class.
class Sales {
    double sales;
    double fees;
    double cost;
    double losses;
    int orders;
    int units;

public:

    Sales() {
        sales = fees = cost = losses = 0;
        orders = units = 0;

        FILE *f = fopen(SALES_FILE, "r");
        if (f == NULL) return;

        char line[256], name[32];
        int id, qty;
        double rev, c, fee;
        while (fgets(line, sizeof(line), f) != NULL) {

            if (sscanf(line, "S|%d|%31[^|]|%d|%lf|%lf|%lf", &id, name, &qty, &rev, &c, &fee) == 6) {
                sales += rev; cost += c; fees += fee; units += qty; orders++;
            }

            else if (sscanf(line, "L|%31[^|]|%d|%lf", name, &qty, &c) == 3) {
                losses += c;
            }
        }
        fclose(f);
    }

    void recordSale(int orderId, const char *name, int qty, double rev, double c, double fee) {
        FILE *f = fopen(SALES_FILE, "a");
        if (f != NULL) {
            fprintf(f, "S|%d|%s|%d|%.2f|%.2f|%.2f\n", orderId, name, qty, rev, c, fee);
            fclose(f);
        }
        sales += rev; cost += c; fees += fee; units += qty; orders++;
    }

    void recordLoss(const char *name, int qty, double c) {
        FILE *f = fopen(SALES_FILE, "a");
        if (f != NULL) {
            fprintf(f, "L|%s|%d|%.2f\n", name, qty, c);
            fclose(f);
        }
        losses += c;
    }

    void showReport() {
        double income = sales + fees;
        printf("\n========== SALES & PROFIT ==========\n");
        printf("Orders delivered : %d  (%d items)\n", orders, units);
        printf("Item sales       : Rs %.2f\n", sales);
        printf("Priority fees    : Rs %.2f\n", fees);
        printf("Total income     : Rs %.2f\n", income);
        printf("Cost of goods    : Rs %.2f\n", cost);
        printf("Expiry losses    : Rs %.2f\n", losses);
        printf("PROFIT           : Rs %.2f\n", income - cost - losses);
    }
};

// ===== NIKHIL: MERGE SORT AND EXPIRY MANAGEMENT =====
// DSA: Merge Sort arranges food products by expiry date.
inline void mergeSort(Product a[], Product temp[], int start, int end) {
    if (end - start < 2) return;

    int middle = (start + end) / 2;
    mergeSort(a, temp, start, middle);
    mergeSort(a, temp, middle, end);

    int i = start;
    int j = middle;
    int k = start;
    while (i < middle && j < end) {
        if (strcmp(a[i].expiry, a[j].expiry) <= 0) { temp[k] = a[i]; i++; }
        else                                       { temp[k] = a[j]; j++; }
        k++;
    }
    while (i < middle) { temp[k] = a[i]; i++; k++; }
    while (j < end)    { temp[k] = a[j]; j++; k++; }

    for (k = start; k < end; k++) a[k] = temp[k];
}

inline int getFoodSorted(Inventory &inv, Product foods[]) {
    Product all[MAX_PRODUCTS];
    Product temp[MAX_PRODUCTS];
    int total = inv.getAll(all);

    int count = 0;
    for (int i = 0; i < total; i++)
        if (all[i].cat == FOOD) { foods[count] = all[i]; count++; }

    mergeSort(foods, temp, 0, count);
    return count;
}

inline int removeExpired(Inventory &inv, Sales &sales) {
    Product foods[MAX_PRODUCTS];
    int count = getFoodSorted(inv, foods);
    int removed = 0;

    for (int i = 0; i < count; i++) {
        Product &p = foods[i];
        if (daysLeft(p.expiry) >= 0) break;

        double lostMoney = p.qty * p.cost;

        FILE *f = fopen(EXPIRED_FILE, "a");
        if (f != NULL) {
            fprintf(f, "%s|%d|%s|%d|%.2f\n", p.expiry, p.id, p.name, p.qty, lostMoney);
            fclose(f);
        }
        sales.recordLoss(p.name, p.qty, lostMoney);
        inv.remove(p.id);
        removed++;
        printf("EXPIRED: %s (ID %d, %d items, expired on %s) -> moved to %s and deleted.\n",
               p.name, p.id, p.qty, p.expiry, EXPIRED_FILE);
    }
    if (removed > 0) inv.save();
    return removed;
}

inline void showExpiryList(Inventory &inv) {
    Product foods[MAX_PRODUCTS];
    int count = getFoodSorted(inv, foods);

    printf("\n===== FOOD SORTED BY EXPIRY (soonest first) =====\n");
    if (count == 0) { printf("No food in the shop.\n"); return; }
    printf("%-5s%-14s%-6s%-12s%s\n", "ID", "Name", "Qty", "Expiry", "Days left");
    for (int i = 0; i < count; i++) {
        int left = daysLeft(foods[i].expiry);
        printf("%-5d%-14s%-6d%-12s%d", foods[i].id, foods[i].name, foods[i].qty, foods[i].expiry, left);
        if (left <= 3) printf("   <-- expiring soon!");
        printf("\n");
    }
}

inline void showExpiredLog() {
    printf("\n===== EXPIRED ITEMS (from %s) =====\n", EXPIRED_FILE);
    FILE *f = fopen(EXPIRED_FILE, "r");
    if (f == NULL) { printf("Nothing has expired yet.\n"); return; }

    printf("%-12s%-5s%-14s%-6s%s\n", "Expired on", "ID", "Name", "Qty", "Money lost");
    char line[256], date[16], name[32];
    int id, qty;
    double lost;
    while (fgets(line, sizeof(line), f) != NULL) {
        if (sscanf(line, "%15[^|]|%d|%31[^|]|%d|%lf", date, &id, name, &qty, &lost) == 5)
            printf("%-12s%-5d%-14s%-6d Rs %.0f\n", date, id, name, qty, lost);
    }
    fclose(f);
    // NIKHIL: lists food that expires within limitDays (uses the merge-sorted list)
inline void showExpiringSoon(Inventory &inv, int limitDays) {
    Product foods[MAX_PRODUCTS];
    int count = getFoodSorted(inv, foods);

    printf("\n===== FOOD EXPIRING IN %d DAY(S) OR LESS =====\n", limitDays);
    int found = 0;
    for (int i = 0; i < count; i++) {
        int left = daysLeft(foods[i].expiry);
        if (left > limitDays) break;              // list is sorted, so the rest expire later
        if (found == 0) printf("%-5s%-14s%-6s%-12s%s\n", "ID", "Name", "Qty", "Expiry", "Days left");
        printf("%-5d%-14s%-6d%-12s%d\n", foods[i].id, foods[i].name, foods[i].qty, foods[i].expiry, left);
        found++;
    }
    if (found == 0) printf("Nothing is expiring soon.\n");
}
    // NIKHIL: total cost of food that will expire within limitDays
inline double expiryRisk(Inventory &inv, int limitDays) {
    Product foods[MAX_PRODUCTS];
    int count = getFoodSorted(inv, foods);
    double risk = 0;
    for (int i = 0; i < count; i++) {
        if (daysLeft(foods[i].expiry) > limitDays) break;
        risk += foods[i].qty * foods[i].cost;
    }
    return risk;
}

inline void showExpiryRisk(Inventory &inv, int limitDays) {
    printf("\nStock worth Rs %.2f (cost price) expires within %d day(s).\n",
           expiryRisk(inv, limitDays), limitDays);
}
}


// ===== AMAN: NORMAL AND PRIORITY QUEUES =====
// DSA: Linked-list queues follow FIFO; priority orders are processed first.
// OOP: Encapsulation - queue nodes and links stay private inside the class.
class Queue {
    struct Box {
        int orderId;
        Box *next;
    };
    Box *front;
    Box *back;
    int count;

public:
    Queue() { front = NULL; back = NULL; count = 0; }
    ~Queue() { while (!isEmpty()) pop(); }

    bool isEmpty() const { return front == NULL; }
    int size() const { return count; }

    void push(int orderId) {
        Box *b = new Box;
        b->orderId = orderId;
        b->next = NULL;
        if (back == NULL) front = b;
        else back->next = b;
        back = b;
        count++;
    }

    int pop() {
        Box *b = front;
        int orderId = b->orderId;
        front = b->next;
        if (front == NULL) back = NULL;
        delete b;
        count--;
        return orderId;
    }

    int position(int orderId) const {
        int place = 1;
        for (Box *b = front; b != NULL; b = b->next) {
            if (b->orderId == orderId) return place;
            place++;
        }
        return 0;
    }

    void removeOrder(int orderId) {
        Box *before = NULL;
        for (Box *b = front; b != NULL; before = b, b = b->next) {
            if (b->orderId == orderId) {
                if (before == NULL) front = b->next;
                else before->next = b->next;
                if (b == back) back = before;
                delete b;
                count--;
                return;
            }
        }
    }
};

struct Order {
    int id;
    char customer[24];
    char product[32];
    int productId;
    int qty;
    double total;
    double cost;
    double fee;
    bool priority;
    bool delivered;
    char date[17];
    int area;                              // delivery area (index in AREA_NAME)
    int eta;                               // estimated delivery time in minutes (0 = none)
};
// ===== RAHUL: DELIVERY TIME (DIJKSTRA SHORTEST PATH) =====
// DSA: The city is a weighted graph (adjacency matrix). Node 0 is the store.
//      Dijkstra finds the shortest road distance from the store to the customer's area.
const int AREAS = 8;
const char *const AREA_NAME[AREAS] = {"Store", "Market", "Station", "Hospital", "College", "Park", "Mall", "Colony"};
const double SPEED_KMH    = 30.0;          // average delivery speed
const int PREP_NORMAL     = 20;            // minutes to pack a normal order
const int PREP_PRIORITY   = 5;             // minutes to pack a priority order
const double INF_DIST     = 1e9;

// OOP: Encapsulation - road map and Dijkstra are hidden inside the class.
class DeliveryMap {
    double road[AREAS][AREAS];             // distance in km, 0 = no direct road

    void addRoad(int a, int b, double km) { road[a][b] = km; road[b][a] = km; }

public:
    DeliveryMap() {
        for (int i = 0; i < AREAS; i++)
            for (int j = 0; j < AREAS; j++) road[i][j] = 0;
        addRoad(0, 1, 2.0);  addRoad(0, 2, 4.5);  addRoad(1, 2, 2.0);
        addRoad(1, 3, 3.0);  addRoad(2, 4, 3.5);  addRoad(3, 5, 2.5);
        addRoad(4, 5, 1.5);  addRoad(3, 6, 5.0);  addRoad(5, 6, 2.0);
        addRoad(4, 7, 4.0);  addRoad(6, 7, 1.5);
    }

    // DSA: Dijkstra from the store (node 0). Returns km to `target`;
    //      fills path[] with the areas to pass through, in order.
    double shortest(int target, int path[], int &len) {
        double dist[AREAS];
        int prev[AREAS];
        bool done[AREAS];
        for (int i = 0; i < AREAS; i++) { dist[i] = INF_DIST; prev[i] = -1; done[i] = false; }
        dist[0] = 0;

        for (int step = 0; step < AREAS; step++) {
            int u = -1;                    // closest area not finished yet
            for (int i = 0; i < AREAS; i++)
                if (!done[i] && (u == -1 || dist[i] < dist[u])) u = i;
            if (u == -1 || dist[u] >= INF_DIST) break;
            done[u] = true;

            for (int v = 0; v < AREAS; v++)            // relax every road from u
                if (road[u][v] > 0 && dist[u] + road[u][v] < dist[v]) {
                    dist[v] = dist[u] + road[u][v];
                    prev[v] = u;
                }
        }

        int tmp[AREAS];                    // walk back from target to store
        len = 0;
        for (int v = target; v != -1; v = prev[v]) tmp[len++] = v;
        for (int i = 0; i < len; i++) path[i] = tmp[len - 1 - i];
        return dist[target];
    }

    // Delivery time = packing time + travel time.
    int eta(int area, bool priority) {
        int prep = priority ? PREP_PRIORITY : PREP_NORMAL;
        if (area == 0) return prep;                    // pickup: only packing time
        int path[AREAS], len;
        double km = shortest(area, path, len);
        return prep + (int)(km / SPEED_KMH * 60.0 + 0.5);
    }

    void describe(int area, bool priority) {
        if (area == 0) {
            printf("Pickup at the store: ready in about %d min.\n", eta(0, priority));
            return;
        }
        int path[AREAS], len;
        double km = shortest(area, path, len);
        printf("Route: ");
        for (int i = 0; i < len; i++) printf("%s%s", AREA_NAME[path[i]], i + 1 < len ? " -> " : "");
        printf("  (%.1f km)\n", km);
        printf("Estimated delivery: about %d min.\n", eta(area, priority));
    }

    void showAreas() {
        printf("\n===== DELIVERY TIMES FROM THE STORE =====\n");
        printf("%-4s%-12s%-10s%-14s%s\n", "No.", "Area", "Distance", "Normal", "Priority");
        for (int a = 1; a < AREAS; a++) {
            int path[AREAS], len;
            double km = shortest(a, path, len);
            printf("%-4d%-12s%-4.1f km    %-3d min       %d min\n", a, AREA_NAME[a], km, eta(a, false), eta(a, true));
        }
    }
};

DeliveryMap city;


// ===== AMAN: ORDERS =====
// OOP: Composition - OrderBook contains normal and priority Queue objects.
class OrderBook {
    static const int MAX_ORDERS = 1000;
    Order list[MAX_ORDERS];
    int count;
    Queue priorityQueue;
    Queue normalQueue;

    void save() {
        FILE *f = fopen(ORDERS_FILE, "w");
        if (f == NULL) { printf("Warning: could not save orders.\n"); return; }
        for (int i = 0; i < count; i++) {
            Order &o = list[i];
            fprintf(f, "%d|%s|%d|%s|%d|%.2f|%.2f|%.2f|%d|%d|%s|%d|%d\n", o.id, o.customer, o.productId,
                    o.product, o.qty, o.total, o.cost, o.fee, o.priority, o.delivered, o.date, o.area, o.eta);
        }
        fclose(f);
    }

public:

    OrderBook() {
        count = 0;
        FILE *f = fopen(ORDERS_FILE, "r");
        if (f == NULL) return;

        char line[512];
        while (count < MAX_ORDERS && fgets(line, sizeof(line), f) != NULL) {
            Order o = Order();
            int pr, dl;
            // 11 fields = old file format, 13 fields = with delivery area and time
            int got = sscanf(line, "%d|%23[^|]|%d|%31[^|]|%d|%lf|%lf|%lf|%d|%d|%16[^|\n]|%d|%d",
                             &o.id, o.customer, &o.productId, o.product, &o.qty,
                             &o.total, &o.cost, &o.fee, &pr, &dl, o.date, &o.area, &o.eta);
            if (got != 11 && got != 13) continue;
            if (got == 11 || o.area < 0 || o.area >= AREAS) { o.area = 0; o.eta = 0; }
            o.priority = (pr == 1);
            o.delivered = (dl == 1);
            list[count] = o;
            count++;
            if (!o.delivered) {
                if (o.priority) priorityQueue.push(o.id);
                else            normalQueue.push(o.id);
            }
        }
        fclose(f);
    }

    int waiting() const { return priorityQueue.size() + normalQueue.size(); }
    int total() const { return count; }
    const Order &at(int i) const { return list[i]; }

    Order *find(int id) {
        for (int i = 0; i < count; i++)
            if (list[i].id == id) return &list[i];
        return NULL;
    }

    int positionOf(int id) {
        Order *o = find(id);
        if (o == NULL || o->delivered) return 0;
        if (o->priority) return priorityQueue.position(id);
        return priorityQueue.size() + normalQueue.position(id);
    }

    int placeOrder(Inventory &inv, const char *customer, Product &p, int qty, bool wantsPriority, int area) {
        if (qty < 1 || qty > p.qty || count >= MAX_ORDERS) return 0;

        Order o = Order();
        o.id = (count == 0) ? 1 : list[count - 1].id + 1;
        snprintf(o.customer, sizeof(o.customer), "%s", customer);
        snprintf(o.product, sizeof(o.product), "%s", p.name);
        o.productId = p.id;
        o.qty = qty;
        o.total = qty * p.price;
        o.cost = qty * p.cost;
        o.priority = wantsPriority;
        o.fee = wantsPriority ? PRIORITY_FEE : 0;
        o.delivered = false;
        o.area = area;
        o.eta = city.eta(area, wantsPriority);         // Dijkstra-based delivery time
        time_t now = time(NULL);
        strftime(o.date, sizeof(o.date), "%Y-%m-%d %H:%M", localtime(&now));

        p.qty = p.qty - qty;
        inv.save();

        list[count] = o;
        count++;
        if (wantsPriority) priorityQueue.push(o.id);
        else               normalQueue.push(o.id);
        save();
        return o.id;
    }

    bool payToSkip(int id, const char *customer) {
        Order *o = find(id);
        if (o == NULL || o->delivered || o->priority) return false;
        if (!sameNoCase(o->customer, customer)) return false;

        normalQueue.removeOrder(id);
        priorityQueue.push(id);
        o->priority = true;
        o->fee = PRIORITY_FEE;
        if (o->eta > 0) o->eta = city.eta(o->area, true);   // faster packing, new time
        save();
        return true;
    }

    void processNext(Sales &sales) {
        if (waiting() == 0) { printf("No orders are waiting.\n"); return; }

        int id;
        if (!priorityQueue.isEmpty()) id = priorityQueue.pop();
        else                          id = normalQueue.pop();

        Order *o = find(id);
        o->delivered = true;
        sales.recordSale(o->id, o->product, o->qty, o->total, o->cost, o->fee);
        save();

        printf("Delivered order #%d: %d x %s to %s (Rs %.0f)%s\n", o->id, o->qty, o->product,
               o->customer, o->total + o->fee, o->priority ? "  [PRIORITY]" : "");
    }

    void printOrder(const Order &o) {
        char status[32];
        if (o.delivered)      snprintf(status, sizeof(status), "Delivered");
        else if (o.priority)  snprintf(status, sizeof(status), "PRIORITY queue #%d", positionOf(o.id));
        else                  snprintf(status, sizeof(status), "Normal queue #%d", positionOf(o.id));
        printf("#%-4d%-12s%-12s x%-4d Rs%-8.0f %-20s %s\n", o.id, o.customer, o.product,
               o.qty, o.total + o.fee, status, o.date);
        if (!o.delivered && o.eta > 0)
            printf("      %s: %s, ready/delivery in about %d min\n",
                   o.area == 0 ? "Pickup" : "Deliver to", AREA_NAME[o.area], o.eta);
    }
    void showAll() {
        printf("\n===== ALL ORDERS =====\n");
        printf("Waiting: %d priority, %d normal\n", priorityQueue.size(), normalQueue.size());
        if (count == 0) printf("No orders yet.\n");
        for (int i = 0; i < count; i++) printOrder(list[i]);
    }
    void showFor(const char *customer) {
        printf("\n===== ORDERS OF %s =====\n", customer);
        int found = 0;
        for (int i = 0; i < count; i++) {
            if (sameNoCase(list[i].customer, customer)) { printOrder(list[i]); found++; }
        }
        if (found == 0) printf("You have not ordered anything yet.\n");
    }
};

// ===== RAHUL: PRODUCT RELATION AND RECOMMENDATION =====
// Two relations give each product a score:
//   1. Bought together: other products ordered by customers who also ordered this one (+10 each)
//   2. Same category: related items from the same section, still in stock (+3)
// The highest scores are shown as "You may also like".
inline int recommend(Inventory &inv, OrderBook &orders, const Product &bought, Product out[], int maxOut) {
    Product all[MAX_PRODUCTS];
    int score[MAX_PRODUCTS];
    int n = inv.getAll(all);
    for (int i = 0; i < n; i++) score[i] = 0;

    for (int i = 0; i < orders.total(); i++) {
        if (orders.at(i).productId != bought.id) continue;
        for (int j = 0; j < orders.total(); j++) {
            const Order &other = orders.at(j);
            if (other.productId == bought.id) continue;
            if (!sameNoCase(other.customer, orders.at(i).customer)) continue;
            for (int k = 0; k < n; k++)
                if (all[k].id == other.productId) score[k] += 10;
        }
    }

    for (int k = 0; k < n; k++)
        if (all[k].cat == bought.cat) score[k] += 3;

    int count = 0;
    while (count < maxOut) {                       // pick the best score each round
        int best = -1;
        for (int k = 0; k < n; k++) {
            if (all[k].id == bought.id || all[k].qty <= 0 || score[k] <= 0) continue;
            if (best == -1 || score[k] > score[best]) best = k;
        }
        if (best == -1) break;
        out[count++] = all[best];
        score[best] = 0;
    }
    return count;
}


// ===== OMKAR: USER ACCOUNTS (FNV-1a HASHING) =====
// DSA: FNV-1a is a fast hash function. It is used for two things here:
//      (1) the password is stored only as a hash in userpass.txt, never as plain text
//      (2) the hash of the username picks the bucket in the hash table
// Note: FNV-1a is not a cryptographic hash; real systems use bcrypt or argon2.
inline unsigned int fnv1a(const char *text) {
    unsigned int hash = 2166136261u;               // FNV offset basis
    for (; *text != '\0'; text++) {
        hash = hash ^ (unsigned char)*text;
        hash = hash * 16777619u;                   // FNV prime
    }
    return hash;
}

// OOP: Encapsulation - the hash table and file handling are private.
class UserDB {
    static const int BUCKETS = 32;

    struct Account {
        char name[24];
        unsigned int passHash;
        Account *next;                             // chaining: same bucket = linked list
    };
    Account *table[BUCKETS];

    static void lower(const char *in, char *out, int size) {
        int i = 0;
        for (; in[i] != '\0' && i < size - 1; i++) out[i] = (char)tolower((unsigned char)in[i]);
        out[i] = '\0';
    }

    int bucketOf(const char *name) {
        char key[24];
        lower(name, key, sizeof(key));
        return (int)(fnv1a(key) % BUCKETS);
    }

    // Hash of "username:password" so equal passwords give different hashes.
    unsigned int hashPassword(const char *name, const char *pass) {
        char key[24], mixed[96];
        lower(name, key, sizeof(key));
        snprintf(mixed, sizeof(mixed), "%s:%s", key, pass);
        return fnv1a(mixed);
    }

    Account *findAccount(const char *name) {
        for (Account *a = table[bucketOf(name)]; a != NULL; a = a->next)
            if (sameNoCase(a->name, name)) return a;
        return NULL;
    }

    void insertAccount(const char *name, unsigned int hash) {
        Account *a = new Account;
        snprintf(a->name, sizeof(a->name), "%s", name);
        a->passHash = hash;
        int b = bucketOf(name);
        a->next = table[b];
        table[b] = a;
    }

public:
    UserDB() {
        for (int i = 0; i < BUCKETS; i++) table[i] = NULL;
        FILE *f = fopen(USERS_FILE, "r");
        if (f == NULL) return;
        char line[128], name[24];
        unsigned int hash;
        while (fgets(line, sizeof(line), f) != NULL)
            if (sscanf(line, "%23[^|]|%x", name, &hash) == 2 && findAccount(name) == NULL)
                insertAccount(name, hash);
        fclose(f);
    }

    ~UserDB() {
        for (int i = 0; i < BUCKETS; i++) {
            Account *a = table[i];
            while (a != NULL) { Account *next = a->next; delete a; a = next; }
        }
    }

    bool exists(const char *name) { return findAccount(name) != NULL; }

    bool createUser(const char *name, const char *pass) {
        if (exists(name)) return false;
        unsigned int hash = hashPassword(name, pass);
        insertAccount(name, hash);
        FILE *f = fopen(USERS_FILE, "a");
        if (f == NULL) { printf("Warning: could not save account.\n"); return true; }
        fprintf(f, "%s|%08x\n", name, hash);
        fclose(f);
        return true;
    }

    bool check(const char *name, const char *pass) {
        Account *a = findAccount(name);
        return a != NULL && a->passHash == hashPassword(name, pass);
    }
};

// ===== OMKAR: CUSTOMER PORTAL AND ORDERING =====
// OOP: CustomerPortal uses Inventory, OrderBook and UserDB objects (composition/references).
class CustomerPortal {
    Inventory &inv;
    OrderBook &orders;
    UserDB &users;
    char name[24];

    void createAccount() {
        char user[24], pass[32], again[32];
        printf("Choose a username: ");
        readLine(user, sizeof(user));
        if (user[0] == '\0')      { printf("Username cannot be empty.\n"); return; }
        if (users.exists(user))   { printf("That username is already taken.\n"); return; }
        printf("Choose a password (at least 4 characters): ");
        readLine(pass, sizeof(pass));
        if (strlen(pass) < 4)     { printf("Password is too short.\n"); return; }
        printf("Type the password again: ");
        readLine(again, sizeof(again));
        if (strcmp(pass, again) != 0) { printf("Passwords do not match.\n"); return; }
        users.createUser(user, pass);
        printf("Account created. You can login now.\n");
    }

    bool login() {
        for (int attempt = 1; attempt <= 3; attempt++) {
            char user[24], pass[32];
            printf("Username: ");
            readLine(user, sizeof(user));
            printf("Password: ");
            readLine(pass, sizeof(pass));
            if (users.check(user, pass)) {
                snprintf(name, sizeof(name), "%s", user);
                return true;
            }
            printf("Wrong username or password.\n");
        }
        return false;
    }

    // Customer must create an account and login before shopping.
    bool authenticate() {
        while (true) {
            printf("\n--- CUSTOMER ACCOUNT ---\n1. Create account\n2. Login\n3. Back\nChoose: ");
            int choice = readInt();
            if (choice == 1)      createAccount();
            else if (choice == 2) { if (login()) return true; }
            else if (choice == 3) return false;
            else printf("Invalid choice.\n");
        }
    }

    void searchById() {
        printf("Product ID: ");
        Product *p = inv.find(readInt());
        if (p == NULL) { printf("No product with that ID.\n"); return; }
        inv.showOne(*p, false);
    }

    void searchByName() {
        char word[32];
        printf("Type a name or category (example: lap, shirt, food): ");
        readLine(word, sizeof(word));
        inv.searchByName(word, false);
    }

    void placeOrder() {
        printf("Product ID to order: ");
        Product *p = inv.find(readInt());
        if (p == NULL)   { printf("No product with that ID.\n"); return; }
        if (p->qty == 0) { printf("Sorry, out of stock.\n"); return; }

        printf("%s costs Rs %.0f each. %d in stock. How many? ", p->name, p->price, p->qty);
        int qty = readInt();
        if (qty < 1 || qty > p->qty) { printf("Invalid quantity.\n"); return; }

        printf("Total: Rs %.0f\n", qty * p->price);
        printf("Pay Rs %.0f extra to skip the queue (PRIORITY)?  1 = Yes, 0 = No: ", PRIORITY_FEE);
        bool wantsPriority = (readInt() == 1);

        city.showAreas();
        printf("Delivery area number (0 = pick up at the store): ");
        int area = readInt();
        if (area < 0 || area >= AREAS) { printf("Invalid area.\n"); return; }

        int id = orders.placeOrder(inv, name, *p, qty, wantsPriority, area);
        if (id == 0) { printf("Order failed.\n"); return; }
        printf("Order #%d placed! You are number %d in the %s queue.\n",
               id, orders.positionOf(id), wantsPriority ? "PRIORITY" : "normal");
        city.describe(area, wantsPriority);
        showRecommendations(*p);
    }

// RAHUL: show related products after a purchase.
    void showRecommendations(const Product &bought) {
        Product suggest[3];
        int n = recommend(inv, orders, bought, suggest, 3);
        if (n == 0) return;
        printf("\nYou may also like:\n");
        inv.showList(suggest, n, false);
    }

    void payToSkip() {
        orders.showFor(name);
        printf("Which order number do you want to make PRIORITY (fee Rs %.0f)? ", PRIORITY_FEE);
        int id = readInt();
        if (orders.payToSkip(id, name)) {
            printf("Done! Order #%d is now number %d in the PRIORITY queue.\n", id, orders.positionOf(id));
            Order *o = orders.find(id);
            if (o != NULL && o->eta > 0) printf("New estimated time: about %d min.\n", o->eta);
        } else {
            printf("Cannot do that (not your order, already delivered, or already priority).\n");
        }
    }

public:
    CustomerPortal(Inventory &i, OrderBook &o, UserDB &u) : inv(i), orders(o), users(u) { name[0] = '\0'; }

    void run() {
        if (!authenticate()) return;

        while (true) {
            printf("\n--- Welcome %s ---\n", name);
            printf("1. View inventory\n");
            printf("2. Search by ID\n");
            printf("3. Search by name or category\n");
            printf("4. Place an order\n");
            printf("5. My orders\n");
            printf("6. Pay extra to skip the queue\n");
            printf("7. Delivery times\n");
            printf("8. Back\n");
            printf("Choose: ");

            int choice = readInt();
            if (choice == 1)      inv.showAll(false);
            else if (choice == 2) searchById();
            else if (choice == 3) searchByName();
            else if (choice == 4) placeOrder();
            else if (choice == 5) orders.showFor(name);
            else if (choice == 6) payToSkip();
            else if (choice == 7) city.showAreas();
            else if (choice == 8) return;
            else printf("Invalid choice.\n");
        }
    }
};


// ===== OMKAR: MANAGER MENU AND MAIN FUNCTION =====
void managerMenu(Inventory &inv, Sales &sales, OrderBook &orders) {
    printf("Password: ");
    char password[32];
    readLine(password, sizeof(password));
    if (strcmp(password, MANAGER_PASS) != 0) { printf("Wrong password.\n"); return; }

    while (true) {
        removeExpired(inv, sales);

        printf("\n--- MANAGER  (%d order(s) waiting) ---\n", orders.waiting());
        printf(" 1. Total inventory\n");
        printf(" 2. Add / restock product\n");
        printf(" 3. Delete product\n");
        printf(" 4. Search product by ID\n");
        printf(" 5. Food sorted by expiry\n");
        printf(" 6. Expired items file\n");
        printf(" 7. View all orders\n");
        printf(" 8. Process next order (priority first)\n");
        printf(" 9. Sales and profit\n");
        printf("10. Back\n");
        printf("Choose: ");

        int choice = readInt();
        if (choice == 1)       inv.showAll(true);
        else if (choice == 2)  inv.addProduct();
        else if (choice == 3)  inv.deleteProduct();
        else if (choice == 4) {
            printf("Product ID: ");
            Product *p = inv.find(readInt());
            if (p != NULL) inv.showOne(*p, true);
            else           printf("Not found.\n");
        }
        else if (choice == 5)  showExpiryList(inv);
        else if (choice == 6)  showExpiredLog();
        else if (choice == 7)  orders.showAll();
        else if (choice == 8)  orders.processNext(sales);
        else if (choice == 9)  sales.showReport();
        else if (choice == 10) return;
        else printf("Invalid choice.\n");
    }
}

// OOP note: This project mainly uses encapsulation and composition; it does not use inheritance.
int main() {
    Inventory inv;
    Sales sales;
    OrderBook orders;
    UserDB users;

    while (true) {
        removeExpired(inv, sales);

        printf("\n==============================\n");
        printf("        S T O C K L I N K\n");
        printf("   Cloth | Electronic | Food\n");
        printf("==============================\n");
        printf("1. Customer\n2. Manager\n3. Exit\nChoose: ");

        int choice = readInt();
        if (choice == 1) {
            CustomerPortal portal(inv, orders, users);
            portal.run();
        }
        else if (choice == 2) managerMenu(inv, sales, orders);
        else if (choice == 3) { printf("Goodbye!\n"); break; }
        else printf("Invalid choice.\n");
    }
    return 0;
}


// ===== OMKAR: MANAGER MENU AND MAIN FUNCTION =====
void managerMenu(Inventory &inv, Sales &sales, OrderBook &orders) {
    printf("Password: ");
    char password[32];
    readLine(password, sizeof(password));
    if (strcmp(password, MANAGER_PASS) != 0) { printf("Wrong password.\n"); return; }

    while (true) {
        removeExpired(inv, sales);

        printf("\n--- MANAGER  (%d order(s) waiting) ---\n", orders.waiting());
        printf(" 1. Total inventory\n");
        printf(" 2. Add / restock product\n");
        printf(" 3. Delete product\n");
        printf(" 4. Search product by ID\n");
        printf(" 5. Food sorted by expiry\n");
        printf(" 6. Expired items file\n");
        printf(" 7. View all orders\n");
        printf(" 8. Process next order (priority first)\n");
        printf(" 9. Sales and profit\n");
        printf("10. Back\n");
        printf("Choose: ");

        int choice = readInt();
        if (choice == 1)       inv.showAll(true);
        else if (choice == 2)  inv.addProduct();
        else if (choice == 3)  inv.deleteProduct();
        else if (choice == 4) {
            printf("Product ID: ");
            Product *p = inv.find(readInt());
            if (p != NULL) inv.showOne(*p, true);
            else           printf("Not found.\n");
        }
        else if (choice == 5)  showExpiryList(inv);
        else if (choice == 6)  showExpiredLog();
        else if (choice == 7)  orders.showAll();
        else if (choice == 8)  orders.processNext(sales);
        else if (choice == 9)  sales.showReport();
        else if (choice == 10) return;
        else printf("Invalid choice.\n");
    }
}

// OOP note: This project mainly uses encapsulation and composition; it does not use inheritance.
int main() {
    Inventory inv;
    Sales sales;
    OrderBook orders;
    UserDB users;

    while (true) {
        removeExpired(inv, sales);

        printf("\n==============================\n");
        printf("        S T O C K L I N K\n");
        printf("   Cloth | Electronic | Food\n");
        printf("==============================\n");
        printf("1. Customer\n2. Manager\n3. Exit\nChoose: ");

        int choice = readInt();
        if (choice == 1) {
            CustomerPortal portal(inv, orders, users);
            portal.run();
        }
        else if (choice == 2) managerMenu(inv, sales, orders);
        else if (choice == 3) { printf("Goodbye!\n"); break; }
        else printf("Invalid choice.\n");
    }
    return 0;
}
