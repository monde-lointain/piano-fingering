#!/usr/bin/env python3
"""
Piano Fingering Optimal Score Calculator

Calculates the exact minimum difficulty scores for the Golden Set test pieces
using dynamic programming to find optimal fingerings.

This program implements the objective function from the piano fingering problem
specification and uses DP to compute provably optimal scores for regression testing.
"""

import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from typing import List, Dict, Tuple, Optional, Set
from itertools import permutations, product
from pathlib import Path
import json
import sys
from functools import lru_cache

# =============================================================================
# Data Structures
# =============================================================================

@dataclass
class Note:
    """Represents a single note."""
    pitch: int          # Modified keyboard distance (0-14 per octave + octave*14)
    octave: int
    duration: int
    staff: int          # 1 = Right Hand, 2 = Left Hand
    voice: int = 1
    is_rest: bool = False
    
    @property
    def absolute_pitch(self) -> int:
        """Get absolute pitch using modified keyboard system."""
        return self.octave * 14 + self.pitch

@dataclass
class Slice:
    """A vertical time segment containing simultaneously played notes."""
    notes: List[Note]
    
    @property
    def is_monophonic(self) -> bool:
        return len(self.notes) == 1
    
    @property
    def pitches(self) -> Tuple[int, ...]:
        return tuple(sorted(n.absolute_pitch for n in self.notes))

@dataclass
class Hand:
    """Represents one hand's sequence of slices."""
    slices: List[Slice]
    is_right: bool
    
    def __len__(self):
        return len(self.slices)

# =============================================================================
# Distance Matrix (Medium Hand - Default)
# =============================================================================

# Distance matrix for right hand finger pairs
# Format: (finger1, finger2) -> (MinPrac, MinComf, MinRel, MaxRel, MaxComf, MaxPrac)
DISTANCE_MATRIX_RIGHT = {
    (1, 2): (-8, -6, 1, 5, 8, 10),
    (1, 3): (-7, -5, 3, 9, 12, 14),
    (1, 4): (-5, -3, 5, 11, 13, 15),
    (1, 5): (-2, 0, 7, 12, 14, 16),
    (2, 3): (1, 1, 1, 2, 5, 7),
    (2, 4): (1, 1, 3, 4, 6, 8),
    (2, 5): (2, 2, 5, 6, 10, 12),
    (3, 4): (1, 1, 1, 2, 2, 4),
    (3, 5): (1, 1, 3, 4, 6, 8),
    (4, 5): (1, 1, 1, 2, 4, 6),
}

def get_distance_limits(f1: int, f2: int, is_right: bool) -> Tuple[int, int, int, int, int, int]:
    """
    Get distance limits for a finger pair.
    Returns (MinPrac, MinComf, MinRel, MaxRel, MaxComf, MaxPrac).
    
    For left hand, swap finger order and negate values appropriately.
    """
    if f1 == f2:
        return (0, 0, 0, 0, 0, 0)
    
    # Normalize to canonical order (smaller finger first)
    if f1 > f2:
        f1, f2 = f2, f1
        swapped = True
    else:
        swapped = False
    
    limits = DISTANCE_MATRIX_RIGHT.get((f1, f2))
    if limits is None:
        # Fallback for any missing pairs
        return (-20, -15, -5, 5, 15, 20)
    
    min_prac, min_comf, min_rel, max_rel, max_comf, max_prac = limits
    
    if swapped:
        # Swap min/max and negate
        min_prac, max_prac = -max_prac, -min_prac
        min_comf, max_comf = -max_comf, -min_comf
        min_rel, max_rel = -max_rel, -min_rel
    
    if not is_right:
        # For left hand, swap finger pair order interpretation
        # This effectively negates all values
        min_prac, max_prac = -max_prac, -min_prac
        min_comf, max_comf = -max_comf, -min_comf
        min_rel, max_rel = -max_rel, -min_rel
    
    return (min_prac, min_comf, min_rel, max_rel, max_comf, max_prac)

# =============================================================================
# Rule Implementations
# =============================================================================

def is_black_key(pitch: int) -> bool:
    """Check if a pitch (in modified keyboard system) is a black key."""
    return (pitch % 14) % 2 == 1 and (pitch % 14) not in [5, 13]  # imaginary keys at 5, 13

def pitch_to_key_color(pitch: int) -> str:
    """Determine if pitch is white or black key."""
    p = pitch % 14
    # In modified system: 0=C, 1=C#, 2=D, 3=D#, 4=E, 5=imaginary, 6=F, 7=F#, 8=G, 9=G#, 10=A, 11=A#, 12=B, 13=imaginary
    # Black keys: 1, 3, 7, 9, 11
    black_keys = {1, 3, 7, 9, 11}
    return 'black' if p in black_keys else 'white'

def rule_1_2_13_score(distance: int, f1: int, f2: int, is_right: bool) -> float:
    """
    Rules 1, 2, 13 combined with cascading penalties (matching C++ reference).
    Penalties stack cumulatively for increasingly severe violations.

    Returns total penalty from all three rules.
    """
    min_prac, min_comf, min_rel, max_rel, max_comf, max_prac = get_distance_limits(f1, f2, is_right)

    score = 0.0

    # Check violations from outer to inner ranges (cascading)
    if distance < min_rel:
        score += (min_rel - distance)  # Rule 2: +1 per unit
        if distance < min_comf:
            score += 2 * (min_comf - distance)  # Rule 1: +2 per unit ADDITIONAL
            if distance < min_prac:
                score += 10 * (min_prac - distance)  # Rule 13: +10 per unit ADDITIONAL
    elif distance > max_rel:
        score += (distance - max_rel)  # Rule 2: +1 per unit
        if distance > max_comf:
            score += 2 * (distance - max_comf)  # Rule 1: +2 per unit ADDITIONAL
            if distance > max_prac:
                score += 10 * (distance - max_prac)  # Rule 13: +10 per unit ADDITIONAL

    return score

def rule_5_score(finger: int) -> float:
    """Rule 5: Penalty for fourth finger usage (+1)."""
    return 1.0 if finger == 4 else 0.0

def rule_6_score(f1: int, f2: int) -> float:
    """Rule 6: Penalty for 3-4 finger combination (+1)."""
    if {f1, f2} == {3, 4}:
        return 1.0
    return 0.0

def rule_7_score(f1: int, f2: int, p1: int, p2: int) -> float:
    """Rule 7: Penalty for finger 3 on white and finger 4 on black (+1)."""
    c1, c2 = pitch_to_key_color(p1), pitch_to_key_color(p2)
    
    if (f1 == 3 and c1 == 'white' and f2 == 4 and c2 == 'black') or \
       (f1 == 4 and c1 == 'black' and f2 == 3 and c2 == 'white'):
        return 1.0
    return 0.0

def rule_8_score(f_prev: Optional[int], f_curr: int, f_next: Optional[int],
                  p_prev: Optional[int], p_curr: int, p_next: Optional[int]) -> float:
    """Rule 8: Penalty for thumb on black key (+0.5, +1 for white before, +1 for white after)."""
    if f_curr != 1:
        return 0.0
    if pitch_to_key_color(p_curr) != 'black':
        return 0.0
    
    score = 0.5
    if f_prev is not None and f_prev != 1 and p_prev is not None:
        if pitch_to_key_color(p_prev) == 'white':
            score += 1.0
    if f_next is not None and f_next != 1 and p_next is not None:
        if pitch_to_key_color(p_next) == 'white':
            score += 1.0
    return score

def rule_9_score(f_prev: Optional[int], f_curr: int, f_next: Optional[int],
                  p_prev: Optional[int], p_curr: int, p_next: Optional[int]) -> float:
    """Rule 9: Penalty for fifth finger on black key (+1 for white before, +1 for white after)."""
    if f_curr != 5:
        return 0.0
    if pitch_to_key_color(p_curr) != 'black':
        return 0.0
    
    score = 0.0
    if f_prev is not None and f_prev != 5 and p_prev is not None:
        if pitch_to_key_color(p_prev) == 'white':
            score += 1.0
    if f_next is not None and f_next != 5 and p_next is not None:
        if pitch_to_key_color(p_next) == 'white':
            score += 1.0
    return score

def rule_10_score(f1: int, f2: int, p1: int, p2: int) -> float:
    """Rule 10: Penalty for thumb crossing on same level (white-white or black-black) (+1)."""
    if f1 != 1 and f2 != 1:
        return 0.0
    
    c1, c2 = pitch_to_key_color(p1), pitch_to_key_color(p2)
    if c1 == c2:
        return 1.0
    return 0.0

def rule_11_score(f1: int, f2: int, p1: int, p2: int) -> float:
    """Rule 11: Penalty for thumb on black crossed by finger on white (+2)."""
    c1, c2 = pitch_to_key_color(p1), pitch_to_key_color(p2)
    
    if (f1 == 1 and c1 == 'black' and f2 != 1 and c2 == 'white') or \
       (f2 == 1 and c2 == 'black' and f1 != 1 and c1 == 'white'):
        return 2.0
    return 0.0

def rule_12_score(f1: int, f2: int, f3: int, p1: int, p2: int, p3: int) -> float:
    """Rule 12: Penalty for same finger reuse with position change (middle pitch) (+1)."""
    if f1 != f3:
        return 0.0
    if p1 == p3:
        return 0.0
    
    # Check if p2 is the middle pitch
    pitches = [p1, p2, p3]
    sorted_pitches = sorted(pitches)
    if p2 == sorted_pitches[1]:  # p2 is middle
        return 1.0
    return 0.0

def rule_15_score(prev_pitches: Tuple[int, ...], curr_pitches: Tuple[int, ...],
                   prev_fingers: Tuple[int, ...], curr_fingers: Tuple[int, ...]) -> float:
    """Rule 15: Penalty for same pitch played by different finger (+1 per occurrence)."""
    score = 0.0
    prev_dict = dict(zip(prev_pitches, prev_fingers))
    curr_dict = dict(zip(curr_pitches, curr_fingers))
    
    for pitch in set(prev_pitches) & set(curr_pitches):
        if prev_dict[pitch] != curr_dict[pitch]:
            score += 1.0
    return score

# =============================================================================
# Triplet Rules (Rules 3, 4)
# =============================================================================

def rule_3_score(f1: int, f2: int, f3: int, p1: int, p2: int, p3: int, is_right: bool) -> float:
    """
    Rule 3: Hand position change penalty in triplet context.
    +1 if distance between first and third note is outside comfort range
    +1 additional if second pitch is middle, played by thumb, and outside practical range
    +1 additional if first and third have same pitch but different finger
    """
    score = 0.0
    distance_1_3 = p3 - p1
    
    min_prac, min_comf, min_rel, max_rel, max_comf, max_prac = get_distance_limits(f1, f3, is_right)
    
    # First condition: outside comfort range
    if distance_1_3 < min_comf or distance_1_3 > max_comf:
        score += 1.0
        
        # Second condition: middle pitch played by thumb, outside practical range
        pitches = sorted([p1, p2, p3])
        if p2 == pitches[1] and f2 == 1:  # p2 is middle and played by thumb
            if distance_1_3 < min_prac or distance_1_3 > max_prac:
                score += 1.0
    
    # Third condition: same pitch, different finger
    if p1 == p3 and f1 != f3:
        score += 1.0
    
    return score

def rule_4_score(f1: int, f3: int, p1: int, p3: int, is_right: bool) -> float:
    """Rule 4: Penalty per unit outside comfort range for triplet distance."""
    distance = p3 - p1
    min_prac, min_comf, min_rel, max_rel, max_comf, max_prac = get_distance_limits(f1, f3, is_right)
    
    score = 0.0
    if distance < min_comf:
        score += (min_comf - distance)
    elif distance > max_comf:
        score += (distance - max_comf)
    return score

# =============================================================================
# Chord Cost (Rule 14)
# =============================================================================

def rule_14_score(pitches: List[int], fingers: List[int], is_right: bool) -> float:
    """
    Rule 14: Apply rules 1, 2 (doubled), and 13 within a chord.
    Uses cascading penalties matching C++ reference implementation.
    """
    if len(pitches) <= 1:
        return 0.0

    score = 0.0
    # Check all pairs within the chord
    for i in range(len(pitches)):
        for j in range(i + 1, len(pitches)):
            p1, p2 = pitches[i], pitches[j]
            f1, f2 = fingers[i], fingers[j]
            distance = p2 - p1

            min_prac, min_comf, min_rel, max_rel, max_comf, max_prac = get_distance_limits(f1, f2, is_right)

            # Cascading penalties with doubled weights for Rule 2 and Rule 1
            if distance < min_rel:
                score += 2 * (min_rel - distance)  # Rule 2 doubled: +2 per unit
                if distance < min_comf:
                    score += 4 * (min_comf - distance)  # Rule 1 doubled: +4 per unit ADDITIONAL
                    if distance < min_prac:
                        score += 10 * (min_prac - distance)  # Rule 13: +10 per unit ADDITIONAL
            elif distance > max_rel:
                score += 2 * (distance - max_rel)  # Rule 2 doubled: +2 per unit
                if distance > max_comf:
                    score += 4 * (distance - max_comf)  # Rule 1 doubled: +4 per unit ADDITIONAL
                    if distance > max_prac:
                        score += 10 * (distance - max_prac)  # Rule 13: +10 per unit ADDITIONAL

    return score

# =============================================================================
# State Representation and Transition Costs
# =============================================================================

def generate_valid_fingerings(num_notes: int) -> List[Tuple[int, ...]]:
    """Generate all valid finger assignments for a chord of given size."""
    if num_notes == 0:
        return [()]
    if num_notes > 5:
        raise ValueError("Cannot have more than 5 notes per hand")
    
    # All permutations of num_notes fingers from {1,2,3,4,5}
    all_fingers = [1, 2, 3, 4, 5]
    return list(permutations(all_fingers, num_notes))

def compute_inter_slice_cost(
    prev_pitches: List[int], prev_fingers: Tuple[int, ...],
    curr_pitches: List[int], curr_fingers: Tuple[int, ...],
    is_right: bool, is_monophonic: bool
) -> float:
    """
    Compute transition cost between two consecutive slices.
    Applies rules 1, 2, 5-11, 13, 15 for monophonic passages.
    Applies rules 1, 2, 13, 15 for polyphonic passages.
    """
    score = 0.0

    # For each note transition
    for i, (p1, f1) in enumerate(zip(prev_pitches, prev_fingers)):
        for j, (p2, f2) in enumerate(zip(curr_pitches, curr_fingers)):
            distance = p2 - p1

            # Rules 1, 2, 13 with cascading penalties
            score += rule_1_2_13_score(distance, f1, f2, is_right)
    
    # Monophonic-specific rules (only if both slices have single notes)
    if is_monophonic and len(prev_pitches) == 1 and len(curr_pitches) == 1:
        p1, f1 = prev_pitches[0], prev_fingers[0]
        p2, f2 = curr_pitches[0], curr_fingers[0]
        
        # Rule 5: Fourth finger usage (on current note)
        score += rule_5_score(f2)
        
        # Rule 6: 3-4 combination
        score += rule_6_score(f1, f2)
        
        # Rule 7: Finger 3 on white, finger 4 on black
        score += rule_7_score(f1, f2, p1, p2)
        
        # Rule 10: Thumb crossing on same level
        score += rule_10_score(f1, f2, p1, p2)
        
        # Rule 11: Thumb on black crossed by finger on white
        score += rule_11_score(f1, f2, p1, p2)
    
    # Rule 15: Same pitch, different finger
    score += rule_15_score(
        tuple(prev_pitches), tuple(curr_pitches),
        prev_fingers, curr_fingers
    )
    
    return score

def compute_intra_slice_cost(pitches: List[int], fingers: Tuple[int, ...], is_right: bool) -> float:
    """Compute cost within a single chord (Rule 14)."""
    return rule_14_score(pitches, list(fingers), is_right)

# =============================================================================
# Dynamic Programming Solver
# =============================================================================

class FingeringSolver:
    """Solves piano fingering using exact dynamic programming."""
    
    def __init__(self, hand: Hand):
        self.hand = hand
        self.is_right = hand.is_right
        self.n_slices = len(hand.slices)
        
    def solve(self) -> Tuple[float, List[Tuple[int, ...]]]:
        """
        Find the optimal fingering using dynamic programming.
        Returns (minimum_score, list_of_fingerings).
        """
        if self.n_slices == 0:
            return 0.0, []
        
        # Get pitches for each slice
        slice_pitches = []
        for s in self.hand.slices:
            pitches = sorted([n.absolute_pitch for n in s.notes])
            slice_pitches.append(pitches)
        
        # Generate valid fingerings for each slice
        slice_fingerings = []
        for pitches in slice_pitches:
            fingerings = generate_valid_fingerings(len(pitches))
            slice_fingerings.append(fingerings)
        
        # DP: dp[state] = (min_cost, backtrack_info)
        # State is the fingering tuple
        
        # Initialize first slice
        dp_prev: Dict[Tuple[int, ...], Tuple[float, None]] = {}
        for fingers in slice_fingerings[0]:
            cost = compute_intra_slice_cost(slice_pitches[0], fingers, self.is_right)
            # Add rule 5 for first note in monophonic
            if len(slice_pitches[0]) == 1:
                cost += rule_5_score(fingers[0])
            dp_prev[fingers] = (cost, None)
        
        # Process remaining slices
        backtrack = [{}]  # backtrack[i][state] = previous_state
        
        for i in range(1, self.n_slices):
            dp_curr: Dict[Tuple[int, ...], Tuple[float, Tuple[int, ...]]] = {}
            backtrack.append({})
            
            is_mono = (len(slice_pitches[i-1]) == 1 and len(slice_pitches[i]) == 1)
            
            for curr_fingers in slice_fingerings[i]:
                best_cost = float('inf')
                best_prev = None
                
                for prev_fingers, (prev_cost, _) in dp_prev.items():
                    # Transition cost
                    trans_cost = compute_inter_slice_cost(
                        slice_pitches[i-1], prev_fingers,
                        slice_pitches[i], curr_fingers,
                        self.is_right, is_mono
                    )
                    
                    # Intra-slice cost
                    intra_cost = compute_intra_slice_cost(
                        slice_pitches[i], curr_fingers, self.is_right
                    )
                    
                    # Triplet costs (rules 3, 4, 12) if we have enough history
                    triplet_cost = 0.0
                    if i >= 2 and is_mono and len(slice_pitches[i-2]) == 1:
                        # We need to find the best path to prev_fingers
                        # This is tricky - we need to consider the state before prev
                        # For exact DP, we'd need to expand state space
                        pass  # Simplified: skip triplet rules for now
                    
                    total_cost = prev_cost + trans_cost + intra_cost + triplet_cost
                    
                    if total_cost < best_cost:
                        best_cost = total_cost
                        best_prev = prev_fingers
                
                dp_curr[curr_fingers] = (best_cost, best_prev)
                backtrack[i][curr_fingers] = best_prev
            
            dp_prev = dp_curr
        
        # Find best final state
        best_final_cost = float('inf')
        best_final_state = None
        for state, (cost, _) in dp_prev.items():
            if cost < best_final_cost:
                best_final_cost = cost
                best_final_state = state
        
        # Backtrack to get solution
        solution = []
        state = best_final_state
        for i in range(self.n_slices - 1, -1, -1):
            solution.append(state)
            if i > 0:
                state = backtrack[i][state]
        solution.reverse()
        
        return best_final_cost, solution

# =============================================================================
# Extended DP with Triplet Support
# =============================================================================

class FingeringSolverWithTriplets:
    """
    Solves piano fingering using DP with expanded state to handle triplet rules.
    State includes current and previous fingering for triplet rule evaluation.
    Tracks all optimal solutions with the same minimum score.
    """
    
    def __init__(self, hand: Hand):
        self.hand = hand
        self.is_right = hand.is_right
        self.n_slices = len(hand.slices)
        
    def solve(self) -> Tuple[float, List[List[Tuple[int, ...]]]]:
        """
        Find optimal fingering(s) with full triplet rule support.
        Returns (minimum_score, list_of_all_optimal_fingerings).
        """
        if self.n_slices == 0:
            return 0.0, [[]]
        
        # Get pitches for each slice
        slice_pitches = []
        for s in self.hand.slices:
            pitches = sorted([n.absolute_pitch for n in s.notes])
            slice_pitches.append(pitches)
        
        # Generate valid fingerings for each slice
        slice_fingerings = []
        for pitches in slice_pitches:
            fingerings = generate_valid_fingerings(len(pitches))
            slice_fingerings.append(fingerings)
        
        # Extended state: (prev_fingers, curr_fingers) for triplet rules
        # dp[(prev, curr)] = (cost, list_of_prev_states)
        
        if self.n_slices == 1:
            # Single slice - just intra cost
            best_cost = float('inf')
            best_fingers_list = []
            for fingers in slice_fingerings[0]:
                cost = compute_intra_slice_cost(slice_pitches[0], fingers, self.is_right)
                if len(slice_pitches[0]) == 1:
                    cost += rule_5_score(fingers[0])
                if cost < best_cost:
                    best_cost = cost
                    best_fingers_list = [fingers]
                elif cost == best_cost:
                    best_fingers_list.append(fingers)
            return best_cost, [[f] for f in best_fingers_list]
        
        # Initialize for slice 0 -> 1 transition
        # State is (fingers_at_0, fingers_at_1)
        dp_prev: Dict[Tuple[Tuple[int, ...], Tuple[int, ...]], Tuple[float, List]] = {}
        
        is_mono_01 = (len(slice_pitches[0]) == 1 and len(slice_pitches[1]) == 1)
        
        for f0 in slice_fingerings[0]:
            intra_0 = compute_intra_slice_cost(slice_pitches[0], f0, self.is_right)
            if len(slice_pitches[0]) == 1:
                intra_0 += rule_5_score(f0[0])
            
            for f1 in slice_fingerings[1]:
                trans_01 = compute_inter_slice_cost(
                    slice_pitches[0], f0,
                    slice_pitches[1], f1,
                    self.is_right, is_mono_01
                )
                intra_1 = compute_intra_slice_cost(slice_pitches[1], f1, self.is_right)
                
                cost = intra_0 + trans_01 + intra_1
                dp_prev[(f0, f1)] = (cost, [None])  # None means start state
        
        # Backtrack storage: backtrack[i][(f_prev, f_curr)] = list of (f_prev_prev, f_prev) states
        backtrack = [{}, {}]
        
        for i in range(2, self.n_slices):
            dp_curr: Dict[Tuple[Tuple[int, ...], Tuple[int, ...]], Tuple[float, List]] = {}
            backtrack.append({})
            
            is_mono = all(len(slice_pitches[j]) == 1 for j in range(i-1, i+1))
            is_mono_triplet = all(len(slice_pitches[j]) == 1 for j in range(i-2, i+1))
            
            for curr_fingers in slice_fingerings[i]:
                for prev_fingers in slice_fingerings[i-1]:
                    best_cost = float('inf')
                    best_prev_states = []
                    
                    for (f_prev_prev, f_prev), (prev_cost, _) in dp_prev.items():
                        if f_prev != prev_fingers:
                            continue
                        
                        # Transition cost
                        trans_cost = compute_inter_slice_cost(
                            slice_pitches[i-1], prev_fingers,
                            slice_pitches[i], curr_fingers,
                            self.is_right, is_mono
                        )
                        
                        # Intra-slice cost
                        intra_cost = compute_intra_slice_cost(
                            slice_pitches[i], curr_fingers, self.is_right
                        )
                        
                        # Triplet costs (rules 3, 4, 12)
                        triplet_cost = 0.0
                        if is_mono_triplet:
                            p1 = slice_pitches[i-2][0]
                            p2 = slice_pitches[i-1][0]
                            p3 = slice_pitches[i][0]
                            f1 = f_prev_prev[0]
                            f2 = prev_fingers[0]
                            f3 = curr_fingers[0]
                            
                            # Rules 3, 4
                            triplet_cost += rule_3_score(f1, f2, f3, p1, p2, p3, self.is_right)
                            triplet_cost += rule_4_score(f1, f3, p1, p3, self.is_right)
                            
                            # Rule 12
                            triplet_cost += rule_12_score(f1, f2, f3, p1, p2, p3)
                        
                        # Rules 8, 9 need context - apply here
                        if is_mono and len(slice_pitches[i-1]) == 1:
                            p_prev = slice_pitches[i-2][0] if i >= 2 and len(slice_pitches[i-2]) == 1 else None
                            f_prev_note = f_prev_prev[0] if i >= 2 and len(slice_pitches[i-2]) == 1 else None
                            p_curr = slice_pitches[i-1][0]
                            p_next = slice_pitches[i][0]
                            f_curr_note = prev_fingers[0]
                            f_next_note = curr_fingers[0]
                            
                            triplet_cost += rule_8_score(f_prev_note, f_curr_note, f_next_note,
                                                         p_prev, p_curr, p_next)
                            triplet_cost += rule_9_score(f_prev_note, f_curr_note, f_next_note,
                                                         p_prev, p_curr, p_next)
                        
                        total_cost = prev_cost + trans_cost + intra_cost + triplet_cost
                        
                        if total_cost < best_cost:
                            best_cost = total_cost
                            best_prev_states = [(f_prev_prev, f_prev)]
                        elif total_cost == best_cost:
                            best_prev_states.append((f_prev_prev, f_prev))
                    
                    if best_cost < float('inf'):
                        state = (prev_fingers, curr_fingers)
                        if state not in dp_curr:
                            dp_curr[state] = (best_cost, best_prev_states)
                            backtrack[i][state] = best_prev_states
                        elif dp_curr[state][0] > best_cost:
                            dp_curr[state] = (best_cost, best_prev_states)
                            backtrack[i][state] = best_prev_states
                        elif dp_curr[state][0] == best_cost:
                            # Same cost - add more paths
                            dp_curr[state][1].extend(best_prev_states)
                            backtrack[i][state].extend(best_prev_states)
            
            dp_prev = dp_curr
        
        # Find best final state(s)
        best_final_cost = float('inf')
        best_final_states = []
        for state, (cost, _) in dp_prev.items():
            if cost < best_final_cost:
                best_final_cost = cost
                best_final_states = [state]
            elif cost == best_final_cost:
                best_final_states.append(state)
        
        # Backtrack to get all solutions
        all_solutions = []
        
        def backtrack_all(slice_idx: int, state: Tuple[Tuple[int, ...], Tuple[int, ...]], path: List[Tuple[int, ...]]):
            if slice_idx == 1:
                # At the beginning - state is (f0, f1)
                full_path = [state[0], state[1]] + path
                all_solutions.append(full_path)
                return
            
            prev_states = backtrack[slice_idx].get(state, [])
            for prev_state in prev_states:
                if prev_state is None:
                    continue
                backtrack_all(slice_idx - 1, prev_state, [state[1]] + path)
        
        for final_state in best_final_states:
            backtrack_all(self.n_slices - 1, final_state, [])
        
        # Remove duplicates
        unique_solutions = []
        seen = set()
        for sol in all_solutions:
            key = tuple(sol)
            if key not in seen:
                seen.add(key)
                unique_solutions.append(sol)
        
        return best_final_cost, unique_solutions

# =============================================================================
# MusicXML Parser
# =============================================================================

def parse_musicxml(filepath: str) -> Tuple[Hand, Hand]:
    """
    Parse a MusicXML file and return left and right hand sequences.
    """
    tree = ET.parse(filepath)
    root = tree.getroot()
    
    # Handle namespace if present
    ns = ''
    if root.tag.startswith('{'):
        ns = root.tag.split('}')[0] + '}'
    
    right_notes = []
    left_notes = []
    
    # Find all parts
    for part in root.findall(f'.//{ns}part'):
        for measure in part.findall(f'{ns}measure'):
            # Track timing within measure
            current_time = 0
            
            for elem in measure:
                if elem.tag == f'{ns}note' or elem.tag == 'note':
                    # Check if it's a rest
                    rest = elem.find(f'{ns}rest')
                    if rest is None:
                        rest = elem.find('rest')
                    if rest is not None:
                        # Skip rests but track duration
                        dur = elem.find(f'{ns}duration')
                        if dur is None:
                            dur = elem.find('duration')
                        if dur is not None:
                            current_time += int(dur.text)
                        continue
                    
                    # Get pitch
                    pitch_elem = elem.find(f'{ns}pitch')
                    if pitch_elem is None:
                        pitch_elem = elem.find('pitch')
                    if pitch_elem is None:
                        continue
                    
                    step = pitch_elem.find(f'{ns}step')
                    if step is None:
                        step = pitch_elem.find('step')
                    octave = pitch_elem.find(f'{ns}octave')
                    if octave is None:
                        octave = pitch_elem.find('octave')
                    alter = pitch_elem.find(f'{ns}alter')
                    if alter is None:
                        alter = pitch_elem.find('alter')
                    
                    if step is None or octave is None:
                        continue
                    
                    # Convert to modified keyboard system
                    step_val = step.text.upper()
                    octave_val = int(octave.text)
                    alter_val = int(alter.text) if alter is not None else 0
                    
                    # Map step to base pitch in modified system
                    # C=0, D=2, E=4, F=6, G=8, A=10, B=12
                    step_map = {'C': 0, 'D': 2, 'E': 4, 'F': 6, 'G': 8, 'A': 10, 'B': 12}
                    pitch = step_map.get(step_val, 0) + alter_val

                    # Handle enharmonic equivalents for imaginary key positions
                    if pitch == 5:  # E# or Fb
                        pitch = 6 if alter_val > 0 else 4  # E# → F, Fb → E
                    elif pitch == 13:  # B#
                        pitch = 0
                        octave_val += 1  # B# → C of next octave
                    elif pitch == -1:  # Cb
                        pitch = 12
                        octave_val -= 1  # Cb → B of previous octave
                    
                    # Get staff assignment
                    staff_elem = elem.find(f'{ns}staff')
                    if staff_elem is None:
                        staff_elem = elem.find('staff')
                    staff = int(staff_elem.text) if staff_elem is not None else 1
                    
                    # Get duration
                    dur_elem = elem.find(f'{ns}duration')
                    if dur_elem is None:
                        dur_elem = elem.find('duration')
                    duration = int(dur_elem.text) if dur_elem is not None else 1
                    
                    # Get voice
                    voice_elem = elem.find(f'{ns}voice')
                    if voice_elem is None:
                        voice_elem = elem.find('voice')
                    voice = int(voice_elem.text) if voice_elem is not None else 1
                    
                    # Check if chord (simultaneous with previous note)
                    chord = elem.find(f'{ns}chord')
                    if chord is None:
                        chord = elem.find('chord')
                    is_chord = chord is not None
                    
                    note = Note(
                        pitch=pitch,
                        octave=octave_val,
                        duration=duration,
                        staff=staff,
                        voice=voice
                    )
                    
                    if staff == 1:
                        right_notes.append((current_time, note, is_chord))
                    else:
                        left_notes.append((current_time, note, is_chord))
                    
                    if not is_chord:
                        current_time += duration
    
    # Group notes into slices by time
    def notes_to_slices(notes_with_time: List[Tuple[int, Note, bool]]) -> List[Slice]:
        if not notes_with_time:
            return []
        
        slices = []
        current_slice_notes = []
        current_time = None
        
        for time, note, is_chord in notes_with_time:
            if is_chord and current_slice_notes:
                current_slice_notes.append(note)
            else:
                if current_slice_notes:
                    slices.append(Slice(notes=current_slice_notes))
                current_slice_notes = [note]
                current_time = time
        
        if current_slice_notes:
            slices.append(Slice(notes=current_slice_notes))
        
        return slices
    
    right_slices = notes_to_slices(right_notes)
    left_slices = notes_to_slices(left_notes)
    
    return Hand(slices=right_slices, is_right=True), Hand(slices=left_slices, is_right=False)

# =============================================================================
# Score Calculator
# =============================================================================

def pitch_to_note_name(absolute_pitch: int) -> str:
    """Convert absolute pitch back to note name (e.g., C4, F#5)."""
    octave = absolute_pitch // 14
    pitch_in_octave = absolute_pitch % 14
    
    # Map from modified keyboard system back to note names
    # 0=C, 1=C#, 2=D, 3=D#, 4=E, 5=imaginary, 6=F, 7=F#, 8=G, 9=G#, 10=A, 11=A#, 12=B, 13=imaginary
    pitch_names = ['C', 'C#', 'D', 'D#', 'E', '?', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B', '?']
    
    return f"{pitch_names[pitch_in_octave]}{octave}"

def format_fingering(hand: Hand, fingerings: List[Tuple[int, ...]]) -> List[Dict]:
    """Format fingering results with note names and finger numbers."""
    result = []
    for i, (slice_obj, fingers) in enumerate(zip(hand.slices, fingerings)):
        pitches = sorted([n.absolute_pitch for n in slice_obj.notes])
        note_names = [pitch_to_note_name(p) for p in pitches]
        
        if len(pitches) == 1:
            result.append({
                'position': i + 1,
                'notes': note_names[0],
                'fingers': fingers[0]
            })
        else:
            result.append({
                'position': i + 1,
                'notes': note_names,
                'fingers': list(fingers)
            })
    return result

def format_fingering_compact(hand: Hand, fingerings: List[Tuple[int, ...]]) -> str:
    """Format fingering as a compact string: note(finger) note(finger) ..."""
    parts = []
    for slice_obj, fingers in zip(hand.slices, fingerings):
        pitches = sorted([n.absolute_pitch for n in slice_obj.notes])
        note_names = [pitch_to_note_name(p) for p in pitches]
        
        if len(pitches) == 1:
            parts.append(f"{note_names[0]}({fingers[0]})")
        else:
            chord_parts = [f"{n}({f})" for n, f in zip(note_names, fingers)]
            parts.append("[" + " ".join(chord_parts) + "]")
    return " ".join(parts)

def format_all_fingerings(hand: Hand, all_fingerings: List[List[Tuple[int, ...]]]) -> List[Dict]:
    """Format all optimal fingering solutions."""
    return [format_fingering(hand, f) for f in all_fingerings]

def format_all_fingerings_compact(hand: Hand, all_fingerings: List[List[Tuple[int, ...]]]) -> List[str]:
    """Format all optimal fingerings as compact strings."""
    return [format_fingering_compact(hand, f) for f in all_fingerings]

def calculate_optimal_score(filepath: str, verbose: bool = False) -> Dict:
    """
    Calculate the optimal fingering score for a MusicXML file.
    Returns a dictionary with scores for each hand and total.
    Includes all optimal fingerings when there are ties.
    """
    try:
        right_hand, left_hand = parse_musicxml(filepath)
    except Exception as e:
        return {
            'error': str(e),
            'filepath': filepath
        }
    
    results = {
        'filepath': filepath,
        'right_hand': {
            'num_slices': len(right_hand),
            'score': 0.0,
            'num_solutions': 0,
            'fingerings': [],
            'fingerings_compact': []
        },
        'left_hand': {
            'num_slices': len(left_hand),
            'score': 0.0,
            'num_solutions': 0,
            'fingerings': [],
            'fingerings_compact': []
        },
        'total_score': 0.0
    }
    
    # Solve right hand
    if len(right_hand) > 0:
        solver = FingeringSolverWithTriplets(right_hand)
        score, all_fingerings = solver.solve()
        results['right_hand']['score'] = score
        results['right_hand']['num_solutions'] = len(all_fingerings)
        results['right_hand']['fingerings'] = format_all_fingerings(right_hand, all_fingerings)
        results['right_hand']['fingerings_compact'] = format_all_fingerings_compact(right_hand, all_fingerings)
    
    # Solve left hand
    if len(left_hand) > 0:
        solver = FingeringSolverWithTriplets(left_hand)
        score, all_fingerings = solver.solve()
        results['left_hand']['score'] = score
        results['left_hand']['num_solutions'] = len(all_fingerings)
        results['left_hand']['fingerings'] = format_all_fingerings(left_hand, all_fingerings)
        results['left_hand']['fingerings_compact'] = format_all_fingerings_compact(left_hand, all_fingerings)
    
    results['total_score'] = results['right_hand']['score'] + results['left_hand']['score']
    
    return results

# =============================================================================
# Golden Set Test Runner
# =============================================================================

GOLDEN_SET = [
    ("c_major_scale.musicxml", "C Major Scale", 15),
    ("hanon_no1.musicxml", "Hanon Exercise No. 1", 64),
    ("bach_invention_1.musicxml", "J.S. Bach - Invention No. 1 in C Major", 350),
    ("fur_elise.musicxml", "Beethoven - Für Elise", 500),
    ("chopin_prelude_op28_no4.musicxml", "Chopin - Prelude Op. 28 No. 4", 200),
    ("burgmuller_arabesque.musicxml", "Burgmüller - Arabesque", 400),
    ("clementi_sonatina_op36_no1.musicxml", "Clementi - Sonatina Op. 36 No. 1, Mvt. 1", 800),
    ("debussy_reverie.musicxml", "Debussy - Rêverie", 300),
    ("joplin_entertainer.musicxml", "Joplin - The Entertainer", 600),
    ("prokofiev_march_op65_no10.musicxml", "Prokofiev - March Op. 65 No. 10", 400),
]

def run_golden_set_tests(test_dir: str = "test_pieces", verbose: bool = False) -> List[Dict]:
    """
    Run optimal score calculation on all Golden Set pieces.
    """
    results = []
    test_path = Path(test_dir)
    
    print("=" * 70)
    print("Golden Set Optimal Fingering Score Calculator")
    print("=" * 70)
    print()
    
    for filename, title, expected_notes in GOLDEN_SET:
        filepath = test_path / filename
        print(f"Processing: {title}")
        print(f"  File: {filepath}")
        
        if not filepath.exists():
            print(f"  WARNING: File not found, skipping...")
            results.append({
                'title': title,
                'filename': filename,
                'error': 'File not found',
                'expected_notes': expected_notes
            })
            print()
            continue
        
        result = calculate_optimal_score(str(filepath), verbose)
        result['title'] = title
        result['expected_notes'] = expected_notes
        results.append(result)
        
        if 'error' in result:
            print(f"  ERROR: {result['error']}")
        else:
            rh = result['right_hand']
            print(f"  Right Hand: {rh['num_slices']} slices, Score: {rh['score']:.1f}, Solutions: {rh['num_solutions']}")
            if rh['fingerings_compact']:
                for i, fingering in enumerate(rh['fingerings_compact'][:3], 1):  # Show up to 3
                    if len(fingering) > 55:
                        fingering = fingering[:52] + "..."
                    print(f"    [{i}] {fingering}")
                if rh['num_solutions'] > 3:
                    print(f"    ... and {rh['num_solutions'] - 3} more")
            
            lh = result['left_hand']
            print(f"  Left Hand:  {lh['num_slices']} slices, Score: {lh['score']:.1f}, Solutions: {lh['num_solutions']}")
            if lh['fingerings_compact']:
                for i, fingering in enumerate(lh['fingerings_compact'][:3], 1):  # Show up to 3
                    if len(fingering) > 55:
                        fingering = fingering[:52] + "..."
                    print(f"    [{i}] {fingering}")
                if lh['num_solutions'] > 3:
                    print(f"    ... and {lh['num_solutions'] - 3} more")
            
            print(f"  TOTAL SCORE: {result['total_score']:.1f}")
        print()
    
    # Summary table
    print("=" * 70)
    print("SUMMARY - Baseline Z Scores for Regression Testing")
    print("=" * 70)
    print(f"{'Piece':<40} {'Z Score':>10} {'RH Sols':>10} {'LH Sols':>10}")
    print("-" * 70)
    
    for r in results:
        if 'error' in r:
            print(f"{r['title']:<40} {'ERROR':>10} {'-':>10} {'-':>10}")
        else:
            print(f"{r['title']:<40} {r['total_score']:>10.1f} {r['right_hand']['num_solutions']:>10} {r['left_hand']['num_solutions']:>10}")
    
    print("=" * 70)
    
    return results

def generate_test_piece_simple(filepath: str, piece_type: str = "scale"):
    """
    Generate a simple test MusicXML file for testing.
    """
    if piece_type == "scale":
        # C Major Scale - 8 notes ascending
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE score-partwise PUBLIC "-//Recordare//DTD MusicXML 3.1 Partwise//EN" "http://www.musicxml.org/dtds/partwise.dtd">
<score-partwise version="3.1">
  <part-list>
    <score-part id="P1">
      <part-name>Piano</part-name>
    </score-part>
  </part-list>
  <part id="P1">
    <measure number="1">
      <attributes>
        <divisions>1</divisions>
        <key><fifths>0</fifths></key>
        <time><beats>4</beats><beat-type>4</beat-type></time>
        <staves>2</staves>
        <clef number="1"><sign>G</sign><line>2</line></clef>
        <clef number="2"><sign>F</sign><line>4</line></clef>
      </attributes>
      <note><pitch><step>C</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>D</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>E</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>F</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
    </measure>
    <measure number="2">
      <note><pitch><step>G</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>A</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>B</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>C</step><octave>5</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
    </measure>
  </part>
</score-partwise>'''
    elif piece_type == "chord":
        # Simple chord progression
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE score-partwise PUBLIC "-//Recordare//DTD MusicXML 3.1 Partwise//EN" "http://www.musicxml.org/dtds/partwise.dtd">
<score-partwise version="3.1">
  <part-list>
    <score-part id="P1">
      <part-name>Piano</part-name>
    </score-part>
  </part-list>
  <part id="P1">
    <measure number="1">
      <attributes>
        <divisions>1</divisions>
        <key><fifths>0</fifths></key>
        <time><beats>4</beats><beat-type>4</beat-type></time>
        <staves>2</staves>
      </attributes>
      <note><pitch><step>C</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><chord/><pitch><step>E</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><chord/><pitch><step>G</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>D</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><chord/><pitch><step>F</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><chord/><pitch><step>A</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
    </measure>
  </part>
</score-partwise>'''
    elif piece_type == "short":
        # Very short piece - 3 notes that might have multiple optimal fingerings
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE score-partwise PUBLIC "-//Recordare//DTD MusicXML 3.1 Partwise//EN" "http://www.musicxml.org/dtds/partwise.dtd">
<score-partwise version="3.1">
  <part-list>
    <score-part id="P1">
      <part-name>Piano</part-name>
    </score-part>
  </part-list>
  <part id="P1">
    <measure number="1">
      <attributes>
        <divisions>1</divisions>
        <key><fifths>0</fifths></key>
        <time><beats>4</beats><beat-type>4</beat-type></time>
        <staves>2</staves>
      </attributes>
      <note><pitch><step>C</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>D</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>E</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
    </measure>
  </part>
</score-partwise>'''
    elif piece_type == "repeated":
        # Repeated notes - likely to have multiple equal fingerings
        xml_content = '''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE score-partwise PUBLIC "-//Recordare//DTD MusicXML 3.1 Partwise//EN" "http://www.musicxml.org/dtds/partwise.dtd">
<score-partwise version="3.1">
  <part-list>
    <score-part id="P1">
      <part-name>Piano</part-name>
    </score-part>
  </part-list>
  <part id="P1">
    <measure number="1">
      <attributes>
        <divisions>1</divisions>
        <key><fifths>0</fifths></key>
        <time><beats>4</beats><beat-type>4</beat-type></time>
        <staves>2</staves>
      </attributes>
      <note><pitch><step>C</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>E</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>C</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
      <note><pitch><step>E</step><octave>4</octave></pitch><duration>1</duration><type>quarter</type><staff>1</staff></note>
    </measure>
  </part>
</score-partwise>'''
    else:
        raise ValueError(f"Unknown piece type: {piece_type}")
    
    with open(filepath, 'w') as f:
        f.write(xml_content)
    print(f"Generated test file: {filepath}")

# =============================================================================
# Main Entry Point
# =============================================================================

def main():
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Calculate optimal piano fingering scores for regression testing"
    )
    parser.add_argument(
        'input',
        nargs='?',
        help="Path to MusicXML file or 'golden-set' to run all Golden Set tests"
    )
    parser.add_argument(
        '--test-dir',
        default='test_pieces',
        help="Directory containing test MusicXML files (default: test_pieces)"
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help="Include fingering details in output"
    )
    parser.add_argument(
        '--generate-test',
        choices=['scale', 'chord', 'short', 'repeated'],
        help="Generate a simple test file"
    )
    parser.add_argument(
        '--output', '-o',
        help="Output file for generated test"
    )
    parser.add_argument(
        '--json',
        action='store_true',
        help="Output results as JSON"
    )
    
    args = parser.parse_args()
    
    if args.generate_test:
        output = args.output or f"test_{args.generate_test}.musicxml"
        generate_test_piece_simple(output, args.generate_test)
        return
    
    if args.input == 'golden-set' or args.input is None:
        results = run_golden_set_tests(args.test_dir, args.verbose)
        if args.json:
            print(json.dumps(results, indent=2))
    else:
        result = calculate_optimal_score(args.input, args.verbose)
        if args.json:
            print(json.dumps(result, indent=2))
        else:
            print(f"File: {args.input}")
            if 'error' in result:
                print(f"Error: {result['error']}")
            else:
                print(f"\n{'='*60}")
                print("RIGHT HAND")
                print(f"{'='*60}")
                print(f"Slices: {result['right_hand']['num_slices']}")
                print(f"Score: {result['right_hand']['score']:.1f}")
                print(f"Optimal Solutions: {result['right_hand']['num_solutions']}")
                if result['right_hand']['fingerings_compact']:
                    for i, fingering in enumerate(result['right_hand']['fingerings_compact'], 1):
                        print(f"  [{i}] {fingering}")
                else:
                    print("  (no notes)")
                
                print(f"\n{'='*60}")
                print("LEFT HAND")
                print(f"{'='*60}")
                print(f"Slices: {result['left_hand']['num_slices']}")
                print(f"Score: {result['left_hand']['score']:.1f}")
                print(f"Optimal Solutions: {result['left_hand']['num_solutions']}")
                if result['left_hand']['fingerings_compact']:
                    for i, fingering in enumerate(result['left_hand']['fingerings_compact'], 1):
                        print(f"  [{i}] {fingering}")
                else:
                    print("  (no notes)")
                
                print(f"\n{'='*60}")
                print(f"TOTAL SCORE: {result['total_score']:.1f}")
                print(f"{'='*60}")

if __name__ == '__main__':
    main()
