// Fill out your copyright notice in the Description page of Project Settings.


#include "CarReplicationComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UCarReplicationComponent::UCarReplicationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}


// Called when the game starts
void UCarReplicationComponent::BeginPlay()
{
	Super::BeginPlay();

	CarMovementComponent = GetOwner()->FindComponentByClass<UCarMovementComponent>();

	if (const APawn* Owner = Cast<APawn>(GetOwner()); Owner)
	{
		IsLocallyControlled = Owner->IsLocallyControlled();	
	}
}


// Called every frame
void UCarReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(CarMovementComponent == nullptr) return;

	// ---- Send/Simulate last input according to role ----
	// retrieve my last input
	const FCarMovementInput LastInput = CarMovementComponent->GetLastInput();
	// if I am client and I have control of the kart
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		// add my last input into unacknowledged
		UnacknowledgedInputs.Add(LastInput);
		// send my input to the server
		// sending my input to the server will trigger the simulation on the server
		Server_SendInput(LastInput);
	}
	// if I am server and I have control of the kart
	if(GetOwnerRole() == ROLE_Authority)
	{
		// update my own state
		UpdateAuthoritativeState(LastInput);
	}
	// if I am a kart simulated on a client
	if(GetOwnerRole() == ROLE_SimulatedProxy)
	{
		// run client tick
		SimulatedProxyTick(DeltaTime);
	}
}

void UCarReplicationComponent::ClearAcknowledgedInputs(const FCarMovementInput& LastInput)
{
	TArray<FCarMovementInput> NewUnacknowledgedInputs;
	for (const FCarMovementInput& Input: UnacknowledgedInputs)
	{
		if(Input.Timestamp > LastInput.Timestamp) NewUnacknowledgedInputs.Add(Input);
	}
	UnacknowledgedInputs = NewUnacknowledgedInputs;
}

void UCarReplicationComponent::SimulatedProxyTick(float DeltaTime)
{
	SimulatedProxyTimeSinceUpdate += DeltaTime;

	if (SimulatedProxyTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;

	// create spline
	FHermiteCubicSpline Spline = CreateSpline();
	// calculate lerp ratio
	const float LerpRatio = SimulatedProxyTimeSinceUpdate / SimulatedProxyTimeBetweenLastUpdates;
	InterpolateLocation(Spline, LerpRatio);
	InterpolateVelocity(Spline, LerpRatio);
	InterpolateRotation(LerpRatio);
	
}

FHermiteCubicSpline UCarReplicationComponent::CreateSpline()
{
	FHermiteCubicSpline Spline;
	Spline.StartLocation = SimulatedProxyStartTransform.GetLocation();
	Spline.TargetLocation = AuthoritativeState.Transform.GetLocation();
	Spline.StartDerivative = SimulatedProxyStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = AuthoritativeState.Velocity * VelocityToDerivative();
	return Spline;
}

float UCarReplicationComponent::VelocityToDerivative()
{
	return SimulatedProxyTimeBetweenLastUpdates * 100;
}

void UCarReplicationComponent::InterpolateLocation(const FHermiteCubicSpline& Spline, const float& LerpRatio)
{
	// calculate new location from start location to target location
	FVector NewLocation = Spline.InterpolateLocation(LerpRatio);
	if(MeshOffsetRoot != nullptr)
	{
		// set new transform
		MeshOffsetRoot->SetWorldLocation(NewLocation);
	}
	
}

void UCarReplicationComponent::InterpolateVelocity(const FHermiteCubicSpline& Spline, const float& LerpRatio)
{
	// calculate new velocity
	FVector NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative();
	// set new velocity
	CarMovementComponent->SetVelocity(NewVelocity);
	
}

void UCarReplicationComponent::InterpolateRotation(const float& LerpRatio)
{
	// calculate new rotation from start rotation to target rotation
	FQuat NewRotation = FQuat::Slerp(SimulatedProxyStartTransform.GetRotation(), AuthoritativeState.Transform.GetRotation(), LerpRatio);
	if(MeshOffsetRoot != nullptr)
	{
		// set new transform
		MeshOffsetRoot->SetWorldRotation(NewRotation);
	}
}

void UCarReplicationComponent::UpdateAuthoritativeState(const FCarMovementInput& Input)
{
	// set the state for the car owned by the server
	AuthoritativeState.LastInput = Input;
	AuthoritativeState.Transform = GetOwner()->GetActorTransform();
	AuthoritativeState.Velocity = CarMovementComponent->GetVelocity();
}

void UCarReplicationComponent::OnRep_AuthoritativeState()
{
	switch(GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		OnRep_AutonomousProxy_AuthoritativeState();
		break;
	case ROLE_SimulatedProxy:
		OnRep_SimulatedProxy_AuthoritativeState();
		break;
	default:
		break;
	}
}

void UCarReplicationComponent::OnRep_SimulatedProxy_AuthoritativeState()
{
	if(CarMovementComponent == nullptr) return;
	// set time between updates
	SimulatedProxyTimeBetweenLastUpdates = SimulatedProxyTimeSinceUpdate;
	// just receive update so reset
	SimulatedProxyTimeSinceUpdate = 0;
	// just make sure it is high enough
	if (SimulatedProxyTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;
	if(MeshOffsetRoot != nullptr)
	{
		// set root component transform
		SimulatedProxyStartTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());
		SimulatedProxyStartTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());
	}
	// set start velocity
	SimulatedProxyStartVelocity = CarMovementComponent->GetVelocity();
	// set the transform of the meshoffsetroot
	GetOwner()->SetActorTransform(AuthoritativeState.Transform);
}

void UCarReplicationComponent::OnRep_AutonomousProxy_AuthoritativeState()
{
	// on replicate authoritative state received by the client
	if(CarMovementComponent == nullptr) return;
	// when receiving new state on the client from the server
	// reset state from authoritative state
	GetOwner()->SetActorTransform(AuthoritativeState.Transform);
	CarMovementComponent->SetVelocity(AuthoritativeState.Velocity);
	// clear acknowledged inputs
	ClearAcknowledgedInputs(AuthoritativeState.LastInput);
	// simulate unacknowledged input
	for (const FCarMovementInput& Input: UnacknowledgedInputs)
	{
		CarMovementComponent->Simulate(Input);
	}
}

void UCarReplicationComponent::Server_SendInput_Implementation(const FCarMovementInput& Input)
{
	SimulatedProxySimulatedTime += Input.DeltaTime;
	if(CarMovementComponent == nullptr) return;
	// simulate the move on the server
	CarMovementComponent->Simulate(Input);
}

bool UCarReplicationComponent::Server_SendInput_Validate(const FCarMovementInput& Input)
{
	const float ProposedTime = SimulatedProxySimulatedTime + Input.DeltaTime;
	bool SimulatedProxyNotRunningAheadOfTime = ProposedTime < GetWorld()->TimeSeconds;
	return Input.IsValid() && SimulatedProxyNotRunningAheadOfTime;
}

void UCarReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// call the super
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// ---- replicate variables ----
	// replicate state
	DOREPLIFETIME(UCarReplicationComponent, AuthoritativeState);
}

