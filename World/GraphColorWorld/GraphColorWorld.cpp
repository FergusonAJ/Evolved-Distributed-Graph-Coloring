//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include "GraphColorWorld.h"

// Used to shuffle a vector using the same random seed as MABE
int randInt(int i){
    return Random::getInt(0, i-1);
}

std::shared_ptr <ParameterLink<int>> GraphColorWorld::modePL = Parameters::register_parameter("WORLD_GRAPH_COLOR-mode", 0, "0 = bit outputs before adding, 1 = add outputs");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::evaluationsPerGenerationPL = Parameters::register_parameter("WORLD_GRAPH_COLOR-evaluationsPerGeneration", 1, "Number of times to test each Genome per "
                                                                                                                                                                   "generation (useful with non-deterministic "
                                                                                                                                                                   "brains)");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::agentLifetimePL = Parameters::register_parameter("WORLD_GRAPH_COLOR-agentLifetime", 1000, "Number of time units the agents experience in a lifetime");
std::shared_ptr <ParameterLink<std::string>> GraphColorWorld::groupNamePL = Parameters::register_parameter("WORLD_GRAPH_COLOR_NAMES-groupNameSpace", (std::string) "root::", "namespace of group to be evaluated");
std::shared_ptr <ParameterLink<std::string>> GraphColorWorld::brainNamePL = Parameters::register_parameter("WORLD_GRAPH_COLOR_NAMES-brainNameSpace", (std::string) "root::", "namespace for parameters used to define brain");

std::shared_ptr <ParameterLink<std::string>> GraphColorWorld::graphFNamePL = Parameters::register_parameter("WORLD_GRAPH_COLOR-graphFileName", (std::string) "NONE", "The filename of the graph we will test on. NONE for random");

std::shared_ptr <ParameterLink<int>> GraphColorWorld::useNewMessageBitPL = Parameters::register_parameter("WORLD_GRAPH_COLOR-useNewMessageBit",  1, "Do we include a bit in the input telling a brain it has a new message? (1 for yes, 0 for no)");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::useSendMessageBitPL = Parameters::register_parameter("WORLD_GRAPH_COLOR-useSendMessageBit",  1, "Do we include an output bit indicating a message should be sent? (1 for yes, 0 for no)");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::useSendMessageVetoBitPL = Parameters::register_parameter("WORLD_GRAPH_COLOR-useSendMessageVetoBit",  1, "Do we include an output bit that allows vetoing of message sending? (1 for yes, 0 for no) (Note: if useSendMessageBit is 0, this will be set to 0 internally.");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::useGetMessageBitPL = Parameters::register_parameter("WORLD_GRAPH_COLOR-useGetMessageBit",  1, "Do we include an output bit that brains activate to receive a message from their queue? (1 for yes, 0 for no)");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::useGetMessageVetoBitPL = Parameters::register_parameter("WORLD_GRAPH_COLOR-useGetMessageVetoBit",  1, "Do we include a bit taht allows vetoing the action of getting a message from the queue? (1 for yes, 0 for no) (Note: if useGetMessage but is 0, this will be set to 0 internally.");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::useSetColorBitPL = Parameters::register_parameter("WORLD_GRAPH_COLOR-useSetColorBit",  1, "Do we include an output bit that brains activate to change their color? (1 for yes, 0 for no)");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::useSetColorVetoBitPL = Parameters::register_parameter("WORLD_GRAPH_COLOR-useSetColorVetoBit",  1, "Do we include a bit taht allows vetoing the action of changing the color? (1 for yes, 0 for no) (Note: if useGetMessage but is 0, this will be set to 0 internally.");

std::shared_ptr <ParameterLink<int>> GraphColorWorld::maximumColorsPL = Parameters::register_parameter("WORLD_GRAPH_COLOR-maximumColors",  -1, "Maximum number of colors to be considered a valid graph coloring. (-1 to match the number of nodes)");

GraphColorWorld::GraphColorWorld(std::shared_ptr <ParametersTable> PT_) : AbstractWorld(PT_) {
    // columns to be added to ave file (configure data collection)
    popFileColumns.clear();
    popFileColumns.push_back("score");
    popFileColumns.push_back("score_VAR"); // specifies to also record the
    // variance (performed automatically
    // because _VAR)
    popFileColumns.push_back("graphScore");
    popFileColumns.push_back("Send_msg");
    popFileColumns.push_back("Change_Color");
    popFileColumns.push_back("Read_msg");
    popFileColumns.push_back("Computation_rounds");

    //read in graph configuration
    std::string graphFilename = graphFNamePL->get(PT);
    if (graphFilename == "NONE") {

        //TODO: Create a random graph here, instead of defaulting!
        std::cout << "No graph file specified! Defaulting! (Should we make this random?)" << std::endl;
        G.load_from_file("./Graphs/default.txt");
    } else {
        std::cout << "Using graph: " << graphFilename << "!" << std::endl;
        G.load_from_file(graphFilename);
    }
    std::cout << "Nodes: " << G.node_count << std::endl;
    std::cout << "Edges: " << G.edge_count << std::endl;
    addressSize = ceil(log2(G.node_count));

    //read in other params
    agentLifetime = agentLifetimePL->get(PT);

    evaluationsPerGeneration = evaluationsPerGenerationPL->get(PT);

    useNewMsgBit = useNewMessageBitPL->get(PT) > 0;

    useSendMsgBit = useSendMessageBitPL->get(PT) > 0;
    useSendMsgVetoBit = (useSendMsgBit && useSendMessageVetoBitPL->get(PT));

    useGetMsgBit = useGetMessageBitPL->get(PT) > 0;
    useGetMsgVetoBit = (useGetMsgBit && useGetMessageVetoBitPL > 0);

    useSetColorBit = useSetColorBitPL->get(PT) > 0;
    useSetColorVetoBit = (useSetColorBit && useSetColorVetoBitPL > 0);

    maxColors = maximumColorsPL->get(PT);
    if(maxColors <= 0)
        maxColors = G.node_count;
    colorSize = ceil(log2(maxColors));
    G.reset_colors(colorSize);
    std::cout << "Maximum colors: " << maxColors << std::endl;
    
    // Set the bit positions based on what all we have configured
    newMsgBitPos = addressSize + colorSize;
    size_t curPos = addressSize + colorSize;
    if(useSendMsgBit)
        sendMsgBitPos = curPos++;
    if(useSendMsgVetoBit)
        sendMsgVetoBitPos = curPos++;
    if(useGetMsgBit)
        getMsgBitPos = curPos++;
    if(useGetMsgVetoBit)
        getMsgVetoBitPos = curPos++;
    if(useSetColorBit)
        setColorBitPos = curPos++;
    if(useSetColorVetoBit)
        setColorVetoBitPos = curPos++;

}

void GraphColorWorld::evaluateSolo(std::shared_ptr <Organism> org, int analyze, int visualize, int debug) {
    //store the org's brain
    auto originalBrain = org->brains[brainNamePL->get(PT)];

    //store some useful data about the original brain
    auto numBrainInputs = originalBrain->nrInputValues;
    auto numBrainOutputs = originalBrain->nrOutputValues;

    //clone the brain, one for each node
    //ALSO create a list of ints: used to visit each node exactly once in a random order each APL
    std::vector<std::shared_ptr<AbstractBrain>> cloneBrains(G.node_count);
    std::vector<size_t> nodeOrder(G.node_count); //TODO just shuffle the brain vector??
    std::vector<std::queue<NodeMessage>> msgQueues(G.node_count);
    std::vector<uint8_t> deliverMsgVec(G.node_count);

    for (size_t i = 0; i < G.node_count; i++) {
        cloneBrains[i] = originalBrain->makeCopy();
        nodeOrder[i] = i;
    }

    double score = 0.0;
    int sends, color_changes, reads;

    for (size_t eval = 0; eval < evaluationsPerGeneration; eval++) {
        //pre-lifetime setup
        score = 0.0;
        sends = 0;
        color_changes = 0;
        reads = 0;
        G.reset_colors(colorSize);
    
        for (auto brain:cloneBrains) {
            brain->resetBrain();
        }

        //lifetime loop (action-perception loop)
        size_t t;
        for (t = 0; t < agentLifetime; t++) {
            
            // give agents their inputs

            //---------------------------------------------------
            //| message sender addr | message sender color | M? |
            //---------------------------------------------------

            for (auto brainID:nodeOrder) {

                bool hasMsg = msgQueues[brainID].size() > 0; //check before message read

                if (deliverMsgVec[brainID]){
                    if (hasMsg){
                        //set message sender addr and message sender color
                        NodeMessage msg = msgQueues[brainID].front();
                        msgQueues[brainID].pop();
                        for(size_t i = 0; i < addressSize; ++i){ // Set sender's address
                            cloneBrains[brainID]->setInput(i, msg.senderAddr[i]);
                        }
                        for(size_t i = 0; i < colorSize; ++i){ // Set msg. contents (color)
                            cloneBrains[brainID]->setInput(i + addressSize, msg.contents[i]);
                        }
                    }
                    else{
                        //set sender and color to all 0s
                        for (int i = 0; i < addressSize + colorSize; i++) {
                            cloneBrains[brainID]->setInput(i, 0);
                        }
                    }
                }

                if(useNewMsgBit){ //Set "You've got mail!" bit if we have more messages
                    cloneBrains[brainID]->setInput(newMsgBitPos, (double)(msgQueues[brainID].size() > 0));
                }


                // bool hasMsg = msgQueues[brainID].size() > 0;
                // if(!hasMsg){ // No message? Give them all zeroes
                //     for (int i = 0; i < numBrainInputs; i++) {
                //         cloneBrains[brainID]->setInput(i, 0);
                //     }
                // }
                // else if(hasMsg && deliverMsgVec[brainID]){ // Deliver the next message
                //     NodeMessage msg = msgQueues[brainID].front();
                //     msgQueues[brainID].pop();
                //     for(size_t i = 0; i < addressSize; ++i){ // Set sender's address
                //         cloneBrains[brainID]->setInput(i, msg.senderAddr[i]);
                //     }
                //     for(size_t i = 0; i < colorSize; ++i){ // Set msg. contents (color)
                //         cloneBrains[brainID]->setInput(i + addressSize, msg.contents[i]);
                //     }
                //     //TODO: Verify this is a "you still have a msg" bit and not a "we delivered" bit
                //     if(useNewMsgBit){ //Set "You've got mail!" bit if we have more messages
                //         cloneBrains[brainID]->setInput(newMsgBitPos, 
                //             (double)(msgQueues[brainID].size() > 0));
                //     }
                // }
                // else{ // Message exists but was not requested
                //     for (int i = 0; i < numBrainInputs; i++) {
                //         cloneBrains[brainID]->setInput(i, 0);
                //     }
                //     if(useNewMsgBit)
                //         cloneBrains[brainID]->setInput(newMsgBitPos, 1);
                // }
            }

            //update each agent (lets agents think for a single time unit)
            for (auto brain:cloneBrains) {
                brain->update();
            }

            //update the world according to each agent's chosen action (visit each node in an unbiased random order)
            std::random_shuffle(nodeOrder.begin(), nodeOrder.end(), randInt);

            //-------------------------------------------------------------
            //| Target Address | Color | S? | SV? | G? | GV? | C? | CV? |
            //-------------------------------------------------------------

            for (auto brainID:nodeOrder) {

                //Update color of the node
                if(!useSetColorBit || Bit(cloneBrains[brainID]->readOutput(setColorBitPos)) == 1){
                    if(!useSetColorVetoBit || Bit(cloneBrains[brainID]->readOutput(setColorVetoBitPos)) != 1){
                        //change color
                        for(size_t i = 0; i < colorSize; i++){ // Fill contents
                            G.set_color_by_index(brainID, i, Bit(cloneBrains[brainID]->readOutput(i + addressSize)));
                        }
                        score += 1/((t+1)*(t+1)); //diminishing reward for changing color (helps agents discover this ability)
                        color_changes++;
                    }
                }

                //TODO: Do we need a threshold on outputs, or do we treat them as binary?
                if(!useSendMsgBit || Bit(cloneBrains[brainID]->readOutput(sendMsgBitPos)) == 1){
                    if(!useSendMsgVetoBit || Bit(cloneBrains[brainID]->readOutput(sendMsgVetoBitPos)) != 1){
                        // Check if we need to send a message
                        size_t targetID = 0;
                        size_t bit = 1;
                        // Convert output the address to send to
                        for(size_t i = 0; i < addressSize; ++i){
                            if(Bit(cloneBrains[brainID]->readOutput(i)) == 1){
                                targetID |= bit;
                            }
                            bit <<= 1;
                        }
                        // Recipient must be a valid node and our neighbor
                        if(targetID < G.node_count && G.check_neighbors(brainID, targetID)){
                            NodeMessage msg(brainID, addressSize, colorSize);
                            for(size_t i = 0; i < colorSize; i++){ // Fill contents
                                // reads last stored color, may not be same as output buffer if "update color" was not executed
                                msg.contents[i] = G.get_color_at_index(brainID, i);
                            }
                            msgQueues[targetID].push(msg); // Send the message!
                        }
                        score += 1/((t+1)*(t+1)); //diminishing reward for sending a message (helps agents discover this ability)
                        sends++;
                    }
                }
                // Did the brain request a message from its queue (for its next input?)
                deliverMsgVec[brainID] = false;
                if(!useGetMsgBit || Bit(cloneBrains[brainID]->readOutput(getMsgBitPos)) == 1){
                    if(!useGetMsgVetoBit || Bit(cloneBrains[brainID]->readOutput(getMsgVetoBitPos)) != 1){
                        deliverMsgVec[brainID] = true;
                        score += 1/((t+1)*(t+1)); //diminishing reward for delivering message (helps agents discover this ability)
                        reads++;
                    } 
                }
            }
            
            //TODO: Do we use the message contents or something else?

            if (G.check_graph_coloring()){ //reward for stopping earlier
                score += agentLifetime - t;
                break;
            }
        } //agent lifetime
        

        auto xXx = G.get_graph_score();
        if(visualize)
            G.print_colors();
        org->dataMap.append("graphScore", xXx);
        score += xXx*xXx;

        //end of life cleanup
        org->dataMap.append("score", score);

        org->dataMap.append("Send_msg", sends);
        org->dataMap.append("Change_Color", color_changes);
        org->dataMap.append("Read_msg", reads);
        org->dataMap.append("Computation_rounds", int(t));

        //TODO: Actually tie score in
        if (visualize)
            std::cout << "organism with ID " << org->ID << " scored " << score << std::endl;
        
    } // evals per generation
}

// Quick and dirty psuedo-code behind setting inputs and reading outputs

// Read outputs
// (I'm assuming if we don't have a send bit we always send and
//  if we don't have a get message bit then we always try to get)

//  if(!useSendMessageBit || readOutput(SEND_BIT)){
//      if(!useSendMessageVetoBit || !readOutput(SEND_VETO_BIT){
//          target = readOutput(first log2(N) bits) as an int
//          message = readOutput(next log2(K) bits) as an int
//          if(G.checkNeighbors(this id, target))
//              queueVec[target].push_back({id = this id, content = message});  // Message struct?
//      }
//  }
//  deliverMessage[this id] = false;
//  if(!useGetMessageBit || readOutput(GET_BIT)){
//      if(!useGetMessageVetoBit || !readOutput(GET_VETO_BIT)){
//          deliverMessage[this id] = true;
//      }
//  }


// Set inputs

//  brain.resetInputs();
//  hasMsg = !queueVec[this id].empty();
//  if(useNewMessageBit)
//      brain.setInput(NEW_MSG_BIT, hasMsg);    // If queue.size() == 1 and we are delivering, 
                                                //      should this be set to 0?
// If we have no messages or didn't request, do we just leave everything else 0?
//  if(hasMsg && deliverMessage[this id]){
//      message = queueVec[this id].front();
//      queueVec[this id].pop();
//      brain.setInput(first log2(N) bits, message.id);
//      brain.setInput(next log2(K) bits, message.content);
//  } 
