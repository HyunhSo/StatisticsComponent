// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "StatisticsComponent.generated.h"



UENUM(BlueprintType)
enum class EStat : uint8
{
	Null,
	Health,
	Mana,
	Stamina,
	Experience
};

USTRUCT(BlueprintType)
struct FStatData
{
	GENERATED_BODY()

	//default constructor
	FStatData()
	{
		MinValue = 0;
		MaxValue = CurrentValue = DisplayedValue = 100.f;
		MinLerpTime = 0.5f;
		MaxLerpTime = 1.5f;
		RegenValue = 1.f;
		RegenTickInterval = 0.05f;
		ReeneableRegenTime = 0.2f;
	}

	FStatData(float minValue, float maxValue) : MinValue(minValue), MaxValue(maxValue), CurrentValue(maxValue), DisplayedValue(maxValue)
	{
		MinLerpTime = 0.5f;
		MaxLerpTime = 1.5f;
		RegenValue = 1.f;
		RegenTickInterval = 0.05f;
		ReeneableRegenTime = 0.2f;
	}


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = StatData)
	bool HasRegeneration;

	/* time after regenenration timer will be set again if it was cleared,
	to avoid insta regen after losing some stat value, which look weird */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = StatData, meta = (ClampMin = 0, ClampMax = 10, UMin = 0, UIMax = 0))
	float ReeneableRegenTime;

	//time between regen timer ticks
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = StatData, meta = (ClampMin = 0, ClampMax = 10, UMin = 0, UIMax = 0))
	float RegenTickInterval;

	/* value added to stat with every timer tick if HasRegeneration is enabled
	can as well be set to negative value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = StatData)
	float RegenValue;

	//current stat value
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = StatData, meta = (ClampMin = 0))
	float CurrentValue;

	//minimum stat value
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = StatData, meta = (ClampMin = 0))
	float MinValue;

	//maximum stat value
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = StatData, meta = (ClampMin = 0))
	float MaxValue;

	//value displayed with remove bar
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = StatData, meta = (ClampMin = 0))
	float DisplayedValue;

	//minimum time to move remove bar to current value 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = StatData, meta = (ClampMin = 0))
	float MinLerpTime;

	//maximum time to move remove bar to current value
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = StatData, meta = (ClampMin = 0))
	float MaxLerpTime;

	//is remove bar currenly animated
	UPROPERTY(BlueprintReadOnly, Category = StatData)
	bool IsCurrentlyAnimated;
	
	//floated time increasing with time when remove bar is ticking
	UPROPERTY(BlueprintReadOnly, Category = StatData)
	float FloatedTime;

	//pointer to stat widget, must be set with SetWidgetReference function on FOnComponentBeginPlay Event !!
	UPROPERTY(BlueprintReadWrite, Category = StatData)
	class UStatBarWidget* BarWidget;

	//handle responsible for regeneration timer
	UPROPERTY(BlueprintReadOnly, Category = StatData)
	FTimerHandle RegenerationTimerHandle;

	//handle responsible for update ticking
	UPROPERTY(BlueprintReadOnly, Category = StatData)
	FTimerHandle UpdateRemoveBarHandle;

	
};

USTRUCT(BlueprintType)
struct FStatLerp
{
	GENERATED_BODY()

	//original value(displayed value)
	UPROPERTY()
	float OriginalValue;

	//value to lerp to(current stat value)
	UPROPERTY()
	float ValueToLerpTo;

	//is lerping value positive or negative
	UPROPERTY()
	bool IsPositive;
};



DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComponentBeginPlay);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COMMUNITYCOMBAT_API UStatisticsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStatisticsComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
		

//////////////////////////////////////////////////////////////////////////////////
								//STAT BARS
//////////////////////////////////////////////////////////////////////////////////


private:

	UPROPERTY()
	TMap<EStat, FStatLerp> StatsLerp;

	//called to set tick timer which updates remove bar with every tick untill lerpTime is up
	UFUNCTION()
	void SetTickTimer(EStat& stat, float& lerpTime);

	//called when update timer started ticking
	UFUNCTION()
	void OnStatLerpStart(EStat& stat);

	//called when update timer stopped ticking
	UFUNCTION()
	void OnStatLerpEnd(EStat& stat);

	//called before update timer to make some preliminary settings
	UFUNCTION()
	void StatLerpDisplay(EStat stat, float LerpTime, bool inPositive);

	//ticking function called by SetTickTimer, updates remove bar widget with every tick
	UFUNCTION()
	void StatLerpTick(EStat stat, float lerpTime);

	//function called to update stat value with every tick, doesnt use animation(remove bar), that could cause some weird behaviour
	UFUNCTION()
	void Regeneration(EStat stat, float& regenValue);

	//called on begin play or when widgets are recreated to update existing stats and set RemoveBar Material parameters to proper values
	UFUNCTION()
	void BeginPlayStatsSetup();

	//called on begin play to set regeneration timers for stats with enabled HasRegeneration property
	UFUNCTION()
	void SetupStatsRegeneration();

public:

	//function called to clear regeneration timer, used in ModifyStat, if given value has enabled isAnimated
	UFUNCTION(BlueprintCallable, Category = Stats)
	void ClearRegenerationTimer(EStat stat);

	//function called to reenable regeneration timer after ReeneableRegenTime 
	UFUNCTION(BlueprintCallable, Category = Stats)
	void RefreshRegenerationTimer(EStat stat);

	/** function used only in c++, to get stat in bp use GetStatData function which is safe to use even if stat is invalid */
	FStatData* GetStat(EStat stat);

	//container with stat bars
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stats, meta = (AllowPrivateAccess = "true"))
	TMap<EStat, FStatData> Stats;
	
	//called to add to subtract given value from statistics based on param stat
	UFUNCTION(BlueprintCallable, Category = Stats)
	void ModifyStat(EStat stat, float value, bool isAnimated);
	
	//returns copy of found element, elements doesn't exist, it will return default value 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	void GetStatData(EStat stat, FStatData& statData);
	
	//set new FStatData based on EStat enum type
	UFUNCTION(BlueprintCallable, Category = Stats)
	void SetStatValue(EStat stat, const FStatData& newValue);
	
	//called to update displayed stat values, after modifying stat 
	UFUNCTION(BlueprintCallable, Category = Stats)
	void UpdateStat(EStat stat);
	
	//should be called on OnComponentBeginPlay Event, without it stats may not work correctly
	UFUNCTION(BlueprintCallable, Category = Stats)
	void SetWidgetReference(EStat stat, class UStatBarWidget* widget);
	
	//delegate called on begin play with little delay to dont collide with other Actors BeginPlay events
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
	FOnComponentBeginPlay OnComponentBeginPlay;
	

};
