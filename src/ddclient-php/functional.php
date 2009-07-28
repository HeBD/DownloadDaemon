<?php

function recv_all(&$socket, &$recv_buf) {
	
	$recv_buf = "";
	$recv_number = "";
	$i = 0;
	while(true) {
		if(!@socket_recv($socket, $recv_buf, 1,  0)) {
			return false;
		}
		if($recv_buf == ":") break;
		$recv_number = $recv_number . $recv_buf;
		if($i > 10) return false;
		$i++;
	}
	$recv_buf = "";
	if(!socket_recv($socket, $recv_buf, (int)$recv_number, 0)) {
		return false;
	}
	return true;
	
}

function send_all(&$socket, $snd) {
	$to_send = strlen($snd) . ":" . $snd;
	if(socket_send($socket, $to_send, strlen($to_send), 0)) {
		return true;
	}
	return false;
	
}


function connect_to_daemon(&$socket) {
	set_time_limit(0);
	if(!isset($_COOKIE['ddclient_host']) || !isset($_COOKIE['ddclient_port'])) {
		return "COOKIE";
	}
	
	$host = $_COOKIE['ddclient_host'];
	$port = $_COOKIE['ddclient_port'];
	
	if(!socket_connect($socket, $host, $port)) {
		return "CONNECT";
	}
	
	if(!recv_all($socket, $recv_buf)) {
		return "RECV";
	}
	
	if($recv_buf[0] == "1") {
		$passwd = $_COOKIE['ddclient_passwd'];
		send_all($socket, $passwd);
		recv_all($socket, $recv_buf);
		if($recv_buf[0] == "-") {
			return "PASSWD";
		}
	}

	return "SUCCESS";
}

?>

