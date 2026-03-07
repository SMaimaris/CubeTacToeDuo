#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Types/CTTTypes.h"
#include "CTTBoardState.generated.h"

/**
 * Pure data + rule logic for the 6-face, 54-cell board.
 * Lives as a UObject so the AI can clone it cheaply without a world context.
 */
UCLASS(BlueprintType)
class CUBETACTOE_API UCTTBoardState : public UObject
{
	GENERATED_BODY()

public:
	UCTTBoardState();

	/** Place a move. Returns a full result including validation and win checks. */
	UFUNCTION(BlueprintCallable, Category = "Board")
	FMoveResult PlaceMove(FCellCoord Coord, ECellOwner Player);

	/** All empty cells across all faces */
	UFUNCTION(BlueprintCallable, Category = "Board")
	TArray<FCellCoord> GetLegalMoves() const;

	/** Empty cells on a specific face */
	UFUNCTION(BlueprintCallable, Category = "Board")
	TArray<FCellCoord> GetLegalMovesOnFace(EFaceIndex Face) const;

	/** Check win/draw state of a single face */
	UFUNCTION(BlueprintCallable, Category = "Board")
	EFaceWinState CheckFaceWin(EFaceIndex Face) const;

	/** WinAnyFace: first player with 3-in-a-row on ANY face */
	UFUNCTION(BlueprintCallable, Category = "Board")
	ECellOwner CheckWinAnyFace() const;

	/** WinMostFaces: player with 4+ face wins */
	UFUNCTION(BlueprintCallable, Category = "Board")
	ECellOwner CheckWinMostFaces() const;

	/** Fill OutX/OutO with how many faces each player has won */
	UFUNCTION(BlueprintCallable, Category = "Board")
	void GetFaceWinCounts(int32& OutX, int32& OutO) const;

	/** Deep copy for AI search tree */
	UFUNCTION(BlueprintCallable, Category = "Board")
	UCTTBoardState* Clone(UObject* Outer) const;

	/** Reset to empty board */
	UFUNCTION(BlueprintCallable, Category = "Board")
	void Reset();

	/** Read-only access to a face */
	UFUNCTION(BlueprintCallable, Category = "Board")
	FFaceBoardState GetFaceState(EFaceIndex Face) const;

	/** Read a single cell */
	UFUNCTION(BlueprintCallable, Category = "Board")
	ECellOwner GetCell(FCellCoord Coord) const;

	/** Cached per-face resolution state (updated on PlaceMove) */
	UFUNCTION(BlueprintCallable, Category = "Board")
	EFaceWinState GetFaceCachedState(EFaceIndex Face) const;

private:
	/** 6 faces × 9 cells */
	FFaceBoardState Faces[6];

	/** Cached outcome per face (avoids re-checking resolved faces) */
	EFaceWinState FaceStates[6];

	/** 8 winning lines (indices into a face's 9-cell array) */
	static constexpr int32 WinLines[8][3] = {
		{0,1,2}, {3,4,5}, {6,7,8},  // rows
		{0,3,6}, {1,4,7}, {2,5,8},  // cols
		{0,4,8}, {2,4,6}            // diagonals
	};

	/** Evaluate win state directly from face data (no caching) */
	EFaceWinState EvaluateFaceWin(EFaceIndex Face) const;

	/** True if all cells on this face are filled */
	bool IsFaceFull(EFaceIndex Face) const;
};
