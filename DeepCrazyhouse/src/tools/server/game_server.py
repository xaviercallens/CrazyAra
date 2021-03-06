"""
@file: game_server.py
Changed last on 17.01.19
@project: crazy_ara_cleaning
@author: queensgambit and matuiss2

Handle the game server

This file contains everything related to handling the game server
"""
import json
import logging
import chess
from flask import Flask, request, send_from_directory
from DeepCrazyhouse.src.domain.agent.neural_net_api import NeuralNetAPI
from DeepCrazyhouse.src.domain.agent.player.mcts_agent import MCTSAgent
from DeepCrazyhouse.src.domain.agent.player.raw_net_agent import RawNetAgent
from DeepCrazyhouse.src.domain.variants.game_state import GameState


FILE_LOOKUP = {"A": 0, "B": 1, "C": 2, "D": 3, "E": 4, "F": 5, "G": 6, "H": 7}
RANK_LOOKUP = {"1": 0, "2": 1, "3": 2, "4": 3, "5": 4, "6": 5, "7": 6, "8": 7}


def get_square_index_from_name(name):
    """ Get the board coordinates using the received annotation"""
    if not name:
        return None

    if len(name) != 2:
        return None

    col = FILE_LOOKUP[name[0]] if name[0] in FILE_LOOKUP else None
    row = RANK_LOOKUP[name[1]] if name[1] in RANK_LOOKUP else None
    if not col or not row:
        return None

    return chess.square(col, row)


BATCH_SIZE = 8
NB_PLAYOUTS = 16
CPUCT = 1
DIRICHLET_EPSILON = 0.25
NB_WORKERS = 64


class ChessServer:
    """ Helper for handling the game server"""

    def __init__(self, name):
        self.app = Flask(name)
        self.app.add_url_rule("/api/state", "api/state", self._wrap_endpoint(ChessServer.serve_state))
        self.app.add_url_rule("/api/new", "api/new", self._wrap_endpoint(ChessServer.serve_new_game))
        self.app.add_url_rule("/api/move", "api/move", self._wrap_endpoint(ChessServer.serve_move))
        self.app.add_url_rule("/", "serve_client_r", self._wrap_endpoint(ChessServer.serve_client))
        self.app.add_url_rule("/<path:path>", "serve_client", self._wrap_endpoint(ChessServer.serve_client))
        self._gamestate = GameState()
        net = NeuralNetAPI()
        # Loading network
        player_agents = {
            "raw_net": RawNetAgent(net),
            "mcts": MCTSAgent(
                net, virtual_loss=3, threads=BATCH_SIZE, cpuct=CPUCT, dirichlet_epsilon=DIRICHLET_EPSILON
            ),
        }
        self.agent = player_agents["raw_net"]  # Setting up agent
        # self.agent = player_agents["mcts"]

    def _wrap_endpoint(self, func):
        """TODO: docstring"""

        def wrapper(kwargs):
            return func(self, **kwargs)

        return lambda **kwargs: wrapper(kwargs)

    def run(self):
        """ Run the flask server"""
        self.app.run()

    @staticmethod
    def serve_client(path=None):
        """Find the client server path"""
        if path is None:
            path = "index.html"
        return send_from_directory("./client", path)

    def serve_state(self):
        """TODO: docstring"""
        return self.serialize_game_state()

    def serve_new_game(self):
        """TODO: docstring"""
        logging.debug("staring new game()")
        self.perform_new_game()
        return self.serialize_game_state()

    def serve_move(self):
        """ Groups the move requests and data to the server and the response from it"""
        # read move data
        drop_piece = request.args.get("drop")
        from_square = request.args.get("from")
        to_square = request.args.get("to")
        promotion_piece = request.args.get("promotion")
        from_square_idx = get_square_index_from_name(from_square)
        to_square_idx = get_square_index_from_name(to_square)
        if (from_square_idx is None and drop_piece is None) or to_square_idx is None:
            return self.serialize_game_state("board name is invalid")

        promotion = drop = None

        if drop_piece:
            from_square_idx = to_square_idx
            if not drop_piece in chess.PIECE_SYMBOLS:
                return self.serialize_game_state("drop piece name is invalid")
            drop = chess.PIECE_SYMBOLS.index(drop_piece)

        if promotion_piece:
            if not promotion_piece in chess.PIECE_SYMBOLS:
                return self.serialize_game_state("promotion piece name is invalid")
            promotion = chess.PIECE_SYMBOLS.index(promotion_piece)

        move = chess.Move(from_square_idx, to_square_idx, promotion, drop)

        # perform move
        try:
            self.perform_move(move)
        except ValueError as err:
            logging.error("ValueError %s", err)
            return self.serialize_game_state(err.args[0])

        # calculate agent response
        if not self.perform_agent_move():
            return self.serialize_game_state("Black has no more moves to play", True)

        return self.serialize_game_state()

    def perform_new_game(self):
        """Initialize a new game on the server"""
        self._gamestate = GameState()

    def perform_move(self, move):
        """ Apply the move on the game and check if the legality of it"""
        logging.debug("perform_move(): %s", move)
        # check if move is valid
        if move not in list(self._gamestate.board.legal_moves):
            raise ValueError("The given move %s is invalid for the current position" % move)
        self._gamestate.apply_move(move)
        if self._gamestate.is_loss():
            logging.debug("Checkmate")
            return False
        return None

    def perform_agent_move(self):
        """TODO: docstring"""
        if self._gamestate.is_loss():
            logging.debug("Checkmate")
            return False

        value, move, _, _ = self.agent.perform_action(self._gamestate)

        if not self._gamestate.is_white_to_move():
            value = -value

        logging.debug("Value %.4f", value)

        if move is None:
            logging.error("None move proposed!")
            return False

        self.perform_move(move)
        return True

    def serialize_game_state(self, message=None, finished=None):
        """ Encodes the game state to a .json file"""
        if message is None:
            message = ""

        board_str = "" + self._gamestate.board.__str__()
        pocket_str = "" + self._gamestate.board.pockets[1].__str__() + "|" + self._gamestate.board.pockets[0].__str__()
        state = {"board": board_str, "pocket": pocket_str, "message": message}
        if finished:
            state["finished"] = finished
        return json.dumps(state)


print("Setting up server")
SERVER = ChessServer("DeepCrazyHouse")
print("RUN")
SERVER.run()
print("SHUTDOWN")
