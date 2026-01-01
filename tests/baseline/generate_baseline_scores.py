#!/usr/bin/env python3
"""
Generate baseline scores for all MusicXML files in tests/baseline/.
Outputs a JSON file with total scores for each piece.
"""

import json
import sys
from pathlib import Path

# Import the scorer module
sys.path.insert(0, str(Path(__file__).parent))
from piano_fingering_scorer import calculate_optimal_score


def main():
    baseline_dir = Path(__file__).parent
    musicxml_files = sorted(baseline_dir.glob("*.musicxml"))

    if not musicxml_files:
        print("No MusicXML files found in baseline directory", file=sys.stderr)
        return 1

    baseline_scores = {}

    for filepath in musicxml_files:
        print(f"Processing {filepath.name}...", file=sys.stderr)
        result = calculate_optimal_score(str(filepath))

        if 'error' in result:
            print(f"  ERROR: {result['error']}", file=sys.stderr)
            baseline_scores[filepath.name] = {"error": result['error']}
        else:
            total_score = result['total_score']
            baseline_scores[filepath.name] = total_score
            print(f"  Total score: {total_score}", file=sys.stderr)

    # Output JSON to stdout
    print(json.dumps(baseline_scores, indent=2))

    return 0


if __name__ == '__main__':
    sys.exit(main())
