// Fill out your copyright notice in the Description page of Project Settings.

#include "APosableCharacter.h"
#include "Kismet/KismetMathLibrary.h"

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


	HandPathSpline = CreateDefaultSubobject<USplineComponent>(TEXT("HandPathSpline"));
	HandPathSpline->SetupAttachment(RootComponent);
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

	TArray<FName> Chain =
	{
		ArmRootBone,
		ArmMidBone,
		ArmHandBone
	};

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

	TArray<FName> Chain =
	{
		ArmRootBone,
		ArmMidBone,
		ArmHandBone
	};

	// Update joint positions from current mesh
	for (int32 i = 0; i < NumJoints; ++i)
	{
		IK_JointPositions[i] =
			posableMeshComponent_reference->GetBoneLocationByName(
				Chain[i], EBoneSpaces::ComponentSpace);
	}

	FVector RootPosition = IK_JointPositions[0];

	// --- palm centroid ---
	FVector PalmCenter = ComputePalmCentroid();
	FVector HandPos = IK_JointPositions[2];
	FVector PalmOffset = PalmCenter - HandPos;

	// wrist target so palm hits sphere
	FVector WristTarget = TargetPosition - PalmOffset;

	float DistanceToTarget = FVector::Distance(RootPosition, WristTarget);
	bool bReachable = DistanceToTarget <= IK_TotalArmLength;

	UE_LOG(LogTemp, Warning,
		TEXT("TARGET | Sphere=%s Palm=%s WristTarget=%s Offset=%s Reachable=%s Dist=%.2f Max=%.2f"),
		*TargetPosition.ToString(),
		*PalmCenter.ToString(),
		*WristTarget.ToString(),
		*PalmOffset.ToString(),
		bReachable ? TEXT("YES") : TEXT("NO"),
		DistanceToTarget,
		IK_TotalArmLength);

	// Solve positions
	SolveFABRIK_Positions(WristTarget, RootPosition, DistanceToTarget);

	float FinalError = FVector::Distance(IK_JointPositions[2], WristTarget);

	UE_LOG(LogTemp, Warning,
		TEXT("SOLVED | Shoulder=%s Elbow=%s Wrist=%s Error=%.3f"),
		*IK_JointPositions[0].ToString(),
		*IK_JointPositions[1].ToString(),
		*IK_JointPositions[2].ToString(),
		FinalError);

	// Apply rotations
	ApplyFABRIKRotations(Chain);
	LockForearmRoll();
}


void AAPosableCharacter::SolveFABRIK_Positions(const FVector& TargetPosition,
	const FVector& RootPosition, float DistanceToTarget)
{
	const int32 NumJoints = IK_JointPositions.Num();

	// Direction from root toward target
	FVector TargetDir = (TargetPosition - RootPosition).GetSafeNormal();
	float Bias = 0.1f;

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
			// Backward pass
			IK_JointPositions.Last() = TargetPosition;

			for (int32 i = NumJoints - 2; i >= 0; --i)
			{
				FVector Dir =
					(IK_JointPositions[i] - IK_JointPositions[i + 1]).GetSafeNormal();

				IK_JointPositions[i] =
					IK_JointPositions[i + 1] + Dir * IK_BoneLengths[i];
			}

			// Apply constraints
			ApplyShoulderConstraint();
			ApplyElbowConstraint();
			ApplyWristConstraint();

			// Forward pass
			IK_JointPositions[0] = RootPosition;

			for (int32 i = 1; i < NumJoints; ++i)
			{
				FVector Dir =
					(IK_JointPositions[i] - IK_JointPositions[i - 1]).GetSafeNormal();
				Dir = (Dir * (1.f - Bias) + TargetDir * Bias).GetSafeNormal();

				IK_JointPositions[i] =
					IK_JointPositions[i - 1] + Dir * IK_BoneLengths[i - 1];
			}

			// Apply constraints
			ApplyShoulderConstraint();
			ApplyElbowConstraint();
			ApplyWristConstraint();

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

	FVector Shoulder = IK_JointPositions[0];
	FVector Elbow = IK_JointPositions[1];
	FVector Hand = IK_JointPositions[2];

	// Direction from shoulder to hand
	FVector ShoulderToHand = (Hand - Shoulder).GetSafeNormal();

	// Character forward keeps elbow bending sideways relative to torso
	FVector BodyForward = GetActorForwardVector();

	// Compute a stable pole vector
	FVector StablePole =
		FVector::CrossProduct(BodyForward, ShoulderToHand).GetSafeNormal();

	// Plane normal defining elbow hinge plane
	FVector PlaneNormal =
		FVector::CrossProduct(ShoulderToHand, StablePole).GetSafeNormal();

	if (!PlaneNormal.IsNearlyZero())
	{
		// Project elbow onto the hinge plane
		FVector ShoulderToElbow = Elbow - Shoulder;

		FVector Projected =
			FVector::VectorPlaneProject(ShoulderToElbow, PlaneNormal).GetSafeNormal();

		IK_JointPositions[1] =
			Shoulder + Projected * IK_BoneLengths[0];
	}

	// Clamp elbow bend angle
	FVector Upper =
		(IK_JointPositions[1] - IK_JointPositions[0]).GetSafeNormal();

	FVector Lower =
		(IK_JointPositions[2] - IK_JointPositions[1]).GetSafeNormal();

	float Dot = FVector::DotProduct(Upper, Lower);
	Dot = FMath::Clamp(Dot, -1.f, 1.f);

	float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));

	// Enforce min/max elbow bend
	if (Angle < IK_MinElbowAngle || Angle > IK_MaxElbowAngle)
	{
		float Clamped = FMath::Clamp(Angle, IK_MinElbowAngle, IK_MaxElbowAngle);
		float Delta = Clamped - Angle;

		// Axis around which elbow bends
		FVector Axis =
			FVector::CrossProduct(Upper, Lower).GetSafeNormal();

		// Rotate forearm back inside allowed range
		FQuat Correction =
			FQuat(Axis, FMath::DegreesToRadians(Delta));

		Lower = Correction.RotateVector(Lower);

		// Rebuild wrist position with corrected forearm direction
		IK_JointPositions[2] =
			IK_JointPositions[1] + Lower * IK_BoneLengths[1];
	}
}

void AAPosableCharacter::ApplyShoulderConstraint()
{
	if (IK_JointPositions.Num() < 2) return;

	FVector Shoulder = IK_JointPositions[0];
	FVector Elbow = IK_JointPositions[1];

	// Current arm direction (component space)
	FVector Dir = (Elbow - Shoulder).GetSafeNormal();

	// Convert to actor-local space so constraint follows torso
	FVector LocalDir =
		GetActorTransform().InverseTransformVectorNoScale(Dir);

	// Rest direction stored in actor space (set once in init)
	FVector RestDir = IK_OriginalUpperDir;
	RestDir.Normalize();

	// Angle from rest direction
	float Dot = FVector::DotProduct(RestDir, LocalDir);
	Dot = FMath::Clamp(Dot, -1.f, 1.f);

	float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));
	float MaxAngle = IK_MaxShoulderAngle;

	if (Angle > MaxAngle)
	{
		float Excess = Angle - MaxAngle;

		FVector Axis =
			FVector::CrossProduct(RestDir, LocalDir).GetSafeNormal();

		FQuat Correction =
			FQuat(Axis, FMath::DegreesToRadians(-Excess));

		LocalDir = Correction.RotateVector(LocalDir);
	}

	// Convert back to component space
	FVector NewDir =
		GetActorTransform().TransformVectorNoScale(LocalDir);

	NewDir.Normalize();

	// Rebuild elbow position
	IK_JointPositions[1] =
		Shoulder + NewDir * IK_BoneLengths[0];
}

void AAPosableCharacter::ApplyWristConstraint()
{
	if (IK_JointPositions.Num() < 3) return;

	FVector Shoulder = IK_JointPositions[0];
	FVector Elbow = IK_JointPositions[1];
	FVector Hand = IK_JointPositions[2];

	// Direction of upper arm (parent direction)
	FVector ParentDir =
		(Elbow - Shoulder).GetSafeNormal();

	// Current forearm direction
	FVector WristDir =
		(Hand - Elbow).GetSafeNormal();

	// Compute angle between upper arm and forearm
	float Dot = FVector::DotProduct(ParentDir, WristDir);
	Dot = FMath::Clamp(Dot, -1.f, 1.f);

	float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));

	// Maximum allowed wrist bend (cone limit)
	float MaxAngle = IK_WristMaxAngle;

	if (Angle > MaxAngle)
	{
		// Amount we exceeded the limit
		float Excess = Angle - MaxAngle;

		// Axis that rotates WristDir back toward ParentDir
		FVector Axis =
			FVector::CrossProduct(ParentDir, WristDir).GetSafeNormal();

		FQuat Correction =
			FQuat(Axis, FMath::DegreesToRadians(-Excess));

		WristDir = Correction.RotateVector(WristDir);
	}

	// Rebuild wrist position using corrected direction
	IK_JointPositions[2] =
		Elbow + WristDir * IK_BoneLengths[1];
}

FVector AAPosableCharacter::ComputePalmCentroid()
{
	if (!posableMeshComponent_reference) return FVector::ZeroVector;

	FVector Sum = FVector::ZeroVector;
	int32 Count = 0;

	for (FName Bone : PalmBones)
	{
		if (doesBoneOrSocketNameExists(Bone))
		{
			Sum += posableMeshComponent_reference->GetBoneLocationByName(
				Bone,
				EBoneSpaces::ComponentSpace);
			Count++;
		}
	}

	if (Count == 0) return FVector::ZeroVector;

	return Sum / Count;
}

void AAPosableCharacter::LockForearmRoll()
{
	FName Upper = ArmRootBone;
	FName Lower = ArmMidBone;

	FTransform UpperT =
		posableMeshComponent_reference->GetBoneTransformByName(
			Upper, EBoneSpaces::ComponentSpace);

	FTransform LowerT =
		posableMeshComponent_reference->GetBoneTransformByName(
			Lower, EBoneSpaces::ComponentSpace);

	FRotator UpperRot = UpperT.Rotator();
	FRotator LowerRot = LowerT.Rotator();

	// copy roll from upper arm (hinge behaviour)
	LowerRot.Roll = UpperRot.Roll;

	posableMeshComponent_reference->SetBoneRotationByName(
		Lower,
		LowerRot,
		EBoneSpaces::ComponentSpace);
}

void AAPosableCharacter::ApplyHeadLookAt(const FVector& Target)
{
	if (!posableMeshComponent_reference) return;

	// get head position
	FVector HeadPos =
		posableMeshComponent_reference->GetBoneLocationByName(
			HeadBone, EBoneSpaces::ComponentSpace);

	// direction from head to target
	FVector AimPoint = Target + FVector(0, 0, 10); // slight upward bias
	FVector Dir = (AimPoint - HeadPos).GetSafeNormal();

	// convert direction into rotation
	FRotator LookRot = Dir.Rotation();

	// convert world rotation into character-local rotation
	FRotator TargetRot = UKismetMathLibrary::NormalizedDeltaRotator(LookRot, GetActorRotation());
	CurrentHeadRot = FMath::RInterpTo(CurrentHeadRot, TargetRot, GetWorld()->GetDeltaSeconds(), 5.f);
	FRotator LocalRot = CurrentHeadRot;

	// clamp natural limits
	LocalRot.Yaw = FMath::Clamp(LocalRot.Yaw, -HeadYawLimit, HeadYawLimit);
	LocalRot.Pitch = FMath::Clamp(LocalRot.Pitch, -HeadPitchLimit, HeadPitchLimit);
	LocalRot.Roll = 0.f;

	// split motion across neck/head
	FRotator NeckOffset = LocalRot * 0.35f;
	FRotator HeadOffset = LocalRot * 0.45f;

	// apply rotations relative to rest pose

	posableMeshComponent_reference->SetBoneRotationByName(
		NeckBone,
		NeckRestRot + NeckOffset,
		EBoneSpaces::ComponentSpace);

	posableMeshComponent_reference->SetBoneRotationByName(
		HeadBone,
		HeadRestRot + HeadOffset,
		EBoneSpaces::ComponentSpace);
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

	NeckRestRot = posableMeshComponent_reference->GetBoneRotationByName(NeckBone, EBoneSpaces::ComponentSpace);
	HeadRestRot = posableMeshComponent_reference->GetBoneRotationByName(HeadBone, EBoneSpaces::ComponentSpace);
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

		// move target along spline
		if (HandPathSpline && targetSphere)
		{
			// advance animation time
			SplineTime += DeltaTime;

			// normalize 0->1 over duration
			float Alpha = FMath::Fmod(SplineTime / SplineDuration, 1.f);

			// ease in/out motion
			Alpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

			// convert alpha to spline distance
			float Distance = Alpha * HandPathSpline->GetSplineLength();

			// sample spline position
			FVector SplinePos =
				HandPathSpline->GetLocationAtDistanceAlongSpline(
					Distance,
					ESplineCoordinateSpace::World);

			// move sphere
			targetSphere->SetWorldLocation(SplinePos);

			// convert to component space for IK
			FVector Target = posableMeshComponent_reference
				->GetComponentTransform()
				.InverseTransformPosition(SplinePos);

			// solve IK
			SolveFABRIK_Arm(Target);
			ApplyHeadLookAt(Target);
		}
		break;

	default:
		break;
	}
}

