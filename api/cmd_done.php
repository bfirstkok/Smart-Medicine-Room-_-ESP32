<?php
header("Content-Type: application/json; charset=utf-8");
require_once "db.php";

$id        = isset($_POST["id"]) ? intval($_POST["id"]) : 0;
$device_id = $_POST["device_id"] ?? "esp32_1";

$cmd   = $_POST["cmd"] ?? "";
$value = $_POST["value"] ?? "";

if ($id <= 0) {
  http_response_code(400);
  echo json_encode(["ok"=>false, "error"=>"Missing id"], JSON_UNESCAPED_UNICODE);
  exit;
}

// mark command = done
$stmt = $conn->prepare("UPDATE device_command SET status='done' WHERE id=?");
$stmt->bind_param("i", $id);
$stmt->execute();

// normalize
$cmdU = strtoupper(trim($cmd));
$valI = intval($value);

// อัปเดต device_state ให้ตรงกับที่ทำจริง (+ updated_at)
if ($cmdU === "RELAY") {
  $st = $conn->prepare("UPDATE device_state SET relay_on=?, updated_at=NOW() WHERE device_id=?");
  $st->bind_param("is", $valI, $device_id);
  $st->execute();
}
else if ($cmdU === "BUZZER") {
  $st = $conn->prepare("UPDATE device_state SET buzzer_on=?, updated_at=NOW() WHERE device_id=?");
  $st->bind_param("is", $valI, $device_id);
  $st->execute();
}
else if ($cmdU === "LED") {
  $st = $conn->prepare("UPDATE device_state SET led_mode=?, updated_at=NOW() WHERE device_id=?");
  $st->bind_param("is", $valI, $device_id);
  $st->execute();
}
else if ($cmdU === "FAN") {
  // 0/1
  $st = $conn->prepare("UPDATE device_state SET fan_mode=?, updated_at=NOW() WHERE device_id=?");
  $st->bind_param("is", $valI, $device_id);
  $st->execute();
}
else if ($cmdU === "FAN_SPEED") {
  // ของเฟิร์สตอนนี้ใช้ servo_angle เป็นค่าควบคุมพัดลม/สปีด
  $st = $conn->prepare("UPDATE device_state SET servo_angle=?, updated_at=NOW() WHERE device_id=?");
  $st->bind_param("is", $valI, $device_id);
  $st->execute();
}
else if ($cmdU === "SERVO") {
  // เผื่อยังมีของเก่า
  $st = $conn->prepare("UPDATE device_state SET servo_angle=?, updated_at=NOW() WHERE device_id=?");
  $st->bind_param("is", $valI, $device_id);
  $st->execute();
}
else if ($cmdU === "DOOR") {
  // ✅ ใหม่: เก็บมุมประตูไว้ที่ door_angle
  $st = $conn->prepare("UPDATE device_state SET door_angle=?, updated_at=NOW() WHERE device_id=?");
  $st->bind_param("is", $valI, $device_id);
  $st->execute();
}

echo json_encode(["ok"=>true], JSON_UNESCAPED_UNICODE);