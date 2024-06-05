// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	// create the movement component
	CarMovementComponent = CreateDefaultSubobject<UCarMovementComponent>(TEXT("CarMovementComponent"));
	// create the replication component
	CarReplicationComponent = CreateDefaultSubobject<UCarReplicationComponent>(TEXT("CarReplicationComponent"));
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	// deactivate to use our own replicate movement
	SetReplicateMovement(false);

	if(HasAuthority())
	{
		// define the frequency to update replicated properties
		NetUpdateFrequency = 1;
	}
	
}

FString GetEnumText(const ENetRole& Role)
{
	switch(Role)
	{
	case ROLE_Authority:
		return "Authority";
	case ROLE_AutonomousProxy:
		return "Autonomous Proxy";
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "Simulated Proxy";
	default:
		return "Error";
	}
}



// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// get controller as player controller
	if(APlayerController* Controller = Cast<APlayerController>(GetController()); Controller)
	{
		// get enhanced input system
		if (UEnhancedInputLocalPlayerSubsystem* EnhancedInputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()); EnhancedInputSystem)
		{
			// bind input mapping to read keys
			if(!InputMapping.IsNull())
			{
				EnhancedInputSystem->AddMappingContext(InputMapping.LoadSynchronous(), 1);
			}
			// bind action to function
			if(UEnhancedInputComponent* InputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
			{
				InputComponent->BindAction(InputActionThrottle.Get(), ETriggerEvent::Triggered, this, &AGoKart::ActThrottle);
				InputComponent->BindAction(InputActionSteering.Get(), ETriggerEvent::Triggered, this, &AGoKart::ActSteering);	
			}
		}
	}
}

void AGoKart::ActThrottle(const FInputActionInstance& Instance)
{
	if(CarMovementComponent == nullptr) return;
	float Value = Instance.GetValue().Get<float>();
	CarMovementComponent->SetThrottle(Value);
}

void AGoKart::ActSteering(const FInputActionInstance& Instance)
{
	if(CarMovementComponent == nullptr) return;
	float Value = Instance.GetValue().Get<float>();
	CarMovementComponent->SetSteering(Value);
}

