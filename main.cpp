#include "headers.h"
#include <mpi.h>

double start_time;
std::queue<std::pair<int, std::vector<int>>> Q;//waiting queue for messages to be dealt with later
std::unordered_map<int,int> vertex_ind_map; // maps index in Edges to vertex

int LN = 0; // level of fragment
int FN = 0; // identifier of framgent
int SN = 0;//state of node sleep->0,find->1,found->2
int parent = 0; // node of in_branch
int rank; // rank of the process also num_node
int n;  // no of neighbours
int best_w; // best min weight outgoing edge found till now
int best_node;  // node corresponding best weight
int rec; // no of received reports
int test_node;  // node that is sent test to handle exception
bool halt = false; // terminate when true
int testwait = 0;  // lock on test
int connectwait = 0; // lock on connect
int reportwait = 0; // lock on report

std::vector<Edge> Edges; // Edges stored in struct

int get_rank(){
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    return rank;
}

int get_size() {
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD,&size);
    return size;
}


void read_file(std::string path) {
    std::ifstream in_file;
    int num_nodes,num_edges;
    int u,v,w;
    int rank,size;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    in_file.open(path);
    in_file >> num_nodes >> num_edges ;
    while(in_file >> u >> v >> w) {
        if(rank == u) {
            Edges.push_back(Edge(v,w));
        }
        if(rank == v) {
            Edges.push_back(Edge(u,w));
        }
    }
    in_file.close();
    n = Edges.size();
    // sort the edges
    std::sort(Edges.begin(),Edges.end());
    for(int i = 0; i < n; ++i) {
        vertex_ind_map[V(i)] = i;
    }
}

int main(int argc, char **argv) {
    int size; // size of processes initiated also corresponds to number of nodes
    MPI_Init(&argc, &argv);                    
    MPI_Comm_size(MPI_COMM_WORLD, &size);  
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
    read_file(argv[1]);
    MPI_Barrier(MPI_COMM_WORLD);  // synchronize all processes
    start_time = MPI_Wtime();

    wakeup();
    while (true) {
        int flag = 0;
        MPI_Status stat;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &stat);
        if (flag) {
            process(stat);
        }
        if (Q.size() > 0) {
            std::pair<int, std::vector<int>> msg_args = Q.front();
            Q.pop();
            if (msg_args.first == CONNECT) {
                int src = msg_args.second[0];
                int msg = msg_args.second[1];
                handle_connect(&msg,src);
            } 
            else if (msg_args.first == REPORT) {
                int src = msg_args.second[0];
                int msg = msg_args.second[1];
                handle_report(&msg,src);
            } 
            else if (msg_args.first == TEST) {
                int src = msg_args.second[0];
                int msg[2] = {msg_args.second[1],msg_args.second[2]};
                handle_test(msg,src);
            }
        }
        if (halt) {
            break;
        }
    }
    for (int i = 0; i < n; ++i) {
        if (T(i) == 1) {
            std::cout << "MST: " << rank << " " <<  V(i) << " " << W(i) << "\n";
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}

// wakes up and sends connect to its MWOE
void wakeup() {
    T(0) = 1;
    LN = 0;
    SN = 2;
    rec = 0;
    connectwait = 0;
    reportwait = 0;
    if (!connectwait) {
        connectwait = 1;
        int msg = 0;
        MPI_Request sent;
        MPI_Isend(&msg, 1, MPI_INT, V(0), CONNECT, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE);
    }
}

void handle_connect(int *msg,int src) {
    int L = msg[0];
    int i = vertex_ind_map[src];
    if (L < LN) {
        T(i) = 1;
        MPI_Request sent;
        int send[3] = {LN, FN, SN};
        MPI_Isend(&send, 3, MPI_INT, src, INITIATE, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE);
    } 
    else if (T(i) == 0) {
        std::vector<int> args(2);
        args[0] = src;
        args[1] = L;
        Q.push({CONNECT, args});
    } 
    else {
        MPI_Request sent;
        int send[3] = {LN + 1, W(i), 1};
        MPI_Isend(&send, 3, MPI_INT, src, INITIATE, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE);
    }
}

void report() {
    int count = 0;
    for (int i = 0; i < n; ++i) {
        if (T(i) == 1 && V(i) != parent) {
            count++;
        }
    }
    if (rec == count && test_node == -1 && !reportwait) {
        reportwait = 1;
        SN = 2;
        MPI_Request sent;
        int send = best_w;
        MPI_Isend(&send, 1, MPI_INT, parent, REPORT, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE);
    }
}

void test() {
    int flag = 0;
    for (int i = 0; i < n; ++i) {
        if (T(i) == 0 && !testwait) {
            testwait = 1;
            flag = 1;
            test_node = V(i);
            MPI_Request sent;
            int send[2] = {LN, FN};
            MPI_Isend(&send, 2, MPI_INT, test_node, TEST, MPI_COMM_WORLD, &sent);
            MPI_Wait(&sent, MPI_STATUS_IGNORE);
            break;
        }
    }
    if (!flag) {
        test_node = -1;
        report();
    }
}

void handle_initiate(int *msg, int src) {
    int L  = msg[0]; // new level
    int F = msg[1];  // new fragment
    int S = msg[2]; // new state
    LN = L;
    FN = F;
    SN = S;
    parent = src;
    best_node = -1;
    best_w = INF;
    test_node = -1;

    connectwait = 0;
    reportwait = 0;

    for (int i = 0; i < n; ++i) {
        if (T(i) == 1 && V(i) != parent) {
            MPI_Request sent;
            int send[3] = {LN, FN, SN};
            MPI_Isend(&send, 3, MPI_INT, V(i), INITIATE, MPI_COMM_WORLD, &sent);
            MPI_Wait(&sent, MPI_STATUS_IGNORE);
        }
    }

    if (SN == 1) {
        rec = 0;
        test();
    }
}

void changeroot() {
    int i = vertex_ind_map[best_node];
    if (T(i) == 1) {
        MPI_Request sent;
        int msg = 0;
        MPI_Isend(&msg, 1, MPI_INT, best_node, CHGROOT, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE);
    } 
    else {
        T(i) = 1;
        if (!connectwait) {
            connectwait = 1;
            int msg = LN;
            MPI_Request sent;
            MPI_Isend(&msg, 1, MPI_INT, best_node, CONNECT, MPI_COMM_WORLD, &sent);
            MPI_Wait(&sent, MPI_STATUS_IGNORE);
        }
    }
}

void handle_report(int* msg,int src) {
    int w = msg[0];
    if(src != parent) {
        if (w < best_w) {
            best_w = w;
            best_node = src;
        }
        rec += 1;
        report();
    } 
    else {
        if (SN == 1) {
            std::vector<int> args(2);
            args[0] = src;
            args[1] = w;
            Q.push({REPORT, args});
        } 
        else if (w > best_w) {
            changeroot();
        } 
        else if (w == best_w && best_w == INF) {
            int msg = 1;
            halt = true;
            for (int i = 0; i < n; ++i) {
                if (T(i) == 1) {
                    MPI_Request sent;
                    MPI_Isend(&msg, 1, MPI_INT, V(i), TERMINATE, MPI_COMM_WORLD, &sent);
                    MPI_Wait(&sent, MPI_STATUS_IGNORE);
                }
            }
        }
    }
    return;
}

void handle_test(int* msg,int src) {
    int L = msg[0];
    int F = msg[1];
    int i = vertex_ind_map[src];

    if (L > LN) {
        std::vector<int> args(3);
        args[0] = src;
        args[1] = L;
        args[2] = F;
        Q.push({TEST, args});
    } 
    else if (F == FN) {
        if (T(i) == 0) {
            T(i) = -1;
        }
        if (V(i) != test_node) {
            int msg = -1;
            MPI_Request sent;
            MPI_Isend(&msg, 1, MPI_INT, src, ACCREJ, MPI_COMM_WORLD, &sent);
            MPI_Wait(&sent, MPI_STATUS_IGNORE);
        } else {
            test();
        }
    } 
    else {
        int msg = 1;
        MPI_Request sent;
        MPI_Isend(&msg, 1, MPI_INT, src, ACCREJ, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE);
    }
}

void handle_acc_rej(int* msg,int src) {
    int val = msg[0];
    int i = vertex_ind_map[src];
    if (val == -1) {
        if (T(i) == 0) {
            T(i) = -1;
        }
        test();
    } 
    else {
        test_node = -1;
        if (W(i) < best_w) {
            best_w = W(i);
            best_node = src;
        }
        report();
    }
}

void handle_terminate(int* msg,int src) {
    for (int i = 0; i < n; ++i) {
        if (T(i) == 1 && V(i) != src) {
            int send = 0;
            MPI_Request sent;
            MPI_Isend(&send, 1, MPI_INT, V(i), TERMINATE, MPI_COMM_WORLD, &sent);
            MPI_Wait(&sent, MPI_STATUS_IGNORE);
        }
    }
    halt = true;
}

/*
 * process the incoming messages
 * */
void process(MPI_Status& stat) {
    switch (stat.MPI_TAG) {
        case CONNECT:{
            int msg;
            MPI_Status stat;
            MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, CONNECT, MPI_COMM_WORLD, &stat);
            handle_connect(&msg,stat.MPI_SOURCE);
            break;
        }
        case INITIATE: {
            int msg[3];
            MPI_Status stat;
            MPI_Recv(&msg, 3, MPI_INT, MPI_ANY_SOURCE, INITIATE, MPI_COMM_WORLD, &stat);
            handle_initiate(msg,stat.MPI_SOURCE);
            break;
        }
        case TEST: {
            int msg[2];
            MPI_Status stat;
            MPI_Recv(&msg, 2, MPI_INT, MPI_ANY_SOURCE, TEST, MPI_COMM_WORLD, &stat);
            handle_test(msg,stat.MPI_SOURCE);
            break;
        }
        case ACCREJ: {
            testwait = 0;
            int msg;
            MPI_Status stat;
            MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, ACCREJ, MPI_COMM_WORLD, &stat);
            handle_acc_rej(&msg,stat.MPI_SOURCE);
            break;
        }
        case REPORT: {
            int msg;
            MPI_Status stat;
            MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, REPORT, MPI_COMM_WORLD, &stat);
            handle_report(&msg,stat.MPI_SOURCE);
            break;
        }
        case CHGROOT: {
            int msg;
            MPI_Status stat;
            MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, CHGROOT, MPI_COMM_WORLD, &stat);
            changeroot();
            break;
        }
        case TERMINATE: {
            int msg;
            MPI_Status stat;
            MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, TERMINATE, MPI_COMM_WORLD, &stat);
            handle_terminate(&msg,stat.MPI_SOURCE);
            break;
        }
    }
}

