<?php
header("Content-Type: application/json; charset=utf-8");
require_once "db.php";

$device_id = $_GET["device_id"] ?? "esp32_1";

/* ---------- latest sensor ---------- */
$latest = null;
$stmt = $conn->prepare("
  SELECT id, device_id, temp_c, humid, ir_state, created_at
  FROM sensor_log
  WHERE device_id=?
  ORDER BY id DESC
  LIMIT 1
");
$stmt->bind_param("s", $device_id);
$stmt->execute();
$res = $stmt->get_result();
if ($row = $res->fetch_assoc()) $latest = $row;

/* ---------- device state (✅ เพิ่ม door_angle) ---------- */
$state = null;
$st = $conn->prepare("
  SELECT device_id,
         relay_on,
         servo_angle,
         door_angle,
         buzzer_on,
         led_mode,
         fan_mode,
         updated_at
  FROM device_state
  WHERE device_id=?
  LIMIT 1
");
$st->bind_param("s", $device_id);
$st->execute();
$stRes = $st->get_result();
if ($row2 = $stRes->fetch_assoc()) {
  // ✅ fallback: ถ้า door_angle ว่าง ให้ใช้ servo_angle แทน
  if (!isset($row2["door_angle"]) || $row2["door_angle"] === null) {
    $row2["door_angle"] = $row2["servo_angle"] ?? null;
  }
  $state = $row2;
}

/* ---------- entry counter ---------- */
$entry_today = 0;
$entry_total = 0;

// total
$ct = $conn->prepare("SELECT COUNT(*) AS c FROM entry_log WHERE device_id=?");
$ct->bind_param("s", $device_id);
$ct->execute();
$ctRes = $ct->get_result();
if ($r = $ctRes->fetch_assoc()) $entry_total = intval($r["c"]);

// today
$cd = $conn->prepare("
  SELECT COUNT(*) AS c
  FROM entry_log
  WHERE device_id=?
    AND DATE(created_at) = CURDATE()
");
$cd->bind_param("s", $device_id);
$cd->execute();
$cdRes = $cd->get_result();
if ($r2 = $cdRes->fetch_assoc()) $entry_today = intval($r2["c"]);

echo json_encode([
  "ok" => true,
  "latest" => $latest,
  "state" => $state,
  "entry" => [
    "today" => $entry_today,
    "total" => $entry_total
  ]
], JSON_UNESCAPED_UNICODE);