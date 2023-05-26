<h1 align="center"><b>Homura (炎)</b></h1>
<p align="center">
  <image src="/Art/HomuraLogoNoBG.png" width="200" height="200" ></image>
</p>
<h4 align="center"><b><i>UCI-Subset Chess Engine and Undergraduate Project</i></b></h4>

#

<p align="center">
  <image src="/Art/Mitakihara_Distance.png" ></image>
</p>

<h6 align="center"><i>"I'll do it over. No matter how many times it takes. I'll re-live it over and over again. I will find the way out... The one path that will save you from a destiny of despair." - Akemi Homura</i></h6>

## Index
1. [Introduction](https://github.com/RedBedHed/Homura/tree/main#introduction)
2. [Strength](https://github.com/RedBedHed/Homura/tree/main#strength)
3. [Search](https://github.com/RedBedHed/Homura/tree/main#search)
4. [Experimentation](https://github.com/RedBedHed/Homura/tree/main#experimentation-with-classical-mcts)
5. [Rollout Alpha-Beta](https://github.com/RedBedHed/Homura/tree/main#algorithm-4-by-dr-huang)
6. [Search Techniques](https://github.com/RedBedHed/Homura/tree/main#search-techniques)
7. [Move Generation](https://github.com/RedBedHed/Homura/tree/main#move-generation)
8. [UCI](https://github.com/RedBedHed/Homura/tree/main#uci)
9. [Build Homura](https://github.com/RedBedHed/Homura/tree/main#build-homura)
10. [Play Homura](https://github.com/RedBedHed/Homura/tree/main#play-homura)

## Introduction

Homura is a Chess Engine that I wrote from 2022-2023 as a branch of my move generator, Charon. As a full-time student (16 credit hours a semester), I made progress whenever and wherever I could&mdash; in campus libraries, coffee shops, on the bus, in bed. I read many papers and found inspiration in many engines, including Stockfish, Leela, Drofa, Scorpio, Fruit, CPW-Engine, PeSTO, Blunder, and Leorik. Homura derives from some of the ideas used in these engines&mdash; most notably PeSTO, Blunder, and Leorik.

## Strength
It is hard to say exactly how strong Homura is, as I ran out of time to implement the full UCI interface, and Homura only recognizes "movetime" or "infinite" time controls. After playing many games with Leorik-1.0 and Leorik-2.0, I believe that the elo might be somewhere between 2300 and 2550 at blitz chess. I also had the great privilege to intermediate a game between Homura and one of the top human players at my university. I am thrilled to be able to say that we won that game.

## Search
Homura uses a hybrid search algorithm that combines Dr. Bojun Huang's [Alpha-Beta rollout](https://www.microsoft.com/en-us/research/wp-content/uploads/2014/11/huang_rollout.pdf) algorithm with the traditional backtracking Alpha-Beta. All PV nodes are searched via rollout, while the remaining nodes are searched with backtracking and a null window. The rollout portion of the algorithm probes the transposition table each time that it enters a node, and its tree policy is a combination of leftmost and greedy selection.

Homura's algorithm&mdash; as a combination of novel and classical algorithms&mdash; searches the game tree in a different and exciting way. It also seems to outperform my backtracking search in matches administered by cutechess-cli.

## Experimentation With Classical MCTS

I initially experimented with the classical Monte Carlo Tree Search algorithm. I spent months trying to write a strong MCTS engine on my laptop, without the help of deep neural networks. I found this task to be insurmountable&mdash; at least with a UCT-based tree policy and short alpha-beta simulations. In desparation, I tried a greedier tree policy and deeper alpha-beta search. This showed some promise, and led me to attempt a full-scale classical search. I found that the classical search was much stronger than anything I had tried previously. This prompted me to do more research into the shortcomings of the MCTS algorithm when it comes to Chess. The general consensus is that classical MCTS simply lacks the tactical awareness to excel at Chess without a great deal of help. At around the same time, I found out about the Dr. Huang paper through Scorpio, which led me to begin work on the algorithm that Homura currently uses.

<ul>
  <li>
<h3>Shortcomings of MCTS for Chess</h3>

MCTS, with a UCT-based tree policy, will converge to the optimal line of play given an infinite amount of time ([L. Kocsis and C. Szepesvári](https://www.researchgate.net/publication/221112399_Bandit_Based_Monte-Carlo_Planning)). However, without strong evaluation and policies, it may not converge fast enough to play Chess competitively, falling short of the high standards set by Alpha-Beta.

Researchers have found that MCTS may miss traps and shallow tactics in sudden-death games like Chess ([R. Ramanujan, A. Sabharwal, and B. Selman](https://arxiv.org/abs/1203.4011)). It un-prunes a highly selective tree and searches its preferred lines much deeper than the others. Policies and values heavily influence its choice of nodes. When policies and values are weak from a tactical standpoint, the search is prone to overestimate or underestimate the value of a node and overlook critical lines in doing so.

Additionally, MCTS backpropagates a reward between 0 and 1 for each new leaf and considers the average reward of a child during selection. Information about life-or-death scenarios near the fringe may initially drown in the large values closer to the root. MCTS Solver can help the search detect imminent wins and losses more quickly ([M. H. M. Winands and Y. Björnsson](https://www.researchgate.net/publication/220962507_Monte-Carlo_Tree_Search_Solver)). However, the required “Secure Child” final selection policy&mdash; in my experience&mdash; hurts playing strength during the midgame, where wins and losses usually aren’t reachable if both sides play optimally.
 </li>
 <li>
<h3>Benefits of Alpha-Beta for Chess</h3>

An iterative deepening Alpha-Beta search prunes away what it can guarantee&mdash; or predict confidently&mdash; that it doesn’t need to see and visits the rest at some depth. It sees many lines, even those that MCTS would abandon. Because of this, Alpha-Beta is much better-equipped to pick up on subtle tactics and traps. It is much harder to force the algorithm into a bad position. One could also say that it is much easier for the algorithm to fight its way into a good position.

Additionally, the score backpropagated to the root by Alpha-Beta is the exact heuristic score of the board after the players play the principal variation. Therefore, Alpha-Beta has no issues detecting imminent wins and losses within its search tree. It can naturally prove whether one of the root’s children is a win or a loss, and&mdash; with the right evaluation&mdash; prefer wins closer to the root.
 </li>
</ul>  
   
## Algorithm 4 by Dr. Huang

The Rollout Paradigm is a general algorithmic paradigm that involves searching a game tree in iterations. Each iteration— called a rollout— starts at the root and executes a line of play to some depth, performing some variation of the phases commonly associated with MCTS. These algorithms build the tree in memory as they complete each rollout, storing information
to be used by their tree policy in subsequent rollouts. Classical MCTS is the flagship algorithm of this paradigm. However, while MCTS takes a very cautious and selective approach to search, expanding one node per rollout, not all rollout algorithms are limited to this strategy.

Algorithm 4&mdash; Huang's Alpha-Beta rollout algorithm&mdash; still relies on the four common MCTS phases. However, the order and quantity of expansions per rollout may differ. 

<p align="center">
  <image src="/Art/Alpha_Beta_Rollout.png" ></image>
</p>

<h6 align="center"><i>An Alpha-Beta rollout to depth 3</i></h6>

Each node contains bounds 𝛼, 𝛽, 𝑉-, and 𝑉+. The search initializes 𝛼 and 𝑉- to negative infinity and 𝛽 and 𝑉+ to positive infinity.

<ol>
  <li>
    <h3>Selection and Expansion</h3>
The Selection step is intertwined with the Expansion step. At the beginning of a rollout, the search sends a depth-limited probe from the root node out into the tree, expanding as needed and selecting a child at each intermediate node. During each selection, the search filters children according to their 𝛼 and 𝛽 bounds which it updates using 𝑉-, 𝑉+, and the 𝛼 and 𝛽 of the current node.
  </li>
  <li>
    <h3>Simulation</h3>
In the Simulation step, the search evaluates the selected leaf node by quiescence search. It then sets Bounds 𝑉- and 𝑉+&mdash; and the score&mdash; to the value returned.
  </li>
  <li>
    <h3>Backpropagation</h3>
In the Backpropagation step, the search pushes 𝑉+, 𝑉-, and the score up the tree in the negamax style. Once 𝑉- ≥ 𝑉+ at a node, its score is accurate, and the search has seen everything it needs to see in the tree below. When 𝑉- ≥ 𝑉+ at the root, its score and principal variation are valid. The best move is the move on the principal variation.
  </li>
</ol>

<p align="center">
  <image src="/Art/BackpropAB.png" ></image>
</p>

<h6 align="center"><i>Backpropagation of bounds and score</i></h6>

## Search Techniques

Homura's search is nearly single-threaded, relying on only one extra thread for garbage collection. It uses both classical and novel techniques.

### // *Base Search* //
- Iterative Deepening
- PVS with PV-nodes searched by [rollout](https://github.com/RedBedHed/Homura/blob/main/src/Rollout.cpp)
- Non-PV-nodes searched with [backtracking](https://github.com/RedBedHed/Homura/blob/main/src/Backtrack.cpp)
- Internal Iterative Deepening by backtracking
- Quiescence Search
- Transposition table with two buckets and clock-based aging

### // *Selectivity* //
- Static Null Move Pruning
- Null Move Pruning
- Razoring
- Futility Pruning
- Late Move Pruning
- Late Move Reductions

### // *Move Ordering* //
- PV Move (Hash Move)
- MVV-LVA Attacks
- Killer Quiets
- History Quiets

### // *Evaluation* //
- Ronald Friedrich's tapered PeSTO evaluation

### // *Tree Policy* //
- Leftmost Selection (classical Alpha-Beta)
    - Used in both backtracking and rollout search.
- Greedy Selection (classical MCTS)
    - Used exclusively in rollout search, near the leaves, if re-searches occur late in the list.
    - Picks the child with the highest negamax value so far.

## Move Generation

Homura implements a strictly legal move generator with either PEXT or Fancy Magic for sliding attacks depending on the target CPU. It is heavily inspired by Stockfish, but differs in many ways. This move generator is very fast&mdash; a fact that I have been using as a bit of a crutch. Homura doesn't implement staged move generation, and I consider this to be a design flaw.

```
Startup  -  0.001 seconds

perft(1) -  0.000 seconds -         20 nodes visited.
perft(2) -  0.000 seconds -        400 nodes visited.
perft(3) -  0.000 seconds -       8902 nodes visited.
perft(4) -  0.001 seconds -     197281 nodes visited.
perft(5) -  0.012 seconds -    4865609 nodes visited.
perft(6) -  0.249 seconds -  119060324 nodes visited.
```
<h6 align="center"><i>Perft 6&mdash; from the starting position</i></h6>

## UCI

Homura recognizes a small subset of uci commands.

<ol>
  <li>
    <h3><i>uci</i></h3>
This command asks Homura to identify itself and confirm that it is a UCI Chess engine. It responds with:
   <br/><br/>
   <blockquote>
      <i>id name Homura</i><br/>
      <i>id author Ellie Moore</i><br/>
      <i>uciok</i>
   </blockquote>
  </li>
  <li>
    <h3><i>ucinewgame</i></h3>
This command re-initializes Homura’s board state to the starting position and resets the game-history stack.
  </li>
  <li>
    <h3><i>isready</i></h3>
This command asks Homura if it is available to start a search. It responds with:
    <br/><br/>
    <blockquote>
      <i>readyok</i>
    </blockquote>
  </li>
  <li>
    <h3><i>go infinite</i></h3>
This command tells Homura to start a five second search from the current position. After searching, it responds with:
    <br/><br/>
    <blockquote>
      <i>bestmove &lt;move in algebraic notation&gt;</i>
    </blockquote>
  </li>
  <li>
    <h3><i>go movetime &lt;milliseconds&gt;</i></h3>
This command tells Homura to search from the current position for the given time in
milliseconds. After searching, it responds with:
    <br/><br/>
    <blockquote>
      <i>bestmove &lt;move in algebraic notation&gt;</i>
    </blockquote>
  </li>
  <li>
    <h3><i>position startpos &lt;list of algebraic moves&gt;</i></h3>
This command tells Homura to update the state of the board, playing the move at the
end of the list of given moves. (This seemed to be the easiest implementation)
  </li>
</ol>

## Build Homura

I developed Homura with a "Windows Subsystem for Linux" Ubuntu distribution on a Windows 11 (Home) laptop. My processor is an 11th generation Intel processor with four physical cores. I have 16 GB of RAM. Although it may be possible to build and use Homura in other environments, I cannot guarantee that it will work correctly elsewhere.

You may build the project by cloning the repository, opening a Bash shell in the project directory, navigating to the src directory, and typing “make.” These steps assume you already have the compilation tools “clang++” and “make” installed.

To run the executable file, you must include the "ospec.txt" file in the executable file's directory. The "ospec.txt" file is needed for the lexical analyzer to correctly lex the supported UCI commands.

## Play Homura

To play against Homura, you must install a UCI-compatible Chess GUI such as “Cutechess." The infinite time control will give you unlimited time to make a move, and limit Homura to five seconds.

## Open Source

Homura is open source with an MIT license. If for any reason you actually like the project, feel free to do stuff with it! :)

#

<h6 align="center"><i>Disclaimer: I used DALL-E 2 to generate Homura's logo. The image beneath comes from the anime movie PMMM: Rebellion.</i></h6>
