/* ==========================================================================
 *  rahul_relationships.c   -   PRODUCT RELATIONSHIP MODULE   (Rahul)
 *
 *  OOP concepts : composition (RelGraph HAS-A Graph), association (products are
 *                 linked by "frequently bought together" edges)
 *  DSA concepts : weighted undirected graph (adjacency lists), BFS, DFS and
 *                 Dijkstra's shortest path (using the min-heap from Aman's module)
 *
 *  Edge model: every link has a STRENGTH 1..10 (10 = always bought together).
 *  Dijkstra needs "smaller = better", so   weight = 11 - strength.
 *  The same Graph + Dijkstra code is reused by Aman for the supplier network.
 * ========================================================================== */
#include "stocklink.h"
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/*  Generic graph                                                            */
/* ------------------------------------------------------------------------- */
void graph_init(Graph *g, int n)
{
    g->n = n;
    for (int i = 0; i < MAX_NODES; i++) g->adj[i] = NULL;
}

void graph_free(Graph *g)
{
    for (int i = 0; i < MAX_NODES; i++) {
        Edge *e = g->adj[i];
        while (e) { Edge *nxt = e->next; free(e); e = nxt; }
        g->adj[i] = NULL;
    }
    g->n = 0;
}

static int add_directed(Graph *g, int u, int v, int weight, int aux)
{
    Edge *e = malloc(sizeof *e);
    if (!e) return 0;
    e->to = v; e->weight = weight; e->aux = aux;
    e->next = g->adj[u];
    g->adj[u] = e;
    return 1;
}

int graph_add_edge(Graph *g, int u, int v, int weight, int aux, int directed)
{
    if (u < 0 || v < 0 || u >= MAX_NODES || v >= MAX_NODES) return 0;
    if (!add_directed(g, u, v, weight, aux)) return 0;
    if (!directed && !add_directed(g, v, u, weight, aux)) return 0;
    return 1;
}

Edge *graph_find_edge(const Graph *g, int u, int v)
{
    for (Edge *e = g->adj[u]; e; e = e->next)
        if (e->to == v) return e;
    return NULL;
}

/* Breadth-first search limited to max_depth.  Fills order[] (visit order) and
 * depth[] (indexed by vertex).  Runs in O(V + E).  Returns vertices visited. */
int graph_bfs(const Graph *g, int src, int max_depth, int order[], int depth[])
{
    int visited[MAX_NODES] = {0};
    int queue[MAX_NODES];
    int head = 0, tail = 0, count = 0;

    visited[src] = 1;
    depth[src] = 0;
    queue[tail++] = src;

    while (head < tail) {
        int u = queue[head++];
        order[count++] = u;
        if (depth[u] >= max_depth) continue;
        for (Edge *e = g->adj[u]; e; e = e->next) {
            if (!visited[e->to]) {
                visited[e->to] = 1;
                depth[e->to] = depth[u] + 1;
                queue[tail++] = e->to;
            }
        }
    }
    return count;
}

/* Depth-first search that collects one connected component. O(V + E). */
static void dfs_visit(const Graph *g, int u, int visited[], int out[], int *count)
{
    visited[u] = 1;
    out[(*count)++] = u;
    for (Edge *e = g->adj[u]; e; e = e->next)
        if (!visited[e->to]) dfs_visit(g, e->to, visited, out, count);
}

int graph_dfs_component(const Graph *g, int src, int visited[], int out[])
{
    int count = 0;
    dfs_visit(g, src, visited, out, &count);
    return count;
}

/* Dijkstra with a binary min-heap and lazy deletion: O((V + E) log V). */
void graph_dijkstra(const Graph *g, int src, int dist[], int parent[])
{
    for (int i = 0; i < g->n; i++) { dist[i] = GRAPH_INF; parent[i] = -1; }
    dist[src] = 0;

    MinHeap h;
    heap_init(&h);
    heap_push(&h, 0, src);

    HeapItem it;
    while (heap_pop(&h, &it)) {
        int u = it.value;
        if (it.key > dist[u]) continue;               /* stale heap entry */
        for (Edge *e = g->adj[u]; e; e = e->next) {
            int nd = dist[u] + e->weight;
            if (nd < dist[e->to]) {
                dist[e->to] = nd;
                parent[e->to] = u;
                heap_push(&h, nd, e->to);
            }
        }
    }
    heap_free(&h);
}

/* ------------------------------------------------------------------------- */
/*  RelGraph : product-id aware wrapper (composition over Graph)             */
/* ------------------------------------------------------------------------- */
void rel_init(RelGraph *r)
{
    graph_init(&r->g, 0);
    r->count = 0;
}

void rel_destroy(RelGraph *r)
{
    graph_free(&r->g);
    r->count = 0;
}

/* Map a product id to its vertex number (optionally creating the vertex). */
int rel_index(RelGraph *r, int product_id, int create)
{
    for (int i = 0; i < r->count; i++)
        if (r->ids[i] == product_id) return i;
    if (!create || r->count >= MAX_NODES) return -1;
    r->ids[r->count] = product_id;
    r->count++;
    r->g.n = r->count;
    return r->count - 1;
}

/* Create or overwrite the link between two products. Returns 1 on success. */
int rel_link(RelGraph *r, int a, int b, int strength)
{
    if (a == b) return 0;
    if (strength < 1) strength = 1;
    if (strength > 10) strength = 10;
    int ia = rel_index(r, a, 1), ib = rel_index(r, b, 1);
    if (ia < 0 || ib < 0) return 0;

    Edge *e1 = graph_find_edge(&r->g, ia, ib);
    Edge *e2 = graph_find_edge(&r->g, ib, ia);
    if (e1 && e2) {
        e1->aux = e2->aux = strength;
        e1->weight = e2->weight = 11 - strength;
        return 1;
    }
    return graph_add_edge(&r->g, ia, ib, 11 - strength, strength, 0);
}

static const char *name_of(const Inventory *inv, int pid)
{
    Product *p = inv_find(inv, pid);
    return p ? p->name : "(removed product)";
}

/* BFS recommendations: direct partners first (strongest first), then friends-of-friends. */
void rel_recommend(RelGraph *r, const Inventory *inv, int product_id, int max_depth)
{
    int src = rel_index(r, product_id, 0);
    if (src < 0 || !r->g.adj[src]) {
        printf("  No relationship data for this product yet.\n");
        return;
    }
    int order[MAX_NODES], depth[MAX_NODES];
    int n = graph_bfs(&r->g, src, max_depth, order, depth);

    typedef struct { int node, depth, strength; } Rec;
    Rec rec[MAX_NODES];
    int m = 0;
    for (int i = 1; i < n; i++) {               /* skip order[0] == the source */
        Edge *e = graph_find_edge(&r->g, src, order[i]);
        rec[m].node = order[i];
        rec[m].depth = depth[order[i]];
        rec[m].strength = e ? e->aux : 0;
        m++;
    }
    /* insertion sort: nearer first, then stronger first */
    for (int i = 1; i < m; i++) {
        int k = i;
        while (k > 0 && (rec[k].depth < rec[k-1].depth ||
              (rec[k].depth == rec[k-1].depth && rec[k].strength > rec[k-1].strength))) {
            Rec tmp = rec[k]; rec[k] = rec[k-1]; rec[k-1] = tmp;
            k--;
        }
    }
    printf("  Customers who buy \"%s\" also look at:\n", name_of(inv, product_id));
    for (int i = 0; i < m; i++) {
        Product *p = inv_find(inv, r->ids[rec[i].node]);
        if (!p) continue;
        if (rec[i].depth == 1)
            printf("    * %-26s Rs %8.2f   [direct, strength %d/10]\n", p->name, p->price, rec[i].strength);
        else
            printf("    * %-26s Rs %8.2f   [%d steps away]\n", p->name, p->price, rec[i].depth);
    }
}

void rel_print_neighbours(RelGraph *r, const Inventory *inv, int product_id)
{
    int idx = rel_index(r, product_id, 0);
    if (idx < 0 || !r->g.adj[idx]) { printf("  No direct links.\n"); return; }
    printf("  Direct links of \"%s\":\n", name_of(inv, product_id));
    for (Edge *e = r->g.adj[idx]; e; e = e->next)
        printf("    -> %-26s strength %d/10\n", name_of(inv, r->ids[e->to]), e->aux);
}

/* DFS: every connected component is a "bundle" of products that belong together. */
void rel_show_bundles(RelGraph *r, const Inventory *inv)
{
    if (r->count == 0) { printf("  No relationships recorded.\n"); return; }
    int visited[MAX_NODES] = {0}, out[MAX_NODES], bundle = 0;
    for (int i = 0; i < r->count; i++) {
        if (visited[i]) continue;
        int n = graph_dfs_component(&r->g, i, visited, out);
        printf("  Bundle %d (%d products): ", ++bundle, n);
        for (int k = 0; k < n; k++)
            printf("%s%s", name_of(inv, r->ids[out[k]]), k + 1 < n ? ", " : "\n");
    }
}

/* Dijkstra: the strongest chain of purchases linking product a to product b. */
void rel_strongest_chain(RelGraph *r, const Inventory *inv, int a, int b)
{
    int ia = rel_index(r, a, 0), ib = rel_index(r, b, 0);
    if (ia < 0 || ib < 0) { printf("  One of the products has no relationships.\n"); return; }

    int dist[MAX_NODES], parent[MAX_NODES];
    graph_dijkstra(&r->g, ia, dist, parent);
    if (dist[ib] >= GRAPH_INF) { printf("  These products are not connected.\n"); return; }

    int path[MAX_NODES], len = 0;
    for (int v = ib; v != -1; v = parent[v]) path[len++] = v;
    printf("  Strongest chain: ");
    for (int i = len - 1; i >= 0; i--)
        printf("%s%s", name_of(inv, r->ids[path[i]]), i ? "  ->  " : "\n");
    printf("  Chain cost %d over %d link(s)  (lower cost = stronger association)\n",
           dist[ib], len - 1);
}

/* Self-learning: items sold together in an order strengthen their link. */
void rel_learn_from_order(RelGraph *r, const Order *o)
{
    for (int i = 0; i < o->line_count; i++)
        for (int j = i + 1; j < o->line_count; j++) {
            int a = o->lines[i].product_id, b = o->lines[j].product_id;
            int ia = rel_index(r, a, 0), ib = rel_index(r, b, 0);
            Edge *e = (ia >= 0 && ib >= 0) ? graph_find_edge(&r->g, ia, ib) : NULL;
            rel_link(r, a, b, e ? e->aux + 1 : 1);
        }
}

/* ------------------------------------------------------------------------- */
/*  Persistence : "idA idB strength" per line (each undirected edge once)    */
/* ------------------------------------------------------------------------- */
int rel_save(const RelGraph *r, const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (!f) return 0;
    for (int u = 0; u < r->count; u++)
        for (Edge *e = r->g.adj[u]; e; e = e->next)
            if (u < e->to) fprintf(f, "%d %d %d\n", r->ids[u], r->ids[e->to], e->aux);
    fclose(f);
    return 1;
}

int rel_load(RelGraph *r, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    int a, b, s;
    while (fscanf(f, "%d %d %d", &a, &b, &s) == 3) rel_link(r, a, b, s);
    fclose(f);
    return 1;
}

void rel_seed_demo(RelGraph *r)
{
    static const int seed[][3] = {
        {1,2,8},{1,3,6},{2,4,5},{3,4,4},{5,1,3},      /* groceries      */
        {6,8,7},{6,7,6},{7,9,5},                      /* electronics    */
        {10,11,9},{10,12,6},                          /* stationery     */
        {13,14,7},{14,15,6},{15,16,4},                /* personal care  */
        {11,13,3}                                     /* cross-category */
    };
    for (size_t i = 0; i < sizeof seed / sizeof seed[0]; i++)
        rel_link(r, seed[i][0], seed[i][1], seed[i][2]);
}
