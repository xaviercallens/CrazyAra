/*
  CrazyAra, a deep learning chess variant engine
  Copyright (C) 2018  Johannes Czech, Moritz Willig, Alena Beyer
  Copyright (C) 2019  Johannes Czech

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
 * @file: rlsettings.h
 * Created on 26.11.2019
 * @author: queensgambit
 *
 * Additional settings for the MCTS search when used during selfplay
 */

#ifndef RLSETTINGS_H
#define RLSETTINGS_H

#include <stddef.h>

struct RLSettings
{
    // number of chunks in a single export file
    size_t numberChunks;
    // size of a single chunk, the product of chunkSize and numberChunks is the amount of samples in an export file
    size_t chunkSize;
    // the amount of nodes for the next search is slightly pertubated by sampling from +/- nodeRandomFactor * nodes
    float nodeRandomFactor;
    // Clips values in policy after move selection below this threshold to 0 in order to reduce noise from dirichletNoise in target policy
    float lowPolicyClipThreshold;
    // Playout Cap Randomization (David J. Wu): https://arxiv.org/pdf/1902.10565.pdf
    // probability that the full MCTS search is ommitted and a quick search with less nodes is done instead
    // the resulting policy won't be saved as a training sample
    float quickSearchProbability;
    // amount of nodes to be used for quick search
    size_t quickSearchNodes;
    // Q-value weight applied on final policy
    // (has been validated to increase strength for low node count: Czech et al., https://arxiv.org/abs/1908.06660)
    float quickSearchQValueWeight;
    // dirchlet noise applied for quick search (recommended is 0, for maximum strength)
    float quickDirichletEpsilon;
};

#endif // RLSETTINGS_H
