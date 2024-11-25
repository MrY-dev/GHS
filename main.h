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
#define ACCEPT 16
#define REJECT 17
#define TERMINATE 18

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

// print for debug
int get_rank();
int get_size();
void print_send_msg(std::string tag,std::vector<int>& msg,int dst);
void print_recv_msg(std::string tag,std::vector<int>& msg);

// process the type and send to respective handlers 
void process(std::vector<int>& msg);

// handlers to process the corresponding <messages>
void handle_test(std::vector<int>& args);
void handle_connect(std::vector<int>& args);
void handle_initiate(std::vector<int>& args);
void handle_terminate(std::vector<int>& args);
void handle_report(std::vector<int>& args);
void handle_accept(std::vector<int>& args);
void handle_reject(std::vector<int>& args);

// functions
void report();
void test();
void changeroot();
void wakeup();

