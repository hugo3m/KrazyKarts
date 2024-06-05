// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "CarMovementComponent.h"
#include "CarReplicationComponent.h"
#include "GoKart.generated.h"


UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// ---- bind inputs ----
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// ---- simulate movement ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCarMovementComponent* CarMovementComponent;
	// ---- replicate ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCarReplicationComponent* CarReplicationComponent;

private:

	// ---- read and execute inputs ----
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> InputActionThrottle;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> InputActionSteering;
	void ActThrottle(const FInputActionInstance& Instance);
	void ActSteering(const FInputActionInstance& Instance);

};
