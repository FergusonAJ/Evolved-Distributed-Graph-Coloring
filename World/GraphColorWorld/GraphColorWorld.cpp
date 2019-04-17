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

std::shared_ptr <ParameterLink<int>> GraphColorWorld::modePL = Parameters::register_parameter("WORLD_GRAPH_COLOR-mode", 0, "0 = bit outputs before adding, 1 = add outputs");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::evaluationsPerGenerationPL = Parameters::register_parameter("WORLD_GRAPH_COLOR-evaluationsPerGeneration", 1, "Number of times to test each Genome per "
                                                                                                                                                                   "generation (useful with non-deterministic "
                                                                                                                                                                   "brains)");
std::shared_ptr <ParameterLink<int>> GraphColorWorld::agentLifetimePL = Parameters::register_parameter("WORLD_GRAPH_COLOR-agentLifetime", 1, "Number of time units the agents experience in a lifetime");
std::shared_ptr <ParameterLink<std::string>> GraphColorWorld::groupNamePL = Parameters::register_parameter("WORLD_GRAPH_COLOR_NAMES-groupNameSpace", (std::string) "root::", "namespace of group to be evaluated");
std::shared_ptr <ParameterLink<std::string>> GraphColorWorld::brainNamePL = Parameters::register_parameter("WORLD_GRAPH_COLOR_NAMES-brainNameSpace", (std::string) "root::", "namespace for parameters used to define brain");

std::shared_ptr <ParameterLink<std::string>> GraphColorWorld::graphFNamePL = Parameters::register_parameter("WORLD_GRAPH_COLOR-graphFileName", (std::string) "NONE", "The filename of the graph we will test on. NONE for random");

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
            std::random_shuffle(nodeOrder.begin(), nodeOrder.end());
            for (auto brainID:nodeOrder) {
                //TODO put real code in here
                for (int i = 0; i < numBrainOutputs; i++) {
                    auto outputBit = Bit(cloneBrains[brainID]->readOutput(i));
                }
            }

        } //agent lifetime
    } // evals per generation

    //end of life cleanup
    org->dataMap.append("score", score);
    if (visualize)
        std::cout << "organism with ID " << org->ID << " scored " << score << std::endl;
}