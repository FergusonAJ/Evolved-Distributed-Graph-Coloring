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
std::shared_ptr <ParameterLink<int>> GraphColorWorld::agentLifetimePL = Parameters::register_parameter("WORLD_GRAPH_COLOR-agentLifetime", 1, "Number of time units the agents experience in a lifetime");
std::shared_ptr <ParameterLink<std::string>> GraphColorWorld::groupNamePL = Parameters::register_parameter("WORLD_GRAPH_COLOR_NAMES-groupNameSpace", (std::string) "root::", "namespace of group to be evaluated");
std::shared_ptr <ParameterLink<std::string>> GraphColorWorld::brainNamePL = Parameters::register_parameter("WORLD_GRAPH_COLOR_NAMES-brainNameSpace", (std::string) "root::", "namespace for parameters used to define brain");

std::shared_ptr <ParameterLink<std::string>> GraphColorWorld::graphFNamePL = Parameters::register_parameter("WORLD_GRAPH_COLOR-graphFileName", (std::string) "NONE", "The filename of the graph we will test on. NONE for random");

std::shared_ptr <ParameterLink<int>> GraphColorWorld::useNewMessageBit = Parameters::register_parameter("WORLD_GRAPH_COLOR-useNewMessageBit",  1, "Do we include a bit in the input telling a brain it has a new message? (1 for yes, 0 for no)");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::useSendMessageBit = Parameters::register_parameter("WORLD_GRAPH_COLOR-useSendMessageBit",  1, "Do we include an output bit indicating a message should be sent? (1 for yes, 0 for no)");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::useSendMessageVetoBit = Parameters::register_parameter("WORLD_GRAPH_COLOR-useSendMessageVetoBit",  1, "Do we include an output bit that allows vetoing of message sending? (1 for yes, 0 for no) (Note: if useSendMessageBit is 0, this will be set to 0 internally.");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::useGetMessageBit = Parameters::register_parameter("WORLD_GRAPH_COLOR-useGetMessageBit",  1, "Do we include an output bit that brains activate to receive a message from their queue? (1 for yes, 0 for no)");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::useGetMessageVetoBit = Parameters::register_parameter("WORLD_GRAPH_COLOR-useGetMessageVetoBit",  1, "Do we include a bit taht allows vetoing the action of getting a message from the queue?(1 for yes, 0 for no) (Note: if useGetMessage but is 0, this will be set to 0 internally.");

std::shared_ptr <ParameterLink<int>> GraphColorWorld::maximumColors = Parameters::register_parameter("WORLD_GRAPH_COLOR-maximumColors",  -1, "Maximum number of colors to be considered a valid graph coloring. (-1 to match the number of nodes)");

GraphColorWorld::GraphColorWorld(std::shared_ptr <ParametersTable> PT_) : AbstractWorld(PT_) {
    // columns to be added to ave file (configure data collection)
    popFileColumns.clear();
    popFileColumns.push_back("score");
    popFileColumns.push_back("score_VAR"); // specifies to also record the
    // variance (performed automatically
    // because _VAR)

    //read in graph configuration
    std::string graphFilename = graphFNamePL->get(PT);
    if (graphFilename == "NONE") {

        //TODO: Create a random graph here, instead of defaulting!
        std::cout << "No graph file specified! Defaulting! (Should we make this random?)" << std::endl;
        G.loadFromFile("./Graphs/default.txt");
    } else {
        std::cout << "Using graph: " << graphFilename << "!" << std::endl;
        G.loadFromFile(graphFilename);
    }
    std::cout << "Nodes: " << G.nodeCount << std::endl;
    std::cout << "Edges: " << G.edgeCount << std::endl;

    //read in other params
    agentLifetime = agentLifetimePL->get(PT);
    evaluationsPerGeneration = evaluationsPerGenerationPL->get(PT);
    useNewMsgBit = useNewMessageBit->get(PT) > 0;
    useSendMsgBit = useSendMessageBit->get(PT) > 0;
    useSendMsgVetoBit = (useSendMessageBit->get(PT) > 0) && (useSendMessageVetoBit->get(PT));
    useGetMsgBit = useGetMessageBit->get(PT) > 0;
    useGetMsgVetoBit = (useGetMessageBit->get(PT) > 0) && (useGetMessageVetoBit > 0);
    maxColors = maximumColors->get(PT);
    if(maxColors <= 0)
        maxColors = G.nodeCount;
    std::cout << "Maximum colors: " << maxColors << std::endl;
}

void GraphColorWorld::evaluateSolo(std::shared_ptr <Organism> org, int analyze, int visualize, int debug) {
    //store the org's brain
    auto originalBrain = org->brains[brainNamePL->get(PT)];

    //store some useful data about the original brain
    auto numBrainInputs = originalBrain->nrInputValues;
    auto numBrainOutputs = originalBrain->nrOutputValues;

    //clone the brain, one for each node
    //ALSO create a list of ints: used to visit each node exactly once in a random order each APL
    std::vector <std::shared_ptr<AbstractBrain>> cloneBrains(G.nodeCount);
    std::vector <size_t> nodeOrder(G.nodeCount); //TODO just shuffle the brain vector??
    for (size_t i = 0; i < G.nodeCount; i++) {
        cloneBrains[i] = originalBrain->makeCopy();
        nodeOrder[i] = i;
    }

    double score = 0.0;

    for (size_t eval = 0; eval < evaluationsPerGeneration; eval++) {
        //pre-lifetime setup
        score = 0.0;

        for (auto brain:cloneBrains) {
            brain->resetBrain();
        }

        //lifetime loop (action-perception loop)
        for (size_t t = 0; t < agentLifetime; t++) {
            // give agents their inputs
            for (auto brain:cloneBrains) {
                //TODO put real code in here
                for (int i = 0; i < numBrainInputs; i++) {
                    brain->setInput(i, 1);
                }
            }

            //update each agent (lets agents think for a single time unit)
            for (auto brain:cloneBrains) {
                brain->update();
            }

            //update the world according to each agent's chosen action (visit each node in an unbiased random order)
            std::random_shuffle(nodeOrder.begin(), nodeOrder.end(), randInt);
            for (auto brainID:nodeOrder) {
                //TODO put real code in here
                for (int i = 0; i < numBrainOutputs; i++) {
                    auto outputBit = Bit(cloneBrains[brainID]->readOutput(i));
                }
            }

        } //agent lifetime

        //end of life cleanup
        org->dataMap.append("score", score);
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
