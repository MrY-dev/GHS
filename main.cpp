#include "headers.h"

double start_time; // start time
std::queue<std::vector<int>> Q;//waiting queue for messages to be dealt with later
std::unordered_map<int,int> vertex_ind_map; // maps index in Edges to vertex

int LN = 0; // level of fragment
int FN = 0; // identifier of framgent
int SN = 0;//state of node sleep->0,find->1,found->2
int parent = 0; // node of in_branch
int rank; // rank of the process also num_node
int n;  // no of neighbours
int best_w; // best min weight outgoing edge found till now
int best_node;  // node corresponding to best weight
int rec; // no of received reports
int test_node;  // node that is sent test 
bool halt = false; // terminate when true

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
    // message recieved will be of type {type,arg1,arg2,arg3};
    // we will push the source to the message and process it locally
    while (true) {
        int flag = 0;
        MPI_Status stat;
        MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, &stat);
        if (flag) {
            int msg[4];
            MPI_Recv(&msg, 4,MPI_INT,stat.MPI_SOURCE, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            std::vector<int> recv_msg = {msg[0],msg[1],msg[2],msg[3],stat.MPI_SOURCE};
            process(recv_msg);
        }
        if (Q.size() > 0) {
            std::vector<int> delayed_msg = Q.front();
            Q.pop();
            process(delayed_msg);
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
    std::vector<int> send_msg = {CONNECT,0,0,0};
    MPI_Request sent;
    MPI_Isend(&send_msg[0], 4, MPI_INT, V(0), 0, MPI_COMM_WORLD, &sent);
    MPI_Wait(&sent, MPI_STATUS_IGNORE);
    print_send_msg("CONNECT",send_msg,V(0));
}

void handle_connect(std::vector<int>& args) {
    int L = args[1];
    int src = args[4];
    int i = vertex_ind_map[src];
    if (L < LN) {
        T(i) = 1;
        MPI_Request sent;
        std::vector<int> send_msg = {INITIATE,LN,FN,SN};
        MPI_Isend(&send_msg[0], 4, MPI_INT, src, 0, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE);
        print_send_msg("INITIATE",send_msg,src);
    } 
    else if (T(i) == 0) {
        Q.push(args);
    } 
    else {
        MPI_Request sent;
        std::vector<int> send_msg = {INITIATE,LN+1,W(i),1};
        MPI_Isend(&send_msg[0], 4, MPI_INT, src, 0, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE);
        print_send_msg("INITIATE",send_msg,src);
    }
}

void report() {
    int count = 0;
    for (int i = 0; i < n; ++i) {
        if (T(i) == 1 && V(i) != parent) {
            count++;
        }
    }
    if (rec == count && test_node == -1) {
        SN = 2;
        MPI_Request sent;
        std::vector<int> send_msg = {REPORT,best_w,0,0};
        MPI_Isend(&send_msg[0], 4, MPI_INT, parent, 0, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE);
        print_send_msg("REPORT",send_msg,parent);
    }
}

void test() {
    int flag = 0;
    for (int i = 0; i < n; ++i) {
        if(T(i) == 0) {
            flag = 1;
            test_node = V(i);
            MPI_Request sent;
            std::vector<int> send_msg = {TEST,LN,FN,0};
            MPI_Isend(&send_msg[0], 4, MPI_INT, test_node, 0, MPI_COMM_WORLD, &sent);
            MPI_Wait(&sent, MPI_STATUS_IGNORE);
            print_send_msg("TEST",send_msg,test_node);
            break;
        }
    }
    if (!flag) {
        test_node = -1;
        report();
    }
}

void handle_initiate(std::vector<int>& args) {
    int L  = args[1]; // new level
    int F = args[2];  // new fragment
    int S = args[3]; // new state
    int src = args[4]; //sourc
    LN = L;
    FN = F;
    SN = S;
    parent = src;
    best_node = -1;
    best_w = INF;
    test_node = -1;

    for (int i = 0; i < n; ++i) {
        if (T(i) == 1 && V(i) != parent) {
            MPI_Request sent;
            std::vector<int> send_msg = {INITIATE,LN,FN,SN};
            MPI_Isend(&send_msg[0], 4, MPI_INT, V(i), 0, MPI_COMM_WORLD, &sent);
            MPI_Wait(&sent, MPI_STATUS_IGNORE);
            print_send_msg("INITIATE",send_msg,V(i));
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
        std::vector<int> send_msg = {CHGROOT,0,0,0};
        MPI_Isend(&send_msg[0], 4, MPI_INT, best_node, 0, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE); 
        print_send_msg("CHGROOT",send_msg,best_node);
    } 
    else {
        T(i) = 1;
        MPI_Request sent;
        std::vector<int> send_msg = {CONNECT,LN,0,0};
        MPI_Isend(&send_msg[0], 4, MPI_INT, best_node, 0, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE);
        print_send_msg("CONNECT",send_msg,best_node);
    }
}

void handle_report(std::vector<int>& args) {
    int w = args[1];
    int src = args[4];
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
            Q.push(args);
        } 
        else if (w > best_w) {
            changeroot();
        } 
        else if (w == best_w && best_w == INF) {
            halt = true;
            for (int i = 0; i < n; ++i) {
                if (T(i) == 1) {
                    std::vector<int> send_msg = {TERMINATE,0,0,0};
                    MPI_Request sent;
                    MPI_Isend(&send_msg[0], 4, MPI_INT, V(i), 0, MPI_COMM_WORLD, &sent);
                    MPI_Wait(&sent, MPI_STATUS_IGNORE);
                    print_send_msg("TERMINATE", send_msg,V(i));
                }
            }
        }
    }
    return;
}

void handle_test(std::vector<int>& args) {
    int L = args[1];
    int F = args[2];
    int src = args[4];
    int i = vertex_ind_map[src];

    if (L > LN) {
        Q.push(args);
    } 
    else if (F == FN) {
        if (T(i) == 0) {
            T(i) = -1;
        }
        if (V(i) != test_node) {
            std::vector<int> send_msg = {REJECT,0,0,0};
            MPI_Request sent;
            MPI_Isend(&send_msg[0], 4, MPI_INT, src, 0, MPI_COMM_WORLD, &sent);
            MPI_Wait(&sent, MPI_STATUS_IGNORE);
            print_send_msg("REJECT",send_msg,src);
        } 
        else {
            test();
        }
    } 
    else {
        std::vector<int> send_msg = {ACCEPT,0,0,0};
        MPI_Request sent;
        MPI_Isend(&send_msg[0], 4, MPI_INT, src, 0, MPI_COMM_WORLD, &sent);
        MPI_Wait(&sent, MPI_STATUS_IGNORE);
        print_send_msg("ACCEPT",send_msg,src);
    }
}

void handle_accept(std::vector<int>& args) {
    int src = args[4];
    int i = vertex_ind_map[src];
    test_node = -1;
    if(W(i) < best_w) {
        best_w = W(i);
        best_node = src;
    }
    report();
}

void handle_reject(std::vector<int>& args) {
    int src = args[4];
    int i = vertex_ind_map[src];
    if(T(i) == 0) {
        T(i) = -1;
    }
    test();
}


void handle_terminate(std::vector<int>& args) {
    halt = true;
    int src = args[4];
    for (int i = 0; i < n; ++i) {
        if (T(i) == 1 && V(i) != src) {
            int send = 0;
            std::vector<int> send_msg = {TERMINATE,0,0,0};
            MPI_Request sent;
            MPI_Isend(&send_msg[0], 4, MPI_INT, V(i), 0, MPI_COMM_WORLD, &sent);
            MPI_Wait(&sent, MPI_STATUS_IGNORE);
            print_send_msg("TERMINATE",send_msg,V(i));
        }
    }
}
// print sent messages
void print_send_msg(std::string tag,std::vector<int>& args,int dst) {
#ifdef DEBUG
    std::cout <<"[" <<  MPI_Wtime() - start_time <<  "] " << get_rank() << " >> " <<  dst << " ";
    std::cout << tag << " " << args[1] << " "<< args[2] << " "<< args[3] << "\n";
#endif
}
// print recieved messages
void print_recv_msg(std::string tag,std::vector<int>& args) {
#ifdef DEBUG
    std::cout <<"[" <<  MPI_Wtime() - start_time <<  "] " << get_rank() <<  " << " <<  args[4] << " ";
    // msgtype <- type,arg1,arg2,arg3,src
    std::cout << tag << " " << args[1] << " "<< args[2] << " "<< args[3] << "\n";
#endif
}
/*
 * process the incoming messages
 * */

void process(std::vector<int>& msg) {
    int type = msg[0];
    switch (type) {
        case CONNECT:{
            print_recv_msg("CONNECT",msg);
            handle_connect(msg);
            break;
        }
        case INITIATE: {
            print_recv_msg("INITIATE",msg);
            handle_initiate(msg);
            break;
        }
        case TEST: {
            print_recv_msg("TEST",msg);
            handle_test(msg);
            break;
        }
        case ACCEPT: {
            print_recv_msg("ACCEPT",msg);
            handle_accept(msg);
            break;
        }
        case REJECT: {
            print_recv_msg("REJECT",msg);
            handle_reject(msg);
            break;
        }
        case REPORT: {
            print_recv_msg("REPORT",msg);
            handle_report(msg);
            break;
        }
        case CHGROOT: {
            print_recv_msg("CHGROOT",msg);
            changeroot();
            break;
        }
        case TERMINATE: {
            print_recv_msg("TERMINATE",msg);
            handle_terminate(msg);
            break;
        }
    }
}

