#pragma once

// ============================================================================
// gps_alpha_guidance.h
//
// Live alpha guidance shown immediately after a gybe.
//
// The metric calculators decide whether an alpha result is valid. This module is
// only a rider prompt: it watches the same GPS geometry and suggests steering
// upwind/downwind to finish near the target closure.
// ============================================================================

enum class AlphaSteerAdvice {
  Inactive,
  Hold,
  GoUp,
  GoDown,
};

struct AlphaGuidanceState {
  bool active = false;
  AlphaSteerAdvice advice = AlphaSteerAdvice::Inactive;

  float targetClosureM = 48.0f;
  float closureM = 0.0f;
  float errorM = 0.0f;
  float pathSinceGybeM = 0.0f;
  float alphaSpeedKnots = 0.0f;
};

// Reset all guidance state at the start of a new logging session.
void gps_alpha_guidance_reset();

// Update guidance from the latest GPS/statistics sample.
void gps_alpha_guidance_update();

// Read-only snapshot for display code.
const AlphaGuidanceState& gps_alpha_guidance_state();

