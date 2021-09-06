#pragma once
#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "PlayerCamManager.generated.h"

UCLASS()
class MYCAMMANAGER_API APlayerCamManager : public APlayerCameraManager
{
	GENERATED_BODY()
public:
	APlayerCamManager( const FObjectInitializer& ObjectInitializer );

	virtual void UpdateViewTarget( FTViewTarget& OutVT, float DeltaTime ) override;
public:
	UFUNCTION( BlueprintCallable )
	void OnPossessCam( class APawn* newPawn );
private:
	bool CalcNewLocation();
private:
	UPROPERTY()
		APawn* m_ControlledPawn;

	UPROPERTY()
		APlayerController* playerController = nullptr;

	void CalculateAxisIndependentLag( const FVector& curLocation, const FVector& tarLocation, const FRotator& camRotation, const FVector& LegSpeeds, FVector& OutPutLocation );
private:
	float m_CamOffsetX;

	FVector m_TargetLoca;
	FVector m_PivotLoca;
	FRotator m_TargetRota;
	FTransform m_SmoothedPivotTarget;
};
