<?php
require_once "db.php";

header('Content-Type: application/json; charset=utf-8');

$device_id = $_GET["device_id"] ?? "esp32_1";

// ✅ แก้ตรงนี้: ตาราง + สถานะ ให้ตรงกับ DB จริง
$sql = "SELECT id, cmd, value 
        FROM device_command 
        WHERE device_id=? AND status='new' 
        ORDER BY id ASC 
        LIMIT 1";

$stmt = $conn->prepare($sql);
$stmt->bind_param("s", $device_id);
$stmt->execute();
$row = $stmt->get_result()->fetch_assoc();

if (!$row) {
  echo json_encode(["ok"=>true, "has_cmd"=>false]);
  exit;
}

echo json_encode([
  "ok"=>true,
  "has_cmd"=>true,
  "id"=>(int)$row["id"],
  "cmd"=>$row["cmd"],
  "value"=>$row["value"]
]);