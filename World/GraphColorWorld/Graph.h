#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <fstream>

#include "./Node.h"

class Graph{
public: 
    std::vector<Node> nodes;
    std::vector<std::set<size_t>> adjVec;
    size_t nodeCount, edgeCount;
    
    Graph(){
        nodeCount = 0;
        edgeCount = 0;
    }

    bool checkGraphColoring(){
        for(size_t n = 0; n < nodeCount; ++n){
            for(size_t m : adjVec[n]){
                if(nodes[n].color == nodes[m].color){
                    return false;
                }
            }
        }
        return true;
    }
    
    size_t getNumColors(){
        std::set<size_t> colorSet;
        for(Node n : nodes)
            colorSet.insert(n.color);
        return colorSet.size();
    } 

    void loadFromFile(std::string filename){
        std::ifstream fp;
        fp.open(filename, std::ios::in);
        fp >> nodeCount >> edgeCount;
        for(size_t n = 0; n < nodeCount; ++n){
            nodes.push_back(Node(n));
        }
        adjVec.resize(nodeCount);
        size_t a, b;
        for(size_t i = 0; i < edgeCount; ++i){
            fp >> a >> b;
            adjVec[a].insert(b);
            adjVec[b].insert(a);
        } 
        fp.close(); 
    }
    
    bool checkNeighbors(size_t a, size_t b){
        if(adjVec[a].find(b) != adjVec[a].end())
            return true;
        // Just in case we switch to directed graphs
        if(adjVec[b].find(a) != adjVec[b].end())
            return true;
        return false;
    }
};
