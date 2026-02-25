<?php

// 🔒 ใส่ TOKEN ใหม่หลังจาก revoke แล้ว
define("TG_TOKEN", "8633494080:AAHXRx0zKqvzrk9Ackw6s0pKgp5tHFqpTNY");
define("TG_CHAT_ID", "7838123673");

function tg_send($text) {

  $url = "https://api.telegram.org/bot" . TG_TOKEN . "/sendMessage";

  $data = [
    "chat_id" => TG_CHAT_ID,
    "text" => $text,
    "parse_mode" => "HTML"
  ];

  $options = [
    "http" => [
      "header"  => "Content-Type: application/x-www-form-urlencoded\r\n",
      "method"  => "POST",
      "content" => http_build_query($data),
      "timeout" => 5
    ]
  ];

  $context  = stream_context_create($options);
  $result = @file_get_contents($url, false, $context);

  return $result !== false;
}