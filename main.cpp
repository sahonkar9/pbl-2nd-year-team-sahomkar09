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

