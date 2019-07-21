# CrazyAra - A Deep Learning Chess Variant Engine
![CRAZYARA_LOGO](media/CrazyAra_Logo.png "rc")

## Table of Contents
* [Description](#description)
* [Download](#download)
* [Compilation](#compilation)
    * [Linux](#linux)
    * [Windows](#windows)
* [Libraries](#libraries)
* [Licence](#licence)

<img align="right" src="media/TU_logo.png" width="128">

## Description

[CrazyAra](https://crazyara.org/) is an open-source neural network based engine, initially developed in pure python by [Johannes Czech](https://github.com/QueensGambit), [Moritz Willig](https://github.com/MoritzWillig) and Alena Beyer in 2018.
It started as a semester project at the [Technische Universität Darmstadt](https://www.tu-darmstadt.de/index.en.jsp) with the goal to train a neural network to play the chess variant [crazyhouse](https://en.wikipedia.org/wiki/Crazyhouse) via supervised learning on human data.
The project was part of the course [_"Deep Learning: Architectures & Methods"_](https://piazza.com/tu-darmstadt.de/summer2019/20001034iv/home) held by [Prof. Dr. Kristian Kersting](https://ml-research.github.io/people/kkersting/index.html), [Prof. Dr. Johannes Fürnkranz](http://www.ke.tu-darmstadt.de/staff/juffi) et al. in summer 2018. 

The development was continued and the engine ported to C++ by [Johannes Czech](https://github.com/QueensGambit). In the course of a master thesis supervised by [Karl Stelzner](https://ml-research.github.io/people/kstelzner/) and [Prof. Dr. Kristian Kersting](https://ml-research.github.io/people/kkersting/index.html), the engine will learn crazyhouse in a reinforcement learning setting and ported to play other chess variants including classical chess.

The project is mainly inspired by the techniques described in the [Alpha-(Go)-Zero papers](https://arxiv.org/abs/1712.01815) by [David Silver](https://arxiv.org/search/cs?searchtype=author&query=Silver%2C+D), [Thomas Hubert](https://arxiv.org/search/cs?searchtype=author&query=Hubert%2C+T), [Julian Schrittwieser](https://arxiv.org/search/cs?searchtype=author&query=Schrittwieser%2C+J), [Ioannis Antonoglou](https://arxiv.org/search/cs?searchtype=author&query=Antonoglou%2C+I), [Matthew Lai](https://arxiv.org/search/cs?searchtype=author&query=Lai%2C+M), [Arthur Guez](https://arxiv.org/search/cs?searchtype=author&query=Guez%2C+A), [Marc Lanctot](https://arxiv.org/search/cs?searchtype=author&query=Lanctot%2C+M), [Laurent Sifre](https://arxiv.org/search/cs?searchtype=author&query=Sifre%2C+L), [Dharshan Kumaran](https://arxiv.org/search/cs?searchtype=author&query=Kumaran%2C+D), [Thore Graepel](https://arxiv.org/search/cs?searchtype=author&query=Graepel%2C+T), [Timothy Lillicrap](https://arxiv.org/search/cs?searchtype=author&query=Lillicrap%2C+T), [Karen Simonyan](https://arxiv.org/search/cs?searchtype=author&query=Simonyan%2C+K), [Demis Hassabis](https://arxiv.org/search/cs?searchtype=author&query=Hassabis%2C+D).

This repository contains the source code of the engine search written in C++ based which based on the previous [Python version](https://github.com/QueensGambit/CrazyAra).
The training scripts, preprocessing and neural network definition source files can be found in the [Python version](https://github.com/QueensGambit/CrazyAra).

## Download
__TODO__

## Compilation

Please follow the given step to build CrazyAra from source.
### Linux
1. Download and install the [**Blaze**](https://bitbucket.org/blaze-lib/blaze/src/master/) version **>=3.6**:
* https://bitbucket.org/blaze-lib/blaze/wiki/Configuration%20and%20Installation
* https://bitbucket.org/blaze-lib/blaze/downloads/

Build the MXNet C++ package
* https://mxnet.incubator.apache.org/versions/master/api/c++/index.html

```$ make -j USE_CPP_PACKAGE=1 USE_OPENCV=0 USE_MKL=1```

2. Download & install yaml-cpp 
* https://github.com/jbeder/yaml-cpp

or alternative for Linux Debian systems

`$ sudo apt-get install libyaml-cpp-dev`

### Windows
__TODO__

## Libraries
The following libraries are used to run CrazyAra:

* [**Multi Variant Stockfish**](https://github.com/ddugovic/Stockfish): Stockfish fork specialized to play chess and some chess variants
	* Used for move generation and board representation as a replacement for [python-chess](https://github.com/niklasf/python-chess).
* [**MXNet C++ Package**](https://github.com/apache/incubator-mxnet/tree/master/cpp-package): A flexible and efficient library for deep learning
	* Used as the deep learning backend for loading and inference of the trained neural network as a replacment for the [MXNet python package](https://pypi.org/project/mxnet/)
* [**Blaze**](https://bitbucket.org/blaze-lib/blaze/src/master/): An open-source, high-performance C++ math library for dense and sparse arithmetic
	* Used for arithmeic, numerical vector operation within the MCTS search as a replacement for [NumPy](https://numpy.org/)
* [**Catch2**](https://github.com/catchorg/Catch2): A multi-paradigm test framework for C++
	* Used as the testing framework as a replacmenet for [Python's unittest framework](https://docs.python.org/3/library/unittest.html)
* [**yaml-cpp**](https://github.com/jbeder/yaml-cpp): A YAML parser and emitter in C++
	* Used for loading the .yaml configuration file

## Licence

CrazyAra is free software, and distributed under the terms of the GNU General Public License version 3 (GPL v3).

For details, please refer to the GPL v3 license definition which ca be found in the file [LICENSE](https://github.com/QueensGambit/CrazyAraMCTS/blob/master/LICENSE).

