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
