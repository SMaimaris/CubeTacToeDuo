#pragma once

#include "CoreMinimal.h"
#include "CTTTypes.generated.h"

/** Which face of the cube */
UENUM(BlueprintType)
enum class EFaceIndex : uint8
{
	Top    UMETA(DisplayName = "Top"),
	Front  UMETA(DisplayName = "Front"),
	Right  UMETA(DisplayName = "Right"),
	Back   UMETA(DisplayName = "Back"),
	Left   UMETA(DisplayName = "Left"),
	Bottom UMETA(DisplayName = "Bottom"),
	Count  UMETA(Hidden)
};

/** Who owns a cell */
UENUM(BlueprintType)
enum class ECellOwner : uint8
{
	None    UMETA(DisplayName = "None"),
	PlayerX UMETA(DisplayName = "Player X"),
	PlayerO UMETA(DisplayName = "Player O")
};

/** Local 2-player or vs AI */
UENUM(BlueprintType)
enum class EGameMode : uint8
{
	LocalTwoPlayer UMETA(DisplayName = "Local Two Player"),
	VsAI           UMETA(DisplayName = "Vs AI")
};

/** Win condition chosen at menu */
UENUM(BlueprintType)
enum class EWinCondition : uint8
{
	WinAnyFace   UMETA(DisplayName = "Win Any Face"),
	WinMostFaces UMETA(DisplayName = "Win Most Faces")
};

/** High-level game phase */
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	MainMenu  UMETA(DisplayName = "Main Menu"),
	Playing   UMETA(DisplayName = "Playing"),
	Paused    UMETA(DisplayName = "Paused"),
	GameOver  UMETA(DisplayName = "Game Over")
};

/** Per-face outcome */
UENUM(BlueprintType)
enum class EFaceWinState : uint8
{
	Undecided UMETA(DisplayName = "Undecided"),
	WonByX    UMETA(DisplayName = "Won By X"),
	WonByO    UMETA(DisplayName = "Won By O"),
	Draw      UMETA(DisplayName = "Draw")
};

/** Identifies one cell by face + grid position */
USTRUCT(BlueprintType)
struct FCellCoord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFaceIndex FaceIndex = EFaceIndex::Top;

	/** 0-2, top-to-bottom in face local space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Row = 0;

	/** 0-2, left-to-right in face local space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Col = 0;

	int32 CellIndex() const { return Row * 3 + Col; }

	bool operator==(const FCellCoord& Other) const
	{
		return FaceIndex == Other.FaceIndex && Row == Other.Row && Col == Other.Col;
	}
};

/** The state of all 9 cells on one face */
USTRUCT(BlueprintType)
struct FFaceBoardState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<ECellOwner> Cells;

	FFaceBoardState()
	{
		Cells.Init(ECellOwner::None, 9);
	}
};

/** Result of a PlaceMove call */
USTRUCT(BlueprintType)
struct FMoveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bValid = false;

	UPROPERTY(BlueprintReadOnly)
	FCellCoord Coord;

	UPROPERTY(BlueprintReadOnly)
	FFaceBoardState NewFaceState;

	UPROPERTY(BlueprintReadOnly)
	bool bGameOver = false;

	/** ECellOwner::None = draw or game not over */
	UPROPERTY(BlueprintReadOnly)
	ECellOwner Winner = ECellOwner::None;

	/** True when this move caused a face to be won/drawn */
	UPROPERTY(BlueprintReadOnly)
	bool bFaceJustResolved = false;

	UPROPERTY(BlueprintReadOnly)
	EFaceWinState FaceWinState = EFaceWinState::Undecided;

	/** True if the move was rejected because the second move cannot be on the same face as the first */
	UPROPERTY(BlueprintReadOnly)
	bool bRejectedSameFace = false;
};
