using System;
using System.Collections.Generic;
using System.Threading.Tasks;

public class AIController
{
    private readonly GameData.AIDifficulty _difficulty;
    private readonly Random _rng;

    private const int AI_PLAYER = 2;
    private const int HUMAN_PLAYER = 1;

    private static readonly int[][] WinPatterns =
    {
        new[] { 0, 1, 2 }, new[] { 3, 4, 5 }, new[] { 6, 7, 8 },
        new[] { 0, 3, 6 }, new[] { 1, 4, 7 }, new[] { 2, 5, 8 },
        new[] { 0, 4, 8 }, new[] { 2, 4, 6 }
    };

    // Bitmask where bit i is set if cell i belongs to pattern p
    private static readonly int[] WinMasks;

    // Maps a power-of-2 value (1<<i, i in 0-8) to the bit index i
    private static readonly int[] Lsb2Idx = new int[512];

    // Positional cell scores (center > corner > edge)
    private static readonly int[] CellOrderScore = { 5, 0, 5, 0, 10, 0, 5, 0, 5 };

    static AIController()
    {
        WinMasks = new int[WinPatterns.Length];
        for (int i = 0; i < WinPatterns.Length; i++)
        {
            var p = WinPatterns[i];
            WinMasks[i] = (1 << p[0]) | (1 << p[1]) | (1 << p[2]);
        }
        for (int i = 0; i < 9; i++)
            Lsb2Idx[1 << i] = i;
    }

    public AIController(GameData.AIDifficulty difficulty, int? seed = null)
    {
        _difficulty = difficulty;
        _rng = seed.HasValue ? new Random(seed.Value) : new Random();
    }

    public (int face, int cell) PickMove(
        CubeFace[] faces, int lockedFaceIndex, GameData.WinCondition winCondition)
    {
        var state = Snapshot(faces, lockedFaceIndex, winCondition);
        return _difficulty switch
        {
            GameData.AIDifficulty.Easy => PickRandom(ref state),
            GameData.AIDifficulty.Medium => RunMinimax(ref state, depth: 3),
            GameData.AIDifficulty.Hard => RunMinimax(ref state, depth: 6),
            _ => PickRandom(ref state),
        };
    }

    // Async snapshots Godot state on the calling (main) thread,
    // then runs the search on a thread-pool thread so the game loop stays responsive.
    public Task<(int face, int cell)> PickMoveAsync(
        CubeFace[] faces, int lockedFaceIndex, GameData.WinCondition winCondition)
    {
        var state = Snapshot(faces, lockedFaceIndex, winCondition);
        if (_difficulty == GameData.AIDifficulty.Easy)
            return Task.FromResult(PickRandom(ref state));

        int depth = _difficulty == GameData.AIDifficulty.Hard ? 6 : 3;
        return Task.Run(() => SearchMove(state, depth));
    }

    private (int, int) SearchMove(BoardState state, int depth)
        => RunMinimax(ref state, depth);

    private struct FaceState
    {
        public int Winner;
        public int AiBits;
        public int HumanBits;
    }

    private struct BoardState
    {
        public FaceState[] Faces;
        public int CurrentPlayer;
        public int MovesThisTurn;
        public int LockedFace;
        public int GameWinner;
        public GameData.WinCondition WinCondition;
    }

    private static BoardState Snapshot(
        CubeFace[] faces, int lockedFaceIndex, GameData.WinCondition wc)
    {
        var s = new BoardState
        {
            Faces = new FaceState[6],
            CurrentPlayer = AI_PLAYER,
            MovesThisTurn = lockedFaceIndex >= 0 ? 1 : 0,
            LockedFace = lockedFaceIndex,
            WinCondition = wc,
        };
        for (int fi = 0; fi < 6; fi++)
        {
            int ai = 0, hm = 0;
            for (int ci = 0; ci < 9; ci++)
            {
                int v = faces[fi].GetCell(ci);
                if (v == AI_PLAYER) ai |= 1 << ci;
                else if (v == HUMAN_PLAYER) hm |= 1 << ci;
            }
            s.Faces[fi] = new FaceState
            {
                Winner = faces[fi].Winner,
                AiBits = ai,
                HumanBits = hm
            };
        }
        return s;
    }

    private struct Undo
    {
        public int Fi, Ci, Player;
        public int PrevFaceWinner, PrevGameWinner;
        public int PrevCurrentPlayer, PrevMovesThisTurn, PrevLockedFace;
    }

    private static Undo Apply(ref BoardState s, int fi, int ci)
    {
        int pl = s.CurrentPlayer;
        var u = new Undo
        {
            Fi = fi,
            Ci = ci,
            Player = pl,
            PrevFaceWinner = s.Faces[fi].Winner,
            PrevGameWinner = s.GameWinner,
            PrevCurrentPlayer = s.CurrentPlayer,
            PrevMovesThisTurn = s.MovesThisTurn,
            PrevLockedFace = s.LockedFace,
        };

        // Place piece
        if (pl == AI_PLAYER) s.Faces[fi].AiBits |= 1 << ci;
        else s.Faces[fi].HumanBits |= 1 << ci;

        // Check face win
        if (s.Faces[fi].Winner == 0)
        {
            int bits = pl == AI_PLAYER ? s.Faces[fi].AiBits : s.Faces[fi].HumanBits;
            foreach (int mask in WinMasks)
            {
                if ((bits & mask) == mask)
                {
                    s.Faces[fi].Winner = pl;
                    s.GameWinner = ResolveGameWinner(ref s);
                    break;
                }
            }
        }

        if (s.MovesThisTurn == 0) { s.MovesThisTurn = 1; s.LockedFace = fi; }
        else { s.MovesThisTurn = 0; s.LockedFace = -1; s.CurrentPlayer = 3 - pl; }

        return u;
    }

    private static void Revert(ref BoardState s, in Undo u)
    {
        if (u.Player == AI_PLAYER) s.Faces[u.Fi].AiBits &= ~(1 << u.Ci);
        else s.Faces[u.Fi].HumanBits &= ~(1 << u.Ci);

        s.Faces[u.Fi].Winner = u.PrevFaceWinner;
        s.GameWinner = u.PrevGameWinner;
        s.CurrentPlayer = u.PrevCurrentPlayer;
        s.MovesThisTurn = u.PrevMovesThisTurn;
        s.LockedFace = u.PrevLockedFace;
    }

    private static int ResolveGameWinner(ref BoardState s)
    {
        if (s.WinCondition == GameData.WinCondition.AnySide)
        {
            foreach (var f in s.Faces)
                if (f.Winner != 0) return f.Winner;
            return 0;
        }
        int p1 = 0, p2 = 0;
        foreach (var f in s.Faces)
        {
            if (f.Winner == HUMAN_PLAYER) p1++;
            else if (f.Winner == AI_PLAYER) p2++;
        }
        if (p1 >= 4) return HUMAN_PLAYER;
        if (p2 >= 4) return AI_PLAYER;
        return 0;
    }

    private static void GetMoves(ref BoardState s, List<(int fi, int ci)> list)
    {
        list.Clear();
        for (int fi = 0; fi < 6; fi++)
        {
            if (fi == s.LockedFace || s.Faces[fi].Winner != 0) continue;
            int empty = ~(s.Faces[fi].AiBits | s.Faces[fi].HumanBits) & 0x1FF;
            while (empty != 0)
            {
                int lsb = empty & -empty;
                list.Add((fi, Lsb2Idx[lsb]));
                empty ^= lsb;
            }
        }
    }

    // Score a move for ordering purposes (higher = try first).
    private static int MoveOrderScore(ref BoardState s, int fi, int ci)
    {
        int bit = 1 << ci;
        int myBits = (s.CurrentPlayer == AI_PLAYER ? s.Faces[fi].AiBits : s.Faces[fi].HumanBits) | bit;
        int oppBits = (s.CurrentPlayer == AI_PLAYER ? s.Faces[fi].HumanBits : s.Faces[fi].AiBits) | bit;
        foreach (int mask in WinMasks)
            if ((myBits & mask) == mask) return 1000; // winning move
        foreach (int mask in WinMasks)
            if ((oppBits & mask) == mask) return 500; // blocking move
        return CellOrderScore[ci]; // positional
    }

    // Allocation-free insertion sort using stackalloc (descending by score).
    private static void SortMoves(ref BoardState s, List<(int fi, int ci)> moves)
    {
        int n = moves.Count;
        if (n <= 1) return;

        Span<int> scores = stackalloc int[n];
        for (int i = 0; i < n; i++)
            scores[i] = MoveOrderScore(ref s, moves[i].fi, moves[i].ci);

        // Insertion sort (descending)
        for (int i = 1; i < n; i++)
        {
            var key = moves[i];
            int ks = scores[i];
            int j = i - 1;
            while (j >= 0 && scores[j] < ks)
            {
                moves[j + 1] = moves[j];
                scores[j + 1] = scores[j];
                j--;
            }
            moves[j + 1] = key;
            scores[j + 1] = ks;
        }
    }

    private const int WIN_SCORE = 100_000;
    private const int FACE_WIN_VALUE = 2_000;
    private const int TWO_IN_ROW_VALUE = 30;
    private const int ONE_IN_ROW_VALUE = 5;

    private static int Evaluate(ref BoardState s)
    {
        int score = 0;
        for (int fi = 0; fi < 6; fi++)
        {
            ref var f = ref s.Faces[fi];
            if (f.Winner == AI_PLAYER) { score += FACE_WIN_VALUE; continue; }
            else if (f.Winner == HUMAN_PLAYER) { score -= FACE_WIN_VALUE; continue; }

            int ai = f.AiBits, hm = f.HumanBits;
            foreach (var p in WinPatterns)
            {
                int aiC = ((ai >> p[0]) & 1) + ((ai >> p[1]) & 1) + ((ai >> p[2]) & 1);
                int hmC = ((hm >> p[0]) & 1) + ((hm >> p[1]) & 1) + ((hm >> p[2]) & 1);

                if (hmC == 0 && aiC > 0)
                    score += aiC == 2 ? TWO_IN_ROW_VALUE : ONE_IN_ROW_VALUE;
                else if (aiC == 0 && hmC > 0)
                    score -= hmC == 2 ? TWO_IN_ROW_VALUE : ONE_IN_ROW_VALUE;
            }
        }
        return score;
    }

    // Depth counts DOWN from max (8) to 1, indices 1-8 used
    private readonly List<(int, int)>[] _depthMoves = new List<(int, int)>[9];

    private List<(int fi, int ci)> GetMoveList(int depth)
    {
        _depthMoves[depth] ??= new List<(int, int)>(54);
        return _depthMoves[depth];
    }

    private int Minimax(ref BoardState s, int depth, int alpha, int beta)
    {
        if (s.GameWinner == AI_PLAYER) return WIN_SCORE + depth;
        if (s.GameWinner == HUMAN_PLAYER) return -(WIN_SCORE + depth);

        if (depth == 0) return Evaluate(ref s);

        var moves = GetMoveList(depth);
        GetMoves(ref s, moves);
        if (moves.Count == 0) return 0;

        SortMoves(ref s, moves);
        if (moves.Count > 12) moves.RemoveRange(12, moves.Count - 12);

        if (s.CurrentPlayer == AI_PLAYER)
        {
            int best = int.MinValue + 1;
            foreach (var (fi, ci) in moves)
            {
                var u = Apply(ref s, fi, ci);
                int sc = Minimax(ref s, depth - 1, alpha, beta);
                Revert(ref s, u);
                if (sc > best) best = sc;
                if (best > alpha) alpha = best;
                if (alpha >= beta) break;
            }
            return best;
        }
        else
        {
            int best = int.MaxValue - 1;
            foreach (var (fi, ci) in moves)
            {
                var u = Apply(ref s, fi, ci);
                int sc = Minimax(ref s, depth - 1, alpha, beta);
                Revert(ref s, u);
                if (sc < best) best = sc;
                if (best < beta) beta = best;
                if (alpha >= beta) break;
            }
            return best;
        }
    }

    private (int, int) RunMinimax(ref BoardState state, int depth)
    {
        var moves = new List<(int fi, int ci)>(54);
        GetMoves(ref state, moves);
        if (moves.Count == 0) return (-1, -1);

        SortMoves(ref state, moves);
        if (moves.Count > 12) moves.RemoveRange(12, moves.Count - 12);

        int bestScore = int.MinValue + 1;
        var bestMoves = new List<(int, int)>(8);

        foreach (var (fi, ci) in moves)
        {
            var u = Apply(ref state, fi, ci);
            int score = Minimax(ref state, depth - 1, int.MinValue + 1, int.MaxValue - 1);
            Revert(ref state, u);

            if (score > bestScore)
            {
                bestScore = score;
                bestMoves.Clear();
            }
            if (score == bestScore)
                bestMoves.Add((fi, ci));
        }

        return bestMoves[_rng.Next(bestMoves.Count)];
    }

    private (int, int) PickRandom(ref BoardState state)
    {
        var moves = new List<(int fi, int ci)>(54);
        GetMoves(ref state, moves);
        return moves.Count == 0 ? (-1, -1) : moves[_rng.Next(moves.Count)];
    }
}
