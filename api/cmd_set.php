<?php
header("Content-Type: application/json; charset=utf-8");
require_once "db.php";

// ✅ รองรับทั้ง GET และ POST
$device_id = $_REQUEST["device_id"] ?? "esp32_1";
$cmd       = strtoupper(trim($_REQUEST["cmd"] ?? ""));
$value     = trim($_REQUEST["value"] ?? "");

// ✅ เพิ่ม DOOR และ FAN_SPEED / RELAY_AUTO (เผื่อใช้)
$allowed = ["RELAY","SERVO","DOOR","BUZZER","LED","FAN","FAN_SPEED","RELAY_AUTO"];
if (!in_array($cmd, $allowed)) {
  http_response_code(400);
  echo json_encode(["ok"=>false, "error"=>"Invalid cmd", "cmd"=>$cmd]);
  exit;
}

// ✅ ใส่ status='new' ให้แน่นอน (เพราะ cmd_get ดึง status='new')
$stmt = $conn->prepare("INSERT INTO device_command (device_id, cmd, value, status, created_at) VALUES (?,?,?,'new',NOW())");
$stmt->bind_param("sss", $device_id, $cmd, $value);
$ok = $stmt->execute();

echo json_encode(["ok"=>$ok, "cmd"=>$cmd, "value"=>$value], JSON_UNESCAPED_UNICODE);