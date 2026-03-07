#include "Game/CTTAIPlayer.h"
#include "Game/CTTBoardState.h"

UCTTAIPlayer::UCTTAIPlayer()
{
}

// ============================================================
//  Game-thread helpers
// ============================================================

FCellCoord UCTTAIPlayer::ChooseMove(const UCTTBoardState* Board, ECellOwner AIOwner,
                                     EWinCondition WinCondition)
{
	check(Board);
	const FCTTBoardSnapshot Snap = TakeSnapshot(Board);
	return RunSearch(Snap, AIOwner, WinCondition, MaxDepth);
}

FCTTBoardSnapshot UCTTAIPlayer::TakeSnapshot(const UCTTBoardState* Board)
{
	FCTTBoardSnapshot Snap;
	for (int32 f = 0; f < 6; ++f)
	{
		const EFaceIndex Face     = static_cast<EFaceIndex>(f);
		const FFaceBoardState FBS = Board->GetFaceState(Face);
		for (int32 c = 0; c < 9; ++c)
		{
			Snap.Cells[f][c] = FBS.Cells[c];
		}
		Snap.FaceStates[f] = Board->GetFaceCachedState(Face);
	}
	return Snap;
}

// ============================================================
//  Thread-safe entry point
// ============================================================

FCellCoord UCTTAIPlayer::RunSearch(const FCTTBoardSnapshot& Snapshot, ECellOwner AIOwner,
                                    EWinCondition WinCondition, int32 MaxDepthArg,
                                    EFaceIndex ExcludedFace)
{
	TArray<FCellCoord> Moves = GetSnapshotMoves(Snapshot, ExcludedFace);
	if (Moves.IsEmpty()) return FCellCoord();

	const ECellOwner Human    = (AIOwner == ECellOwner::PlayerX) ? ECellOwner::PlayerO : ECellOwner::PlayerX;
	const EFaceWinState AIWin  = (AIOwner == ECellOwner::PlayerX) ? EFaceWinState::WonByX : EFaceWinState::WonByO;
	const EFaceWinState HumWin = (Human  == ECellOwner::PlayerX) ? EFaceWinState::WonByX : EFaceWinState::WonByO;

	// Move ordering: wins > blocks > center > active face
	Moves.Sort([&Snapshot, AIOwner, Human, AIWin, HumWin](const FCellCoord& A, const FCellCoord& B)
	{
		auto Priority = [&Snapshot, AIOwner, Human, AIWin, HumWin](const FCellCoord& M) -> int32
		{
			const int32 F = static_cast<int32>(M.FaceIndex);

			// Immediate win for AI
			FCTTBoardSnapshot WinBranch = Snapshot;
			SnapshotPlaceMove(WinBranch, M, AIOwner);
			if (WinBranch.FaceStates[F] == AIWin) return 1000;

			// Immediate win for human (block it)
			FCTTBoardSnapshot BlockBranch = Snapshot;
			SnapshotPlaceMove(BlockBranch, M, Human);
			if (BlockBranch.FaceStates[F] == HumWin) return 500;

			int32 P = 0;
			if (M.Row == 1 && M.Col == 1) P += 2; // center cell bonus
			for (int32 c = 0; c < 9; ++c)
			{
				if (Snapshot.Cells[F][c] != ECellOwner::None) { P += 1; break; }
			}
			return P;
		};
		return Priority(A) > Priority(B);
	});

	FCellCoord BestMove  = Moves[0];
	int32      BestScore = TNumericLimits<int32>::Min();

	for (const FCellCoord& Move : Moves)
	{
		FCTTBoardSnapshot Branch = Snapshot; // cheap 60-byte copy
		SnapshotPlaceMove(Branch, Move, AIOwner);

		const int32 Score = Minimax(Branch, MaxDepthArg - 1,
		                            TNumericLimits<int32>::Min(),
		                            TNumericLimits<int32>::Max(),
		                            /*bMaximizing=*/false, AIOwner, WinCondition);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestMove  = Move;
		}
	}

	return BestMove;
}

// ============================================================
//  POD minimax internals
// ============================================================

void UCTTAIPlayer::SnapshotPlaceMove(FCTTBoardSnapshot& Snap, const FCellCoord& Coord,
                                      ECellOwner Player)
{
	const int32 F = static_cast<int32>(Coord.FaceIndex);
	const int32 C = Coord.CellIndex();
	Snap.Cells[F][C] = Player;

	// Re-evaluate this face's win state
	EFaceWinState NewState = EFaceWinState::Undecided;
	for (const auto& Line : WinLines)
	{
		const ECellOwner A = Snap.Cells[F][Line[0]];
		if (A == ECellOwner::None) continue;
		if (A == Snap.Cells[F][Line[1]] && A == Snap.Cells[F][Line[2]])
		{
			NewState = (A == ECellOwner::PlayerX) ? EFaceWinState::WonByX : EFaceWinState::WonByO;
			break;
		}
	}
	if (NewState == EFaceWinState::Undecided)
	{
		bool bFull = true;
		for (int32 i = 0; i < 9; ++i)
		{
			if (Snap.Cells[F][i] == ECellOwner::None) { bFull = false; break; }
		}
		if (bFull) NewState = EFaceWinState::Draw;
	}
	Snap.FaceStates[F] = NewState;
}

TArray<FCellCoord> UCTTAIPlayer::GetSnapshotMoves(const FCTTBoardSnapshot& Snap,
                                                    EFaceIndex ExcludedFace)
{
	TArray<FCellCoord> Moves;
	for (int32 f = 0; f < 6; ++f)
	{
		if (ExcludedFace != EFaceIndex::Count && static_cast<EFaceIndex>(f) == ExcludedFace) continue;
		if (Snap.FaceStates[f] != EFaceWinState::Undecided) continue;
		for (int32 c = 0; c < 9; ++c)
		{
			if (Snap.Cells[f][c] == ECellOwner::None)
			{
				FCellCoord Coord;
				Coord.FaceIndex = static_cast<EFaceIndex>(f);
				Coord.Row       = c / 3;
				Coord.Col       = c % 3;
				Moves.Add(Coord);
			}
		}
	}
	return Moves;
}

bool UCTTAIPlayer::IsSnapshotTerminal(const FCTTBoardSnapshot& Snap, EWinCondition WinCondition)
{
	int32 WonX = 0, WonO = 0;
	for (int32 f = 0; f < 6; ++f)
	{
		if (Snap.FaceStates[f] == EFaceWinState::WonByX) ++WonX;
		else if (Snap.FaceStates[f] == EFaceWinState::WonByO) ++WonO;
	}

	if (WinCondition == EWinCondition::WinAnyFace)
	{
		if (WonX > 0 || WonO > 0) return true;
	}
	else
	{
		if (WonX >= 4 || WonO >= 4) return true;
	}

	return GetSnapshotMoves(Snap).IsEmpty(); // draw: board full
}

int32 UCTTAIPlayer::EvaluateSnapshot(const FCTTBoardSnapshot& Snap, ECellOwner AIOwner,
                                      EWinCondition WinCondition)
{
	int32 WonX = 0, WonO = 0;
	for (int32 f = 0; f < 6; ++f)
	{
		if (Snap.FaceStates[f] == EFaceWinState::WonByX) ++WonX;
		else if (Snap.FaceStates[f] == EFaceWinState::WonByO) ++WonO;
	}

	const bool     bAIisX = (AIOwner == ECellOwner::PlayerX);
	const ECellOwner Human = bAIisX ? ECellOwner::PlayerO : ECellOwner::PlayerX;
	const int32 AIWins     = bAIisX ? WonX : WonO;
	const int32 HumWins    = bAIisX ? WonO : WonX;

	// Count threats on each undecided face:
	//   2-in-a-row with 1 empty = immediate threat (±20)
	//   1-in-a-row with 2 empty = positional start  (±3)
	auto CountThreats = [&](int32& AIThreat, int32& HumThreat, int32& AIStart, int32& HumStart)
	{
		for (int32 f = 0; f < 6; ++f)
		{
			if (Snap.FaceStates[f] != EFaceWinState::Undecided) continue;
			for (const auto& Line : WinLines)
			{
				int32 AIC = 0, HumC = 0, EmptyC = 0;
				for (int32 i = 0; i < 3; ++i)
				{
					const ECellOwner Cell = Snap.Cells[f][Line[i]];
					if      (Cell == AIOwner) ++AIC;
					else if (Cell == Human)   ++HumC;
					else                      ++EmptyC;
				}
				if      (AIC == 2 && EmptyC == 1) ++AIThreat;
				else if (HumC == 2 && EmptyC == 1) ++HumThreat;
				else if (AIC == 1 && EmptyC == 2)  ++AIStart;
				else if (HumC == 1 && EmptyC == 2) ++HumStart;
			}
		}
	};

	if (WinCondition == EWinCondition::WinAnyFace)
	{
		if (AIWins  > 0) return  1000;
		if (HumWins > 0) return -1000;

		int32 AIThreat = 0, HumThreat = 0, AIStart = 0, HumStart = 0;
		CountThreats(AIThreat, HumThreat, AIStart, HumStart);

		// Threats are near-decisive; keep below terminal ±1000
		return (AIThreat - HumThreat) * 20 + (AIStart - HumStart) * 3;
	}
	else // WinMostFaces
	{
		if (AIWins  >= 4) return  1000;
		if (HumWins >= 4) return -1000;

		int32 Score = (AIWins - HumWins) * 100;
		if (AIWins  >= 3) Score += 50;
		if (HumWins >= 3) Score -= 50;

		int32 AIThreat = 0, HumThreat = 0, AIStart = 0, HumStart = 0;
		CountThreats(AIThreat, HumThreat, AIStart, HumStart);
		Score += (AIThreat - HumThreat) * 10 + (AIStart - HumStart) * 2;

		return Score;
	}
}

int32 UCTTAIPlayer::Minimax(FCTTBoardSnapshot Board, int32 Depth, int32 Alpha, int32 Beta,
                              bool bMaximizing, ECellOwner AIOwner, EWinCondition WinCondition)
{
	if (Depth == 0 || IsSnapshotTerminal(Board, WinCondition))
	{
		return EvaluateSnapshot(Board, AIOwner, WinCondition);
	}

	TArray<FCellCoord> Moves = GetSnapshotMoves(Board);
	if (Moves.IsEmpty())
	{
		return EvaluateSnapshot(Board, AIOwner, WinCondition);
	}

	const ECellOwner Human         = (AIOwner == ECellOwner::PlayerX)
	                                     ? ECellOwner::PlayerO : ECellOwner::PlayerX;
	const ECellOwner CurrentPlayer = bMaximizing ? AIOwner : Human;
	const EFaceWinState CurWin     = (CurrentPlayer == ECellOwner::PlayerX)
	                                     ? EFaceWinState::WonByX : EFaceWinState::WonByO;

	// Order moves so winning moves for the current player come first — improves alpha-beta pruning
	Moves.Sort([&Board, CurrentPlayer, CurWin](const FCellCoord& A, const FCellCoord& B)
	{
		auto WinsNow = [&Board, CurrentPlayer, CurWin](const FCellCoord& M) -> bool
		{
			FCTTBoardSnapshot Branch = Board;
			SnapshotPlaceMove(Branch, M, CurrentPlayer);
			return Branch.FaceStates[static_cast<int32>(M.FaceIndex)] == CurWin;
		};
		return WinsNow(A) && !WinsNow(B);
	});

	if (bMaximizing)
	{
		int32 MaxEval = TNumericLimits<int32>::Min();
		for (const FCellCoord& Move : Moves)
		{
			FCTTBoardSnapshot Branch = Board;
			SnapshotPlaceMove(Branch, Move, CurrentPlayer);
			const int32 Eval = Minimax(Branch, Depth - 1, Alpha, Beta, false, AIOwner, WinCondition);
			MaxEval = FMath::Max(MaxEval, Eval);
			Alpha   = FMath::Max(Alpha, Eval);
			if (Beta <= Alpha) break; // beta cut-off
		}
		return MaxEval;
	}
	else
	{
		int32 MinEval = TNumericLimits<int32>::Max();
		for (const FCellCoord& Move : Moves)
		{
			FCTTBoardSnapshot Branch = Board;
			SnapshotPlaceMove(Branch, Move, CurrentPlayer);
			const int32 Eval = Minimax(Branch, Depth - 1, Alpha, Beta, true, AIOwner, WinCondition);
			MinEval = FMath::Min(MinEval, Eval);
			Beta    = FMath::Min(Beta, Eval);
			if (Beta <= Alpha) break; // alpha cut-off
		}
		return MinEval;
	}
}
