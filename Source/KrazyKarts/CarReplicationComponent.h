// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CarMovementComponent.h"
#include "CarReplicationComponent.generated.h"

// ustruct necessary for serializing
USTRUCT()
struct FCarMovementState
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	FCarMovementInput LastInput;
	UPROPERTY()
	FVector Velocity;
	UPROPERTY()
	FTransform Transform;
};

struct FHermiteCubicSpline
{
	FVector StartLocation, StartDerivative, TargetLocation, TargetDerivative;

	FVector InterpolateLocation(const float LerpRatio) const
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	};

	FVector InterpolateDerivative(float LerpRatio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	};
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UCarReplicationComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Sets default values for this component's properties
	UCarReplicationComponent();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// Override Replicate Properties function
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintCallable)
	void SetMeshOffsetRoot(USceneComponent* Value) { MeshOffsetRoot = Value; }

private:
	// ---- authoritative state, send and receive ----
	UPROPERTY(ReplicatedUsing=OnRep_AuthoritativeState)
	FCarMovementState AuthoritativeState;
	void UpdateAuthoritativeState(const FCarMovementInput& Input);
	UFUNCTION()
	void OnRep_AuthoritativeState();
	void OnRep_SimulatedProxy_AuthoritativeState();
	void OnRep_AutonomousProxy_AuthoritativeState();
	bool IsLocallyControlled = false;
	// send input from client to server
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendInput(const FCarMovementInput& Input);
	// ---- history inputs ----
	TArray<FCarMovementInput> UnacknowledgedInputs;
	void ClearAcknowledgedInputs(const FCarMovementInput& LastInput);
	// ---- simulated proxy interpolate ----
	// time since last update
	float SimulatedProxyTimeSinceUpdate;
	// time between the updates
	float SimulatedProxyTimeBetweenLastUpdates;
	// starting location
	FTransform SimulatedProxyStartTransform;
	// starting velocity
	FVector SimulatedProxyStartVelocity;
	// Called every frame
	void SimulatedProxyTick(float DeltaTime);
	// create spline for interpolation
	FHermiteCubicSpline CreateSpline();
	// velocity to cubic derivative
	float VelocityToDerivative();
	void InterpolateLocation(const FHermiteCubicSpline& Spline, const float& LerpRatio);
	void InterpolateVelocity(const FHermiteCubicSpline& Spline, const float& LerpRatio);
	void InterpolateRotation(const float& LerpRatio);
	// follow the time spent on the server
	float SimulatedProxySimulatedTime = 0;
	

	UPROPERTY()
	UCarMovementComponent* CarMovementComponent;
	UPROPERTY()
	USceneComponent* MeshOffsetRoot;
};
