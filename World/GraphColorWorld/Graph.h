#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
// MABE includes
#include "../../Utilities/Random.h"
// Local includes
#include "./Node.h"

class Graph{
public: 
    std::vector<Node> nodes;
    std::vector<std::set<size_t>> adj_vec;
    size_t node_count, edge_count;
    
    Graph(){
        node_count = 0;
        edge_count = 0;
    }

    void randomize(size_t num_nodes, double edge_chance){
        node_count = num_nodes; 
        edge_count = 0;
        nodes.clear();
        adj_vec.clear();
        for(size_t n = 0; n < node_count; ++n){
            nodes.push_back(Node(n));
        }
        adj_vec.resize(node_count);
        size_t a, b;
        double rand_pct;
        for(size_t a = 0; a < node_count - 1; ++a){
            for(size_t b = a + 1; b < node_count - 1; ++b){ 
                rand_pct = Random::getDouble(0,1); // In [0, 1) which should be fine
                if(rand_pct <= edge_chance){
                    adj_vec[a].insert(b);
                    adj_vec[b].insert(a);
                    ++edge_count;
                }
            }
        } 
    }

    bool check_graph_coloring(){
        for(size_t n = 0; n < node_count; ++n){
            for(size_t m : adj_vec[n]){
                if(nodes[n].color == nodes[m].color){
                    return false;
                }
            }
        }
        return true;
    }

    double get_graph_score(){
        double score = edge_count * 2;
        for(size_t n = 0; n < node_count; ++n){
            for(size_t m : adj_vec[n]){
                if(nodes[n].color == nodes[m].color){
                    score -= 1.0;
                }
            }
        }
        return score;
    }

    size_t get_num_colors(){
        std::set<std::vector<size_t>> color_set;
        for(Node n : nodes)
            color_set.insert(n.color);
        return color_set.size();
    } 

    void load_from_file(std::string filename){
        std::ifstream fp;
        fp.open(filename, std::ios::in);
        fp >> node_count >> edge_count;
        for(size_t n = 0; n < node_count; ++n){
            nodes.push_back(Node(n));
        }
        adj_vec.resize(node_count);
        size_t a, b;
        for(size_t i = 0; i < edge_count; ++i){
            fp >> a >> b;
            adj_vec[a].insert(b);
            adj_vec[b].insert(a);
        } 
        fp.close(); 
    }
    
    void reset_colors(size_t num_bits){
        for(size_t n = 0; n < nodes.size(); ++n){
            nodes[n].color = std::vector<size_t>(num_bits, 0);
            for(size_t i = 0; i < num_bits; ++i){// Randomize each bit
                nodes[n].color[i] = (size_t)(Random::getDouble(0,1) < 0.5);
            }
        }
    }
 
    bool check_neighbors(size_t a, size_t b){
        if(adj_vec[a].find(b) != adj_vec[a].end())
            return true;
        // Just in case we switch to directed graphs
        if(adj_vec[b].find(a) != adj_vec[b].end())
            return true;
        return false;
    }
    
    void set_color(size_t node_id, std::vector<size_t> color){
        if(node_id >= nodes.size()){
            std::cout << "Error! Tried to assign color to node " << node_id << ", but "
                      << " node list has only " << nodes.size() << "nodes!" << std::endl;
            exit(-1);
        }
        nodes[node_id].color = color;
    }

    void set_color_by_index(size_t node_id, size_t idx, size_t val){
        if(node_id >= nodes.size()){
            std::cout << "Error! Tried to assign color to node " << node_id << ", but "
                      << "node list has only " << nodes.size() << " nodes!" << std::endl;
            exit(-1);
        }
        //This isn't safe, but I want sppeeeeeeedddd
        nodes[node_id].color[idx] = val;
    }

    size_t get_color_at_index(size_t node_id, size_t idx){
        return nodes[node_id].color[idx];
    }
    
    void print_colors(){
        for(size_t n = 0; n < nodes.size(); ++n){
            std::cout << n << ":  ";
            for(size_t i = 0; i < nodes[n].color.size(); ++i){
                std::cout << nodes[n].color[i] << " ";
            }
            std::cout << std::endl;
        }
    }
    std::string get_csv_string(){
        std::ostringstream oss;
        oss << node_count;
        oss << ",";
        oss << edge_count;
        oss << ",";
        oss << "\"[";
        for(size_t a = 0; a < node_count; ++a){
            for(size_t b = 0; b < node_count; ++b){
                if(a != 0 || b != 0)
                    oss << ",";
                oss << (size_t)(adj_vec[a].find(b) != adj_vec[a].end());
            }
        }
        oss << "]\"";
        return oss.str();
    }
};
