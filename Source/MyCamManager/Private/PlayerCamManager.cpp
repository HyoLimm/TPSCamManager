#include "PlayerCamManager.h"

//#include "Core/BasePlayerController.h"

#include "Camera/CameraActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/CameraComponent.h"

#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC( PlayerCamManager, Log, All );

APlayerCamManager::APlayerCamManager( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
	m_ControlledPawn = nullptr;
}

void APlayerCamManager::UpdateViewTarget( FTViewTarget& OutVT, float DeltaTime )
{
	// Don't update outgoing viewtarget during an interpolation 
	if( ( PendingViewTarget.Target != NULL ) && BlendParams.bLockOutgoing && OutVT.Equal( ViewTarget ) )
	{
		return;
	}

	playerController = Cast<APlayerController>( GetOwningPlayerController() );

	if( playerController != nullptr )
	{
		// store previous POV, in case we need it later
		FMinimalViewInfo OrigPOV = OutVT.POV;

		//@TODO: CAMERA: Should probably reset the view target POV fully here
		OutVT.POV.FOV = DefaultFOV;
		OutVT.POV.OrthoWidth = DefaultOrthoWidth;
		OutVT.POV.AspectRatio = DefaultAspectRatio;
		OutVT.POV.bConstrainAspectRatio = bDefaultConstrainAspectRatio;
		OutVT.POV.bUseFieldOfViewForLOD = true;
		OutVT.POV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
		OutVT.POV.PostProcessSettings.SetBaseValues();
		OutVT.POV.PostProcessBlendWeight = 1.0f;

		bool bDoNotApplyModifiers = false;

		m_ControlledPawn = UGameplayStatics::GetPlayerPawn( GetWorld(), 0 );

		if( ACameraActor* CamActor = Cast<ACameraActor>( OutVT.Target ) )
		{
			// Viewing through a camera actor.
			CamActor->GetCameraComponent()->GetCameraView( DeltaTime, OutVT.POV );
		}
		else if( m_ControlledPawn != nullptr )
		{

			/*플레이어 위치*/
			const FTransform pivotTarget = m_ControlledPawn->GetActorTransform();

			/*현재 카메라 회전을 PlayerController 회전으로 빠르게 보간함 */
			m_TargetRota = FMath::RInterpTo( OutVT.POV.Rotation, playerController->GetControlRotation(), DeltaTime, 20.0f );


			//오차 보간 스피드
			FVector legSpeed = FVector( 7.0f, 7.0f, 15.0f );
			FVector targetLocation;
			CalculateAxisIndependentLag( m_SmoothedPivotTarget.GetLocation(), pivotTarget.GetLocation(), m_TargetRota, legSpeed, targetLocation );
			m_SmoothedPivotTarget = FTransform( pivotTarget.GetRotation(), targetLocation, FVector::OneVector );


			//DrawDebugSphere( GetWorld(), m_SmoothedPivotTarget.GetLocation(), 50.0f, 16, FColor::Red, false, 0.0f );
			const FRotator tempValue = m_SmoothedPivotTarget.GetRotation().Rotator();
			m_PivotLoca = ( tempValue.Vector() +
				UKismetMathLibrary::GetRightVector( tempValue ) +
				UKismetMathLibrary::GetUpVector( tempValue ) +
				m_SmoothedPivotTarget.GetLocation() );


			const float CamOffsetX = -400.0f;
			const float CamOffsetY = 0.0f;
			const float CamOffsetZ = 50.0f;
			m_TargetLoca = ( m_TargetRota.Vector() * CamOffsetX +
				UKismetMathLibrary::GetRightVector( m_TargetRota ) * CamOffsetY +
				UKismetMathLibrary::GetUpVector( m_TargetRota ) * CamOffsetZ +
				m_PivotLoca );


			/*캐릭터와 카메라 사이 장애물 탐지*/
			CalcNewLocation();


			OutVT.POV.Location = m_TargetLoca;

			OutVT.POV.Rotation = m_TargetRota;
		}


		if( !bDoNotApplyModifiers || this->bAlwaysApplyModifiers )
		{
			// Apply camera modifiers at the end (view shakes for example)
			ApplyCameraModifiers( DeltaTime, OutVT.POV );
		}
		// Synchronize the actor with the view target results
		SetActorLocationAndRotation( OutVT.POV.Location, OutVT.POV.Rotation, false );

		UpdateCameraLensEffects( OutVT );
	}
	else
	{
		UE_LOG( PlayerCamManager, Warning, TEXT("Player Controller Not Valid"));
		Super::UpdateViewTarget( OutVT, DeltaTime );
	}
}

void APlayerCamManager::OnPossessCam( class APawn* newPawn )
{
	if( newPawn == nullptr )
		return;

	m_ControlledPawn = newPawn;

	playerController = Cast<APlayerController>( GetOwningPlayerController() );
}

bool APlayerCamManager::CalcNewLocation()
{
	return true;
}

void APlayerCamManager::CalculateAxisIndependentLag( const FVector& curLocation, const FVector& tarLocation, const FRotator& camRotation, const FVector& LegSpeeds, FVector& OutPutLocation )
{
	const FRotator camRotationYaw = FRotator( 0.0f, camRotation.Yaw, 0.0f );

	FVector tempCurVector = camRotationYaw.UnrotateVector( curLocation );
	FVector tempTarVector = camRotationYaw.UnrotateVector( tarLocation );

	FVector result;
	result.X = FMath::FInterpTo( tempCurVector.X, tempTarVector.X, GetWorld()->GetDeltaSeconds(), LegSpeeds.X );
	result.Y = FMath::FInterpTo( tempCurVector.Y, tempTarVector.Y, GetWorld()->GetDeltaSeconds(), LegSpeeds.Y );
	result.Z = FMath::FInterpTo( tempCurVector.Z, tempTarVector.Z, GetWorld()->GetDeltaSeconds(), LegSpeeds.Z );

	OutPutLocation = camRotationYaw.RotateVector( result );
}
