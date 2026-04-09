# Viva Cheat Sheet (TCP ML Models)

## Files you changed in base ns-3
- `src/internet/model/tcp-westwood-ml.h/.cc`
- `src/internet/model/tcp-westwood-experts.h/.cc`
- `src/internet/model/tcp-westwood-kalman.h/.cc`
- `src/internet/model/tcp-westwood-experts-adaptive.h/.cc`
- Build wiring: `src/internet/CMakeLists.txt`

## Core extension points
- `PktsAcked(...)` → model update (RTT prediction)
- `IncreaseWindow(...)` → cwnd behavior
- `Fork()` → preserve algorithm per new socket

## Model one-liners
- **ML (EWMA):** light, fast, low-complexity RTT smoother.
- **Experts:** paper baseline, weighted ensemble of RTT experts.
- **Kalman:** uncertainty-aware estimator with dynamic gain.
- **Adaptive Experts:** modified paper model (adaptive η/α + momentum + revival).

## Common congestion policy
If `PredictedRTT/BaseRTT > RatioThreshold`, reduce `cwnd` by 10%, else normal additive increase.

## Why this is fair comparison
Same scenario setup + same cwnd trigger style; only predictor internals differ.

## Why `Fork()` is very important
Without proper `Fork()`, ns-3 may clone sockets using parent/default behavior and invalidate experiment conclusions.

## Important final observation
In short-run aggregate data, Adaptive Experts ≈ Experts; this indicates adaptation did not strongly diverge under those settings, not that implementation failed.

## What to say if asked “what is your main contribution?”
“I implemented and integrated multiple RTT-prediction TCP variants in ns-3 at model level, including a modified version of the paper’s Experts algorithm, and evaluated them with reproducible scenario automation, metrics, and plots.”