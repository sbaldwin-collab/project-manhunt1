#include "AI/Tasks/BTService_UpdateSuspicion.h"
#include "AI/ManhuntHunterController.h"
#include "Components/ManhuntDetectionComponent.h"
#include "Data/ManhuntGameSettings.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_UpdateSuspicion::UBTService_UpdateSuspicion()
{
	NodeName = TEXT("Manhunt: Update Suspicion Meter");
	Interval = 0.f; // every tick -- this is a fast-changing gameplay value, not a perf-sensitive poll.

	SuspicionMeterKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateSuspicion, SuspicionMeterKey));
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateSuspicion, TargetActorKey), AActor::StaticClass());
}

void UBTService_UpdateSuspicion::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AManhuntHunterController* HunterController = Cast<AManhuntHunterController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!HunterController || !Blackboard)
	{
		return;
	}

	UObject* TargetObject = Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName);
	AActor* TargetActor = Cast<AActor>(TargetObject);

	const bool bDetected = TargetActor && HunterController->GetDetectionComponent()
		&& HunterController->GetDetectionComponent()->IsTargetCurrentlyDetected(TargetActor);

	const UManhuntGameSettings* Settings = UManhuntGameSettings::Get();
	float Meter = Blackboard->GetValueAsFloat(SuspicionMeterKey.SelectedKeyName);

	Meter += bDetected ? DeltaSeconds : -DeltaSeconds * DecayRateMultiplier;
	Meter = FMath::Clamp(Meter, 0.f, Settings->SuspiciousToChaseSeconds);

	Blackboard->SetValueAsFloat(SuspicionMeterKey.SelectedKeyName, Meter);
}
