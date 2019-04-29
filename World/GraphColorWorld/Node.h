#include <iostream>
#include <vector>

class Node{
public:
    size_t id;
    std::vector<size_t> color;
     
    Node(size_t id_) : id(id_){
    };

    Node(size_t id_, const std::vector<size_t>& color_) : id(id_), color(color_){
    }
};
