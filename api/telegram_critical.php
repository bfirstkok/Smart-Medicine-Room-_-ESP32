<?php
header("Content-Type: application/json; charset=utf-8");
require_once "telegram.php";

$device_id = $_REQUEST["device_id"] ?? "esp32_1";
$temp_c    = $_REQUEST["temp_c"] ?? "-";
$humid     = $_REQUEST["humid"] ?? "-";

$msg = "🚨 CRITICAL ALERT\n📟 Device: {$device_id}\n🌡 {$temp_c}°C | 💧 {$humid}%";

$ok = tg_send($msg);
echo json_encode(["ok"=>$ok], JSON_UNESCAPED_UNICODE);