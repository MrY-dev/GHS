#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>
// generate random connected graphs of required size for testing mst
struct random_graph
{
    int num_nodes;
    int num_edges;
    std::vector<std::vector<int>> adj_matrix;
    std::vector<std::pair<int,int>> edges;
    std::vector<int> weights;


    void generate_rand_weights() {
        int n = num_nodes;
        for(int i = 1;  i <= n*(n+1)/2; ++i) {
            weights.push_back(i);
        }
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle(weights.begin(), weights.end(), std::default_random_engine(seed));
    }

    void generate_rand_pairs() {
        int n = num_nodes;
        for(int i = 0 ; i < n; ++i) {
            for(int j = i+1; j < n; ++j) { 
                edges.push_back({i,j});
            }
        }
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle(edges.begin(), edges.end(), std::default_random_engine(seed));
    }

    random_graph(int nodes) {
        num_nodes = nodes;
        num_edges = 0;
        adj_matrix.assign(num_nodes,std::vector<int>(num_nodes,0));
        generate_rand_pairs();
        generate_rand_weights();
        num_edges = num_nodes - 1 + (rand()%((num_nodes - 1)*(num_nodes-2)/2)); 
        std::cout << num_edges << "\n";
        for(int i = 0; i < num_edges; ++i) {
            auto edge = edges[i];
            adj_matrix[edge.first][edge.second] = weights[i];
            adj_matrix[edge.second][edge.first] = weights[i];
        }
    }

    // write graph to test file write mst to diff file
    void m_write(std::string path){
        std::ofstream file;
        std::filesystem::remove(path);
        file.open(path);
        file << num_nodes << " " << num_edges << "\n";
        for(int i = 0; i < num_nodes; ++i){
            for(int j = i+1; j < num_nodes; ++j){
                if(adj_matrix[i][j] != 0){
                    file << i << " " << j << " " << adj_matrix[i][j] <<  "\n";
                }
            }
        }
        file.close();
        return;
    }

    void print(){
        for(auto i : adj_matrix){
            for(auto j : i){
                std::cout << j << " ";
            }
            std::cout << "\n";
        }
    }

};

int main(int argc,char** argv)
{
    if(argc < 3){
        std::cout << "incorrect usage" << "\n";
        exit(0);
    }
    std::string dir = argv[1];
    int num_testcases = std::stoi(argv[2]);
    std::cout << "storing the test cases in directory " << dir << "\n";

    if(std::filesystem::exists(dir) == false){
        std::cout << "creating directory " << dir << "\n";
        std::filesystem::create_directory(dir);
    }

    // generate random graphs fmt:graphtest_no,graphtest_no_mst,graphtest_no_ghs
    for(int i = 0; i < num_testcases; ++i) {
        random_graph g((i+1)*10);
        g.m_write(dir+"/graph"+std::to_string(i+1));
    }
    return 0;
}
