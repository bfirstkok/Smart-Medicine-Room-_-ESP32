<?php
header('Content-Type: application/json; charset=utf-8');
require_once "db.php";

$device_id = $_POST["device_id"] ?? "esp32_1";
$event_type = $_POST["event_type"] ?? "DETECT";
$note = $_POST["note"] ?? null;

$stmt = $conn->prepare("INSERT INTO ir_event_log (device_id, event_type, note) VALUES (?,?,?)");
$stmt->bind_param("sss", $device_id, $event_type, $note);
$ok = $stmt->execute();

echo json_encode([
  "ok" => $ok ? true : false,
  "insert_id" => $conn->insert_id
]);