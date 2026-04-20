DEMO 2 – TWO-CHARACTER INTERACTION
FULL TECHNICAL REPORT (FINAL – HIGH DETAIL, RUBRIC-ALIGNED VERSION)

------------------------------------------------------------
1. INTRODUCTION
------------------------------------------------------------
This project implements a complete two-character interaction system in Unreal Engine based on a Rock-Paper-Scissors (RPS) gameplay loop. The system consists of a fully controllable third-person MetaHuman player and an NPC opponent, both driven through synchronized animation pipelines and coordinated via a central manager Blueprint.

The focus of this implementation is not only functional correctness but also animation clarity, responsiveness, and technical depth. The system integrates animation blending, montage sequencing, UI-driven interaction, motion warping, and synchronized character behaviour into a single cohesive gameplay loop.

This report documents the system at a technical level, including implementation decisions, encountered issues, and how each component contributes to the overall interaction.

------------------------------------------------------------
2. SYSTEM ARCHITECTURE AND RESPONSIBILITY DISTRIBUTION
------------------------------------------------------------
The system is divided into three major Blueprint classes, each with a clearly defined role.

BP_Pri_Player (Playable Character):
- Inherits from Character
- Handles all player input and movement
- Contains CharacterMovementComponent for locomotion
- Uses SpringArm + Camera for third-person view
- Owns Skeletal Mesh that drives animation via ABP_Pri_Player

BP_Girl (NPC):
- Secondary character using similar skeletal structure
- Has independent Animation Blueprint
- Does not process player input
- Maintains visual focus using LookAt logic toward player

BP_RPS_Manager:
- Central system controller
- Responsible for:
  • Creating and managing UI
  • Receiving player selection input
  • Generating NPC move (random selection)
  • Comparing moves and resolving outcome
  • Triggering animation sequences on both characters
  • Updating score values
  • Enforcing round state (prevent overlap)

This separation ensures that animation logic, gameplay logic, and input handling remain decoupled, which simplifies debugging and improves scalability.

------------------------------------------------------------
3. ANIMATION ASSET TYPES AND USAGE
------------------------------------------------------------
Three distinct animation asset types are used throughout the system:

Animation Sequences:
- Raw mocap animations imported as .uasset files
- Used for all base motion clips
- Includes locomotion (idle, walk, run), ready pose, prep motion, throws, and result animations

Animation Montages:
- Each gameplay animation is wrapped inside a montage
- Used for event-driven playback inside Blueprints
- Allows access to On Blend Out, On Interrupted, and other execution pins
- Enables chaining of animations without using state machine transitions

Blend Spaces:
- Used for continuous interpolation between animations
- Player uses a 2D blend space
- NPC uses a 1D blend space

The combination of these three asset types allows both continuous animation (movement) and discrete animation (interaction) to coexist cleanly.

------------------------------------------------------------
4. PLAYER ANIMATION SYSTEM (2D BLEND SPACE – DETAILED)
------------------------------------------------------------
The player animation system is built around a 2D Blend Space that merges locomotion and interaction readiness into a single graph.

Axis Configuration:
- X-axis: Speed (0 to ~400)
- Y-axis: State (0 to 1)

Speed Calculation:
- Derived using Get Velocity → Vector Length
- Updated every tick inside AnimBP EventGraph

State Variable:
- Float variable manually set from Blueprint
- Controlled externally when interaction starts

Animation Placement:
- Idle at (0,0)
- Walk/Run distributed along X-axis at State=0
- ReadyLoop positioned along State=1

Behaviour:
- During normal gameplay: State = 0
- On interaction start: State = 1
- This shifts the blend vertically into the ReadyLoop animation

Technical Reasoning:
- Avoids using separate state machine branches
- Keeps locomotion and interaction unified
- Reduces transition complexity

Tradeoff:
- Less modular than a state machine
- But significantly easier to control from Blueprint

------------------------------------------------------------
5. NPC ANIMATION SYSTEM (1D BLEND SPACE – DETAILED)
------------------------------------------------------------
The NPC uses a simpler 1D Blend Space focused purely on interaction readiness.

Axis:
- State (0 to 1)

Animation Mapping:
- 0 → Idle animation
- 1 → Ready pose / ReadyLoop

Additional Adjustment:
- A fixed pose is used in certain positions to stabilize interpolation and prevent unwanted blending artifacts

LookAt System:
- Implemented using rotation updates every tick
- NPC continuously rotates to face the player

Technical Importance:
- Ensures that interaction always appears intentional
- Prevents NPC from appearing disconnected or static

------------------------------------------------------------
6. INTERACTION SYSTEM AND UI (FULL DETAIL)
------------------------------------------------------------
Interaction is entirely player-driven and controlled through UI.

Trigger Mechanism:
- Player presses F key

On Press F:
- "Press F to Play" prompt is removed
- UI widget is created dynamically
- Input mode switches to Game + UI

UI Design:
- Three buttons are presented:
  • Rock
  • Paper
  • Scissors

Each button:
- Calls a function inside BP_RPS_Manager
- Passes selected move as an enum value

After Selection:
- NPC move is randomly generated
- Manager compares moves
- Outcome is resolved using conditional logic

Technical Notes:
- UI is not persistent (spawned only when needed)
- Prevents accidental input during locomotion

------------------------------------------------------------
7. ANIMATION PIPELINE (PREP → THROW → RESULT)
------------------------------------------------------------
The core animation system is structured as a three-stage pipeline.

Stage 1 – Preparation:
- Single shared animation
- Represents hand tapping motion

Stage 2 – Throw:
- Three animations:
  • Rock
  • Paper
  • Scissors

Stage 3 – Result:
- Six animations:
  • Win variants (RWin, PWin, SWin)
  • Lose variants (RLose, PLose, SLose)
- One shared Draw animation

Total Count:
- 1 Prep
- 3 Throw
- 6 Result
- 1 Draw
= 11 animations

All animations are executed through montages for precise control.

------------------------------------------------------------
8. MONTAGE CHAINING IMPLEMENTATION (CRITICAL DISCUSSION)
------------------------------------------------------------
Animation chaining is handled entirely through the On Blend Out execution pin.

Why NOT On Completed:
- On Completed introduces a visible delay
- Breaks continuity between stages

Why Blend Out:
- Triggers next animation before previous fully ends
- Maintains continuous motion

Blend Configuration:
- Blend In ≈ 0.5 seconds
- Blend Out ≈ 0.5 seconds

Observed Behaviour:
- Smooth transitions between stages
- Slight overlap between animations

Limitation:
- Some transitions still produce snapping
- Caused by mismatched start/end poses in mocap

Conclusion:
- System is functioning correctly
- Remaining issues are asset-level

------------------------------------------------------------
9. MIRRORING SYSTEM (IMPLEMENTATION DETAILS)
------------------------------------------------------------
Mirroring is implemented using Unreal’s Mirror node.

Components:
- Mirror node in AnimGraph
- Mirror Data Table defining bone mapping
- Boolean variable (bMirrorPose)

Workflow:
- bMirrorPose set before montage plays
- Mirror node conditionally flips animation
- Variable reset after animation

Use Case:
- Only applied to specific animations (e.g. PaperWin)

Benefit:
- Eliminates need for duplicate animations
- Reduces asset count

------------------------------------------------------------
10. MOTION WARPING (FULL TECHNICAL BREAKDOWN)
------------------------------------------------------------
Motion warping is used to align characters spatially before animation execution.

Required Setup:
- Motion Warping component added to character
- Warp target defined (TargetPoint actor)
- Root motion enabled in AnimBP

Critical Issues Encountered:
- Montages initially played on child mesh (no root motion applied)
- AnimBP incorrectly configured (no root motion extraction)

Fixes Applied:
- Montages played on main Mesh component
- Root Motion Mode set to "Root Motion from Montages Only"
- Warp target updated before montage execution

Result:
- Character moves into correct position dynamically
- Eliminates manual positioning

------------------------------------------------------------
11. SYNCHRONIZATION BETWEEN CHARACTERS
------------------------------------------------------------
Synchronization is managed through BP_RPS_Manager.

Process:
- Same montage stage triggered on both characters
- Timing controlled centrally

Ensures:
- both characters perform actions simultaneously
- interaction appears coordinated

------------------------------------------------------------
12. GAME STATE MANAGEMENT
------------------------------------------------------------
Key Variable:
- bRoundActive

Function:
- prevents multiple simultaneous interactions
- ensures animation sequence completes before restart

Flow Control:
- input disabled during active round
- re-enabled after completion

------------------------------------------------------------
13. KEYFRAME EDITING AND CLEANUP (SEQUENCER USAGE)
------------------------------------------------------------
Keyframe-level adjustments were performed using Unreal Engine’s Sequencer to refine motion quality and reduce visible artifacts from the original mocap data.

Sequencer was used for:
- Trimming unwanted lead-in and tail frames from animation sequences
- Adjusting bone rotations where mocap produced unnatural poses
- Slightly re-timing sections of animations to better align transitions
- Reducing jitter in hand and upper-body motion

Workflow:
- Animation sequences were opened in Sequencer
- Problematic frames were identified (typically at transition boundaries)
- Keyframes were edited or removed to stabilize motion

Impact on system:
- Improved visual continuity between montage stages
- Reduced harsh snapping in some transitions
- Made blending more effective when using 0.5s blend windows

Limitation:
- Sequencer adjustments can only partially compensate for poor mocap
- Extreme pose differences still produce visible snapping

------------------------------------------------------------
14. DEDICATED CRITICAL EVALUATION (INCLUDING SHORTCOMINGS AND RESOLUTION ATTEMPTS)
------------------------------------------------------------
This section evaluates the system in terms of technical execution, design decisions, and explicitly documents shortcomings encountered during development, including how they were resolved or why they were ultimately accepted.

Animation Concatenation:
- The three-stage pipeline (Prep → Throw → Result) provides clear structure and readability
- Chaining via Blend Out ensures continuity
- However, transitions between certain animations still produce visible snapping

Root Cause:
- Source mocap animations have inconsistent end/start poses
- Especially noticeable in extreme result animations (e.g. some win/lose poses)

Resolution Attempt:
- Blend timing increased (~0.5s)
- Sequencer used to trim and adjust keyframes

Outcome:
- Improved smoothness overall
- Some snapping remains unavoidable
- Issue classified as asset limitation rather than system flaw

------------------------------------------------------------
Smoothing and Transition Issues:
- Initial attempts using On Completed caused noticeable delays between animations

Resolution:
- Fully replaced On Completed with On Blend Out chaining

Result:
- Continuous animation flow achieved
- Minor jitter introduced due to overlap, but visually preferable to delay

Decision:
- Accepted overlap + slight jitter as best compromise

------------------------------------------------------------
Blend Space Design Limitations:
- Player uses 2D blend space (Speed + State)
- NPC uses simplified 1D blend space

Shortcoming:
- Using a State float instead of a full animation state machine reduces scalability

Resolution Attempt:
- Considered expanding into state machine

Decision:
- Rejected due to increased complexity and limited benefit for current scope
- Current approach retained for simplicity and control

------------------------------------------------------------
Motion Warping Issues:
- Initial implementation failed completely (no movement occurred)

Root Causes:
- Montages were being played on a child mesh
- AnimBP was not extracting root motion

Fixes:
- Switched montage playback to main Mesh component
- Set AnimBP to "Root Motion from Montages Only"

Outcome:
- Motion warping fully functional

------------------------------------------------------------
Mirroring System Limitations:
- Mirroring only applied to selected animations

Shortcoming:
- Not generalized across entire animation set

Reason:
- Not all animations produced acceptable results when mirrored

Decision:
- Use mirroring selectively rather than universally

------------------------------------------------------------
NPC Behaviour Limitations:
- LookAt system implemented via continuous rotation toward player

Shortcoming:
- Behaviour is simplistic (no anticipation, delay, or variation)

Decision:
- Accepted as sufficient for assignment scope

------------------------------------------------------------
UI and Gameplay Scope Limitations:
- Retry system was considered but not implemented

Reason:
- Not part of assignment requirements
- Would introduce additional state complexity

Decision:
- Excluded to maintain system stability

------------------------------------------------------------
Sequencer Limitations:
- Used to clean up mocap inconsistencies

Shortcoming:
- Cannot fully correct extreme pose mismatches

Outcome:
- Improved transitions in some cases
- Did not eliminate snapping in all cases

------------------------------------------------------------
Overall Evaluation:
- The system successfully meets all technical requirements
- Major systems (animation pipeline, warping, UI, blending) are fully functional
- Remaining issues are primarily due to source animation quality

------------------------------------------------------------
15. EVALUATION (RUBRIC-ALIGNED SUMMARY)
------------------------------------------------------------
Interaction + UI:
- Fully implemented with button-based selection system

Animation Concatenation:
- Multi-stage montage pipeline (Prep → Throw → Result)

Smoothing:
- Achieved via blend timing and Blend Out chaining

Blend Spaces:
- Player uses 2D blend space (Speed + State)
- NPC uses 1D blend space (Idle → Ready)

Keyframe Editing:
- Applied using Sequencer

Warping:
- Fully implemented using motion warping

Alignment:
- Achieved dynamically using warping and LookAt

------------------------------------------------------------
16. CONCLUSION
------------------------------------------------------------
The final system delivers a complete and technically robust two-character interaction experience. It demonstrates strong integration of Unreal Engine animation systems while handling real-world limitations such as imperfect mocap data.

Key shortcomings were identified, tested, and either resolved or consciously accepted based on technical constraints and project scope.

------------------------------------------------------------
END OF DOCUMENT
------------------------------------------------------------