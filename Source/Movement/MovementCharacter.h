// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MovementCharacter.generated.h"


UCLASS(config=Game)
class AMovementCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AMovementCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	float ClimbArrowRadius;

protected:

	void Jump();

	void StopJumping();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);
	
	void RightForwardTracer();

	void RightHeightTracer();

	void LeftForwardTracer();

	void LeftHeightTracer();

	bool bRightSuccessfulForwardTrace;

	bool bLeftSuccessfulForwardTrace;

	FVector RightHeightLocation;

	FVector RightWallLocation;

	FVector RightWallNormal;

	FVector LeftHeightLocation;

	FVector LeftWallLocation;

	FVector LeftWallNormal;

	bool bIsLedgeClimbing;

	bool bIsHanging;

	void GrabLedge(FVector HeightLocation, FVector WallLocation, FVector WallNormal);

	void ExitLedge();

	void ClimbLedgeEvent();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AnimMessage")
	void ClimbLedgeEventOver();
	virtual void ClimbLedgeEventOver_Implementation();

	// Forward Tracing Arrows
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UArrowComponent* LeftClimbArrow;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UArrowComponent* RightClimbArrow;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UArrowComponent* LeftClimbArrow2;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UArrowComponent* RightClimbArrow2;

	// Move Left and Right Arrows
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UArrowComponent* LeftArrow;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UArrowComponent* RightArrow;
	
	void RightTracer();
	void LeftTracer();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "LedgeClimbing")
	bool bCanLedgeMoveRight;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "LedgeClimbing")
	bool bCanLedgeMoveLeft;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "LedgeClimbing")
	bool bMovingLedgeRight;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "LedgeClimbing")
	bool bMovingLedgeLeft;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UArrowComponent*  LeftLedgeJumpArrow;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UArrowComponent*  RightLedgeJumpArrow;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "LedgeClimbing")
	bool bCanLedgeJumpLeft;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "LedgeClimbing")
	bool bCanLedgeJumpRight;

	void LeftJumpTracer();
	void RightJumpTracer();
	


protected:
	virtual void Tick(float DeltaSeconds) override;

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

