// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MovementCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "WorldCollision.h"
#include "LedgeClimbInterface.h"

//////////////////////////////////////////////////////////////////////////
// AMovementCharacter

AMovementCharacter::AMovementCharacter()
{
	SetActorTickEnabled(true);
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	ClimbArrowRadius = 10.0f;

	LeftClimbArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("LeftClimbArrow"));
	LeftClimbArrow->SetupAttachment(RootComponent);
	LeftClimbArrow->SetRelativeLocation(FVector(0.0f, -(GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.0f - ClimbArrowRadius), 0.0f));

	RightClimbArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("RightClimbArrow"));
	RightClimbArrow->SetupAttachment(RootComponent);
	RightClimbArrow->SetRelativeLocation(FVector(0.0f, GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.0f - ClimbArrowRadius, 0.0f));

	LeftClimbArrow2 = CreateDefaultSubobject<UArrowComponent>(TEXT("LeftClimbArrow2"));
	LeftClimbArrow2->SetupAttachment(RootComponent);
	LeftClimbArrow2->SetRelativeLocation(FVector(0.0f, -(GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.0f + ClimbArrowRadius), 0.0f));

	RightClimbArrow2 = CreateDefaultSubobject<UArrowComponent>(TEXT("RightClimbArrow2"));
	RightClimbArrow2->SetupAttachment(RootComponent);
	RightClimbArrow2->SetRelativeLocation(FVector(0.0f, GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.0f + ClimbArrowRadius, 0.0f));


	bIsLedgeClimbing = false;
	bIsHanging = false;

	LeftArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("LeftArrow"));
	LeftArrow->SetupAttachment(RootComponent);
	LeftArrow->SetRelativeLocation(FVector(40.0f, -60.0f, 40.0f));

	RightArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("RightArrow"));
	RightArrow->SetupAttachment(RootComponent);
	RightArrow->SetRelativeLocation(FVector(40.0f, 60.0f, 40.0f));

	
	bCanLedgeMoveRight = false;
	bCanLedgeMoveLeft = false;

	LeftLedgeJumpArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("LeftLedgeArrow"));
	LeftLedgeJumpArrow->SetupAttachment(RootComponent);
	LeftLedgeJumpArrow->SetRelativeLocation(FVector(50, -150, 40));

	RightLedgeJumpArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("RightLedgeArrow"));
	RightLedgeJumpArrow->SetupAttachment(RootComponent);
	RightLedgeJumpArrow->SetRelativeLocation(FVector(50, 150, 40));

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMovementCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMovementCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMovementCharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMovementCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMovementCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMovementCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMovementCharacter::LookUpAtRate);

	PlayerInputComponent->BindAction("ExitLedge", IE_Pressed, this, &AMovementCharacter::ExitLedge);


}

void AMovementCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMovementCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMovementCharacter::RightForwardTracer()
{
	FVector StartTrace = RightClimbArrow->GetComponentLocation();
	FVector StartTrace2 = RightClimbArrow2->GetComponentLocation();
	FVector EndTrace = GetActorRotation().Vector();
	EndTrace.X *= 150.0f;
	EndTrace.Y *= 150.0f;
	FVector EndTrace2 = EndTrace + StartTrace2;
	EndTrace += StartTrace;
	TArray<AActor*> ActorsToIgnore;

	FHitResult Hit;
	FHitResult Hit2;

	if (UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartTrace, EndTrace, ClimbArrowRadius, ETraceTypeQuery::TraceTypeQuery3,
		false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit, true) &&
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartTrace2, EndTrace2, ClimbArrowRadius, ETraceTypeQuery::TraceTypeQuery3,
			false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit2, true))
	{
		if (Hit.Normal.Equals(Hit2.Normal))
		{
			RightWallLocation = Hit.ImpactPoint;
			RightWallNormal = Hit.Normal;
			//UE_LOG(LogTemp, Warning, TEXT("ImpactPoint: %s, Normal: %s"), *RightWallLocation.ToString(), *RightWallNormal.ToString());
			bRightSuccessfulForwardTrace = true;
		}
		else
		{
			bRightSuccessfulForwardTrace = false;
		}
	}
	else
	{
		bRightSuccessfulForwardTrace = false;
	}
	
}

void AMovementCharacter::RightHeightTracer()
{
	FVector StartTrace = RightClimbArrow->GetComponentLocation();
	FVector StartTrace2 = RightClimbArrow2->GetComponentLocation();
	StartTrace.Z += 500.0f;
	StartTrace2.Z += 500.0f;
	FVector ForwardDirection = GetActorRotation().Vector() * 70.0f;

	StartTrace += ForwardDirection;
	StartTrace2 += ForwardDirection;

	FVector EndTrace = RightClimbArrow->GetComponentLocation() + ForwardDirection;
	FVector EndTrace2 = RightClimbArrow2->GetComponentLocation() + ForwardDirection;
	
	TArray<AActor*> ActorsToIgnore;

	FHitResult Hit;
	FHitResult Hit2;

	if (UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartTrace, EndTrace, ClimbArrowRadius, ETraceTypeQuery::TraceTypeQuery3,
		false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit, true) &&
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartTrace2, EndTrace2, ClimbArrowRadius, ETraceTypeQuery::TraceTypeQuery3,
			false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit2, true) &&
		bRightSuccessfulForwardTrace)
	{
		RightHeightLocation = Hit.ImpactPoint;
		float PelvisDuringImpact = GetMesh()->GetSocketLocation("PelvisSocket").Z - RightHeightLocation.Z;
		float MinHeight = -50.0f;
		float MaxHeight = 0.0f;
		//UE_LOG(LogTemp, Warning, TEXT("Check Climb %.2f"), PelvisDuringImpact);
		if (MinHeight < PelvisDuringImpact && PelvisDuringImpact < MaxHeight)
		{
			if (!bIsLedgeClimbing)
			{
				
				//UE_LOG(LogTemp, Warning, TEXT("Right Climb"));
				GrabLedge(RightHeightLocation, RightWallLocation, RightWallNormal);
			}
		}
	}

}

void AMovementCharacter::LeftForwardTracer()
{
	FVector StartTrace = LeftClimbArrow->GetComponentLocation();
	FVector StartTrace2 = LeftClimbArrow2->GetComponentLocation();
	FVector EndTrace = GetActorRotation().Vector();
	EndTrace.X *= 150.0f;
	EndTrace.Y *= 150.0f;
	FVector EndTrace2 = EndTrace + StartTrace2;
	EndTrace += StartTrace;
	TArray<AActor*> ActorsToIgnore;

	FHitResult Hit;
	FHitResult Hit2;

	if (UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartTrace, EndTrace, ClimbArrowRadius, ETraceTypeQuery::TraceTypeQuery3,
		false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit, true) &&
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartTrace2, EndTrace2, ClimbArrowRadius, ETraceTypeQuery::TraceTypeQuery3,
			false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit2, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("ImpactPoint: %s, Normal: %s"), *Hit.Normal.ToString(), *Hit2.Normal.ToString());
		if (Hit.Normal.Equals(Hit2.Normal))
		{
			LeftWallLocation = Hit.ImpactPoint;
			LeftWallNormal = Hit.Normal;
			bLeftSuccessfulForwardTrace = true;
			UE_LOG(LogTemp, Warning, TEXT("Assign"));
		}
		else
		{
			bLeftSuccessfulForwardTrace = false;
		}
	}
	else
	{
		bLeftSuccessfulForwardTrace = false;
	}

}

void AMovementCharacter::LeftHeightTracer()
{
	FVector StartTrace = LeftClimbArrow->GetComponentLocation();
	FVector StartTrace2 = LeftClimbArrow2->GetComponentLocation();
	StartTrace.Z += 500.0f;
	StartTrace2.Z += 500.0f;
	FVector ForwardDirection = GetActorRotation().Vector() * 70.0f;

	StartTrace += ForwardDirection;
	StartTrace2 += ForwardDirection;

	FVector EndTrace = LeftClimbArrow->GetComponentLocation() + ForwardDirection;
	FVector EndTrace2 = LeftClimbArrow2->GetComponentLocation() + ForwardDirection;

	TArray<AActor*> ActorsToIgnore;

	FHitResult Hit;
	FHitResult Hit2;

	if (UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartTrace, EndTrace, ClimbArrowRadius, ETraceTypeQuery::TraceTypeQuery3,
		false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit, true) &&
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartTrace2, EndTrace2, ClimbArrowRadius, ETraceTypeQuery::TraceTypeQuery3,
			false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit2, true) &&
		bLeftSuccessfulForwardTrace)
	{
		LeftHeightLocation = Hit.ImpactPoint;
		float PelvisDuringImpact = GetMesh()->GetSocketLocation("PelvisSocket").Z - LeftHeightLocation.Z;
		float MinHeight = -50.0f;
		float MaxHeight = 0.0f;
		//UE_LOG(LogTemp, Warning, TEXT("Check Climb %.2f"), PelvisDuringImpact);
		if (MinHeight < PelvisDuringImpact && PelvisDuringImpact < MaxHeight)
		{
			if (!bIsLedgeClimbing)
			{

				//UE_LOG(LogTemp, Warning, TEXT("Left Climb"));
				GrabLedge(LeftHeightLocation, LeftWallLocation, LeftWallNormal);
			}
		}
	}

}


void AMovementCharacter::GrabLedge(FVector HeightLocation, FVector WallLocation, FVector WallNormal)
{
	UObject* pointerToAnyUObject = GetMesh()->GetAnimInstance();
	ILedgeClimbInterface* LedgeClimb = Cast<ILedgeClimbInterface>(pointerToAnyUObject);
	if (LedgeClimb)
	{
		LedgeClimb->Execute_CanGrab(pointerToAnyUObject, true);

		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		
		bIsHanging = true;

		FVector TargetLocation = FVector(WallLocation.X + WallNormal.X*42.0f, WallLocation.Y + WallNormal.Y*42.0f, HeightLocation.Z-GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

		FRotator TargetRotation = WallNormal.Rotation();
		TargetRotation.Yaw += 180.0f;
		FLatentActionInfo LatentInfo;
		LatentInfo.CallbackTarget = this;
		UKismetSystemLibrary::MoveComponentTo(GetCapsuleComponent(), TargetLocation, TargetRotation, false, false, 0.13f, false, EMoveComponentAction::Move, LatentInfo);
		
		GetCharacterMovement()->StopMovementImmediately();
	}


}

void AMovementCharacter::ExitLedge()
{
	if (bIsHanging)
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		UObject* pointerToAnyUObject = GetMesh()->GetAnimInstance();
		ILedgeClimbInterface* LedgeClimb = Cast<ILedgeClimbInterface>(pointerToAnyUObject);
		if (LedgeClimb)
		{
			LedgeClimb->Execute_CanGrab(pointerToAnyUObject, false);
		}
		bIsHanging = false;
	}
}

void AMovementCharacter::ClimbLedgeEvent()
{
	if (!bIsLedgeClimbing)
	{
		UObject* pointerToAnyUObject = GetMesh()->GetAnimInstance();
		ILedgeClimbInterface* LedgeClimb = Cast<ILedgeClimbInterface>(pointerToAnyUObject);
		if (LedgeClimb)
		{
			LedgeClimb->Execute_ClimbLedge(pointerToAnyUObject, true);
		}
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		bIsLedgeClimbing = true;
		bIsHanging = false;
	}
}


void AMovementCharacter::ClimbLedgeEventOver_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("OVER"));
	bIsLedgeClimbing = false;

	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
}

void AMovementCharacter::RightTracer()
{
	if (RightArrow)
	{
		FVector StartTrace = RightArrow->GetComponentLocation();
		FVector EndTrace = RightArrow->GetComponentLocation();

		float Radius = 20.0f;
		float HalfHeight = 60.0f;
		TArray<AActor*> ActorsToIgnore;

		FHitResult Hit;

		if (UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), StartTrace, EndTrace, Radius, HalfHeight, ETraceTypeQuery::TraceTypeQuery3,
			false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit, true))
		{
			bCanLedgeMoveRight = true;
		}
		else
		{
			bCanLedgeMoveRight = false;
		}
	}
}

void AMovementCharacter::LeftTracer()
{
	if (LeftArrow)
	{
		FVector StartTrace = LeftArrow->GetComponentLocation();
		FVector EndTrace = LeftArrow->GetComponentLocation();

		float Radius = 20.0f;
		float HalfHeight = 60.0f;
		TArray<AActor*> ActorsToIgnore;

		FHitResult Hit;

		if (UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), StartTrace, EndTrace, Radius, HalfHeight, ETraceTypeQuery::TraceTypeQuery3,
			false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit, true))
		{
			bCanLedgeMoveLeft = true;
		}
		else
		{
			bCanLedgeMoveLeft = false;
		}
	}
}



void AMovementCharacter::LeftJumpTracer()
{
	FVector StartTrace = LeftLedgeJumpArrow->GetComponentLocation();
	FVector EndTrace = LeftLedgeJumpArrow->GetComponentLocation();

	float Radius = 25.0f;
	float HalfHeight = 60.0f;
	TArray<AActor*> ActorsToIgnore;

	FHitResult Hit;

	if (UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), StartTrace, EndTrace, Radius, HalfHeight, ETraceTypeQuery::TraceTypeQuery3,
		false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit, true))
	{
		if (bCanLedgeMoveLeft)
		{
			bCanLedgeJumpLeft = false;
		}
		else
		{
			bCanLedgeJumpLeft = true;
		}
	}
	else
	{
		bCanLedgeJumpLeft = false;
	}
}

void AMovementCharacter::RightJumpTracer()
{
	FVector StartTrace = RightLedgeJumpArrow->GetComponentLocation();
	FVector EndTrace = RightLedgeJumpArrow->GetComponentLocation();

	float Radius = 25.0f;
	float HalfHeight = 60.0f;
	TArray<AActor*> ActorsToIgnore;

	FHitResult Hit;

	if (UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), StartTrace, EndTrace, Radius, HalfHeight, ETraceTypeQuery::TraceTypeQuery3,
		false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit, true))
	{
		if (bCanLedgeMoveRight)
		{
			bCanLedgeJumpRight = false;
		}
		else
		{
			bCanLedgeJumpRight = true;
		}
	}
	else
	{
		bCanLedgeJumpRight = false;
	}
}

void AMovementCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RightForwardTracer();
	RightHeightTracer();
	LeftForwardTracer();
	LeftHeightTracer();
	if (bIsHanging)
	{
		RightTracer();
		LeftTracer();
		if (bCanLedgeMoveRight)
		{
			bCanLedgeJumpRight = false;
		}
		else
		{
			RightJumpTracer();
		}
		if (bCanLedgeMoveLeft)
		{
			bCanLedgeJumpLeft = false;
		}
		else
		{
			LeftJumpTracer();
		}
	}
}

void AMovementCharacter::Jump()
{
	if (bIsHanging)
	{
		ClimbLedgeEvent();
	}
	else
	{
		ACharacter::Jump();
	}
}

void AMovementCharacter::StopJumping()
{
	ACharacter::StopJumping();
}

void AMovementCharacter::MoveForward(float Value)
{
	if (!bIsHanging && (Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AMovementCharacter::MoveRight(float Value)
{

	if (bIsHanging)

	{
		if (Value > 0.0f && bCanLedgeMoveRight)
		{
			FVector NewLocation = UKismetMathLibrary::VInterpTo(GetActorLocation(), FRotationMatrix(GetActorRotation()).GetScaledAxis(EAxis::Y) * 20.0f + GetActorLocation(), GetWorld()->DeltaTimeSeconds, 5);
			SetActorLocation(NewLocation);
			bMovingLedgeRight = true;
			bMovingLedgeLeft = false;
		}

		else if (Value < 0.0f && bCanLedgeMoveLeft)
		{
			FVector NewLocation = UKismetMathLibrary::VInterpTo(GetActorLocation(), FRotationMatrix(GetActorRotation()).GetScaledAxis(EAxis::Y) * -20.0f + GetActorLocation(), GetWorld()->DeltaTimeSeconds, 5);
			SetActorLocation(NewLocation);
			bMovingLedgeRight = false;
			bMovingLedgeLeft = true;

		}
		else if (Value > 0.0f && GetCharacterMovement()->IsFalling())
		{

		}
		else if (Value == 0.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Not Moving"));
			bMovingLedgeRight = false;
			bMovingLedgeLeft = false;
		}
	}


	if (!bIsHanging && (Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
