// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CarMovementComponent.generated.h"

// ustruct necessary for serializing
USTRUCT()
struct FCarMovementInput
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	float Throttle;
	UPROPERTY()
	float Steering;
	UPROPERTY()
	float DeltaTime;
	UPROPERTY()
	float Timestamp;

	bool IsValid() const
	{
		return FMath::Abs(Throttle) <= 1 && FMath::Abs(Steering) <= 1;
	};
};



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UCarMovementComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Sets default values for this component's properties
	UCarMovementComponent();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// ---- simulate movement ----
	void Simulate(const FCarMovementInput& Input);
	// ---- set-get state movement ----
	void SetVelocity(const FVector& Value);
	FVector GetVelocity() const;
	void SetThrottle(const float& Value);
	void SetSteering(const float& Value);
	FCarMovementInput GetLastInput() const;

private:
	// ---- simulate movement ----
	FCarMovementInput CreateInput(const float& DeltaTime);
	bool IsLocallyControlled = false;
	// ---- state movement ----
	// throttle to move the car forward, negative for backward
	float Throttle;
	// steering to turn the car, positive right, negative left
	float Steering;
	// car velocity
	UPROPERTY()
	FVector Velocity;
	FCarMovementInput LastInput;
	// ---- update state movement ----
	void UpdateLocationFromTranslation(const FVector& DeltaTranslation);
	void UpdateRotation(const float& DeltaTime, const float& SteeringInput);
	// ---- movement properties ----
	// minimum radius of the car turning circle at full lock (m)
	UPROPERTY(EditDefaultsOnly, Category = "Car movement")
	float MinTurningRadius = 10;
	// air resistance (kg/m)
	UPROPERTY(EditDefaultsOnly, Category = "Car movement")
	float DragResistance = 16;
	// air resistance (kg/m)
	UPROPERTY(EditDefaultsOnly, Category = "Car movement")
	float RollingResistance = 0.015;
	// mass of the car (kg)
	UPROPERTY(EditDefaultsOnly, Category = "Car movement")
	float Mass = 1000;
	// force applied to the car when throttle is full down (N)
	UPROPERTY(EditDefaultsOnly, Category = "Car movement")
	float MaxDrivingForce = 10000;
	FVector GetAirResistance();
	FVector GetRollingResistance();
};
