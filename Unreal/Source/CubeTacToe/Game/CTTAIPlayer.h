#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Types/CTTTypes.h"
#include "CTTAIPlayer.generated.h"

class UCTTBoardState;

/**
 * Pure-data board snapshot — safe to build on the game thread and use on any thread.
 * Avoids UObject allocation during the minimax search.
 */
struct FCTTBoardSnapshot
{
	ECellOwner    Cells[6][9]   = {};
	EFaceWinState FaceStates[6] = {};
};

/**
 * Stateless minimax AI with alpha-beta pruning.
 *
 * The public surface has two entry points:
 *   - ChooseMove()  — synchronous, game-thread only (thin wrapper around RunSearch).
 *   - RunSearch()   — static, thread-safe; operates entirely on POD data with no UObject
 *                     allocations. Safe to call from a background thread pool task.
 *
 * Typical async usage (in CTTGameMode::ExecuteAIMove):
 *   1. Capture a FCTTBoardSnapshot on the game thread via TakeSnapshot().
 *   2. Pass it to RunSearch() inside Async(EAsyncExecution::ThreadPool, ...).
 *   3. Return the chosen FCellCoord to the game thread via AsyncTask(GameThread).
 */
UCLASS(BlueprintType)
class CUBETACTOE_API UCTTAIPlayer : public UObject
{
	GENERATED_BODY()

public:
	UCTTAIPlayer();

	/** Maximum search depth; lower = easier, higher = harder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	int32 MaxDepth = 4;

	/**
	 * Synchronous convenience wrapper — must be called on the game thread.
	 * For async use, call TakeSnapshot() + RunSearch() instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI")
	FCellCoord ChooseMove(const UCTTBoardState* Board, ECellOwner AIOwner,
	                      EWinCondition WinCondition);

	/** Build a POD snapshot of the board. Must be called on the game thread. */
	static FCTTBoardSnapshot TakeSnapshot(const UCTTBoardState* Board);

	/**
	 * Thread-safe minimax search on a POD snapshot.
	 * Safe to call from any thread — no UObject allocations occur internally.
	 * ExcludedFace = EFaceIndex::Count means "no face exclusion" (normal search).
	 */
	static FCellCoord RunSearch(const FCTTBoardSnapshot& Snapshot, ECellOwner AIOwner,
	                            EWinCondition WinCondition, int32 MaxDepth,
	                            EFaceIndex ExcludedFace = EFaceIndex::Count);

private:
	// ---- Pure-C++ minimax internals (all operate on FCTTBoardSnapshot) ----

	static int32 Minimax(FCTTBoardSnapshot Board, int32 Depth, int32 Alpha, int32 Beta,
	                     bool bMaximizing, ECellOwner AIOwner, EWinCondition WinCondition);

	static int32 EvaluateSnapshot(const FCTTBoardSnapshot& Snap, ECellOwner AIOwner,
	                              EWinCondition WinCondition);

	/** Get all legal moves, optionally excluding one face. Count = no exclusion. */
	static TArray<FCellCoord> GetSnapshotMoves(const FCTTBoardSnapshot& Snap,
	                                           EFaceIndex ExcludedFace = EFaceIndex::Count);

	/** Place a move on a snapshot in-place; updates face win state. */
	static void SnapshotPlaceMove(FCTTBoardSnapshot& Snap, const FCellCoord& Coord, ECellOwner Player);

	/** Returns true if the position is terminal (win or no legal moves). */
	static bool IsSnapshotTerminal(const FCTTBoardSnapshot& Snap, EWinCondition WinCondition);

	/** 8 standard tic-tac-toe win lines for a 3×3 face. */
	static constexpr int32 WinLines[8][3] = {
		{0,1,2}, {3,4,5}, {6,7,8},  // rows
		{0,3,6}, {1,4,7}, {2,5,8},  // cols
		{0,4,8}, {2,4,6}            // diagonals
	};
};
