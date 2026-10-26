# Connect Four AI

A console Connect Four game in C with three bot difficulties. The hardest searches the game tree in parallel, one thread per column, using Minimax with Alpha-Beta pruning.

<!-- TODO: add a short terminal GIF of a game against the Hard bot, e.g. docs/demo.gif -->
![Connect Four AI demo](docs/demo.gif)

## Features

- **Four modes:** Easy, Medium and Hard bots, plus local two-player.
- **Easy bot:** picks a random valid column.
- **Medium bot:** wins if it can, blocks your immediate win, avoids moves that hand you a win, then picks the best 1-ply heuristic score.
- **Hard bot:** depth-6 Minimax with Alpha-Beta pruning, with each root move evaluated on its own POSIX thread.
- **Input handling:** columns 1–7, `q` to quit, and Ctrl+D exits cleanly.

## How the Hard bot works

**Parallel search at the root.** For each column that isn't full, the bot drops its piece on a private copy of the board and starts a `pthread` to score that move. The threads share nothing except the read-only starting board, so no locks are needed. Once every thread is joined, the bot picks the highest score, checking columns from the centre outwards so ties go to central moves.

**Minimax with Alpha-Beta pruning.** Each thread runs a depth-limited Minimax that alternates between maximising the bot's score and minimising it for the opponent. Alpha and beta carry the best score each side is already guaranteed. As soon as `beta <= alpha`, the remaining sibling moves can't change the result, so that branch is cut. Immediate wins and losses return early, weighted by depth so the bot prefers faster wins and slower losses.

**Evaluation.** Leaf positions are scored by sliding a 4-cell window across every row, column and diagonal. Three-in-a-row with an open cell is worth +500 for the bot and −450 for the opponent. Two-in-a-row is worth +50 / −40. Pieces in the centre column add a small bonus.

## Tech stack

C11 · POSIX threads (`pthread`) · Make · GCC

## Run it locally

Requires GCC (or Clang) and Make, on Linux, macOS or WSL.

```bash
git clone https://github.com/Ina-06/Connect4KHI.git
cd Connect4KHI
make
./connect4
```

Choose a mode from the menu, then enter a column number (1–7) on your turn.

## Project layout

| File | Responsibility |
| --- | --- |
| `board.c/.h` | 6×7 board, dropping pieces, win and full-board detection |
| `game.c/.h` | Game loop, the three bots, heuristic evaluation, parallel Minimax |
| `io.c/.h` | Reading and validating column input |
| `main.c` | Entry point |
