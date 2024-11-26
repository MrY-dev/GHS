#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <istream>
#include <string>
#include <utility>
#include <vector>


struct Edge {
    int u;
    int v;
    int w;
    Edge(int src=0,int dest=0,int weight=0) {
        u = src;
        v = dest;
        w = weight;
    }
    //copy constructor
    Edge(const Edge & e){
        u = e.u;
        v = e.v;
        w = e.w;
    }

    bool operator <(const Edge& e2) {
        return w < e2.w;
    }

    friend std::istream& operator >>  (std::istream &in, Edge e) {
        in >> e.u >> e.v >> e.w;
        return in;
    }
    friend std::ostream& operator <<  (std::ostream &out, Edge e) {
        out << e.u << " " <<  e.v << " " <<e.w;
        return out;
    }
};

std::vector<int> parent;
std::vector<int> rank;

void make_set(int v) {
    parent[v] = v;
    rank[v] = 0;
}

int find_set(int v) {
    if(parent[v] == v) {
        return v;
    }
    return parent[v] = find_set(parent[v]);
}

void union_sets(int a,int b) {
    a = find_set(a);
    b = find_set(b);
    if(a != b) {
        if(rank[a] < rank[b]) {
            std::swap(a,b);
        }
        parent[b] = a;
        if(rank[a] == rank[b]) {
            rank[a]++;
        }

    }
}


int main(int argc , char ** argv) {
    if(argc < 3) {
        std::cout << "incorrect usage" << "\n" ;
    }
    std::ifstream in_file;
    // take input
    in_file.open(argv[1]);
    std::vector<Edge> Edges;
    int num_edges, num_nodes;
    in_file >> num_nodes >> num_edges;

    std::cout << num_edges << " " << num_nodes << "\n";
    int u,v,w;
    while(in_file >> u >> v >> w){
        Edge tmp(u,v,w);
        Edges.push_back(tmp);

    } 
    in_file.close();

    // use disjoint set union for calculating mst;
    auto start_time = std::chrono::high_resolution_clock::now();
    parent.assign(num_nodes,0);
    rank.assign(num_nodes,0);
    for(int i = 0; i < num_nodes; ++i) {
        make_set(i);
    }
    std::sort(Edges.begin(),Edges.end());
    int cost = 0;
    std::vector<Edge> mst;
    for(Edge e : Edges) {
        if(find_set(e.u) != find_set(e.v)) {
            cost += e.w;
            mst.push_back(e);
            union_sets(e.u, e.v);
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::ofstream out_file;
    std::string outpath =  std::string(argv[2])  +  std::to_string(num_nodes) + "_mst";
    std::cout << outpath << "\n";
    out_file.open(outpath);
    out_file << duration.count() << " " << cost << "\n";
    for(Edge e : mst) {
        out_file << e << "\n";
    }
    out_file.close();
    return 0;
    
}
