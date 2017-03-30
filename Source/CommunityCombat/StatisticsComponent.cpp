// Fill out your copyright notice in the Description page of Project Settings.

#include "CommunityCombat.h"
#include "StatisticsComponent.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Materials/MaterialInstanceConstant.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "StatBarWidget.h"


// Sets default values for this component's properties
UStatisticsComponent::UStatisticsComponent()
{
	//
}


// Called when the game starts
void UStatisticsComponent::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle beginHandle;
	FTimerDelegate beginDelegate;

	beginDelegate.BindLambda([&]() {
		//lambda body
		if (OnComponentBeginPlay.IsBound())
		{
			OnComponentBeginPlay.Broadcast();
			BeginPlayStatsSetup();
			SetupStatsRegeneration();
		}
	});

	//add 0.05s delay to prevent any collision with other actors beginPlay events
	GetWorld()->GetTimerManager().SetTimer(beginHandle, beginDelegate, 0.05f , false);
	
}


/*//////////////////////////////////////////////////////////////////////////////////
									STAT BARS
//////////////////////////////////////////////////////////////////////////////////*/


FStatData* UStatisticsComponent::GetStat(EStat stat)
{
	return Stats.Find(stat);
}

//safe function used in blueprints
void UStatisticsComponent::GetStatData(EStat stat, FStatData& statData)
{
	statData = Stats.FindRef(stat);
}

void UStatisticsComponent::SetStatValue(EStat stat, const FStatData& newValue)
{
	Stats.Add(stat, newValue);
}

void UStatisticsComponent::ModifyStat(EStat stat, float value, bool isAnimated)
{
	//get pointer to correct stat and check if its valid
	FStatData* statData = GetStat(stat);
	if (!statData) return;

	//if stat is full and value is positive
	//or stat is equal minumum and value is negative
	//pause regen timer and end function here
	if (statData->CurrentValue >= statData->MaxValue && value > 0 ||
		statData->CurrentValue <= statData->MinValue && value < 0)
	{
		ClearRegenerationTimer(stat);
		return;
	}

	if (isAnimated)
	{
		//clear regenerating timer when stat is animated
		if (GetWorld()->GetTimerManager().IsTimerActive(statData->RegenerationTimerHandle))
			GetWorld()->GetTimerManager().ClearTimer(statData->RegenerationTimerHandle);


		statData->CurrentValue = UKismetMathLibrary::FClamp(statData->CurrentValue + value, statData->MinValue, statData->MaxValue);
		float lerpValue = UKismetMathLibrary::Abs(statData->DisplayedValue - statData->CurrentValue);
		float LerpAlpha = UKismetMathLibrary::FClamp(lerpValue / statData->MaxValue, statData->MinValue, statData->MaxValue);
		float LerpTime = UKismetMathLibrary::Lerp(statData->MinLerpTime, statData->MaxLerpTime, LerpAlpha);
		bool localIsPositive = (statData->CurrentValue >= statData->DisplayedValue);

		StatLerpDisplay(stat, LerpTime, localIsPositive);

	}
	//if not, just add new value to current value and update displayed value
	else
	{
		statData->CurrentValue = UKismetMathLibrary::FClamp(statData->CurrentValue + value, statData->MinValue, statData->MaxValue);
		statData->DisplayedValue = statData->CurrentValue;
		UpdateStat(stat);
	}
}

void UStatisticsComponent::OnStatLerpStart(EStat& stat)
{
	GetStat(stat)->IsCurrentlyAnimated = true;
}

void UStatisticsComponent::OnStatLerpEnd(EStat& stat)
{

	FStatData* statData = GetStat(stat);

	statData->IsCurrentlyAnimated = false;
	statData->DisplayedValue = statData->CurrentValue;
	statData->FloatedTime = 0;
	GetWorld()->GetTimerManager().ClearTimer(statData->UpdateRemoveBarHandle);

	if (!IsValid(statData->BarWidget)) return;
	statData->BarWidget->GetStatRemoveBar()->SetVisibility(ESlateVisibility::Hidden);

	//unpause regeneration timer when lerping ends
	if(statData->HasRegeneration)
		RefreshRegenerationTimer(stat);
}

void UStatisticsComponent::UpdateStat(EStat stat)
{

	FStatData* statData = GetStat(stat);
	if (!IsValid(statData->BarWidget) || !statData) return;

	float Percent = UKismetMathLibrary::FClamp(statData->DisplayedValue / statData->MaxValue, 0.f, 1.f);
	statData->BarWidget->GetStatProgressBar()->SetPercent(Percent);

	/* optionally color change to 'desired color' when stat is getting lower

	if (stat == EStat::Health)
	{
	FLinearColor desiredColor = UKismetMathLibrary::LinearColorLerpUsingHSV(LerpToColor!, statData->BarWidget->BarColor, ALFA);
	statData->BarWidget->GetStatProgressBar()->SetFillColorAndOpacity(desiredColor);
	}*/
}


void UStatisticsComponent::StatLerpDisplay(EStat stat, float lerpTime, bool isPositive)
{

	//get stat based on given enum
	FStatData* statData = GetStat(stat);
	if (!statData->BarWidget) return;

	OnStatLerpStart(stat);

	//create fstatlerp struct var and fill it, cuz it will be added to map StatsLerp
	FStatLerp StatLerpToAdd;
	StatLerpToAdd.OriginalValue = statData->DisplayedValue;
	StatLerpToAdd.ValueToLerpTo = statData->CurrentValue;
	StatLerpToAdd.IsPositive = isPositive;

	StatsLerp.Add(stat, StatLerpToAdd);

	//set dynamic material parameter name and new value
	FName ParamName = isPositive ? TEXT("PercentRight") : TEXT("PercentLeft");
	float ParamValue = statData->CurrentValue / statData->MaxValue;

	//set new value for stat remove bar
	statData->BarWidget->GetDynamicMaterial()->SetScalarParameterValue(ParamName, ParamValue);


	SetTickTimer(stat, lerpTime);

}

void UStatisticsComponent::SetTickTimer(EStat& stat, float& lerpTime)
{

	FStatData* statData = GetStat(stat);

	float LoopTime = GetWorld()->GetDeltaSeconds();
	statData->FloatedTime = 0.f;

	FTimerDelegate timerDel(FTimerDelegate::CreateUObject<UStatisticsComponent, EStat, float>
		(this, &UStatisticsComponent::StatLerpTick, stat, lerpTime));

	GetWorld()->GetTimerManager().SetTimer(statData->UpdateRemoveBarHandle, timerDel, LoopTime, true);

	

}

void UStatisticsComponent::StatLerpTick(EStat stat, float lerpTime)
{
	//get pointer to proper stat
	FStatData* statData = GetStat(stat);
	if (!statData->BarWidget) return;

	statData->FloatedTime += GetWorld()->GetDeltaSeconds();

	//get pointer to proper lerp stat
	FStatLerp* statLerp = StatsLerp.Find(stat);

	//get alpha value used to lerp between original value and desired
	float alpha = statData->FloatedTime / lerpTime;
	float newDisplayValue = UKismetMathLibrary::Lerp(statLerp->OriginalValue, statLerp->ValueToLerpTo, alpha);

	statData->DisplayedValue = newDisplayValue;
	float paramValue = statData->DisplayedValue / statData->MaxValue;


	//keep updating remove bar material with every tick till it reach current value
	FName paramName = (statLerp->IsPositive) ? TEXT("PercentLeft") : TEXT("PercentRight");
	statData->BarWidget->GetDynamicMaterial()->SetScalarParameterValue(paramName, paramValue);

	//update displayed value
	UpdateStat(stat);


	//set visilibity here to avoid showing it before two sides of remove bar material were set
	//otherwise it will cause unwanted behaviour 
	if(statData->BarWidget->GetStatRemoveBar()->GetVisibility() != ESlateVisibility::SelfHitTestInvisible)
		statData->BarWidget->GetStatRemoveBar()->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	//if floated time overstep floated time (time since timer was set) end updating remove bar
	if (statData->FloatedTime >= lerpTime)
		OnStatLerpEnd(stat);
}

void UStatisticsComponent::SetupStatsRegeneration()
{
	for (auto& itr : Stats)
	{

		if (itr.Value.HasRegeneration)
		{

			//delegate which will be fired by timer to tick stat regeneration
			FTimerDelegate timerDel(FTimerDelegate::CreateUObject<UStatisticsComponent, EStat, float&>
				(this, &UStatisticsComponent::Regeneration, itr.Key, itr.Value.RegenValue));

			GetWorld()->GetTimerManager().SetTimer(itr.Value.RegenerationTimerHandle, timerDel, itr.Value.RegenTickInterval, true);

		}
	}
}

void UStatisticsComponent::RefreshRegenerationTimer(EStat stat)
{

	FStatData* statData = GetStat(stat);
	if (!statData) return;

	if (GetWorld()->GetTimerManager().IsTimerActive(statData->RegenerationTimerHandle)) return;

	FTimerDelegate timerDel(FTimerDelegate::CreateUObject<UStatisticsComponent, EStat, float&>
		(this, &UStatisticsComponent::Regeneration, stat, statData->RegenValue));

	//set regen timer with first delay of ReeneableRegenTime value
	GetWorld()->GetTimerManager().SetTimer(statData->RegenerationTimerHandle, timerDel,
		statData->RegenTickInterval, true, statData->ReeneableRegenTime);

}

void UStatisticsComponent::BeginPlayStatsSetup()
{
	for (auto& itr : Stats)
	{
		if (itr.Value.BarWidget)
		{
			UpdateStat(itr.Key);
			float value = itr.Value.CurrentValue / itr.Value.MaxValue;
			itr.Value.BarWidget->GetDynamicMaterial()->SetScalarParameterValue("PercentLeft", value);
			itr.Value.BarWidget->GetDynamicMaterial()->SetScalarParameterValue("PercentRight", value);
			if (itr.Value.CurrentValue != itr.Value.DisplayedValue)
				itr.Value.BarWidget->GetStatRemoveBar()->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
}

void UStatisticsComponent::Regeneration(EStat stat, float& regenValue)
{
	ModifyStat(stat, regenValue, false);
}

void UStatisticsComponent::ClearRegenerationTimer(EStat stat)
{
	FStatData* statData = GetStat(stat);
	if (!statData) return;
	GetWorld()->GetTimerManager().ClearTimer(statData->RegenerationTimerHandle);
}


void UStatisticsComponent::SetWidgetReference(EStat stat, class UStatBarWidget* widget)
{
	FStatData* statData = GetStat(stat);
	if (!statData || !widget) return;

	statData->BarWidget = widget;
}