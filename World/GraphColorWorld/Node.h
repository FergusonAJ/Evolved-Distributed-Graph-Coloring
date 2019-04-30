#pragma once

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
    
    size_t get_uint_color(){
        size_t bit = 1;
        size_t res = 0;
        for(auto I = color.rbegin(); I != color.rend(); ++I){
            if(*I)
                res |= bit;
            bit <<= 1;
        }
        return res;
    }   
};
