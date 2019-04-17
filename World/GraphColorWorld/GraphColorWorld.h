//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#pragma once

#include "../AbstractWorld.h"
#include "./Graph.h"

#include <cstdlib>
#include <thread>
#include <vector>
#include <cmath>


class GraphColorWorld : public AbstractWorld {

public:
    static std::shared_ptr <ParameterLink<int>> modePL;
    static std::shared_ptr <ParameterLink<int>> evaluationsPerGenerationPL;
    static std::shared_ptr <ParameterLink<int>> agentLifetimePL;
    static std::shared_ptr <ParameterLink<std::string>> graphFNamePL;
    static std::shared_ptr <ParameterLink<int>> useNewMessageBit;
    static std::shared_ptr <ParameterLink<int>> useSendMessageBit;
    static std::shared_ptr <ParameterLink<int>> useSendMessageVetoBit;
    static std::shared_ptr <ParameterLink<int>> useGetMessageBit;
    static std::shared_ptr <ParameterLink<int>> useGetMessageVetoBit;
    static std::shared_ptr <ParameterLink<int>> maximumColors;

    bool useNewMsgBit, useSendMsgBit, useSendMsgVetoBit, useGetMsgBit, useGetMsgVetoBit;
    int maxColors = -1;

    int evaluationsPerGeneration;
    int agentLifetime;

    static std::shared_ptr <ParameterLink<std::string>> groupNamePL;
    static std::shared_ptr <ParameterLink<std::string>> brainNamePL;


    Graph G;

    GraphColorWorld(std::shared_ptr <ParametersTable> PT_ = nullptr);

    virtual ~GraphColorWorld() = default;

    void evaluateSolo(std::shared_ptr <Organism> org, int analyze, int visualize, int debug);

    virtual void evaluate(std::map <std::string, std::shared_ptr<Group>> &groups, int analyze, int visualize, int debug) {
        int popSize = groups[groupNamePL->get(PT)]->population.size();
        for (int i = 0; i < popSize; i++) {
            evaluateSolo(groups[groupNamePL->get(PT)]->population[i], analyze, visualize, debug);
        }
    }

    virtual std::unordered_map <std::string, std::unordered_set<std::string>> requiredGroups() override {
        //Calculate the number of input bits required
        size_t inputSize = 0;
        if(useNewMsgBit)
            ++inputSize;
        inputSize += (size_t)ceil(log2(G.nodeCount)); // To address nodes in binary
        //TODO: set a maximum size on the number of colors allowed?
        inputSize += (size_t)ceil(maxColors); // Allow messages to pass a color
        
        //Calculate the number of output bits required
        size_t outputSize = 0;
        if(useSendMsgBit)
            ++outputSize;
        if(useSendMsgVetoBit)
            ++outputSize;
        if(useGetMsgBit)
            ++outputSize;
        if(useGetMsgVetoBit)
            ++outputSize;
        outputSize += (size_t)ceil(log2(G.nodeCount)); // To address nodes in binary
        outputSize += (size_t)ceil(log2(maxColors)); // For color
        std::cout << "Input bits: " << inputSize << std::endl;
        std::cout << "Output bits: " << outputSize << std::endl;
        return {{groupNamePL->get(PT), {"B:" + brainNamePL->get(PT) + "," + std::to_string(inputSize) + "," + std::to_string(outputSize)}}}; //TODO "inputs, outputs"
        // requires a root group and a brain (in root namespace) and no addtional
        // genome,
        // the brain must have 1 input, and the variable numberOfOutputs outputs
    }
};

