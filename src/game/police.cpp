#include "police.h"

#include <algorithm>
#include <cmath>

namespace {
// Normaliza ângulo para o intervalo [-pi, pi]
float NormalizeAngle(float angle) {
  const float twoPi = 6.2831853f;
  const float pi = 3.1415926f;
  while (angle > pi) {
    angle -= twoPi;
  }
  while (angle < -pi) {
    angle += twoPi;
  }
  return angle;
}

// Calcula velocidade ideal para curvas
float CalculateCurveSpeed(float angleRadians, float maxSpeed) {
  float absAngle = std::abs(angleRadians);
  // Curvas fechadas reduzem mais a velocidade
  if (absAngle > 0.785f) { // 45 graus
    return maxSpeed * 0.5f;
  } else if (absAngle > 0.524f) { // 30 graus
    return maxSpeed * 0.7f;
  } else if (absAngle > 0.262f) { // 15 graus
    return maxSpeed * 0.85f;
  }
  return maxSpeed;
}

// Prediz posição futura do alvo
Vec3 PredictTargetPosition(const VehicleState &target, float predictionTime) {
  return target.position + target.velocity * predictionTime;
}

// Estado da IA entre frames (reset no recomeço)
float gStuckTimer = 0.0f;
float gReverseTimer = 0.0f;
float gPreviousSpeed = 0.0f;
} // namespace

void ResetPoliceChaseState() {
  // Limpa timers internos
  gStuckTimer = 0.0f;
  gReverseTimer = 0.0f;
  gPreviousSpeed = 0.0f;
}

void UpdatePoliceChase(VehicleState &police, const VehicleState &target,
                       float dt, float elapsedSeconds, float startDelaySeconds,
                       const MovementConfig &config, float trackHalfExtent,
                       const std::vector<Vec3> &trail) {
  float clampedDt = std::max(dt, 0.0f);

  // Modo de destravar (marcha-atrás)
  if (gReverseTimer > 0.0f) {
    gReverseTimer -= clampedDt;

    // Recuar a controlar a velocidade
    police.speed = std::max(police.speed - config.braking * clampedDt,
                            -config.maxSpeed * 0.5f);

    // Contra-virar para sair da esquina
    police.heading -= config.turnRate * clampedDt * 0.5f;

    // Aplica física básica
    const float headingOffset = 0.0f; // Sem offset extra
    float worldHeading = police.heading + headingOffset;
    Vec3 forwardDir = {std::cos(worldHeading), 0.0f, std::sin(worldHeading)};
    police.velocity = forwardDir * police.speed;
    police.position = police.position + police.velocity * clampedDt;
    police.position.x =
        std::clamp(police.position.x, -trackHalfExtent, trackHalfExtent);
    police.position.z =
        std::clamp(police.position.z, -trackHalfExtent, trackHalfExtent);
    return;
  }

  if (elapsedSeconds < startDelaySeconds) {
    // Ainda não começa a perseguir
    police.velocity = {0.0f, 0.0f, 0.0f};
    police.speed = 0.0f;
    return;
  }

  // Previsão simples do alvo
  Vec3 targetPos = target.position;
  
  float targetSpeed = Length(target.velocity);
  float predictionTime = 0.5f; // Meio segundo
  if (targetSpeed > 0.1f) {
    Vec3 predictedPos = PredictTargetPosition(target, predictionTime);
    targetPos = predictedPos;
  }

  bool chasingTrail = false;
  float distanceToTarget = Length(targetPos - police.position);

  // Seguir rasto quando está longe
  if (!trail.empty() && distanceToTarget > 5.0f) {
    float minDistSq = 1e9f;
    size_t closestIdx = 0;

    // Encontra o ponto do rasto mais próximo
    for (size_t i = 0; i < trail.size(); ++i) {
      Vec3 d = trail[i] - police.position;
      float dSq = d.x * d.x + d.z * d.z;
      if (dSq < minDistSq) {
        minDistSq = dSq;
        closestIdx = i;
      }
    }

    // Look-ahead varia com a velocidade
    float speedRatio = std::abs(police.speed) / config.maxSpeed;
    size_t lookAhead = static_cast<size_t>(5 + speedRatio * 15); // 5-20 pontos
    size_t targetIdx = closestIdx + lookAhead;
    
    if (targetIdx < trail.size()) {
      targetPos = trail[targetIdx];
      chasingTrail = true;
    } else if (!trail.empty()) {
      // Se passou do fim, mira direto no jogador
      targetPos = target.position;
      chasingTrail = false;
    }
  }

  Vec3 toTarget = targetPos - police.position;
  float distance = Length(toTarget);
  if (distance < 0.0001f) {
    return;
  }

  const float headingOffset = 0.0f; // Sem offset adicional
  float worldHeading = police.heading + headingOffset;

  float desiredYaw = std::atan2(toTarget.z, toTarget.x);
  float yawDiff = NormalizeAngle(desiredYaw - worldHeading);

  // Direção com suavização
  float steerInput = std::clamp(yawDiff * 1.2f, -1.0f, 1.0f);
  
  // Steering mais forte em alta velocidade
  if (steerInput != 0.0f) {
    float speedFactor =
        std::clamp(std::abs(police.speed) / config.maxSpeed, 0.3f, 1.0f);
    // Suaviza a rotação
    float steerAmount = steerInput * config.turnRate * speedFactor * clampedDt;
    police.heading += steerAmount;
    worldHeading = police.heading + headingOffset;
  }

  Vec3 forwardDir = {std::cos(worldHeading), 0.0f, std::sin(worldHeading)};

  // Ajusta a velocidade pelas curvas
  float curveSpeed = CalculateCurveSpeed(yawDiff, config.maxSpeed);
  float desiredSpeed = curveSpeed;

  if (!chasingTrail) {
    // Aproximação final
    if (distance < 3.0f) {
      desiredSpeed = std::min(curveSpeed, distance * 1.5f);
    } else {
      desiredSpeed = std::min(curveSpeed, config.maxSpeed * 0.9f);
    }
  } else {
    // Seguindo rasto
    if (std::abs(yawDiff) < 0.174f) { // < 10 graus = reta
      desiredSpeed = config.maxSpeed;
    }
  }

  // Aceleração/desaceleração suave
  float speedError = desiredSpeed - police.speed;
  
  // Ajuste da aceleração
  float accelMultiplier = 1.0f;
  if (speedError > 0.0f) {
    // Acelerar mais suave no início
    accelMultiplier = std::min(1.0f, 0.3f + std::abs(police.speed) / config.maxSpeed * 0.7f);
  } else {
    // Travar mais em curvas
    float curveIntensity = std::abs(yawDiff) / 1.57f; // Normalizado por 90°
    accelMultiplier = 1.0f + curveIntensity * 1.5f;
  }
  
  float accelCmd = std::clamp(speedError * accelMultiplier, 
                               -config.acceleration, 
                               config.acceleration);
  police.speed += accelCmd * clampedDt;

  // Drag mais forte em alta velocidade
  float dragEffect = config.drag * (1.0f + std::abs(police.speed) / config.maxSpeed * 0.5f);
  if (police.speed > 0.0f) {
    police.speed = std::max(0.0f, police.speed - dragEffect * clampedDt);
  } else if (police.speed < 0.0f) {
    police.speed = std::min(0.0f, police.speed + dragEffect * clampedDt);
  }

  police.speed =
      std::clamp(police.speed, -config.maxSpeed * 0.3f, config.maxSpeed);
  
  police.velocity = forwardDir * police.speed;
  police.position = police.position + police.velocity * clampedDt;
  police.position.x =
      std::clamp(police.position.x, -trackHalfExtent, trackHalfExtent);
  police.position.z =
      std::clamp(police.position.z, -trackHalfExtent, trackHalfExtent);

  // Deteta se ficou preso
  float speedChange = std::abs(police.speed - gPreviousSpeed);
  bool isStuck = (std::abs(police.speed) < 0.8f && distance > 2.0f) || 
                 (speedChange < 0.1f && std::abs(police.speed) < 1.5f);
  
  if (isStuck && elapsedSeconds > startDelaySeconds) {
    gStuckTimer += clampedDt;
  } else {
    gStuckTimer = std::max(0.0f, gStuckTimer - clampedDt * 0.5f);
  }

  // Dispara a manobra de recuperação
  if (gStuckTimer > 1.2f) {
    gReverseTimer = 1.0f + (gStuckTimer * 0.3f); // Tempo variável baseado no quanto ficou preso
    gStuckTimer = 0.0f;
  }
  
  gPreviousSpeed = police.speed;
}
