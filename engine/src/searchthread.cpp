/*
  CrazyAra, a deep learning chess variant engine
  Copyright (C) 2018       Johannes Czech, Moritz Willig, Alena Beyer
  Copyright (C) 2019-2020  Johannes Czech

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
 * @file: searchthread.cpp
 * Created on 23.05.2019
 * @author: queensgambit
 */

#include "searchthread.h"
#include "inputrepresentation.h"
#include "outputrepresentation.h"
#include "util/blazeutil.h"
#include "uci.h"

SearchThread::SearchThread(NeuralNetAPI *netBatch, SearchSettings* searchSettings, MapWithMutex* mapWithMutex):
    netBatch(netBatch), isRunning(false), mapWithMutex(mapWithMutex), searchSettings(searchSettings)
{
    // allocate memory for all predictions and results
    inputPlanes = new float[searchSettings->batchSize * NB_VALUES_TOTAL];
    valueOutputs = new NDArray(Shape(searchSettings->batchSize, 1), Context::cpu());

    if (netBatch->is_policy_map()) {
        probOutputs = new NDArray(Shape(searchSettings->batchSize, NB_LABELS_POLICY_MAP), Context::cpu());
    } else {
        probOutputs = new NDArray(Shape(searchSettings->batchSize, NB_LABELS), Context::cpu());
    }
    searchLimits = nullptr;  // will be set by set_search_limits() every time before go()
}

SearchThread::~SearchThread()
{
    delete [] inputPlanes;
    delete valueOutputs;
    delete probOutputs;
}

void SearchThread::set_root_node(Node *value)
{
    rootNode = value;
}

void SearchThread::set_search_limits(SearchLimits *s)
{
    searchLimits = s;
}

bool SearchThread::get_is_running() const
{
    return isRunning;
}

void SearchThread::set_is_running(bool value)
{
    isRunning = value;
}

void SearchThread::add_new_node_to_tree(Node* parentNode, size_t childIdx)
{
    StateInfo* newState = new StateInfo;
    Board* newPos = new Board(*parentNode->get_pos());
    newPos->do_move(parentNode->get_move(childIdx), *newState);

    mapWithMutex->mtx.lock();
    unordered_map<Key, Node*>::const_iterator it = mapWithMutex->hashTable->find(newPos->hash_key());
    mapWithMutex->mtx.unlock();
    if(searchSettings->useTranspositionTable && it != mapWithMutex->hashTable->end() &&
            is_transposition_verified(it, newPos->get_state_info())) {
        Node *newNode = new Node(*it->second);
        parentNode->add_transposition_child_node(newNode, newPos, childIdx);

        parentNode->increment_no_visit_idx();
        transpositionNodes.push_back(newNode);
    }
    else {
        parentNode->increment_no_visit_idx();
        assert(parentNode != nullptr);
        Node *newNode = new Node(newPos, parentNode, childIdx, searchSettings);
        // fill a new board in the input_planes vector
        // we shift the index by NB_VALUES_TOTAL each time
        board_to_planes(newNode->get_pos(), newNode->get_pos()->number_repetitions(), true, inputPlanes+newNodes.size()*NB_VALUES_TOTAL);

        // connect the Node to the parent
        parentNode->add_new_child_node(newNode, childIdx);

        // save a reference newly created list in the temporary list for node creation
        // it will later be updated with the evaluation of the NN
        newNodes.push_back(newNode);
    }
}

void SearchThread::stop()
{
    isRunning = false;
}

Node *SearchThread::get_root_node() const
{
    return rootNode;
}

SearchLimits *SearchThread::get_search_limits() const
{
    return searchLimits;
}

Node* get_new_child_to_evaluate(Node* rootNode, size_t& childIdx, NodeDescription& description)
{
    Node* currentNode = rootNode;
    description.depth = 0;
    while (true) {
        currentNode->lock();
        childIdx = currentNode->select_child_node();
        currentNode->apply_virtual_loss_to_child(childIdx);
        Node* nextNode = currentNode->get_child_node(childIdx);
        description.depth++;
        if (nextNode == nullptr) {
            description.isCollision = false;
            description.isTerminal = false;
            currentNode->unlock();
            return currentNode;
        }
        if (nextNode->is_terminal()) {
            description.isCollision = false;
            description.isTerminal = true;
            currentNode->unlock();
            return currentNode;
        }
        if (!nextNode->has_nn_results()) {
            description.isCollision = true;
            description.isTerminal = false;
            currentNode->unlock();
            return currentNode;
        }
        currentNode->unlock();
        currentNode = nextNode;
    }
}

void SearchThread::set_nn_results_to_child_nodes()
{
    size_t batchIdx = 0;
    for (auto node: newNodes) {
        if (!node->is_terminal()) {
            fill_nn_results(batchIdx, netBatch->is_policy_map(), valueOutputs, probOutputs, node, searchSettings->nodePolicyTemperature);
        }
        ++batchIdx;
        mapWithMutex->mtx.lock();
        mapWithMutex->hashTable->insert({node->get_pos()->hash_key(), node});
        mapWithMutex->mtx.unlock();
    }
}

void SearchThread::backup_value_outputs()
{
    backup_values(newNodes);
    backup_values(transpositionNodes);
    backup_values(terminalNodes);
}

void SearchThread::backup_collisions()
{
    for (auto node: collisionNodes) {
        node->get_parent_node()->backup_collision(node->get_child_idx_for_parent());
    }
    collisionNodes.clear();
}

bool SearchThread::nodes_limits_ok()
{
    return searchLimits->nodes == 0 || (rootNode->get_visits() < searchLimits->nodes);
}

void SearchThread::create_mini_batch()
{
    // select nodes to add to the mini-batch
    Node *parentNode;
    NodeDescription description;
    size_t childIdx;

    while (newNodes.size() < searchSettings->batchSize &&
           collisionNodes.size() < searchSettings->batchSize &&
           transpositionNodes.size() < searchSettings->batchSize &&
           terminalNodes.size() < searchSettings->batchSize) {
        parentNode = get_new_child_to_evaluate(rootNode, childIdx, description);

        if(description.isTerminal) {
            terminalNodes.push_back(parentNode->get_child_node(childIdx));
        }
        else if (description.isCollision) {
            // store a pointer to the collision node in order to revert the virtual loss of the forward propagation
            collisionNodes.push_back(parentNode->get_child_node(childIdx));
        }
        else {
            add_new_node_to_tree(parentNode, childIdx);
        }
    }
}

void SearchThread::thread_iteration()
{
    create_mini_batch();
    if (newNodes.size() != 0) {
        netBatch->predict(inputPlanes, *valueOutputs, *probOutputs);
        set_nn_results_to_child_nodes();
    }
    backup_value_outputs();
    backup_collisions();
}

void go(SearchThread *t)
{
    t->set_is_running(true);
    while(t->get_is_running() && t->nodes_limits_ok()) {
        t->thread_iteration();
    }
}

void backup_values(vector<Node*>& nodes)
{
    for (auto node: nodes) {
        node->get_parent_node()->backup_value(node->get_child_idx_for_parent(), -node->get_value());
    }
    nodes.clear();
}

void prepare_node_for_nn(Node* newNode, vector<Node*>& newNodes, float* inputPlanes)
{
    // fill a new board in the input_planes vector
    // we shift the index by NB_VALUES_TOTAL each time
    board_to_planes(newNode->get_pos(), newNode->get_pos()->number_repetitions(), true, inputPlanes+newNodes.size()*NB_VALUES_TOTAL);

    // save a reference newly created list in the temporary list for node creation
    // it will later be updated with the evaluation of the NN
    newNodes.push_back(newNode);
}

void fill_nn_results(size_t batchIdx, bool isPolicyMap, NDArray* valueOutputs, NDArray* probOutputs, Node *node, float temperature)
{
    node->set_probabilities_for_moves(get_policy_data_batch(batchIdx, probOutputs, isPolicyMap), get_current_move_lookup(node->side_to_move()));
    if (!isPolicyMap) {
        node->apply_softmax_to_policy();
    }
    node->apply_temperature_to_prior_policy(temperature);
    node->set_value(valueOutputs->At(batchIdx, 0));
    node->enable_has_nn_results();
}

bool is_transposition_verified(const unordered_map<Key,Node*>::const_iterator& it, const StateInfo* stateInfo) {
    return  it->second->has_nn_results() &&
            it->second->get_pos()->get_state_info()->pliesFromNull == stateInfo->pliesFromNull &&
            it->second->get_pos()->get_state_info()->rule50 == stateInfo->rule50 &&
            stateInfo->repetition == 0;
}

