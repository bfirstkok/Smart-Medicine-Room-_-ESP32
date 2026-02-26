<?php
header('Content-Type: application/json; charset=utf-8');
ini_set('display_errors', 0);
error_reporting(E_ALL);

require_once "db.php";

$device_id = $_GET["device_id"] ?? "esp32_1";
$range = $_GET["range"] ?? "6h";

$interval = "6 HOUR";
if ($range === "1h")  $interval = "1 HOUR";
if ($range === "6h")  $interval = "6 HOUR";
if ($range === "24h") $interval = "24 HOUR";
if ($range === "7d")  $interval = "7 DAY";

$sql = "
  SELECT
    DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS ts,
    temp_c,
    humid,
    ir_state,
    0 AS entry_count
  FROM sensor_log
  WHERE device_id = ?
    AND created_at >= (NOW() - INTERVAL $interval)
  ORDER BY created_at ASC
";

$stmt = $conn->prepare($sql);
$stmt->bind_param("s", $device_id);
$stmt->execute();
$res = $stmt->get_result();

$rows = [];
while($r = $res->fetch_assoc()){
  $rows[] = $r;
}

echo json_encode([
  "ok" => true,
  "device_id" => $device_id,
  "range" => $range,
  "rows" => $rows
]);