// Fill out your copyright notice in the Description page of Project Settings.


#include "PlanetSixEnemy.h"
#include "WeaponComponent.h"
#include "PlanetSixCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PlanetSixGameInstance.h"
#include "Engine/StaticMesh.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnemyNameWidget.h"
#include "Components/WidgetComponent.h"
#include "Curves/CurveFloat.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIEnemyBaseController.h"

#define print(text, i) if (GEngine) GEngine->AddOnScreenDebugMessage(i, 1.5, FColor::White,text)

// Sets default values
APlanetSixEnemy::APlanetSixEnemy(const FObjectInitializer& ObjectInitializer)
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//Collider = CreateDefaultSubobject<UBoxComponent>(TEXT("Collider"));
	MovComp = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovComp"));
	Attributes = CreateDefaultSubobject<UAttributesComponent>(TEXT("Attributes"));

	/*Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));
	Mesh->CanCharacterStepUpOn = ECB_No;

	Collider->SetGenerateOverlapEvents(true);
	Collider->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collider->CanCharacterStepUpOn = ECB_No;
	Collider->SetNotifyRigidBodyCollision(true);*/

	//AIControllerClass = AEnemyController::StaticClass();

	//RootComponent = Collider;
	//Collider->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1,ECollisionResponse::ECR_Block);

	SetReplicates(true);
}

// Called when the game starts or when spawned
void APlanetSixEnemy::BeginPlay()
{
	Super::BeginPlay();
	//Find Player
	//NameWidget = CreateWidget<UEnemyNameWidget>(GetWorld(), NameWidgetClass);

	SetWidget();

	if (EnemyMaterial) {
		DynamicMat = GetMesh()->CreateDynamicMaterialInstance(0, EnemyMaterial);
	}

	if (TextureCurve) 
	{
		FOnTimelineFloat TimelineCallback;
		FOnTimelineEventStatic TimelineFinishedCallback;

		TimelineCallback.BindUFunction(this, FName("ControlMaterial"));
		TimelineFinishedCallback.BindUFunction(this, FName("DestroyEnemy"));
		MyTimeline.AddInterpFloat(TextureCurve, TimelineCallback);
		MyTimeline.SetTimelineFinishedFunc(TimelineFinishedCallback);
	}
}

// Called every frame
void APlanetSixEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bIsDead) {
		MyTimeline.TickTimeline(DeltaTime);
	}

	UpdateWidget();

	if (IsDead()) 
	{
		if (!bIsDeadOnce)
		{
			Death();
			bIsDeadOnce = true;
		}
	}
}

// Called to bind functionality to input
void APlanetSixEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

//bool APlanetSixEnemy::IsDead(float damage) {
//
//	return Info.bIsDead;
//
//}

void APlanetSixEnemy::Death()
{
	MyTimeline.PlayFromStart();
	//add xp to the player
	auto OwnerPlayer = Cast<APlanetSixCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	OwnerPlayer->Attributes->GainExperience(Experience);

	//To check if Quest has a Killing condition
	UPlanetSixGameInstance* GameInstance = Cast<UPlanetSixGameInstance>(GetGameInstance());
	int objectiveNumber = GameInstance->GetCurrentQuest().AtObjectiveNumber;
	FQuestData CurrentQuest = GameInstance->GetCurrentQuest();
	if (CurrentQuest.objectives.Num() > 0)
	{
		//If at location
		if (CurrentQuest.objectives[objectiveNumber].LocationToGo == UGameplayStatics::GetCurrentLevelName(GetWorld()))
		{
			//If needs to kill
			if (CurrentQuest.objectives[objectiveNumber].Objectivetype == EObjectiveType::Kill)
			{
				print("Applying killing quest", -1);
				GameInstance->ReduceCurrentTargetNumber(GetID());
			}
		}
	}
	if(!DynamicMat){
		Destroy();
	}

}

void APlanetSixEnemy::EnemyReceieveDamage(ABaseCharacter* Actor)
{
	if (GetController()) {
		UBlackboardComponent* Blackboard = UAIBlueprintHelperLibrary::GetBlackboard(GetController());
		if (Blackboard) { Blackboard->SetValueAsObject(FName("TargetToFollow"), Actor); }
		print("Recieve enemy target in blackboard", -1);
	}

	AAIEnemyBaseController* AICont = Cast<AAIEnemyBaseController>(GetController());
	if (AICont) {
		AICont->PlayerRef = Cast<APlanetSixCharacter>(Actor);
		print("Recieve enemy target in AI", -1);

	}
	
}

void APlanetSixEnemy::DestroyEnemy()
{
	Destroy();
}

int APlanetSixEnemy::GetID()
{
	return ID;
}

void APlanetSixEnemy::ControlMaterial()
{
	TimelineValue = MyTimeline.GetPlaybackPosition();
	CurveFloatValue = TextureCurve->GetFloatValue(TimelineValue);
	print("CurveFloatValue : " + FString::SanitizeFloat(CurveFloatValue) + " at " + FString::SanitizeFloat(TimelineValue), 11);
	if (DynamicMat) 
	{
		DynamicMat->SetScalarParameterValue(FName("Opacity"), CurveFloatValue);
	}
}

//void APlanetSixEnemy::GiveExperience(TArray<APlanetSixCharacter*> Players, float Exp)
//{
//	for (int i = 0; i < Players.Max(); i++)
//	{
//		Players[i]->Attributes->Experience.SetCurrentValue(Players[i]->Attributes->Experience.GetCurrentValue() + Experience);
//	}
//}