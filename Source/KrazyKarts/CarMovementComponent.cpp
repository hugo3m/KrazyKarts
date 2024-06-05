// Fill out your copyright notice in the Description page of Project Settings.


#include "CarMovementComponent.h"
#include "GameFramework/GameStateBase.h"

// Sets default values for this component's properties
UCarMovementComponent::UCarMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}


// Called when the game starts
void UCarMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (const APawn* Owner = Cast<APawn>(GetOwner()); Owner)
	{
		IsLocallyControlled = Owner->IsLocallyControlled();	
	}
}


// Called every frame
void UCarMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// if the car is locally controlled
	if (IsLocallyControlled)
	{
		// run simulation for locally controlled car
		Simulate(CreateInput(DeltaTime));
	}
}

void UCarMovementComponent::Simulate(const FCarMovementInput& Input)
{
	// set last input before simulating
	LastInput = Input;
	// handle throttle
	FVector Force = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Input.Throttle;
	// handle air
	Force += GetAirResistance();
	// handle rolling resistance
	Force += GetRollingResistance();
	// a = f / m
	FVector Acceleration = Force / Mass;
	// dv = da * dt
	Velocity = Velocity + Acceleration * Input.DeltaTime;
	// dx = dv * dt
	FVector DeltaTranslation = Velocity * Input.DeltaTime * 100;

	UpdateRotation(Input.DeltaTime, Input.Steering);
	UpdateLocationFromTranslation(DeltaTranslation);
}

FCarMovementInput UCarMovementComponent::CreateInput(const float& DeltaTime)
{
	// create input
	FCarMovementInput Input;
	Input.DeltaTime = DeltaTime;
	Input.Steering = Steering;
	Input.Throttle = Throttle;
	if(const UWorld* World = GetWorld())
	{
		Input.Timestamp = World->TimeSeconds;
		if(const AGameStateBase* GameState = World->GetGameState())
		{
			Input.Timestamp = GameState->GetServerWorldTimeSeconds();
		}
	}
	return Input;
}

void UCarMovementComponent::SetVelocity(const FVector& Value)
{
	Velocity = Value;
}

FVector UCarMovementComponent::GetVelocity() const
{
	return Velocity;
}

void UCarMovementComponent::SetThrottle(const float& Value)
{
	Throttle = Value;
}

void UCarMovementComponent::SetSteering(const float& Value)
{
	Steering = Value;
}

FCarMovementInput UCarMovementComponent::GetLastInput() const
{
	return LastInput;
}

FVector UCarMovementComponent::GetAirResistance()
{
	float Speed = Velocity.Size();
	// f = v^2 * coef
	float AirResistance =  FMath::Square(Speed) * DragResistance;
	return - Velocity.GetSafeNormal() * AirResistance;
}

FVector UCarMovementComponent::GetRollingResistance()
{
	// transform to meter
	float AccelerationGravity = GetWorld()->GetGravityZ() / 100;
	// gravity force
	float Force = Mass * AccelerationGravity;
	return Velocity.GetSafeNormal() * RollingResistance * Force;
}

void UCarMovementComponent::UpdateLocationFromTranslation(const FVector& DeltaTranslation)
{
	FHitResult HitResult;
	GetOwner()->AddActorWorldOffset(DeltaTranslation, true, &HitResult);
	if (HitResult.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

void UCarMovementComponent::UpdateRotation(const float& DeltaTime, const float& SteeringInput)
{
	// handle steering
	AActor* Owner = GetOwner();
	float DeltaLocation = FVector::DotProduct(Owner->GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * SteeringInput;
	FQuat DeltaRotation(Owner->GetActorUpVector(), RotationAngle);
	Velocity = DeltaRotation.RotateVector(Velocity);
	Owner->AddActorWorldRotation(DeltaRotation);
}
