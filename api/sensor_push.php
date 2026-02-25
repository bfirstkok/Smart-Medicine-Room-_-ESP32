<?php
header("Content-Type: application/json; charset=utf-8");
require_once "db.php";
require_once "telegram.php";

$device_id = $_POST["device_id"] ?? "esp32_1";
$temp_c    = isset($_POST["temp_c"]) ? floatval($_POST["temp_c"]) : null;
$humid     = isset($_POST["humid"]) ? floatval($_POST["humid"]) : null;
$ir_state  = isset($_POST["ir_state"]) ? intval($_POST["ir_state"]) : 0;

// 1) อ่านค่า IR ล่าสุดก่อนหน้า (เอาไว้เช็ค 0->1)
$prev_ir = 0;
$check = $conn->prepare("SELECT ir_state FROM sensor_log WHERE device_id=? ORDER BY id DESC LIMIT 1");
$check->bind_param("s", $device_id);
$check->execute();
$res = $check->get_result();
if ($row = $res->fetch_assoc()) {
  $prev_ir = intval($row["ir_state"]);
}

// 2) Insert log
$stmt = $conn->prepare("INSERT INTO sensor_log (device_id, temp_c, humid, ir_state) VALUES (?,?,?,?)");
$stmt->bind_param("sddi", $device_id, $temp_c, $humid, $ir_state);
$stmt->execute();

// 3) แจ้ง Telegram เฉพาะตอน “0 -> 1”
if ($prev_ir == 0 && $ir_state == 1) {
  $msg = "🚨 ตรวจจับด้วย IR!\n"
       . "device: {$device_id}\n"
       . "Temp: " . ($temp_c !== null ? $temp_c : "-") . " °C\n"
       . "Humid: " . ($humid !== null ? $humid : "-") . " %\n"
       . "เวลา: " . date("Y-m-d H:i:s");
  tg_send($msg);
}

echo json_encode(["ok"=>true], JSON_UNESCAPED_UNICODE);