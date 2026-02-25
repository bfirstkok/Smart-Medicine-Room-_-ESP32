<?php
header("Content-Type: application/json; charset=utf-8");
require_once "db.php";

$device_id = $_POST["device_id"] ?? "";
if ($device_id === "") {
  echo json_encode(["ok"=>false, "msg"=>"missing device_id"], JSON_UNESCAPED_UNICODE);
  exit;
}

$stmt = $conn->prepare("INSERT INTO entry_log (device_id) VALUES (?)");
$stmt->bind_param("s", $device_id);
$ok = $stmt->execute();

echo json_encode(["ok"=>$ok, "id"=>$conn->insert_id], JSON_UNESCAPED_UNICODE);