#include "Game/CTTBoardState.h"
#include "CubeTacToe.h"

UCTTBoardState::UCTTBoardState()
{
	Reset();
}

void UCTTBoardState::Reset()
{
	for (int32 f = 0; f < 6; ++f)
	{
		Faces[f] = FFaceBoardState();
		FaceStates[f] = EFaceWinState::Undecided;
	}
	UE_LOG(LogCTT, Log, TEXT("BoardState: Reset — all 54 cells cleared"));
}

FMoveResult UCTTBoardState::PlaceMove(FCellCoord Coord, ECellOwner Player)
{
	FMoveResult Result;
	Result.Coord = Coord;

	const int32 FaceIdx = static_cast<int32>(Coord.FaceIndex);
	const int32 CellIdx = Coord.CellIndex();

	if (FaceIdx < 0 || FaceIdx >= 6 || CellIdx < 0 || CellIdx >= 9)
	{
		UE_LOG(LogCTT, Warning, TEXT("PlaceMove: INVALID coord — Face=%d Row=%d Col=%d"),
		       FaceIdx, Coord.Row, Coord.Col);
		return Result;
	}

	if (Faces[FaceIdx].Cells[CellIdx] != ECellOwner::None)
	{
		UE_LOG(LogCTT, Warning, TEXT("PlaceMove: Cell already occupied — Face=%d Row=%d Col=%d"),
		       FaceIdx, Coord.Row, Coord.Col);
		return Result;
	}

	if (FaceStates[FaceIdx] != EFaceWinState::Undecided)
	{
		UE_LOG(LogCTT, Warning, TEXT("PlaceMove: Face %d is already resolved, move rejected"), FaceIdx);
		return Result;
	}

	// Place the piece
	Faces[FaceIdx].Cells[CellIdx] = Player;
	Result.bValid = true;
	Result.NewFaceState = Faces[FaceIdx];

	UE_LOG(LogCTT, Log, TEXT("PlaceMove: %s placed on Face=%d Row=%d Col=%d (cell %d)"),
	       *UEnum::GetValueAsString(Player), FaceIdx, Coord.Row, Coord.Col, CellIdx);

	// Re-evaluate this face
	EFaceWinState NewFaceState = EvaluateFaceWin(Coord.FaceIndex);
	if (NewFaceState != EFaceWinState::Undecided)
	{
		FaceStates[FaceIdx] = NewFaceState;
		Result.bFaceJustResolved = true;
		Result.FaceWinState = NewFaceState;
		UE_LOG(LogCTT, Log, TEXT("PlaceMove: Face %d resolved → %s"),
		       FaceIdx, *UEnum::GetValueAsString(NewFaceState));
	}

	return Result;
}

TArray<FCellCoord> UCTTBoardState::GetLegalMoves() const
{
	TArray<FCellCoord> Moves;
	for (int32 f = 0; f < 6; ++f)
	{
		if (FaceStates[f] != EFaceWinState::Undecided) continue;
		TArray<FCellCoord> FaceMoves = GetLegalMovesOnFace(static_cast<EFaceIndex>(f));
		Moves.Append(FaceMoves);
	}
	return Moves;
}

TArray<FCellCoord> UCTTBoardState::GetLegalMovesOnFace(EFaceIndex Face) const
{
	TArray<FCellCoord> Moves;
	const int32 FaceIdx = static_cast<int32>(Face);
	if (FaceStates[FaceIdx] != EFaceWinState::Undecided) return Moves;

	for (int32 i = 0; i < 9; ++i)
	{
		if (Faces[FaceIdx].Cells[i] == ECellOwner::None)
		{
			FCellCoord Coord;
			Coord.FaceIndex = Face;
			Coord.Row = i / 3;
			Coord.Col = i % 3;
			Moves.Add(Coord);
		}
	}
	return Moves;
}

EFaceWinState UCTTBoardState::CheckFaceWin(EFaceIndex Face) const
{
	return FaceStates[static_cast<int32>(Face)];
}

EFaceWinState UCTTBoardState::EvaluateFaceWin(EFaceIndex Face) const
{
	const int32 FaceIdx = static_cast<int32>(Face);
	const TArray<ECellOwner>& Cells = Faces[FaceIdx].Cells;

	for (const auto& Line : WinLines)
	{
		ECellOwner A = Cells[Line[0]];
		if (A == ECellOwner::None) continue;
		if (A == Cells[Line[1]] && A == Cells[Line[2]])
		{
			return (A == ECellOwner::PlayerX) ? EFaceWinState::WonByX : EFaceWinState::WonByO;
		}
	}

	if (IsFaceFull(Face))
	{
		return EFaceWinState::Draw;
	}

	return EFaceWinState::Undecided;
}

bool UCTTBoardState::IsFaceFull(EFaceIndex Face) const
{
	const int32 FaceIdx = static_cast<int32>(Face);
	for (ECellOwner Cell : Faces[FaceIdx].Cells)
	{
		if (Cell == ECellOwner::None) return false;
	}
	return true;
}

ECellOwner UCTTBoardState::CheckWinAnyFace() const
{
	for (int32 f = 0; f < 6; ++f)
	{
		if (FaceStates[f] == EFaceWinState::WonByX) return ECellOwner::PlayerX;
		if (FaceStates[f] == EFaceWinState::WonByO) return ECellOwner::PlayerO;
	}
	return ECellOwner::None;
}

ECellOwner UCTTBoardState::CheckWinMostFaces() const
{
	int32 X = 0, O = 0;
	GetFaceWinCounts(X, O);

	if (X >= 4) return ECellOwner::PlayerX;
	if (O >= 4) return ECellOwner::PlayerO;
	return ECellOwner::None;
}

void UCTTBoardState::GetFaceWinCounts(int32& OutX, int32& OutO) const
{
	OutX = 0; OutO = 0;
	for (int32 f = 0; f < 6; ++f)
	{
		if (FaceStates[f] == EFaceWinState::WonByX) ++OutX;
		else if (FaceStates[f] == EFaceWinState::WonByO) ++OutO;
	}
}

UCTTBoardState* UCTTBoardState::Clone(UObject* Outer) const
{
	UCTTBoardState* Copy = NewObject<UCTTBoardState>(Outer);
	for (int32 f = 0; f < 6; ++f)
	{
		Copy->Faces[f] = Faces[f];
		Copy->FaceStates[f] = FaceStates[f];
	}
	return Copy;
}

FFaceBoardState UCTTBoardState::GetFaceState(EFaceIndex Face) const
{
	return Faces[static_cast<int32>(Face)];
}

ECellOwner UCTTBoardState::GetCell(FCellCoord Coord) const
{
	const int32 FaceIdx = static_cast<int32>(Coord.FaceIndex);
	const int32 CellIdx = Coord.CellIndex();
	if (FaceIdx < 0 || FaceIdx >= 6 || CellIdx < 0 || CellIdx >= 9)
	{
		return ECellOwner::None;
	}
	return Faces[FaceIdx].Cells[CellIdx];
}

EFaceWinState UCTTBoardState::GetFaceCachedState(EFaceIndex Face) const
{
	return FaceStates[static_cast<int32>(Face)];
}
