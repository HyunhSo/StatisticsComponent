// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"
#include "StatBarWidget.generated.h"

/**
 * 
 */
UCLASS()
class COMMUNITYCOMBAT_API UStatBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	//pointer to material instance corresponding for remove stat bar
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Stat)
	class UMaterialInstanceDynamic* DynamicMaterial;

public:
	//returns pointer to progress bar, implemented in blueprints((simple return value)
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, BlueprintCallable, Category = Stat)
	class UProgressBar* GetStatProgressBar();

	//returns pointer to image of remove progress bar, implemented in bluprints(simple return value)
	UFUNCTION(BlueprintImplementableEvent, BlueprintPure, BlueprintCallable, Category = Stat)
	class UImage* GetStatRemoveBar();

	//fill color of progress bar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat, meta = (ExposeOnSpawn = true))
	FLinearColor BarColor;

	//fill color of remove progress bar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stat, meta = (ExposeOnSpawn = true))
	FLinearColor RemoveColor;

	//getter for dynamic material
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	FORCEINLINE class UMaterialInstanceDynamic* GetDynamicMaterial() { return DynamicMaterial; }
	
};
