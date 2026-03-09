# CubeTacToe by Stylianos Maimaris & Panayiotis Yiannoukkos

3D Tic-Tac-Toe played across the six faces of a cube. Built as a pair of prototypes for an employability portfolio university presentation for students in Game Development - one in **Godot 4 (C#)** with a medieval theme, one in **Unreal Engine** with a neon aesthetic. Same rules, two engines.

## Rules

- Each face is a standard 3×3 grid. Six faces = six simultaneous boards.
- On your turn you place **two pieces**, but after the first placement that face is **locked** - your second piece must go on a differet face.
- Win a face by getting three in a row.
- **Win conditions** (selectable at the menu):
  - *Any Side* - first to win any face wins the game.
  - *Majority* - first to win 4 of 6 faces wins.

## Modes

| Mode | Description |
|------|-------------|
| Local 2-Player | Pass-and-play on one device |
| vs AI | Easy (random) · Medium (minimax depth 3) · Hard (minimax depth 4-6) |
| Online (LAN) | Real-time peer-to-peer multiplayer (Godot) |

## Tech

- **Godot 4 / C#**
- **Unreal Engine 5 / BP + C++**
- **AI** - minimax with alpha-beta pruning, bitmask board state, background thread so the game loop stays responsive
- **HUD** - tween-driven panel scaling, animated turn label, face-win tracker
- **Audio** - adaptive music, contextual SFX

## Purpose

Prototype built to demonstrate how to showcase cross-engine skills, AI implementation, and networked multiplayer as part of a *Prototyping to Enhance Employability*.
