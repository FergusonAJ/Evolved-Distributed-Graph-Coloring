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
        // TODO: Do we want extra, room for arbitrary data in messages? 
        //      (i.e., data that isn't color info)
        //      My gut feeling is to make it another parameter


        // Data map (Because ASCII art is the best):
        
        // Input
        //  -------------------------------
        // | Sender Address | Message | M? |
        //  -------------------------------
        // Sender address - log_2(N) bits, where N is the number of nodes in G
        // Message - log_2(K) bits, where K is the max number of colors
        //      Note: this *should* often be color info, but I don't think we plan on enforcing that
        // M? - New message bit (optional) - 1 if there is a message waiting in the queue

        // Output  
        //  ------------------------------------------------
        // | Target Address | Message | S? | SV? | G? | GV? |
        //  ------------------------------------------------
        // Target address - log_2(N) bits, where N is the number of nodes in G
        // Message - log_2(N) - See message in input map above
        // S? - Send message bit (optional) - Set to 1 to send the given message to
        //      the target address (will only go through if you are neighbors)
        // SV? - Send veto bit (optional) - Set to 1 to ignore the send flag 
        //      (i.e., guarantee you do not send a message this round)
        // G? - Get message bit (optional) - If set to 1, next message in the queue 
        //      will be in the next input
        // GV? - Get message veto bit (optional) - Set to 1 to ignore the get message bit
        //      (i.e., guarantee the next message will NOT be retrieved this round)


        //Calculate the number of input bits required
        size_t inputSize = 0;
        if(useNewMsgBit)
            ++inputSize;
        inputSize += (size_t)ceil(log2(G.nodeCount));   // To address nodes in binary
        inputSize += (size_t)ceil(log2(maxColors));     // Allow messages to pass a color
                                                        //    Number of colors <= the number of nodes
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
        outputSize += (size_t)ceil(log2(G.nodeCount));  // To address nodes in binary
        outputSize += (size_t)ceil(log2(maxColors));    // To allow for colors to be sent
        std::cout << "Input bits: " << inputSize << std::endl;
        std::cout << "Output bits: " << outputSize << std::endl;
        return {{groupNamePL->get(PT), 
            {"B:" + brainNamePL->get(PT) + "," + std::to_string(inputSize) + 
             "," + std::to_string(outputSize) }
            }};
    }
};

