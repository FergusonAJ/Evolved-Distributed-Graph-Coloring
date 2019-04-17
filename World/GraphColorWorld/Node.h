#include <iostream>
#include <string>

class Node{
public:
    size_t id, color;
    
    Node(size_t id_) : id(id_){
    };

    Node(size_t id_, size_t color_) : id(id_), color(color_){
    }
};
