#include <algorithm>
#include <cassert>
#include <vector>
#include <queue>
#include <iostream>
#include <mpi.h>
#include <unordered_map>
#include <fstream>

const int INF = 1e5;

#define CONNECT 11
#define INITIATE 12
#define TEST 13
#define REPORT 14
#define CHGROOT 15
#define ACCREJ 16
#define TERMINATE 17

struct Edge {
    int v,w,type;
    Edge(int a, int b) {
        v = a;
        w = b;
        type = 0; // initally all the edges are basic
    }
    Edge(const Edge& e) {
        v = e.v;
        w = e.w;
        type = e.type;
    }
    bool operator <(const Edge& other) {
        return w < other.w;
    }
};

#define T(i) Edges[i].type
#define W(i) Edges[i].w
#define V(i) Edges[i].v

int get_rank();
int get_size();
// print for debug
void print_msg(int* msg,int msg_type);
void process(MPI_Status& stat);
// handlers to process the corresponding <messages>
void handle_acc_rej(int* msg,int src);
void handle_test(int* msg,int src);
void handle_connect(int* msg,int src);
void handle_initiate(int* msg,int src);
void handle_terminate(int* msg,int src);
void handle_report(int* msg,int src);

// functions
void report();
void test();
void changeroot();
void wakeup();

