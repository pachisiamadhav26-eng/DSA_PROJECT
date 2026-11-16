# Analyst experiment report

Author: Sumit. Status: code and predictions prepared; formal experiments will run later. Component compatibility checks have passed. Add measured findings from completed result directories and profiler exports. Earlier 100-player outputs remain practice evidence.

## Predictions

The revised pre-measurement model is `docs/derivations/D6.md`, dated 17 November 2026. AVL rank/select and split have logarithmic cost in n. Fenwick rank/select have logarithmic cost in U, and split rebuilds arrays in O(U). Supplied-pivot ordered AVL join depends on the input height gap. Public AVL rejoin includes searches and pivot extraction. Public Fenwick merge includes sorting all IDs, adding an N log N term. Match the model to the source versions recorded by each formal experiment.

## Reproducibility

Cite result metadata, raw CSVs and plotting commands. State processor, cache sizes, compiler/options, seeds, coordinate policy, query/update mix, repetitions and timing boundaries. Explain always-on counters. Complete missing machine information before submission.

## Rank and select

Include both coordinate-policy charts and their spread. Compare growth with O(log n) and O(log U). Explain that batch means include loop/checksum work while excluding application lookup, generation, updates and printing.

## Split and join

Include split/rejoin and supplied-pivot join results. State input heights, tie/cutoff rules and validation/pivot work included in the public APIs. Distinguish a wrapper's measured cost from the unchecked join height-gap bound.

## Counters and adversarial experiments

Identify generator commands, seeds and trace hashes. Present available rotation, bucket-shift and join-visit evidence. State which actual backend versions Om's tests validated. Keep per-call trace timings distinguished from batched timings.

## Cache evidence

State tool/event, recording settings, workload, source commits and machine. Cite raw traces and exported analysis, check lost samples and filter to the benchmark PID/phase. Distinguish samples from exact misses. Support cache explanations with the evidence actually captured.

## Comparison and limitations

Explain whether measurements follow D6 and what supported factors explain gaps. Consider validation, allocation, tied IDs and memory access. Label untested explanations as hypotheses. Include the scope of correctness checks and remaining measurement limitations.
