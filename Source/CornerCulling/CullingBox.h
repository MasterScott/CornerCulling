// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
//#include "Components/StaticMeshComponent.h"
//#include "Components/BoxComponent.h"
#include "CullingBox.generated.h"

UCLASS()
class CORNERCULLING_API ACullingBox : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACullingBox();

	UPROPERTY(VisibleDefaultsOnly)
	class UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* Box;

	// Number of corners.
	UPROPERTY(Category = Box, VisibleAnywhere)
	int N = 4;
	// List of corner positions
	UPROPERTY(Category = Box, VisibleAnywhere)
	TArray<FVector2D> CornerLocations;
	// Center of box
	UPROPERTY(Category = Box, VisibleAnywhere)
	FVector Center;
	UPROPERTY(Category = Box, VisibleAnywhere)
	FVector2D Center2D;
	// Z coordinate of the top of the box
	UPROPERTY(Category = Box, VisibleAnywhere)
	float TopZ;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Return pointers to the two corners that define the edges of the object
	// from the player's perspective.
	void GetRelevantCorners(const FVector2D& PlayerLocation, FVector2D& CornerLeft, FVector2D& CornerRight);
};
