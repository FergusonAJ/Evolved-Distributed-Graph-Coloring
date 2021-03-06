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

#include <cstdlib>
#include <thread>
#include <vector>
#include <cmath>
#include <queue>
#include <fstream>
// MABE includes
#include "../AbstractWorld.h"
// Local includes
#include "./Graph.h"


class GraphColorWorld : public AbstractWorld {

public:
    static std::shared_ptr <ParameterLink<int>> modePL;
    static std::shared_ptr <ParameterLink<int>> evaluationsPerGenerationPL;
    static std::shared_ptr <ParameterLink<int>> agentLifetimePL;
    static std::shared_ptr <ParameterLink<int>> useNewMessageBitPL;
    static std::shared_ptr <ParameterLink<int>> useSendMessageBitPL;
    static std::shared_ptr <ParameterLink<int>> useSendMessageVetoBitPL;
    static std::shared_ptr <ParameterLink<int>> useGetMessageBitPL;
    static std::shared_ptr <ParameterLink<int>> useGetMessageVetoBitPL;
    static std::shared_ptr <ParameterLink<int>> maximumColorsPL;
    static std::shared_ptr <ParameterLink<int>> useSetColorBitPL;
    static std::shared_ptr <ParameterLink<int>> useSetColorVetoBitPL;
    static std::shared_ptr <ParameterLink<int>> minGraphNodesPL;
    static std::shared_ptr <ParameterLink<int>> maxGraphNodesPL;
    static std::shared_ptr <ParameterLink<double>> minEdgeChancePL;
    static std::shared_ptr <ParameterLink<double>> maxEdgeChancePL;
    static std::shared_ptr <ParameterLink<std::string>> graphOutputDirPL;;

    struct NodeMessage{
        size_t senderID;
        std::vector<double> senderAddr;
        std::vector<double> contents;
        NodeMessage(size_t id, size_t addrSize, size_t colSize){
            senderID = id;
            senderAddr.resize(addrSize, 0);
            contents.resize(colSize, 0);
            size_t bit = 1;
            for(size_t i = 0; i < addrSize; ++i){
                if(senderID & bit)
                    senderAddr[i] = 1.0; 
            }
        }
    };

    bool useNewMsgBit, useSendMsgBit, useSendMsgVetoBit, useGetMsgBit, useGetMsgVetoBit, useSetColorBit, useSetColorVetoBit;
    size_t newMsgBitPos, sendMsgBitPos, sendMsgVetoBitPos, getMsgBitPos, getMsgVetoBitPos, setColorBitPos, setColorVetoBitPos;
    int maxColors = -1;
    int minGraphNodes, maxGraphNodes;
    double minEdgeChance, maxEdgeChance;
    size_t addressSize, colorSize;

    int evaluationsPerGeneration;
    int agentLifetime;

    static std::shared_ptr <ParameterLink<std::string>> groupNamePL;
    static std::shared_ptr <ParameterLink<std::string>> brainNamePL;


    Graph G;
    std::string graphOutputDir;
    std::ofstream graph_file;

    GraphColorWorld(std::shared_ptr <ParametersTable> PT_ = nullptr);

    virtual ~GraphColorWorld(){
        graph_file.close();
    };

    void evaluateSolo(std::shared_ptr <Organism> org, int analyze, int visualize, int debug);

    virtual void evaluate(std::map <std::string, std::shared_ptr<Group>> &groups, int analyze, int visualize, int debug) {
        // Randomize the graph
        G.randomize(
            Random::getInt(minGraphNodes, maxGraphNodes), 
            Random::getDouble(minEdgeChance, maxEdgeChance));
        graph_file << G.get_csv_string() << std::endl;
        int popSize = groups[groupNamePL->get(PT)]->population.size();
        //for (int i = 0; i < popSize; i++) {
        for (auto org:groups[groupNamePL->get(PT)]->population){
            evaluateSolo(org, analyze, visualize, debug);
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
        // TODO: add color here


        //Calculate the number of input bits required
        size_t inputSize = 0;
        if(useNewMsgBit)
            ++inputSize;
        inputSize += addressSize;                   // To address nodes in binary
        inputSize += colorSize;                     // Allow messages to pass a color
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
        if(useSetColorBit)
            ++outputSize;
        if(useSetColorVetoBit)
            ++outputSize;

        outputSize += addressSize;  // To address nodes in binary
        outputSize += colorSize;    // To allow for colors to be sent
        std::cout << "Input bits: " << inputSize << std::endl;
        std::cout << "Output bits: " << outputSize << std::endl;
        return {{groupNamePL->get(PT), 
            {"B:" + brainNamePL->get(PT) + "," + std::to_string(inputSize) + 
             "," + std::to_string(outputSize) }
            }};
    }
    
    void verifyGraphGenVars(){
        if(minGraphNodes < 1){
            std::cout << "minGraphNodes must be >= 1, was passed " << minGraphNodes << std::endl;
            exit(-1);
        }
        if(maxGraphNodes < 1){
            std::cout << "maxGraphNodes must be >= 1, was passed " << maxGraphNodes << std::endl;
            exit(-1);
        }
        if(maxGraphNodes < minGraphNodes){
            std::cout << "maxGraphNodes must be >= minGraphNodes, given "
                      << maxGraphNodes << " vs " << minGraphNodes << std::endl;
            exit(-1);
        }
        if(minEdgeChance < 0 || minEdgeChance > 1){
            std::cout << "minEdgeChance must be in range [0,1], was passed " 
                      << minEdgeChance << std::endl;
            exit(-1);
        }
        if(maxEdgeChance < 0 || maxEdgeChance > 1){
            std::cout << "maxEdgeChance must be in range [0,1], was passed " 
                      << maxEdgeChance << std::endl;
            exit(-1);
        }
        if(maxEdgeChance < minEdgeChance){
            std::cout << "maxEdgeChance >= minEdgeChance, given " 
                      << maxEdgeChance << " vs " << minEdgeChance << std::endl;
            exit(-1);
        }
    }
};

