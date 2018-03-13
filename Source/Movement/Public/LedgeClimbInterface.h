// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LedgeClimbInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class ULedgeClimbInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MOVEMENT_API ILedgeClimbInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "MyCategory")
	void CanGrab(bool bCanGrab);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "MyCategory")
	void ClimbLedge(bool bClimbLedge);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "MyCategory")
	void MoveLedgeLeftRight(float Value);
};
