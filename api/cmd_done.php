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

// อัปเดต device_state ให้ตรงกับที่ทำจริง
if ($cmd === "RELAY") {
  $v = intval($value);
  $st = $conn->prepare("UPDATE device_state SET relay_on=? WHERE device_id=?");
  $st->bind_param("is", $v, $device_id);
  $st->execute();
}

if ($cmd === "BUZZER") {
  $v = intval($value);
  $st = $conn->prepare("UPDATE device_state SET buzzer_on=? WHERE device_id=?");
  $st->bind_param("is", $v, $device_id);
  $st->execute();
}

if ($cmd === "SERVO") {
  $v = intval($value);
  $st = $conn->prepare("UPDATE device_state SET servo_angle=? WHERE device_id=?");
  $st->bind_param("is", $v, $device_id);
  $st->execute();
}

if ($cmd === "LED") {
  $v = intval($value);
  $st = $conn->prepare("UPDATE device_state SET led_mode=? WHERE device_id=?");
  $st->bind_param("is", $v, $device_id);
  $st->execute();
}

if ($cmd === "FAN") {
  $v = intval($value); // 0/1
  $st = $conn->prepare("UPDATE device_state SET fan_mode=? WHERE device_id=?");
  $st->bind_param("is", $v, $device_id);
  $st->execute();
}

echo json_encode(["ok"=>true], JSON_UNESCAPED_UNICODE);