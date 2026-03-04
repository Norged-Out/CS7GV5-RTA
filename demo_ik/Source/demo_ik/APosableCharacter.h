// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/PoseableMeshComponent.h"
#include "APosableCharacter.generated.h"

/**
 * It is a skeletal mesh which pose can be modify directly on the main thread (posable mesh).
 * it has to be initialized starting from a source skeletal mesh component (which can only be modify on the animation thread).
 * It is used to display modifications such as IK.
 */

UCLASS()
class DEMO_IK_API AAPosableCharacter : public AActor
{
	GENERATED_BODY()

public:

	/**
	* the poseable mesh component.
	**/
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class UPoseableMeshComponent* posableMeshComponent_reference;

	/**
	* the default skeletal mesh component.
	**/
	UPROPERTY(EditAnywhere, Category = Mesh)
	USkeletalMesh* default_skeletalMesh_reference;

	/**
	* the waving animation speed.
	**/
	UPROPERTY(EditAnywhere, Category = "waving animation")
	float waving_animationSpeed = 5.0f;

	/**
	* the waving animation amplitude.
	**/
	UPROPERTY(EditAnywhere, Category = "waving animation")
	float waving_amplitude = 30.0f;

	/**
	* target sphere asset.
	**/
	UPROPERTY(EditAnywhere, Category = "target")
	UStaticMesh* targetSphereAsset;

	/**
	* scaling factor for the target sphere.
	**/
	UPROPERTY(EditAnywhere, Category = "target")
	float targetSphereScaling = 0.1f;

	/**
	* color of the target sphere.
	**/
	UPROPERTY(EditAnywhere, Category = "target")
	FColor targetSphereColor = FColor::Red;

	/**
	* test position for the target sphere.
	**/
	UPROPERTY(EditAnywhere, Category = "target|test")
	FVector targetSphereTestRelativePosition = FVector(0, 0, 0);


public:

	/**
	* constructor.
	**/
	AAPosableCharacter();

	/**
	* simplified function for student example, without parameters.
	* @return: true if the initialization was successful, false otherwise
	**/
	bool initializePosableMesh();

	/**
	* function to play or stop the waving animation.
	**/
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "waving animation")
	void waving_playStop();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Idle")
	void idle_playStop();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "IK|Arm")
	void ik_arm_playStop();

	/**
	* function to change the target sphere position.
	**/
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "target|test")
	void testSetTargetSphereRelativePosition();

	/**
	* function to change the target sphere position.
	* @param newPosition: the new position of the target sphere.
	**/
	void setTargetSphereRelativePosition(FVector newPosition);

	/**
	* check if a Bone or Socket name exists.
	**/
	bool doesBoneOrSocketNameExists(FName inputName);

	/**
	* set the visibility of the mesh.
	**/
	void setVisibility(bool visible);


protected:

	/**
	* target sphere for IK.
	**/
	UStaticMeshComponent* targetSphere;

	/**
	* target sphere material.
	**/
	UMaterialInstanceDynamic* targetSphereMaterial;

	/**
	* the set of initial bone rotations for the waving animation.
	**/
	TArray<FRotator> waving_initialBoneRotations;

	TArray<FRotator> idle_initialBoneRotations;

	/*
	Animation settings
	*/
	enum class EAnimMode : uint8
	{
		None,
		Idle,
		Wave,
		IK_Arm
	};

	EAnimMode CurrentMode = EAnimMode::Idle;

	/**
	* store current pose rotations.
	**/
	void storeCurrentPoseRotations(TArray<FRotator>& storedPose);

	/**
	* waving animation initialization.
	**/
	void waving_initializeStartingPose();

	/**
	* waving animation tick.
	**/
	void waving_tickAnimation();

	/**
	* idle animation tick.
	**/
	void idle_tickAnimation(float DeltaTime);


	/* -------- FABRIK -------- */

	// Bone chain 
	UPROPERTY(EditAnywhere, Category = "IK|Arm")
	FName ArmRootBone = "upperarm_r";

	UPROPERTY(EditAnywhere, Category = "IK|Arm")
	FName ArmMidBone = "lowerarm_r";

	UPROPERTY(EditAnywhere, Category = "IK|Arm")
	FName ArmHandBone = "hand_r";

	// Palm centroid bones
	UPROPERTY(EditAnywhere, Category = "IK|Arm")
	TArray<FName> PalmBones =
	{
		"index_metacarpal_r",
		"middle_metacarpal_r",
		"ring_metacarpal_r",
		"pinky_metacarpal_r"
	};

	// Solver settings
	UPROPERTY(EditAnywhere, Category = "IK|Arm")
	int32 IK_MaxIterations = 10;

	UPROPERTY(EditAnywhere, Category = "IK|Arm")
	float IK_Tolerance = 1.0f;

	// Internal solver data
	TArray<FVector> IK_JointPositions;
	TArray<float> IK_BoneLengths;
	float IK_TotalArmLength = 0.0f;

	// Shoulder limits
	FVector IK_OriginalUpperDir;
	float IK_MaxShoulderAngle = 110.f;

	// Elbow limits
	FVector IK_PoleVector;
	float IK_MinElbowAngle = 5.0f;
	float IK_MaxElbowAngle = 150.0f;
	float IK_ForearmMinTwist = -80.f;
	float IK_ForearmMaxTwist = 80.f;

	// Wrist limits
	float IK_WristMinPitch = -60.f;
	float IK_WristMaxPitch = 60.f;

	float IK_WristMinYaw = -45.f;
	float IK_WristMaxYaw = 45.f;

	// Entry points
	void InitializeFABRIK_Arm();
	void SolveFABRIK_Arm(const FVector& TargetPosition);

	// Internal helpers
	void SolveFABRIK_Positions(const FVector& TargetPosition,
		const FVector& RootPosition, float DistanceToTarget);
	void ApplyFABRIKRotations(const TArray<FName>& Chain);
	void ApplyElbowConstraint();
	void ApplyShoulderConstraint();
	void ApplyWristConstraint();
	FVector ComputePalmCentroid();
	void LockForearmRoll();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};