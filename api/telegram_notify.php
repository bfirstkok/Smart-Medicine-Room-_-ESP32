<?php
require_once "telegram.php";

$type = $_REQUEST["type"] ?? "test";
$temp = $_REQUEST["temp_c"] ?? "-";
$hum  = $_REQUEST["humid"] ?? "-";

$msg = "🚨 TYPE: {$type}\n🌡 Temp: {$temp}\n💧 Hum: {$hum}";

tg_send($msg);

echo json_encode(["ok"=>true]);