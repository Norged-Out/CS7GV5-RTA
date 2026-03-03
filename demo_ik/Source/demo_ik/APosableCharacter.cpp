// Fill out your copyright notice in the Description page of Project Settings.

#include "APosableCharacter.h"

// Sets default values
AAPosableCharacter::AAPosableCharacter()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// create the component instance to attach to the actor (this is required)
	posableMeshComponent_reference = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("PoseableMesh"));

	// attach your posable mesh component to the root (make it a child of the root component)
	posableMeshComponent_reference->SetMobility(EComponentMobility::Movable);
	posableMeshComponent_reference->SetVisibility(true);
	posableMeshComponent_reference->SetupAttachment(RootComponent);
	RootComponent = posableMeshComponent_reference;
	
	// reference the mannequin asset by path (be aware to check the path if this is not working). 
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannequinMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"));	
	if (MannequinMesh.Succeeded())
	{
		// set default mesh with the mannequin access and apply it to the posable mesh.
		default_skeletalMesh_reference = MannequinMesh.Object;
		posableMeshComponent_reference->SetSkinnedAssetAndUpdate(default_skeletalMesh_reference);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("manequin not found, check the path."));
	}

	// create the component instance to attach to the actor (this is required)
	targetSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("targetSphere"));

	// define the properties of the target sphere and attach it to the root.
	targetSphere->SetMobility(EComponentMobility::Movable);
	targetSphere->SetVisibility(true);
	targetSphere->SetupAttachment(RootComponent);

	// reference the sphere asset by path (be aware to check the path if this is not working).
	static ConstructorHelpers::FObjectFinder<UStaticMesh> sphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (sphereMesh.Succeeded())
	{
		// set the mesh with the source sphere asset.
		targetSphereAsset = sphereMesh.Object;
		targetSphere->SetStaticMesh(targetSphereAsset);
		// set the scaling of the sphere.
		targetSphere->SetWorldScale3D(FVector(targetSphereScaling, targetSphereScaling, targetSphereScaling));
		
		// reference the sphere material by path (be aware to check the path if this is not working).
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> sphereMaterial(TEXT("/Game/material/M_targetSphere"));
		if (sphereMaterial.Succeeded())
		{
			// set the material with the source material asset.
			targetSphereMaterial = UMaterialInstanceDynamic::Create(sphereMaterial.Object, this, FName("sphereTarget_dynamicMaterial"));
			if (targetSphereMaterial)
			{
				targetSphere->SetMaterial(0, targetSphereMaterial);
				// set the color of the material with the provided color.
				targetSphereMaterial->SetVectorParameterValue(TEXT("Color"), targetSphereColor);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("sphere not found, check the path."));
	}

}

bool AAPosableCharacter::initializePosableMesh()
{
	// initialization checks to avoid crashes.
	if (!posableMeshComponent_reference)
	{
		UE_LOG(LogTemp, Warning, TEXT("Posable mesh component not attached or registerd"));
		return false;
	}
	if (!default_skeletalMesh_reference)
	{
		UE_LOG(LogTemp, Warning, TEXT("No skeletal mesh reference provided."));
		return false;
	}

	// set up the poseable mesh component.
	posableMeshComponent_reference->SetSkinnedAssetAndUpdate(default_skeletalMesh_reference);
	return true;
}

void AAPosableCharacter::waving_playStop()
{
	CurrentMode = (CurrentMode == EAnimMode::Wave)
		? EAnimMode::None
		: EAnimMode::Wave;
}

void AAPosableCharacter::idle_playStop()
{
	CurrentMode = (CurrentMode == EAnimMode::Idle)
		? EAnimMode::None
		: EAnimMode::Idle;
}

void AAPosableCharacter::ik_arm_playStop()
{
	CurrentMode = (CurrentMode == EAnimMode::IK_Arm)
		? EAnimMode::None
		: EAnimMode::IK_Arm;
}

void AAPosableCharacter::testSetTargetSphereRelativePosition()
{
	setTargetSphereRelativePosition(targetSphereTestRelativePosition);
}

void AAPosableCharacter::setTargetSphereRelativePosition(FVector newPosition)
{
	if (targetSphere)
	{
		targetSphere->SetRelativeLocation(newPosition);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("target sphere not found!"));
	}
}

bool AAPosableCharacter::doesBoneOrSocketNameExists(FName inputName)
{
	// initialization check to avoid crashes.
	if (!posableMeshComponent_reference)
	{
		UE_LOG(LogTemp, Warning, TEXT("Posable mesh component not attached or registerd"));
		return false;
	}
	// check over all the bones names
	TArray<FName> _boneNames = TArray<FName>();
	posableMeshComponent_reference->GetBoneNames(_boneNames);
	return(
		_boneNames.Contains(inputName)
		or posableMeshComponent_reference->DoesSocketExist(inputName)
	);
}

void AAPosableCharacter::setVisibility(bool visible)
{
	// initialization check to avoid crashes.
	if (!posableMeshComponent_reference)
	{
		UE_LOG(LogTemp, Warning, TEXT("Posable mesh component not attached or registerd"));
	}
	else
		posableMeshComponent_reference->SetVisibility(visible);
}

void AAPosableCharacter::storeCurrentPoseRotations(TArray<FRotator>& storedPose)
{
	// initialization check to avoid crashes.
	if (!posableMeshComponent_reference)
	{
		UE_LOG(LogTemp, Warning, TEXT("Posable mesh component not attached or registerd"));
		return;
	}

	const int32 NumBones = posableMeshComponent_reference->GetNumBones();
	storedPose.SetNum(NumBones);

	for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
	{
		FName BoneName = posableMeshComponent_reference->GetBoneName(BoneIndex);
		if (BoneName != NAME_None)
		{
			storedPose[BoneIndex] = posableMeshComponent_reference->GetBoneRotationByName(BoneName, EBoneSpaces::ComponentSpace);
		}
	}
}

void AAPosableCharacter::waving_initializeStartingPose()
{
	// initialization check to avoid crashes.
	if (!posableMeshComponent_reference)
	{
		UE_LOG(LogTemp, Warning, TEXT("Posable mesh component not attached or registerd"));
		return;
	}

	// get time (for the periodic function)
	const float Time = GetWorld()->GetTimeSeconds();

	// we declare the bones we want to edit.
	FName upperArmBoneName = FName("upperarm_r");
	FName clavicleBoneName = FName("clavicle_r");

	// upperarm
	if (doesBoneOrSocketNameExists(upperArmBoneName))
	{
		
		// (2) get the bone transform in component space.
		FTransform bone_componentSpaceTransform  = posableMeshComponent_reference->GetBoneTransformByName(
			upperArmBoneName,
			EBoneSpaces::ComponentSpace
		);
		
		// (3) get the bone's parent transform in component space.
		FTransform parent_componentSpaceTransform = posableMeshComponent_reference->GetBoneTransformByName(
			posableMeshComponent_reference->GetParentBone(upperArmBoneName),
			EBoneSpaces::ComponentSpace
		);
		
		// (4) extract the bone transform relative to the parent reference system (using FTransform built in function). 
		FTransform bone_relativeTransform  = bone_componentSpaceTransform .GetRelativeTransform(parent_componentSpaceTransform);
		
		// (5) define the relative rotation we want to set.
		FRotator relativeBoneRotation = FRotator(21.435965, 21.709806, -92.235083);
		//(Pitch = 21.435965, Yaw = 21.709806, Roll = -92.235083)


		// (6) set the rotation in the relative transform. (note that transform use quaternions internally, so we need to use the built in function to extract the quaternion from the Rotator).
		bone_relativeTransform .SetRotation(relativeBoneRotation.Quaternion());

		// (7) change back from relative space to component space. 
		FTransform newBoneTransform_world = bone_relativeTransform  * parent_componentSpaceTransform;
		
		// (8) finally we set the rotation, in component space, by name.
		posableMeshComponent_reference->SetBoneRotationByName(
			upperArmBoneName,
			newBoneTransform_world.Rotator(),
			EBoneSpaces::ComponentSpace
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("bone: %s not found!"), *upperArmBoneName.ToString());
	}

	if (doesBoneOrSocketNameExists(clavicleBoneName))
	{
		FTransform bone_componentSpaceTransform  = posableMeshComponent_reference->GetBoneTransformByName(
			clavicleBoneName,
			EBoneSpaces::ComponentSpace
		);
		FTransform parent_componentSpaceTransform = posableMeshComponent_reference->GetBoneTransformByName(
			posableMeshComponent_reference->GetParentBone(clavicleBoneName),
			EBoneSpaces::ComponentSpace
		);
		FTransform bone_relativeTransform  = bone_componentSpaceTransform .GetRelativeTransform(parent_componentSpaceTransform);
		
		FRotator relativeBoneRotation = FRotator(-78.486128, 177.309228, 13.290207);
		
		bone_relativeTransform .SetRotation(relativeBoneRotation.Quaternion());
		FTransform newBoneTransform_world = bone_relativeTransform  * parent_componentSpaceTransform;
		posableMeshComponent_reference->SetBoneRotationByName(
			clavicleBoneName,
			newBoneTransform_world.Rotator(),
			EBoneSpaces::ComponentSpace
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("bone: %s not found!"), *clavicleBoneName.ToString());
	}

	// Update transforms to ensure the changes are applied.
	posableMeshComponent_reference->RefreshBoneTransforms();
}

void AAPosableCharacter::waving_tickAnimation()
{
	// initialization check to avoid crashes.
	if (!posableMeshComponent_reference)
	{
		UE_LOG(LogTemp, Warning, TEXT("Posable mesh component not attached or registerd"));
		return;
	}
	const float currentTime = GetWorld()->GetTimeSeconds();
	const int32 numBones = posableMeshComponent_reference->GetNumBones();
	if (waving_initialBoneRotations.Num() != numBones)
	{
		UE_LOG(LogTemp, Warning, TEXT("You need to call session1_initialBoneRotations first!"));
		return;
	}

	FName lowerarmBoneName = FName("lowerarm_r");

	if (doesBoneOrSocketNameExists(lowerarmBoneName))
	{
		FTransform bone_componentSpaceTransform  = posableMeshComponent_reference->GetBoneTransformByName(
			lowerarmBoneName,
			EBoneSpaces::ComponentSpace
		);
		int currentBoneIndex = posableMeshComponent_reference->GetBoneIndex(lowerarmBoneName);
		FRotator storedRotation = waving_initialBoneRotations[currentBoneIndex];
		bone_componentSpaceTransform .SetRotation(storedRotation.Quaternion());
			
		FTransform parent_componentSpaceTransform = posableMeshComponent_reference->GetBoneTransformByName(
			posableMeshComponent_reference->GetParentBone(lowerarmBoneName),
			EBoneSpaces::ComponentSpace
		);
		FTransform bone_relativeTransform  = bone_componentSpaceTransform .GetRelativeTransform(parent_componentSpaceTransform);
			
		// calculate the rotation offset angle using a sine wave function
		float angleOffset = FMath::Sin(waving_animationSpeed * currentTime) * waving_amplitude; // 30 degrees amplitude
		FRotator rotationOffset(0.0f, angleOffset, 0.0f);

		// apply the offset to the initial rotation
		FRotator relativeBoneRotation = bone_relativeTransform .Rotator() + rotationOffset;

		bone_relativeTransform .SetRotation(relativeBoneRotation.Quaternion());

		FTransform newBoneTransform_world = bone_relativeTransform  * parent_componentSpaceTransform;
		posableMeshComponent_reference->SetBoneRotationByName(
			lowerarmBoneName,
			newBoneTransform_world.Rotator(),
			EBoneSpaces::ComponentSpace
		);

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("bone: %s not found!"), *lowerarmBoneName.ToString());
	}

}


void AAPosableCharacter::idle_tickAnimation(float DeltaTime)
{
	if (!posableMeshComponent_reference) return;

	float Time = GetWorld()->GetTimeSeconds();

	int32 NumBones = posableMeshComponent_reference->GetNumBones();
	if (idle_initialBoneRotations.Num() != NumBones) return;

	// Helper: applies offset relative to stored base pose
	auto ApplyOffset = [&](FName Bone, FRotator Offset)
		{
			int32 Index = posableMeshComponent_reference->GetBoneIndex(Bone);
			if (Index == INDEX_NONE) return;

			FRotator BaseRot = idle_initialBoneRotations[Index];
			FRotator FinalRot = BaseRot + Offset;

			posableMeshComponent_reference->SetBoneRotationByName(
				Bone,
				FinalRot,
				EBoneSpaces::ComponentSpace);
		};

	/* ---------- NORMAL IDLE MOTION ---------- */

	float breathe = FMath::Sin(Time * 1.2f) * 3.f;            // chest breathing
	float headYaw = FMath::Sin(Time * 0.6f) * 12.f;           // look left/right
	float headPitch = FMath::Sin(Time * 0.8f) * 4.f;          // slight nod
	float shoulderRoll = FMath::Sin(Time * 1.4f) * 6.f;       // shoulder loosen
	float armSwing = FMath::Sin(Time * 1.0f) * 20.f;          // warm-up swing
	float hipShift = FMath::Sin(Time * 0.5f) * 4.f;           // torso sway
	float pelvisYaw = FMath::Sin(Time * 0.5f) * 6.f;          // pelvic rotation
	float pelvisRoll = FMath::Sin(Time * 0.5f) * 3.f;         // weight shift

	ApplyOffset("spine_02", FRotator(breathe, hipShift, 0));
	ApplyOffset("spine_03", FRotator(breathe * 0.5f, 0, 0));

	ApplyOffset("head", FRotator(headPitch, headYaw, 0));

	ApplyOffset("clavicle_l", FRotator(shoulderRoll, 0, 0));
	ApplyOffset("clavicle_r", FRotator(-shoulderRoll, 0, 0));

	ApplyOffset("upperarm_l", FRotator(armSwing, 0, 0));
	ApplyOffset("upperarm_r", FRotator(-armSwing, 0, 0));

	ApplyOffset("pelvis", FRotator(0, pelvisYaw, pelvisRoll));

	posableMeshComponent_reference->RefreshBoneTransforms();
}


/// <summary>
/// FABRIK for Arm
/// </summary>
void AAPosableCharacter::InitializeFABRIK_Arm()
{
	if (!posableMeshComponent_reference) return;

	IK_JointPositions.Empty();
	IK_BoneLengths.Empty();
	IK_TotalArmLength = 0.0f;

	TArray<FName> Chain = { ArmRootBone, ArmMidBone, ArmEndBone };

	for (FName Bone : Chain)
	{
		FVector Pos = posableMeshComponent_reference
			->GetBoneLocationByName(Bone, EBoneSpaces::ComponentSpace);

		IK_JointPositions.Add(Pos);
	}

	for (int32 i = 0; i < IK_JointPositions.Num() - 1; ++i)
	{
		float Length = FVector::Distance(
			IK_JointPositions[i],
			IK_JointPositions[i + 1]);

		IK_BoneLengths.Add(Length);
		IK_TotalArmLength += Length;
	}

	// Compute initial upper arm direction
	FVector Upper = (IK_JointPositions[1] - IK_JointPositions[0]).GetSafeNormal();
	FVector Lower = (IK_JointPositions[2] - IK_JointPositions[1]).GetSafeNormal();

	// Store original shoulder direction
	IK_OriginalUpperDir = Upper;

	// Store elbow direction
	IK_PoleVector = IK_JointPositions[1] - IK_JointPositions[0];
	IK_PoleVector.Normalize();
}

void AAPosableCharacter::SolveFABRIK_Arm(const FVector& TargetPosition)
{
	if (!posableMeshComponent_reference) return;

	const int32 NumJoints = IK_JointPositions.Num();
	if (NumJoints < 3) return;

	TArray<FName> Chain = { ArmRootBone, ArmMidBone, ArmEndBone };

	// Update joint positions from current mesh
	for (int32 i = 0; i < NumJoints; ++i)
	{
		IK_JointPositions[i] =
			posableMeshComponent_reference->GetBoneLocationByName(
				Chain[i], EBoneSpaces::ComponentSpace);
	}

	FVector RootPosition = IK_JointPositions[0];
	float DistanceToTarget = FVector::Distance(RootPosition, TargetPosition);

	UE_LOG(LogTemp, Warning,
		TEXT("BEFORE | Root=%s  Elbow=%s  Hand=%s  Target=%s  Dist=%.2f  Total=%.2f"),
		*IK_JointPositions[0].ToString(),
		*IK_JointPositions[1].ToString(),
		*IK_JointPositions[2].ToString(),
		*TargetPosition.ToString(),
		FVector::Distance(IK_JointPositions[0], TargetPosition),
		IK_TotalArmLength);

	// Solve positions
	SolveFABRIK_Positions(TargetPosition, RootPosition, DistanceToTarget);

	UE_LOG(LogTemp, Warning,
		TEXT("AFTER  | Root=%s  Elbow=%s  Hand=%s"),
		*IK_JointPositions[0].ToString(),
		*IK_JointPositions[1].ToString(),
		*IK_JointPositions[2].ToString());

	// Apply constraints
	ApplyElbowConstraint();
	ApplyShoulderConstraint();
	ApplyWristConstraint();

	IK_JointPositions[2] = TargetPosition;

	IK_JointPositions[1] =
		IK_JointPositions[2] -
		(IK_JointPositions[2] - IK_JointPositions[1]).GetSafeNormal()
		* IK_BoneLengths[1];

	IK_JointPositions[0] = RootPosition;

	// Apply rotations
	ApplyFABRIKRotations(Chain);
}


void AAPosableCharacter::SolveFABRIK_Positions(const FVector& TargetPosition,
	const FVector& RootPosition, float DistanceToTarget)
{
	const int32 NumJoints = IK_JointPositions.Num();

	if (DistanceToTarget > IK_TotalArmLength)
	{
		FVector Dir = (TargetPosition - RootPosition).GetSafeNormal();

		for (int32 i = 1; i < NumJoints; ++i)
		{
			IK_JointPositions[i] =
				IK_JointPositions[i - 1] + Dir * IK_BoneLengths[i - 1];
		}
	}
	else
	{
		for (int32 Iter = 0; Iter < IK_MaxIterations; ++Iter)
		{
			// Backward
			IK_JointPositions.Last() = TargetPosition;

			for (int32 i = NumJoints - 2; i >= 0; --i)
			{
				FVector Dir =
					(IK_JointPositions[i] - IK_JointPositions[i + 1]).GetSafeNormal();

				IK_JointPositions[i] =
					IK_JointPositions[i + 1] + Dir * IK_BoneLengths[i];
			}

			// Forward
			IK_JointPositions[0] = RootPosition;

			for (int32 i = 1; i < NumJoints; ++i)
			{
				FVector Dir =
					(IK_JointPositions[i] - IK_JointPositions[i - 1]).GetSafeNormal();

				IK_JointPositions[i] =
					IK_JointPositions[i - 1] + Dir * IK_BoneLengths[i - 1];
			}

			if (FVector::Distance(IK_JointPositions.Last(), TargetPosition) < IK_Tolerance)
				break;
		}
	}
}

void AAPosableCharacter::ApplyFABRIKRotations(const TArray<FName>& Chain)
{
	const int32 NumJoints = Chain.Num();

	for (int32 i = 0; i < NumJoints - 1; ++i)
	{
		FName Bone = Chain[i];
		FName Child = Chain[i + 1];

		// Current mesh direction (bind/reference direction)
		FVector A = posableMeshComponent_reference
			->GetBoneLocationByName(Bone, EBoneSpaces::ComponentSpace);

		FVector B = posableMeshComponent_reference
			->GetBoneLocationByName(Child, EBoneSpaces::ComponentSpace);

		FVector CurrentDir = (B - A).GetSafeNormal();

		// Desired direction from FABRIK
		FVector DesiredDir =
			(IK_JointPositions[i + 1] - IK_JointPositions[i]).GetSafeNormal();

		FQuat Delta = FQuat::FindBetweenNormals(CurrentDir, DesiredDir);

		FTransform CurTransform =
			posableMeshComponent_reference->GetBoneTransformByName(
				Bone, EBoneSpaces::ComponentSpace);

		FQuat NewRot = Delta * CurTransform.GetRotation();

		posableMeshComponent_reference->SetBoneRotationByName(
			Bone,
			NewRot.Rotator(),
			EBoneSpaces::ComponentSpace);
	}

	posableMeshComponent_reference->RefreshBoneTransforms();
}

void AAPosableCharacter::ApplyElbowConstraint()
{
	if (IK_JointPositions.Num() < 3) return;

	FVector Upper = (IK_JointPositions[1] - IK_JointPositions[0]).GetSafeNormal();
	FVector Lower = (IK_JointPositions[2] - IK_JointPositions[1]).GetSafeNormal();

	// Clamp elbow angle 
	float Dot = FVector::DotProduct(Upper, Lower);
	Dot = FMath::Clamp(Dot, -1.f, 1.f);

	float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));

	if (Angle < IK_MinElbowAngle || Angle > IK_MaxElbowAngle)
	{
		float Clamped = FMath::Clamp(Angle, IK_MinElbowAngle, IK_MaxElbowAngle);
		float Delta = Clamped - Angle;

		FVector Axis = FVector::CrossProduct(Upper, Lower).GetSafeNormal();
		FQuat Correction = FQuat(Axis, FMath::DegreesToRadians(Delta));

		Lower = Correction.RotateVector(Lower);

		IK_JointPositions[2] =
			IK_JointPositions[1] + Lower * IK_BoneLengths[1];
	}

	// Stable pole projection
	FVector ShoulderToHand =
		(IK_JointPositions[2] - IK_JointPositions[0]).GetSafeNormal();

	FVector DesiredElbowDir =
		FVector::VectorPlaneProject(IK_PoleVector, ShoulderToHand).GetSafeNormal();

	IK_JointPositions[1] =
		IK_JointPositions[0] +
		DesiredElbowDir * IK_BoneLengths[0];
}

void AAPosableCharacter::ApplyShoulderConstraint()
{
	// Safety check
	if (IK_JointPositions.Num() < 2) return;

	// Current upper arm direction
	FVector CurrentDir =
		(IK_JointPositions[1] - IK_JointPositions[0]).GetSafeNormal();

	// Compare with original rest direction
	float Dot = FVector::DotProduct(IK_OriginalUpperDir, CurrentDir);
	Dot = FMath::Clamp(Dot, -1.f, 1.f);

	// Compute how far the shoulder has rotated
	float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));

	// If beyond allowed range, clamp it
	if (Angle > IK_MaxShoulderAngle)
	{
		// How much we exceeded limit
		float Excess = Angle - IK_MaxShoulderAngle;

		// Axis around which rotation happened
		FVector Axis =
			FVector::CrossProduct(IK_OriginalUpperDir, CurrentDir).GetSafeNormal();

		// Rotate back by the excess amount
		FQuat Correction =
			FQuat(Axis, FMath::DegreesToRadians(-Excess));

		FVector ClampedDir =
			Correction.RotateVector(CurrentDir);

		// Reapply clamped shoulder direction
		IK_JointPositions[1] =
			IK_JointPositions[0] + ClampedDir * IK_BoneLengths[0];
	}
}

void AAPosableCharacter::ApplyWristConstraint()
{
	if (IK_JointPositions.Num() < 3) return;

	FVector Lower =
		(IK_JointPositions[2] - IK_JointPositions[1]).GetSafeNormal();

	FRotator WristRot = Lower.Rotation();

	WristRot.Pitch = FMath::Clamp(WristRot.Pitch,
		IK_WristMinPitch, IK_WristMaxPitch);

	WristRot.Yaw = FMath::Clamp(WristRot.Yaw,
		IK_WristMinYaw, IK_WristMaxYaw);

	FVector ClampedDir = WristRot.Vector();

	IK_JointPositions[2] =
		IK_JointPositions[1] + ClampedDir * IK_BoneLengths[1];
}



// Called when the game starts or when spawned
void AAPosableCharacter::BeginPlay()
{
	Super::BeginPlay();
	initializePosableMesh();

	// Store base pose for waving
	waving_initialBoneRotations = TArray<FRotator>();
	storeCurrentPoseRotations(waving_initialBoneRotations);

	// Store base pose for idle
	idle_initialBoneRotations = TArray<FRotator>();
	storeCurrentPoseRotations(idle_initialBoneRotations);

	// Initialize FABRIK leg solver
	InitializeFABRIK_Arm();
}

// Called every frame
void AAPosableCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	switch (CurrentMode)
	{
	case EAnimMode::Idle:
		idle_tickAnimation(DeltaTime);
		break;

	case EAnimMode::Wave:
		waving_tickAnimation();
		break;

	case EAnimMode::IK_Arm:
		if (targetSphere)
		{
			FVector Target = posableMeshComponent_reference
				->GetComponentTransform()
				.InverseTransformPosition(targetSphere->GetComponentLocation());

			SolveFABRIK_Arm(Target);
		}
		break;

	default:
		break;
	}

}

