# Deep Technical Explanation of Custom TCP ML Files

This is an expanded, implementation-level guide for the 8 TCP files you asked about.

It explains:
- exactly what each file contributes,
- how control flows in ns-3 from ACK arrival to cwnd change,
- why each member variable exists,
- what each TypeId attribute means experimentally,
- algorithm equations and implementation mapping,
- edge cases, safeguards, and tuning advice,
- how to explain all of this in viva.

---

## 0) Big Picture: Where These Classes Sit in ns-3 TCP

All four custom classes inherit from `TcpWestwoodPlus`.

Runtime call flow (simplified):
1. TCP receives ACK.
2. RTT sample is computed by socket internals.
3. `PktsAcked(...)` of active congestion-control class is called.
4. Your model updates RTT predictor state.
5. Later, in congestion-avoidance step, `IncreaseWindow(...)` is called.
6. Your model decides whether to reduce or normally increase `cwnd`.

Common control policy across models:
- Define `baseRTT` = minimum RTT seen so far.
- Define `predictedRTT` from model.
- If `predictedRTT / baseRTT > RatioThreshold` ⇒ reduce `cwnd` by 10%.
- Else use normal additive increase in congestion avoidance.

This gives fair comparison: only predictor differs, response template is mostly same.

---

## 1) `src/internet/model/tcp-westwood-ml.h`

## Role in project
Header declaration for the EWMA-based lightweight ML model.

## Class contract
`TcpWestwoodMl : public TcpWestwoodPlus`

Overridden hooks:
- `PktsAcked(...)`: update model each RTT sample.
- `IncreaseWindow(...)`: apply RTT-aware cwnd logic.
- `Fork()`: clone same algorithm for child sockets.

## Member variables (deep meaning)
- `m_predictedRtt` (`Time`): current EWMA estimate; your model output.
- `m_baseRtt` (`Time`): best-so-far RTT floor, proxy for uncongested path delay.
- `m_alpha` (`double`): update gain; higher = faster but noisier.
- `m_ratioThreshold` (`double`): sensitivity knob for congestion reaction.

## Why this header matters
It defines the minimum state required to convert a classical TCP into RTT-aware control without adding heavy math/state.

---

## 2) `src/internet/model/tcp-westwood-ml.cc`

## Implementation responsibilities
1. Register class with ns-3 TypeId system.
2. Expose runtime-tunable attributes.
3. Implement ACK-time EWMA update.
4. Implement cwnd increase/reduction logic.

## TypeId attributes and interpretation
- `Alpha` (default `0.25`)
  - Low (e.g., 0.1): smoother, slower adaptation.
  - High (e.g., 0.5): responsive, jitter-sensitive.
- `RatioThreshold` (default `1.25`)
  - Lower threshold: more defensive congestion reaction.
  - Higher threshold: more aggressive throughput pursuit.

## Constructor defaults
- Predicted/base RTT initialized to zero (uninitialized state).
- Uses sane baseline values (`alpha=0.25`, threshold `1.25`).

## `PktsAcked(...)` detailed flow
Pseudo-flow:
1. If RTT sample is zero ⇒ return.
2. Update `m_baseRtt = min(m_baseRtt, rtt)`.
3. If first valid sample: `pred=rtt`.
4. Else EWMA:
   $$
   \hat r_t = \alpha r_t + (1-\alpha)\hat r_{t-1}
   $$
5. Call `TcpWestwoodPlus::PktsAcked(...)`.

Why parent call is important:
- Keeps original Westwood+ bandwidth estimation path active.
- Ensures your predictor augments rather than breaks parent behavior.

## `IncreaseWindow(...)` detailed flow
1. Slow start: `cwnd += MSS`.
2. Congestion avoidance:
   - compute `ratio = predicted/base`.
   - if `ratio > threshold`: `cwnd = 0.9*cwnd`.
   - else additive increase:
     $$
     cwnd \leftarrow cwnd + \frac{MSS^2}{cwnd}
     $$

## `Fork()` deep note
In ns-3, TCP operations objects are cloned per connection.
If `Fork()` is not properly overridden, runtime may silently use parent/default behavior in child sockets.

---

## 3) `src/internet/model/tcp-westwood-experts.h`

## Role in project
Declaration of paper-faithful Fixed-Share Experts predictor.

## Major design elements
- Ensemble of `N` fixed RTT experts.
- Online weight adaptation from prediction losses.
- Redistribution step to prevent single-expert lock-in.

## Parameter members and semantics
- `m_nExperts`: number of experts (resolution of hypothesis space).
- `m_eta`: learning rate in exponential update.
- `m_shareAlpha`: weight-sharing strength.
- `m_rttMin/m_rttMax`: span of expert RTT hypotheses.

## State members and semantics
- `m_expertValues`: static hypothesis values (`x_i`).
- `m_weights`: dynamic credibility weights (`w_i`).
- `m_initialised`: lazy-init flag (first valid RTT triggers setup).
- `m_predictedRttMs`: weighted aggregate output.
- `m_baseRttMs`: floor RTT for congestion ratio.
- `m_trialCount`: processed ACK samples.

## Stability constants
- `RESCALE_LOWER`, `RESCALE_INTERVAL` protect numerical stability as exponentials can underflow quickly.

---

## 4) `src/internet/model/tcp-westwood-experts.cc`

## Algorithm implemented
Per ACK trial `t`:

1) Prediction:
$$
\hat y_t = \frac{\sum_i w_{t,i}x_i}{\sum_i w_{t,i}}
$$

2) Asymmetric loss:
$$
L_{i,t}=
\begin{cases}
(x_i-y_t)^2,&x_i\ge y_t\\
2y_t,&x_i<y_t
\end{cases}
$$

3) Exponential weight update:
$$
w'_{t,i}=w_{t,i}\exp(-\eta L_{i,t})
$$

4) Fixed-share redistribution:
$$
w_{t+1,i}=(1-\alpha)w'_{t,i}+\frac{\alpha}{N}\sum_j w'_{t,j}
$$

## Why asymmetric loss?
Undershooting RTT often implies risky optimism about network state; this can induce overly aggressive sending.
So underestimation is penalized strongly.

## `InitExperts()` details
Expert spacing:
$$
x_i = RTT_{min}+RTT_{max}\cdot 2^{(i/N)/4}
$$
This biases density to lower RTT region (typically more informative in normal operation).

## Numerical safeguards
- Clamp very tiny weights (`1e-300`).
- Periodic `RescaleWeights()` to avoid all weights collapsing to denormals/zeros.

## Complexity
Per ACK: `O(N)` for prediction + loss/update + sharing.
With `N=100`, this is still practical for ns-3 runs but heavier than EWMA/Kalman.

## Congestion behavior mapping
`IncreaseWindow(...)` uses predicted/base ratio trigger and additive increase fallback, same template as other models.

---

## 5) `src/internet/model/tcp-westwood-kalman.h`

## Role in project
Declaration for uncertainty-aware RTT tracking using scalar Kalman filtering.

## Parameter meanings
- `m_processNoise` (`Q`): expected RTT drift between ACKs.
- `m_measurementNoise` (`R`): expected sample jitter noise.
- `m_adaptiveNoise`: whether to adapt `R` online.
- `m_adaptAlpha`: smoothing of innovation power estimate.

## State meanings
- `m_stateEstimate` (`x̂`): current RTT estimate.
- `m_errorCovariance` (`P`): confidence (lower P = more certainty).
- `m_kalmanGain` (`K`): dynamic blend factor between model and measurement.
- `m_innovationEma`: tracks residual energy to tune `R`.

## Why this model matters
It separates “how noisy samples are” from “how fast system state moves”, which EWMA cannot explicitly represent.

---

## 6) `src/internet/model/tcp-westwood-kalman.cc`

## Kalman math mapping to code
Given measurement `z` (RTT in ms):

Predict:
$$
x_t^- = x_{t-1},\quad P_t^- = P_{t-1}+Q
$$

Innovation:
$$
\nu_t = z_t - x_t^-
$$

Gain:
$$
K_t = \frac{P_t^-}{P_t^-+R_t}
$$

Update:
$$
x_t = x_t^- + K_t\nu_t,\quad P_t=(1-K_t)P_t^-
$$

## Adaptive measurement noise
When enabled, code tracks EMA of `ν²` and estimates `R` from that energy.
Effect:
- Noisy period ⇒ larger `R` ⇒ smaller `K` ⇒ smoother filter.
- Clean period ⇒ smaller `R` ⇒ larger `K` ⇒ more responsive tracking.

## Initialization strategy
First sample initializes state directly (`x̂=z`) and sets initial covariance from `R`.

## Edge handling
- Ignores zero RTT.
- Clamps estimate to minimum positive value.

## Congestion window effect
Same ratio-trigger policy as other models, keeping comparison fair.

---

## 7) `src/internet/model/tcp-westwood-experts-adaptive.h`

## Role in project
Declaration for your modified-paper model (`TcpWestwoodExpertsAdaptive`).

## New adaptation parameters
- `m_etaBase` and `m_alphaBase`: baseline paper values.
- `m_adaptBeta`: scales how much η grows with innovation.
- `m_adaptGamma`: scales how much α grows with innovation.
- `m_momentum`: temporal smoothing of predicted RTT.
- `m_revivalWindow` and `m_revivalThreshold`: anti-dead-expert mechanism.

## Extra state beyond baseline Experts
- `m_prevPredictedRttMs`: previous output for momentum blend.
- `m_meanRttMs`: normalization reference for innovation.
- `m_meanAlpha`: smoothing coefficient for mean RTT tracker.

## Intent of these additions
Baseline experts can become brittle under fast regime changes.
Adaptive version attempts faster re-learning without discarding the paper’s core structure.

---

## 8) `src/internet/model/tcp-westwood-experts-adaptive.cc`

## Attribute surface (what can be tuned in experiments)
Paper-like:
- `NumExperts`, `LearningRateBase`, `ShareAlphaBase`, `RttMin`, `RttMax`.

Adaptive:
- `AdaptBeta`, `AdaptGamma`, `Momentum`, `RevivalWindow`, `RevivalThreshold`.

Congestion:
- `RatioThreshold`.

## Four implemented modifications

### 8.1 Adaptive learning rate
$$
\eta_t = \eta_{base}(1+\beta\cdot \text{normInnovation})
$$

Meaning:
- When model error rises, learning speeds up.
- When stable, update aggressiveness drops.

### 8.2 Adaptive sharing rate
$$
\alpha_t = \min\left(\alpha_{base}(1+\gamma\cdot \text{normInnovation}),0.5\right)
$$

Meaning:
- Volatile phase: more exploration via stronger sharing.
- Stable phase: preserve concentrated expert confidence.

### 8.3 Momentum prediction
$$
\hat y_t=(1-\mu)\cdot \hat y_t^{raw}+\mu\cdot \hat y_{t-1}
$$

Meaning:
- Reduces high-frequency oscillation in prediction.
- Helps avoid overreacting cwnd to noise spikes.

### 8.4 Expert revival
Every `W` trials, experts below threshold are reset toward uniform mass and then renormalized.

Meaning:
- Avoids permanent elimination of experts that may become relevant again after network regime changes.

## Shared baseline logic retained
- Same asymmetric loss form.
- Same exponential update style.
- Same fixed-share mechanism (but with adaptive α).
- Same RTT-ratio cwnd policy.

## Important practical note from your runs
In short-mode aggregate results, adaptive and baseline experts are numerically identical.
This means adaptation did not activate strongly enough to alter aggregate outcome under those settings.

---

## 9) Method-Level Comparison Across All Four Models

| Aspect | EWMA (`Ml`) | Experts | Kalman | Adaptive Experts |
|---|---|---|---|---|
| State size | tiny | large (`N` weights) | medium | large+ |
| ACK cost | O(1) | O(N) | O(1) | O(N) |
| Explicit uncertainty | no | no | yes (`P`) | no |
| Response to abrupt change | moderate | moderate | high (if Q/R tuned) | high (if β/γ tuned) |
| Numerical fragility | low | medium/high | low | medium/high |
| Interpretability | high | medium | medium | medium |

---

## 10) Common Implementation Pattern You Used Correctly

Every model file follows the correct ns-3 extension pattern:
1. Type registration with `NS_OBJECT_ENSURE_REGISTERED`.
2. Public tunables via `AddAttribute(...)`.
3. Predictor update in `PktsAcked(...)`.
4. Congestion reaction in `IncreaseWindow(...)`.
5. Proper cloning in `Fork()`.

This is exactly what your sir usually expects in a clean ns-3 model extension.

---

## 11) Where these models are selected in simulations

Model selection/activation is done in:
- `scratch/my-experts-simulation.cc`
- `scratch/my-bursty-simulation.cc`

The scripts set `Config::SetDefault("ns3::TcpL4Protocol::SocketType", ...)`
to one of:
- `ns3::TcpWestwoodExperts`
- `ns3::TcpWestwoodExpertsAdaptive`
- `ns3::TcpWestwoodKalman`
- `ns3::TcpWestwoodMl`
- `ns3::TcpWestwoodPlus`
- `ns3::TcpNewReno`

So these 8 source files are not isolated; they are fully wired into experiment pipeline.

---

## 12) Tuning Guide (Practical)

## EWMA (`Ml`)
- Start: `Alpha=0.25`, `RatioThreshold=1.25`.
- If too jumpy: lower alpha.
- If too sluggish: raise alpha.

## Experts
- `N=100` is good baseline.
- Large η can collapse weights fast.
- α too small: low exploration; α too large: noisy averaging.

## Kalman
- Increase `Q` for faster adaptation to real RTT drift.
- Increase `R` (or adaptive smoothing) to ignore noisy spikes.
- If unstable oscillation appears, inspect gain behavior in logs.

## Adaptive Experts
- Increase `AdaptBeta` if adaptation is too weak.
- Increase `AdaptGamma` if model gets stuck in narrow expert subset.
- Raise `Momentum` for stronger smoothing.
- Shorten `RevivalWindow` if experts die too early.

---

## 13) Edge Cases and Safeguards in your code

Implemented safeguards:
- Ignore zero RTT sample.
- Maintain base RTT as minimum.
- Clamp tiny expert weights.
- Periodic weight rescaling.
- Positive-clamp Kalman state.

Potential future hardening:
- Integer-safe cwnd scaling helper.
- Additional tracing hooks for adaptive η/α over time.
- Unit tests/regression traces for each model.

---

## 14) “What changed in base ns-3” (short statement for sir)

You changed base ns-3 by adding and wiring custom TCP congestion-control classes in `src/internet/model/`, each implementing RTT prediction in ACK path and RTT-aware cwnd logic in window-growth path.

Specifically, you introduced:
- a simple predictor (`TcpWestwoodMl`),
- a paper-faithful experts model (`TcpWestwoodExperts`),
- a novel uncertainty-aware model (`TcpWestwoodKalman`),
- and a modified paper model with adaptive mechanisms (`TcpWestwoodExpertsAdaptive`).

This is a model-level algorithmic extension, not just scripting.

---

## 15) Viva Q&A (Fast Prep)

### Q1: Why override `PktsAcked`?
Because RTT samples are naturally available at ACK time; this is where predictor state should update.

### Q2: Why override `IncreaseWindow`?
Because congestion reaction (increase/decrease behavior) must be directly controlled there.

### Q3: Why `Fork()` is mandatory?
Without it, child sockets may lose custom algorithm state/type and results become misleading.

### Q4: Why compare all models using same ratio-threshold policy?
To keep predictor as main variable and avoid unfair policy differences.

### Q5: Why did Adaptive Experts match Experts in your short results?
Adaptive triggers likely remained near base values under those conditions; longer/full runs or stronger parameter sweeps are needed to expose divergence.

---

## 16) One-Minute Summary for Presentation

“I extended ns-3 TCP Westwood+ at model level in four ML directions. EWMA provides a simple baseline, Experts reproduces the paper algorithm, Kalman adds uncertainty-aware RTT estimation, and Adaptive Experts modifies the paper model with adaptive η/α, momentum, and expert revival. All models update prediction in `PktsAcked`, apply RTT-aware cwnd action in `IncreaseWindow`, and preserve behavior via proper `Fork`. This design gives a fair, reproducible comparison across all scenarios and metrics.”